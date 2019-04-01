/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <glib.h>
#include <pthread.h>
#include <libmondai.h>
#include <RecParser.h>

#include "LDparser.h"
#include "term.h"
#include "wfcdata.h"
#include "wfc.h"
#include "debug.h"

static SessionData *head;
static pthread_mutex_t lock;

extern SessionCtrl *NewSessionCtrl(SessionCtrlType type) {
  SessionCtrl *ctrl;

  ctrl = g_new0(SessionCtrl, 1);
  ctrl->type = type;
  ctrl->sysdbval = RecParseValueMem(SYSDBVAL_DEF, NULL);
  InitializeValue(ctrl->sysdbval);
  ctrl->sysdbvals = RecParseValueMem(SYSDBVALS_DEF, NULL);

  InitializeValue(ctrl->sysdbvals);
  return ctrl;
}

extern void FreeSessionCtrl(SessionCtrl *ctrl) {
  if (ctrl->sysdbval != NULL) {
    FreeValueStruct(ctrl->sysdbval);
  }
  if (ctrl->sysdbvals != NULL) {
    FreeValueStruct(ctrl->sysdbvals);
  }
  g_free(ctrl);
}

static void TimevalToString(char *buf, size_t size, struct timeval tv) {
  snprintf(buf, size, "%lld.%06d", (long long)tv.tv_sec, (int)tv.tv_usec);
}

static void _strftime(char *ret, size_t size, time_t time) {
  struct tm tmp;
  localtime_r(&time, &tmp);
  strftime(ret, size, "%a %b %d %H:%M:%S %Y", &tmp);
}

static void SetSysdbval(SessionData *session) {
  ValueStruct *v, *sysdbval;
  char buf[128];

  sysdbval = session->sysdbval;
  v = GetRecordItem(sysdbval, "id");
  SetValueString(v, session->hdr->uuid, NULL);
  v = GetRecordItem(sysdbval, "user");
  SetValueString(v, session->hdr->user, NULL);
  v = GetRecordItem(sysdbval, "host");
  SetValueString(v, session->hdr->host, NULL);
  v = GetRecordItem(sysdbval, "agent");
  SetValueString(v, session->agent, NULL);
  v = GetRecordItem(sysdbval, "window");
  SetValueString(v, session->hdr->window, NULL);
  v = GetRecordItem(sysdbval, "widget");
  SetValueString(v, session->hdr->widget, NULL);
  v = GetRecordItem(sysdbval, "event");
  SetValueString(v, session->hdr->event, NULL);
  v = GetRecordItem(sysdbval, "in_process");
  SetValueString(v, (session->fInProcess ? "T" : "F"), NULL);
  v = GetRecordItem(sysdbval, "create_time");
  _strftime(buf, sizeof(buf), session->create_time.tv_sec);
  SetValueString(v, buf, NULL);
  v = GetRecordItem(sysdbval, "access_time");
  _strftime(buf, sizeof(buf), session->access_time.tv_sec);
  SetValueString(v, buf, NULL);
  v = GetRecordItem(sysdbval, "process_time");
  TimevalToString(buf, sizeof(buf), session->process_time);
  SetValueString(v, buf, NULL);
  v = GetRecordItem(sysdbval, "total_process_time");
  TimevalToString(buf, sizeof(buf), session->total_process_time);
  SetValueString(v, buf, NULL);
  v = GetRecordItem(sysdbval, "count");
  snprintf(buf, sizeof(buf), "%d", session->count);
  SetValueString(v, buf, NULL);
}

static void InsertSession(SessionCtrl *ctrl) {
  SessionData *p;

  ctrl->rc = SESSION_CONTROL_NG;
  if (ctrl->session == NULL) {
    Warning("ctrl->session is NULL");
    return;
  }
  p = head;
  while (p != NULL) {
    if (!strcmp(p->hdr->uuid, ctrl->session->hdr->uuid)) {
      Warning("[%s] is already exist", ctrl->session->hdr->uuid);
      return;
    }
    p = p->next;
  }
  ctrl->session->next = NULL;
  SetSysdbval(ctrl->session);

  if (head == NULL) {
    head = ctrl->session;
  } else {
    p = head;
    while (p->next != NULL) {
      p = p->next;
    }
    p->next = ctrl->session;
  }
  ctrl->rc = SESSION_CONTROL_OK;
}

static void UpdateSession(SessionCtrl *ctrl) {
  ctrl->rc = SESSION_CONTROL_NG;

  if (ctrl->session == NULL) {
    Warning("ctrl->session is NULL");
    return;
  }
  SetSysdbval(ctrl->session);
  ctrl->rc = SESSION_CONTROL_OK;
}

static void DeleteSession(SessionCtrl *ctrl) {
  SessionData *p, *q;

  ctrl->rc = SESSION_CONTROL_NG;
  if (ctrl->session == NULL) {
    Warning("ctrl->session is NULL");
    return;
  }

  p = NULL;
  q = head;
  while (q != NULL) {
    if (!strcmp(q->hdr->uuid, ctrl->session->hdr->uuid)) {
      if (p == NULL) {
        head = q->next;
      } else {
        p->next = q->next;
      }
      break;
    }
    p = q;
    q = q->next;
  }

  ctrl->rc = SESSION_CONTROL_OK;
}

static void LookupSession(SessionCtrl *ctrl) {
  SessionData *p;

  ctrl->rc = SESSION_CONTROL_NG;
  ctrl->session = NULL;
  p = head;
  while (p != NULL) {
    if (!strcmp(p->hdr->uuid, ctrl->id)) {
      ctrl->session = p;
      break;
    }
    p = p->next;
  }
  ctrl->rc = SESSION_CONTROL_OK;
}

static void GetSessionNum(SessionCtrl *ctrl) {
  SessionData *p;

  ctrl->size = 0;
  p = head;
  while (p != NULL) {
    p = p->next;
    ctrl->size += 1;
  }
  ctrl->rc = SESSION_CONTROL_OK;
}

static void GetData(SessionCtrl *ctrl) {
  SessionData *p;
  ValueStruct *v;
  char *id;

  ctrl->rc = SESSION_CONTROL_NG;
  if (ctrl->sysdbval == NULL) {
    Warning("ctrl->sysdbval is NULL");
    return;
  }

  v = GetRecordItem(ctrl->sysdbval, "id");
  id = ValueToString(v, NULL);
  p = head;
  while (p != NULL) {
    if (!strcmp(p->hdr->uuid, id)) {
      CopyValue(ctrl->sysdbval, p->sysdbval);
      break;
    }
    p = p->next;
  }

  ctrl->rc = SESSION_CONTROL_OK;
}

static void GetMessage(SessionCtrl *ctrl) { GetData(ctrl); }

static void ResetMessage(SessionCtrl *ctrl) {
  SessionData *session, *p;
  ValueStruct *v;
  char *id;

  ctrl->rc = SESSION_CONTROL_NG;
  if (ctrl->sysdbval == NULL) {
    Warning("ctrl->sysdbval is NULL");
    return;
  }
  v = GetRecordItem(ctrl->sysdbval, "id");
  id = ValueToString(v, NULL);
  session = NULL;
  p = head;
  while (p != NULL) {
    if (!strcmp(p->hdr->uuid, id)) {
      session = p;
      break;
    }
    p = p->next;
  }
  if (session == NULL) {
    return;
  }
  v = GetRecordItem(session->sysdbval, "popup");
  SetValueString(v, "", NULL);
  v = GetRecordItem(session->sysdbval, "dialog");
  SetValueString(v, "", NULL);
  v = GetRecordItem(session->sysdbval, "abort");
  SetValueString(v, "", NULL);
  ctrl->rc = SESSION_CONTROL_OK;
}

static void SetMessage(SessionCtrl *ctrl) {
  ValueStruct *v, *w;
  SessionData *session, *p;
  char *id;

  ctrl->rc = SESSION_CONTROL_NG;
  if (ctrl->sysdbval == NULL) {
    Warning("ctrl->sysdbval is NULL");
    return;
  }
  v = GetRecordItem(ctrl->sysdbval, "id");
  id = ValueToString(v, NULL);
  session = NULL;
  p = head;
  while (p != NULL) {
    if (!strcmp(p->hdr->uuid, id)) {
      session = p;
      break;
    }
    p = p->next;
  }
  if (session == NULL) {
    return;
  }
  v = GetRecordItem(session->sysdbval, "popup");
  w = GetRecordItem(ctrl->sysdbval, "popup");
  SetValueString(v, ValueToString(w, NULL), NULL);
  v = GetRecordItem(session->sysdbval, "dialog");
  w = GetRecordItem(ctrl->sysdbval, "dialog");
  SetValueString(v, ValueToString(w, NULL), NULL);
  v = GetRecordItem(session->sysdbval, "abort");
  w = GetRecordItem(ctrl->sysdbval, "abort");
  SetValueString(v, ValueToString(w, NULL), NULL);
  ctrl->rc = SESSION_CONTROL_OK;
}

static void _SetMessage(SessionData *session, SessionCtrl *ctrl) {
  ValueStruct *v, *w;

  if (ctrl->sysdbval == NULL) {
    Warning("ctrl->sysdbval is NULL");
    return;
  }
  if (session == NULL) {
    return;
  }
  v = GetRecordItem(session->sysdbval, "popup");
  w = GetRecordItem(ctrl->sysdbval, "popup");
  SetValueString(v, ValueToString(w, NULL), NULL);
  v = GetRecordItem(session->sysdbval, "dialog");
  w = GetRecordItem(ctrl->sysdbval, "dialog");
  SetValueString(v, ValueToString(w, NULL), NULL);
  v = GetRecordItem(session->sysdbval, "abort");
  w = GetRecordItem(ctrl->sysdbval, "abort");
  SetValueString(v, ValueToString(w, NULL), NULL);
}

static void SetMessageAll(SessionCtrl *ctrl) {
  SessionData *p;

  ctrl->rc = SESSION_CONTROL_NG;
  p = head;
  while (p != NULL) {
    _SetMessage(p, ctrl);
    p = p->next;
  }
  ctrl->rc = SESSION_CONTROL_OK;
}

static void _GetData(SessionData *session, SessionCtrl *ctrl) {
  ValueStruct *v, *a;
  int size;

  if (session == NULL) {
    return;
  }
  if (ctrl->sysdbvals == NULL) {
    Warning("ctrl->sysdbvals is null");
    return;
  }
  a = GetRecordItem(ctrl->sysdbvals, "values");
  size = ValueArraySize(a);
  if (ctrl->size >= size) {
    if (ctrl->size == size) {
      Warning("GetDataAll SYSDBVALS_SIZE(%d) over", size);
    }
    ctrl->size += 1;
    return;
  }
  v = GetArrayItem(a, ctrl->size);
  if (v != NULL) {
    CopyValue(v, session->sysdbval);
  }
  ctrl->size += 1;
}

static void GetDataAll(SessionCtrl *ctrl) {
  SessionData *p;

  ctrl->rc = SESSION_CONTROL_NG;
  ctrl->size = 0;
  p = head;
  while (p != NULL) {
    _GetData(p, ctrl);
    p = p->next;
  }
  ctrl->rc = SESSION_CONTROL_OK;
}

extern SessionCtrl *ExecSessionCtrl(SessionCtrl *ctrl) {
  pthread_mutex_lock(&lock);
  switch (ctrl->type) {
  case SESSION_CONTROL_INSERT:
    InsertSession(ctrl);
    break;
  case SESSION_CONTROL_UPDATE:
    UpdateSession(ctrl);
    break;
  case SESSION_CONTROL_DELETE:
    DeleteSession(ctrl);
    break;
  case SESSION_CONTROL_LOOKUP:
    LookupSession(ctrl);
    break;
  case SESSION_CONTROL_GET_SESSION_NUM:
    GetSessionNum(ctrl);
    break;
  case SESSION_CONTROL_GET_DATA:
    GetData(ctrl);
    break;
  case SESSION_CONTROL_GET_MESSAGE:
    GetMessage(ctrl);
    break;
  case SESSION_CONTROL_RESET_MESSAGE:
    ResetMessage(ctrl);
    break;
  case SESSION_CONTROL_SET_MESSAGE:
    SetMessage(ctrl);
    break;
  case SESSION_CONTROL_SET_MESSAGE_ALL:
    SetMessageAll(ctrl);
    break;
  case SESSION_CONTROL_GET_DATA_ALL:
    GetDataAll(ctrl);
    break;
  default:
    Warning("do not reach");
    ctrl->rc = SESSION_CONTROL_NG;
    break;
  }
  pthread_mutex_unlock(&lock);
  return ctrl;
}

extern void InitSessionCtrl() {
  RecParserInit();
  head = NULL;
  pthread_mutex_init(&lock, NULL);
}
