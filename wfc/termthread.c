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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>
#include <glib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <json.h>
#include <uuid/uuid.h>

#include "enum.h"
#include "libmondai.h"
#include "RecParser.h"
#include "lock.h"
#include "socket.h"
#include "net.h"
#include "comm.h"
#include "comms.h"
#include "queue.h"
#include "directory.h"
#include "wfcdata.h"
#include "wfc.h"
#include "glterm.h"
#include "termthread.h"
#include "corethread.h"
#include "sessionctrl.h"
#include "dirs.h"
#include "message.h"
#include "debug.h"

extern void TermEnqueue(TermNode *term, SessionData *data) {
  EnQueue(term->que, data);
}

static SessionData *NewSessionData(void) {
  SessionData *data;
  uuid_t u;

  data = New(SessionData);
  memclear(data, sizeof(SessionData));
  data->type = SESSION_TYPE_TERM;
  data->status = SESSION_STATUS_NORMAL;
  data->hdr = New(MessageHeader);
  memclear(data->hdr, sizeof(MessageHeader));
  data->hdr->command = APL_COMMAND_LINK;
  uuid_generate(u);
  uuid_unparse(u, data->hdr->uuid);
  strcpy(data->hdr->tenant, MCP_TENANT);
  data->apsid = -1;
  data->spadata = NewNameHash();
  data->scrpool = NewNameHash();
  data->window_table = NewNameHash();
  gettimeofday(&(data->create_time), NULL);
  gettimeofday(&(data->access_time), NULL);
  timerclear(&(data->process_time));
  timerclear(&(data->total_process_time));
  data->apidata = New(APIData);
  data->apidata->status = WFC_API_OK;
  data->apidata->rec = NewLBS();
  data->sysdbval = RecParseValueMem(SYSDBVAL_DEF, NULL);
  InitializeValue(data->sysdbval);
  data->count = 0;
  data->w.sp = 0;
  snprintf(data->hdr->tempdir, SIZE_PATH, "%s/%s", TempDirRoot,
           data->hdr->uuid);
  if (!MakeDir(data->hdr->tempdir, 0700)) {
    Error("cannot make session tempdir %s", data->hdr->tempdir);
  }
  return (data);
}

static guint FreeSpa(char *name, LargeByteString *spa, void *dummy) {
  if (name != NULL) {
    xfree(name);
  }
  if (spa != NULL) {
    FreeLBS(spa);
  }
  return TRUE;
}

static guint FreeScr(char *name, LargeByteString *scr, void *dummy) {
  if (name != NULL) {
    xfree(name);
  }
  if (scr != NULL) {
    FreeLBS(scr);
  }
  return TRUE;
}

static guint FreeWindowTable(char *name, void *data, void *dummy) {
  if (name != NULL) {
    xfree(name);
  }
  return TRUE;
}

static void FreeSessionData(SessionData *data) {
  if (data->type != SESSION_TYPE_API) {
    MessageLogPrintf("session end %s [%s@%s] %s", data->hdr->uuid,
                     data->hdr->user, data->hdr->host, data->agent);
  }
  if (data->linkdata != NULL) {
    FreeLBS(data->linkdata);
  }
  if (data->agent != NULL) {
    xfree(data->agent);
  }
  /* セッションデータの削除 */
  rm_r_old_depth(TempDirRoot,86400,1); /* 86400 = 1day */
  xfree(data->hdr);
  g_hash_table_foreach_remove(data->spadata, (GHRFunc)FreeSpa, NULL);
  DestroyHashTable(data->spadata);
  g_hash_table_foreach_remove(data->scrpool, (GHRFunc)FreeScr, NULL);
  DestroyHashTable(data->scrpool);
  g_hash_table_foreach_remove(data->window_table, (GHRFunc)FreeWindowTable,
                              NULL);
  DestroyHashTable(data->window_table);
  xfree(data->scrdata);
  FreeLBS(data->apidata->rec);
  xfree(data->apidata);
  FreeValueStruct(data->sysdbval);
  xfree(data);
}

static LargeByteString *NewLinkData(void) {
  LargeByteString *linkdata;
  size_t size;

  if (ThisEnv->linkrec != NULL) {
    linkdata = NewLBS();
    size = NativeSizeValue(NULL, ThisEnv->linkrec->value);
    LBS_ReserveSize(linkdata, size, FALSE);
    NativePackValue(NULL, LBS_Body(linkdata), ThisEnv->linkrec->value);
  } else {
    linkdata = NULL;
  }
  return linkdata;
}

static void RegisterSession(SessionData *data) {
  SessionCtrl *ctrl;

  ctrl = NewSessionCtrl(SESSION_CONTROL_INSERT);
  ctrl->session = data;
  ctrl = ExecSessionCtrl(ctrl);
  FreeSessionCtrl(ctrl);
}

static SessionData *LookupSession(const char *term) {
  SessionData *data;
  SessionCtrl *ctrl;
  ctrl = NewSessionCtrl(SESSION_CONTROL_LOOKUP);
  strcpy(ctrl->id, term);
  ctrl = ExecSessionCtrl(ctrl);
  data = ctrl->session;
  FreeSessionCtrl(ctrl);
  return (data);
}

static void DeregisterSession(SessionData *data) {
  SessionCtrl *ctrl;
  ctrl = NewSessionCtrl(SESSION_CONTROL_DELETE);
  ctrl->session = data;
  ctrl = ExecSessionCtrl(ctrl);
  FreeSessionCtrl(ctrl);
  FreeSessionData(data);
}

static void UpdateSession(SessionData *data) {
  SessionCtrl *ctrl;

  gettimeofday(&(data->access_time), NULL);
  ctrl = NewSessionCtrl(SESSION_CONTROL_UPDATE);
  ctrl->session = data;
  ctrl = ExecSessionCtrl(ctrl);
  FreeSessionCtrl(ctrl);

}

static unsigned int GetSessionNum() {
  SessionCtrl *ctrl;
  unsigned int size;
  ctrl = NewSessionCtrl(SESSION_CONTROL_GET_SESSION_NUM);
  ctrl = ExecSessionCtrl(ctrl);
  size = ctrl->size;
  FreeSessionCtrl(ctrl);
  return size;
}

static SessionData *InitAPISession(const char *user, const char *wname,
                                   const char *host) {
  SessionData *data;
  LD_Node *ld;

  data = NewSessionData();
  data->type = SESSION_TYPE_API;
  strcpy(data->hdr->window, wname);
  strcpy(data->hdr->user, user);
  strcpy(data->hdr->host, host);

  if ((ld = g_hash_table_lookup(ComponentHash, wname)) != NULL) {
    data->ld = ld;
    data->linkdata = NULL;
    data->cWindow = ld->info->cWindow;
    data->scrdata = NULL;
    data->hdr->puttype = SCREEN_NULL;
  } else {
    Warning("[%s] session fail Window [%s] not found.", data->hdr->uuid, wname);
    data = NULL;
  }
  return (data);
}

static SessionData *Process(SessionData *data) {
  struct timeval tv1;
  struct timeval tv2;
  data->fInProcess = TRUE;
  gettimeofday(&tv1, NULL);
  CoreEnqueue(data);
  data = DeQueue(data->term->que);
  data->count += 1;
  gettimeofday(&tv2, NULL);
  timersub(&tv2, &tv1, &(data->process_time));
  timeradd(&(data->total_process_time), &(data->process_time), &tv1);
  data->total_process_time = tv1;
  data->fInProcess = FALSE;
  return data;
}

static json_object *MakeJSONResponseTemplate(json_object *obj) {
  json_object *res, *child;

  res = json_object_new_object();
  json_object_object_add(res, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_get_ex(obj, "id", &child);
  json_object_object_add(res, "id",
                         json_object_new_int(json_object_get_int(child)));

  return res;
}

static void JSONRPC_Error(TermNode *term, json_object *obj, int no,
                          const char *msg) {
  json_object *error, *res;

  res = MakeJSONResponseTemplate(obj);
  error = json_object_new_object();
  json_object_object_add(error, "code", json_object_new_int(no));
  json_object_object_add(error, "message", json_object_new_string(msg));
  json_object_object_add(res, "error", error);

  SendString(term->fp, (char *)json_object_to_json_string_ext(
                           res, JSON_C_TO_STRING_PLAIN));
  if (CheckNetFile(term->fp)) {
    CloseNet(term->fp);
  }
  json_object_put(res);
}

static gboolean CheckClientVersion(char *version) {
  if (strcmp("2.0.0", version) <= 0) {
    return TRUE;
  }
  return FALSE;
}

static void RPC_StartSession(TermNode *term, json_object *obj) {
  json_object *params, *meta, *child, *result, *res;
  SessionData *data;
  int sesnum;
  gchar *prefix, *rpcuri, *resturi;
  if (!json_object_object_get_ex(obj, "params", &params)) {
    Warning("request have not params");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(params, "meta", &meta)) {
    Warning("request have not meta");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(meta, "client_version", &child)) {
    Warning("request have not client_version");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!CheckClientVersion((char *)json_object_get_string(child))) {
    Warning("invalid client version:%s", json_object_get_string(child));
    JSONRPC_Error(term, obj, -20001, "Invalid Client Version");
    return;
  }
  if (!json_object_object_get_ex(meta, "server_url_prefix", &child)) {
    Warning("request have not client_version");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  prefix = (char *)json_object_get_string(child);

  sesnum = GetSessionNum();
  if (SesNum != 0 && sesnum >= SesNum) {
    Warning("Discard new session(%s);max session number(%d)", term, SesNum);
    CloseNet(term->fp);
    return;
  }

  data = NewSessionData();
  data->linkdata = NewLinkData();
  data->term = term;

  if (!json_object_object_get_ex(meta, "user", &child)) {
    Warning("request have not user");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  memset(data->hdr->user, 0, SIZE_USER + 1);
  strncpy(data->hdr->user, (char *)json_object_get_string(child), SIZE_USER);

  if (!json_object_object_get_ex(meta, "host", &child)) {
    Warning("request have not host");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  memset(data->hdr->host, 0, SIZE_HOST + 1);
  strncpy(data->hdr->host, json_object_get_string(child), SIZE_HOST);

  if (!json_object_object_get_ex(meta, "agent", &child)) {
    Warning("request have not agent");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  data->agent = strdup(json_object_get_string(child));

  MessageLogPrintf("session start(%d) %s [%s@%s] %s", sesnum + 1,
                   data->hdr->uuid, data->hdr->user, data->hdr->host,
                   data->agent);

  data->hdr->puttype = SCREEN_INIT;
  RegisterSession(data);

  res = MakeJSONResponseTemplate(obj);
  result = json_object_new_object();
  meta = json_object_new_object();
  json_object_object_add(meta, "session_id",
                         json_object_new_string(data->hdr->uuid));
  json_object_object_add(result, "meta", meta);
  rpcuri = g_strdup_printf("%s/rpc/", prefix);
  resturi = g_strdup_printf("%s/rest/", prefix);
  json_object_object_add(result, "app_rpc_endpoint_uri",
                         json_object_new_string(rpcuri));
  json_object_object_add(result, "app_rest_api_uri_root",
                         json_object_new_string(resturi));
  g_free(rpcuri);
  g_free(resturi);
  json_object_object_add(res, "result", result);

  SendString(term->fp, (char *)json_object_to_json_string_ext(
                           res, JSON_C_TO_STRING_PLAIN));
  if (CheckNetFile(term->fp)) {
    CloseNet(term->fp);
  } else {
    DeregisterSession(data);
  }
  json_object_put(res);
}

static void RPC_EndSession(TermNode *term, json_object *obj) {
  json_object *params, *meta, *child, *result, *res;
  SessionData *data;
  LD_Node *ld;
  const char *session_id;
  if (!json_object_object_get_ex(obj, "params", &params)) {
    Warning("request have not params");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(params, "meta", &meta)) {
    Warning("request have not meta");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(meta, "session_id", &child)) {
    Warning("request have not session_id");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  session_id = json_object_get_string(child);
  data = LookupSession(session_id);
  if (data == NULL) {
    Warning("session [%s] does not found", session_id);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }

  data->term = term;

  if ((ld = g_hash_table_lookup(APS_Hash, "session")) != NULL) {
    strncpy(data->hdr->window, "session_end", sizeof(data->hdr->window));
    data->hdr->widget[0] = 0;
    sprintf(data->hdr->event, "SESSIONEND");
    data->hdr->puttype = SCREEN_NULL;
    ChangeLD(data, ld);
    data->status = SESSION_STATUS_END;
    data = Process(data);
  }

  DeregisterSession(data);

  res = MakeJSONResponseTemplate(obj);
  result = json_object_new_object();
  json_object_object_add(res, "result", result);

  SendString(term->fp, (char *)json_object_to_json_string_ext(
                           res, JSON_C_TO_STRING_PLAIN));
  json_object_put(res);

  CloseNet(term->fp);
}

static void MakeErrorLog(const char *buf) {
  uuid_t u;
  char uuid[128], fname[256];

  uuid_generate(u);
  uuid_unparse(u, uuid);

  snprintf(fname, sizeof(fname), "/tmp/%s.log", uuid);
  g_file_set_contents(fname, buf, strlen(buf), NULL);
  Warning("make error log.see %s", fname);
}

static json_object *MakeEventResponse(json_object *obj, SessionData *data,
                                      unsigned long total_exec_time,
                                      unsigned long app_exec_time) {
  json_object *result, *meta, *res, *window_data, *windows, *w, *child;
  RecordStruct *rec;
  ValueStruct *val;
  LargeByteString *scrdata;
  char *buf, *wname;
  const char *puttype;
  int i;

  res = MakeJSONResponseTemplate(obj);
  result = json_object_new_object();
  json_object_object_add(res, "result", result);

  meta = json_object_new_object();
  json_object_object_add(result, "meta", meta);

  child = json_object_new_int((int)total_exec_time);
  json_object_object_add(meta, "total_exec_time", child);

  child = json_object_new_int((int)app_exec_time);
  json_object_object_add(meta, "app_exec_time", child);

  window_data = json_object_new_object();
  json_object_object_add(window_data, "focused_window",
                         json_object_new_string(data->hdr->window));
  json_object_object_add(window_data, "focused_widget",
                         json_object_new_string(data->hdr->widget));

  windows = json_object_new_array();
  for (i = 0; i < data->w.sp; i++) {
    w = json_object_new_object();

    switch (data->w.s[i].puttype) {
    case SCREEN_CURRENT_WINDOW:
      puttype = "current";
      break;
    case SCREEN_NEW_WINDOW:
      puttype = "new";
      break;
    case SCREEN_CLOSE_WINDOW:
      puttype = "close";
      break;
    default:
      Warning("invalid puttype %s:%d", data->w.s[i].window,
              data->w.s[i].puttype);
      puttype = "new";
      break;
    }
    json_object_object_add(w, "put_type", json_object_new_string(puttype));
    json_object_object_add(w, "window",
                           json_object_new_string(data->w.s[i].window));

    scrdata = GetScreenData(data, data->w.s[i].window);
    if (scrdata == NULL) {
      Warning("scrdata %s null", data->w.s[i].window);
      json_object_object_add(w, "screen_data", json_object_new_object());
    } else {
      rec = GetWindow(data->w.s[i].window);
      val = DuplicateValue(rec->value, FALSE);
      NativeUnPackValue(NULL, LBS_Body(scrdata), val);
      if ((g_hash_table_lookup(data->window_table, data->w.s[i].window)) ==
          NULL) {
        wname = g_strdup(data->w.s[i].window);
        g_hash_table_insert(data->window_table, wname, wname);
      }
      buf = xmalloc(JSON_SizeValue(NULL, val));
      JSON_PackValue(NULL, buf, val);
      child = json_tokener_parse(buf);
      if (child == NULL) {
        Warning("JSON_PackValue Error");
        MakeErrorLog(buf);
      }
      xfree(buf);
      FreeValueStruct(val);

      if (child == NULL) {
        json_object_object_add(w, "screen_data", json_object_new_object());
      } else {
        json_object_object_add(w, "screen_data", child);
      }
    }
    json_object_array_add(windows, w);
  }
  json_object_object_add(window_data, "windows", windows);
  json_object_object_add(result, "window_data", window_data);
  return res;
}

static void RPC_GetWindow(TermNode *term, json_object *obj) {
  json_object *params, *meta, *child, *res;
  SessionData *data;
  LD_Node *ld;
  const char *session_id;
  int i;
  if (!json_object_object_get_ex(obj, "params", &params)) {
    Warning("request have not params");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(params, "meta", &meta)) {
    Warning("request have not meta");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(meta, "session_id", &child)) {
    Warning("request have not session_id");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  session_id = json_object_get_string(child);
  data = LookupSession(session_id);
  if (data == NULL) {
    Warning("session [%s] does not found", session_id);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }

  if (data->fInProcess) {
    Warning("Error: %s is proccesing in other thread", session_id);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }

  if (data->hdr->puttype != SCREEN_INIT) {
    Warning("Error: %s recall get_window", session_id);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }

  if ((ld = g_hash_table_lookup(APS_Hash, ThisEnv->InitialLD)) == NULL) {
    Error("cannot find initial ld:%s.check directory", ThisEnv->InitialLD);
  }

  data->term = term;
  data->ld = ld;
  data->cWindow = ld->info->cWindow;
  data->scrdata = (LargeByteString **)xmalloc(sizeof(void *) * data->cWindow);
  for (i = 0; i < data->cWindow; i++) {
    if (data->ld->info->windows[i] != NULL) {
      dbgprintf("[%s]", data->ld->info->windows[i]->name);
      data->scrdata[i] = GetScreenData(data, data->ld->info->windows[i]->name);
    } else {
      data->scrdata[i] = NULL;
    }
  }
  data->hdr->puttype = SCREEN_NULL;
  UpdateSession(data);

  data = Process(data);

  if (data->status != SESSION_STATUS_NORMAL) {
    Warning("Error: %s Session Abort", data->hdr->uuid);
    JSONRPC_Error(term, obj, -20002, "Session Abort");
    DeregisterSession(data);
    return;
  } else {
    UpdateSession(data);
  }

  res = MakeEventResponse(obj, data, 0, 0);
  SendString(term->fp, (char *)json_object_to_json_string_ext(
                           res, JSON_C_TO_STRING_PLAIN));
  if (CheckNetFile(term->fp)) {
    CloseNet(term->fp);
  } else {
    DeregisterSession(data);
  }
  json_object_put(res);
}

static void RPC_SendEvent(TermNode *term, json_object *obj) {
  json_object *params, *meta, *child, *res, *event_data;
  SessionData *data;
  LD_Node *ld;
  RecordStruct *rec;
  ValueStruct *val;
  LargeByteString *scrdata;
  const char *session_id, *window, *widget, *event;
  size_t size;
  int i;
  unsigned long t1, t2, t3, t4;
  t1 = GetNowTime();

  if (!json_object_object_get_ex(obj, "params", &params)) {
    Warning("request have not params");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(params, "meta", &meta)) {
    Warning("request have not meta");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(meta, "session_id", &child)) {
    Warning("request have not session_id");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  session_id = json_object_get_string(child);
  data = LookupSession(session_id);
  if (data == NULL) {
    Warning("session [%s] does not found", session_id);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }

  if (data->fInProcess) {
    Warning("Error: %s is proccesing in other thread", session_id);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }

  data->term = term;

  // readterminal
  if (!json_object_object_get_ex(params, "event_data", &event_data)) {
    Warning("request have not event_data");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(event_data, "window", &child)) {
    Warning("request have not event_data->window");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  window = json_object_get_string(child);

  if (!json_object_object_get_ex(event_data, "widget", &child)) {
    Warning("request have not event_data->widget");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  widget = json_object_get_string(child);

  if (!json_object_object_get_ex(event_data, "event", &child)) {
    Warning("request have not event_data->event");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  event = json_object_get_string(child);

  if (!json_object_object_get_ex(event_data, "screen_data", &child)) {
    Warning("request have not event_data->screen_data");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }

  memset(data->hdr->window, 0, SIZE_NAME + 1);
  memset(data->hdr->widget, 0, SIZE_NAME + 1);
  memset(data->hdr->event, 0, SIZE_NAME + 1);

  strncpy(data->hdr->window, window, SIZE_NAME);
  strncpy(data->hdr->widget, widget, SIZE_NAME);
  strncpy(data->hdr->event, event, SIZE_NAME);

  ld = g_hash_table_lookup(ComponentHash, data->hdr->window);
  if (ld == NULL) {
    Warning("invalid window name:%s", data->hdr->window);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  dbgprintf("ld = [%s]", ld->info->name);
  dbgprintf("window = [%s]", data->hdr->window);

  {
    rec = GetWindow(data->hdr->window);
    scrdata = GetScreenData(data, data->hdr->window);
    val = DuplicateValue(rec->value, FALSE);
    NativeUnPackValue(NULL, LBS_Body(scrdata), val);
    JSON_UnPackValue(
        NULL,
        (char *)json_object_to_json_string_ext(child, JSON_C_TO_STRING_PLAIN),
        val);
    size = NativeSizeValue(NULL, val);
    LBS_ReserveSize(scrdata, size, FALSE);
    NativePackValue(NULL, LBS_Body(scrdata), val);
    FreeValueStruct(val);
  }

  data->hdr->puttype = SCREEN_NULL;
  data->hdr->command = APL_COMMAND_GET;
  data->term = term;
  data->retry = 0;

  UpdateSession(data);

  t2 = GetNowTime();
  data = Process(data);
  t3 = GetNowTime();

  if (data->status != SESSION_STATUS_NORMAL) {
    Warning("Error: %s Session Abort", data->hdr->uuid);
    JSONRPC_Error(term, obj, -20002, "Session Abort");
    DeregisterSession(data);
    return;
  } else {
    UpdateSession(data);
  }

  t4 = GetNowTime();
  res = MakeEventResponse(obj, data, (t4 - t1), (t3 - t2));

  for (i = 0; i < data->w.sp; i++) {
    if (data->w.s[i].puttype == SCREEN_CLOSE_WINDOW) {
      data->w.s[i].puttype = SCREEN_NULL;
    }
  }

  SendString(term->fp, (char *)json_object_to_json_string_ext(
                           res, JSON_C_TO_STRING_PLAIN));
  if (CheckNetFile(term->fp)) {
    CloseNet(term->fp);
  } else {
    DeregisterSession(data);
  }
  json_object_put(res);
}

static void RPC_PandaAPI(TermNode *term, json_object *obj) {
  json_object *params, *meta, *child, *res;
  SessionData *data;
  RecordStruct *rec;
  ValueStruct *val;
  APIData *api;
  const char *user, *host, *ld, *window;
  char *buf;
  size_t size;
  if (!json_object_object_get_ex(obj, "params", &params)) {
    Warning("request have not params");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(params, "meta", &meta)) {
    Warning("request have not meta");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(meta, "user", &child)) {
    Warning("request have not user");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  user = json_object_get_string(child);

  if (!json_object_object_get_ex(meta, "host", &child)) {
    Warning("request have not host");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  host = json_object_get_string(child);

  if (!json_object_object_get_ex(meta, "ld", &child)) {
    Warning("request have not ld");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  ld = json_object_get_string(child);

  if (!json_object_object_get_ex(meta, "window", &child)) {
    Warning("request have not window");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  window = json_object_get_string(child);

  data = InitAPISession(user, window, host);
  if (data == NULL) {
    Warning("api %s not found", ld);
    JSONRPC_Error(term, obj, -20003, "Invalid Window");
    return;
  }
  data->term = term;
  data->retry = 0;
  api = data->apidata;

  {
    rec = GetWindow((char *)window);
    val = DuplicateValue(rec->value, FALSE);
    JSON_UnPackValue(
        NULL,
        (char *)json_object_to_json_string_ext(params, JSON_C_TO_STRING_PLAIN),
        val);
    size = NativeSizeValue(NULL, val);
    LBS_ReserveSize(api->rec, size, FALSE);
    NativePackValue(NULL, LBS_Body(api->rec), val);
    FreeValueStruct(val);
  }

  data = Process(data);
  api = data->apidata;

  {
    res = MakeJSONResponseTemplate(obj);
    val = DuplicateValue(rec->value, FALSE);
    NativeUnPackValue(NULL, LBS_Body(api->rec), val);
    buf = xmalloc(JSON_SizeValue(NULL, val));

    JSON_PackValue(NULL, buf, val);
    child = json_tokener_parse(buf);
    xfree(buf);
    FreeValueStruct(val);
  }

  json_object_object_add(res, "result", child);
  json_object_object_add(res, "api_status", json_object_new_int(api->status));

  SendString(term->fp, (char *)json_object_to_json_string_ext(
                           res, JSON_C_TO_STRING_PLAIN));
  if (CheckNetFile(term->fp)) {
    CloseNet(term->fp);
  }
  FreeSessionData(data);
  json_object_put(res);
}

static json_object *ReadDownloadMetaFile(const char *metafile) {
  json_object *obj, *result;
  char *buf, *tmp;
  size_t size;

  if (g_file_get_contents(metafile, &buf, &size, NULL)) {
    tmp = realloc(buf, size + 1);
    if (tmp == NULL) {
      Error("realloc(3) failure");
    } else {
      buf = tmp;
      memset(tmp + size, 0, 1);
    }
    obj = json_tokener_parse(buf);
    if (obj != NULL) {
      if (json_object_object_get_ex(obj, "result", &result)) {
        json_object_get(result);
      } else {
        result = json_object_new_array();
      }
      json_object_put(obj);
    } else {
      result = json_object_new_array();
    }
    g_free(buf);
    if (unlink(metafile) == -1) {
      Error("unlink(2) failure %s %s", metafile, strerror(errno));
    }
  } else {
    result = json_object_new_array();
  }
  return result;
}

static json_object *CheckDownloadList(json_object *obj, SessionData *data) {
  json_object *res, *result;
  char lockfile[1024], metafile[1024];
  int fd;

  res = MakeJSONResponseTemplate(obj);

  snprintf(lockfile, sizeof(lockfile) - 1, "%s/__download.lock",
           data->hdr->tempdir);
  snprintf(metafile, sizeof(metafile) - 1, "%s/__download.json",
           data->hdr->tempdir);
  lockfile[sizeof(lockfile) - 1] = 0;
  metafile[sizeof(metafile) - 1] = 0;

  /* lock */
  if ((fd = open(lockfile, O_RDONLY)) != -1) {
    if (flock(fd, LOCK_EX | LOCK_NB) != -1) {
      /* read file */
      result = ReadDownloadMetaFile(metafile);
      /* unlock  */
      if (flock(fd, LOCK_UN) == -1) {
        Error("flock(2) failure %s %s", lockfile, strerror(errno));
      }
    } else {
      result = json_object_new_array();
    }
    if ((close(fd)) == -1) {
      Error("close(2) failure %s %s", lockfile, strerror(errno));
    }
  } else {
    result = json_object_new_array();
  }
  json_object_object_add(res, "result", result);

  return res;
}

static void RPC_ListDownloads(TermNode *term, json_object *obj) {
  json_object *params, *meta, *child, *res;
  SessionData *data;
  const char *session_id;
  if (!json_object_object_get_ex(obj, "params", &params)) {
    Warning("request have not params");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(params, "meta", &meta)) {
    Warning("request have not meta");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  if (!json_object_object_get_ex(meta, "session_id", &child)) {
    Warning("request have not session_id");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  session_id = json_object_get_string(child);
  data = LookupSession(session_id);
  if (data == NULL) {
    Warning("session [%s] does not found", session_id);
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    return;
  }
  res = CheckDownloadList(obj, data);
  SendString(term->fp, (char *)json_object_to_json_string_ext(
                           res, JSON_C_TO_STRING_PLAIN));
  if (CheckNetFile(term->fp)) {
    CloseNet(term->fp);
  }
  json_object_put(res);
}

static void JSONRPCHandler(TermNode *term) {
  char *reqjson, *method;
  json_object *obj, *child;
  reqjson = RecvStringNew(term->fp);
  ON_IO_ERROR(term->fp, badio);
  obj = json_tokener_parse(reqjson);
  xfree(reqjson);
  if (!CheckJSONObject(obj, json_type_object)) {
    Warning("jsonrpc json parse error");
    JSONRPC_Error(term, NULL, -32700, "Parse Error");
    return;
  }
  if (!json_object_object_get_ex(obj, "jsonrpc", &child)) {
    Warning("jsonrpc invalid reqeust");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    json_object_put(obj);
    return;
  }
  if (strcmp((char *)json_object_get_string(child), "2.0")) {
    Warning("jsonrpc invalid reqeust");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    json_object_put(obj);
    return;
  }
  if (!json_object_object_get_ex(obj, "id", &child)) {
    Warning("jsonrpc invalid reqeust");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    json_object_put(obj);
    return;
  }
  if (!json_object_object_get_ex(obj, "params", &child)) {
    Warning("jsonrpc invalid reqeust");
    JSONRPC_Error(term, obj, -32600, "Invalid Request");
    json_object_put(obj);
    return;
  }
  if (!json_object_object_get_ex(obj, "method", &child)) {
    Warning("jsonrpc method not found");
    JSONRPC_Error(term, obj, -32601, "Method not found");
    json_object_put(obj);
    return;
  }
  method = (char *)json_object_get_string(child);
  if (!strcmp(method, "start_session")) {
    RPC_StartSession(term, obj);
  } else if (!strcmp(method, "end_session")) {
    RPC_EndSession(term, obj);
  } else if (!strcmp(method, "get_window")) {
    RPC_GetWindow(term, obj);
  } else if (!strcmp(method, "send_event")) {
    RPC_SendEvent(term, obj);
  } else if (!strcmp(method, "panda_api")) {
    RPC_PandaAPI(term, obj);
  } else if (!strcmp(method, "list_downloads")) {
    RPC_ListDownloads(term, obj);
  } else {
    Warning("jsonrpc method(%s) not found", method);
    JSONRPC_Error(term, obj, -32601, "Method not found");
  }
  json_object_put(obj);
badio:
  return;
}

static void TermThread(TermNode *term) {
  JSONRPCHandler(term);
  FreeQueue(term->que);
  xfree(term);
}

extern int ConnectTerm(int _fhTerm) {
  int fhTerm;
  pthread_t thr;
  pthread_attr_t attr;
  TermNode *term;

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 256 * 1024);
  if ((fhTerm = accept(_fhTerm, 0, 0)) < 0) {
    Error("accept(2) failure:%s", strerror(errno));
  }
  term = New(TermNode);
  term->que = NewQueue();
  term->fp = SocketToNet(fhTerm);
  pthread_create(&thr, &attr, (void *(*)(void *))TermThread, (void *)term);
  pthread_detach(thr);
  return (fhTerm);
}

extern void InitTerm(void) {
  RecParserInit();
  if (ThisEnv->linkrec != NULL) {
    InitializeValue(ThisEnv->linkrec->value);
  }
}
