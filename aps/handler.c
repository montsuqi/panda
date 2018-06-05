/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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

/*
#define	NEW_SEQUENCT
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dlfcn.h>
#include <glib.h>
#include <unistd.h>
#include <sys/time.h>

#include "defaults.h"
#include "const.h"
#include "libmondai.h"
#include "struct.h"
#include "DDparser.h"
#include "enum.h"
#include "load.h"
#define _HANDLER
#include "dbgroup.h"
#include "handler.h"
#include "aps_main.h"
#include "apsio.h"
#include "dblib.h"
#include "glterm.h"
#include "BDparser.h"
#include "debug.h"

static char *DBSTATUS[8] = {"NORE", "CONN", "WAIT", "FAIL",
                            "DISC", "RERR", "REDL", "SYNC"};

static char *APS_HandlerLoadPath;

static GHashTable *HandlerClassTable;

static MessageHandlerClass *EnterHandlerClass(char *name) {
  void *dlhandle;
  MessageHandlerClass *klass;
  MessageHandlerClass *(*finit)(void);
  char filename[SIZE_LONGNAME + 1];

  ENTER_FUNC;
  if ((klass = g_hash_table_lookup(HandlerClassTable, name)) == NULL) {
    dbgprintf("%s handlerClass invoke.", name);
    sprintf(filename, "%s." SO_SUFFIX, name);
    klass = NULL;
    if ((dlhandle = LoadFile(APS_HandlerLoadPath, filename)) != NULL) {
      if ((finit = (void *)dlsym(dlhandle, name)) == NULL) {
        Warning("[%s] is invalid.", name);
      } else {
        klass = (*finit)();
        if (g_hash_table_lookup(HandlerClassTable, name) == NULL) {
          g_hash_table_insert(HandlerClassTable, StrDup(name), klass);
        }
      }
    } else {
      Error("[%s] not found.", name);
    }
  }
  LEAVE_FUNC;
  return (klass);
}

static void InitHandler(void) {
  ENTER_FUNC;
  HandlerClassTable = NewNameHash();
  LEAVE_FUNC;
}

static void _InitiateHandler(MessageHandler *handler) {
  MessageHandlerClass *klass;

  if ((handler->fInit & INIT_LOAD) == 0) {
    handler->fInit |= INIT_LOAD;
    if ((klass = EnterHandlerClass((char *)handler->klass)) == NULL) {
      Message("[%s] is invalid handler class.", (char *)handler->klass);
    } else {
      handler->klass = klass;
    }
    if (handler->serialize != NULL) {
      handler->serialize = GetConvFunc((char *)handler->serialize);
    }
  }
}

static void _OnlineInit(char *name, WindowBind *bind, void *dummy) {
  _InitiateHandler(bind->handler);
}

extern void InitiateHandler(void) {
  ENTER_FUNC;
  APS_HandlerLoadPath = getenv("APS_HANDLER_LOAD_PATH");
  if (APS_HandlerLoadPath == NULL) {
    if (ThisLD->handlerpath != NULL) {
      APS_HandlerLoadPath = ThisLD->handlerpath;
    } else {
      APS_HandlerLoadPath = MONTSUQI_LIBRARY_PATH;
    }
  }
  InitHandler();
  dbgprintf("LD = [%s]", ThisLD->name);
  g_hash_table_foreach(ThisLD->bhash, (GHFunc)_OnlineInit, NULL);
  LEAVE_FUNC;
}

static void _BatchInit(char *name, BatchBind *bind, void *dummy) {
  _InitiateHandler(bind->handler);
}

extern void InitiateBatchHandler(void) {
  ENTER_FUNC;
  APS_HandlerLoadPath = getenv("APS_HANDLER_LOAD_PATH");
  if (APS_HandlerLoadPath == NULL) {
    if (ThisBD->handlerpath != NULL) {
      APS_HandlerLoadPath = ThisBD->handlerpath;
    } else {
      APS_HandlerLoadPath = MONTSUQI_LIBRARY_PATH;
    }
  }
  InitHandler();
  g_hash_table_foreach(ThisBD->BatchTable, (GHFunc)_BatchInit, NULL);
  LEAVE_FUNC;
}

static void _ReadyDC(char *name, WindowBind *bind, void *dummy) {
  MessageHandler *handler;

  handler = bind->handler;
  ENTER_FUNC;
  if ((handler->fInit & INIT_EXECUTE) == 0) {
    handler->fInit |= INIT_EXECUTE;
    if (handler->klass->ReadyExecute != NULL) {
      handler->klass->ReadyExecute(handler, ThisLD->loadpath);
    }
  }
  if ((handler->fInit & INIT_READYDC) == 0) {
    handler->fInit |= INIT_READYDC;
    if (handler->klass->ReadyDC != NULL) {
      handler->klass->ReadyDC(handler);
    }
  }
  LEAVE_FUNC;
}

extern void ReadyDC(void) {
  ENTER_FUNC;
  g_hash_table_foreach(ThisLD->bhash, (GHFunc)_ReadyDC, NULL);
  LEAVE_FUNC;
}

extern void ReadyHandlerDB(MessageHandler *handler) {
  if ((handler->fInit & INIT_READYDB) == 0) {
    handler->fInit |= INIT_READYDB;
    if (handler->klass->ReadyDB != NULL) {
      handler->klass->ReadyDB(handler);
    }
  }
}

static void _ReadyOnlineDB(char *name, WindowBind *bind, void *dummy) {
  ReadyHandlerDB(bind->handler);
}

extern int ReadyOnlineDB(char *application_name) {
  int rc;
  ENTER_FUNC;
  InitDB_Process(application_name);
  rc = OpenDB(NULL);
  g_hash_table_foreach(ThisLD->bhash, (GHFunc)_ReadyOnlineDB, NULL);
  LEAVE_FUNC;
  return rc;
}

static const char *CommandString(char command) {
  switch (command) {
  case APL_COMMAND_LINK:
    return "LINK";
  case APL_COMMAND_GET:
    return "PUTG";
  default:
    return "";
  }
  return "";
}

#ifdef DEBUG
static const char *strputtype(unsigned char p) {
  switch (p) {
  case SCREEN_CURRENT_WINDOW:
    return "current";
  case SCREEN_NEW_WINDOW:
    return "new";
  case SCREEN_CLOSE_WINDOW:
    return "close";
  case SCREEN_CHANGE_WINDOW:
    return "change";
  case SCREEN_JOIN_WINDOW:
    return "join";
  case SCREEN_NULL:
    return "null";
  default:
    return "nondef";
  }
}
#endif

static void CallBefore(WindowBind *bind, ProcessNode *node) {
  ValueStruct *mcp;
  WindowStack w;
  int i, sp;

  ENTER_FUNC;
#ifdef DEBUG
  fprintf(stderr, "==== callbefore 1\n");
  for (i = 0; i < node->w.sp; i++) {
    fprintf(stderr, "%02d %-16s %s\n", i, node->w.s[i].window,
            strputtype(node->w.s[i].puttype));
  }
#endif
  mcp = node->mcprec->value;
  SetValueString(GetItemLongName(mcp, "dc.status"),
                 CommandString(node->command), NULL);
  dbgprintf("node->dbstatus = %d", node->dbstatus);
  SetValueString(GetItemLongName(mcp, "dc.dbstatus"),
                 DBSTATUS[(int)node->dbstatus], NULL);
  SetValueInteger(GetItemLongName(mcp, "db.rcount"), 0);
  SetValueInteger(GetItemLongName(mcp, "db.limit"), 1);
  node->thisscrrec = bind->rec;

  /* SCREEN_NULLを除去  */
  w.sp = 0;
  for (i = 0; i < node->w.sp; i++) {
    if (node->w.s[i].puttype != SCREEN_NULL) {
      w.s[w.sp].puttype = node->w.s[i].puttype;
      strncpy(w.s[w.sp].window, node->w.s[i].window, SIZE_NAME);
      w.sp++;
    }
  }
  node->w.sp = w.sp;
  for (i = 0; i < w.sp; i++) {
    node->w.s[i].puttype = w.s[i].puttype;
    strncpy(node->w.s[i].window, w.s[i].window, SIZE_NAME);
  }

  /* windowがあるかどうかチェック */
  sp = -1;
  for (i = 0; i < node->w.sp; i++) {
    if (!strcmp(node->window, node->w.s[i].window)) {
      sp = i;
      break;
    }
  }
  /* なければtopに追加 */
  if (sp == -1 && strlen(node->window) > 0) {
    if (node->w.sp > WINDOW_STACK_SIZE) {
      Warning("window stack size over[%d],can't add [%s]", WINDOW_STACK_SIZE,
              node->window);
      Warning("---- window stack ----");
      for (i = 0; i < node->w.sp; i++) {
        Warning("[%02d] [%s] [%d]", i, node->w.s[i].window,
                node->w.s[i].puttype);
      }
      Warning("---- window stack end ----");
      return;
    }
    strncpy(node->w.s[node->w.sp].window, node->window, SIZE_NAME);
    node->w.s[node->w.sp].puttype = node->puttype;
    node->w.sp++;
  }
#ifdef DEBUG
  fprintf(stderr, "==== callbefore 2\n");
  for (i = 0; i < node->w.sp; i++) {
    fprintf(stderr, "%02d %-16s %s\n", i, node->w.s[i].window,
            strputtype(node->w.s[i].puttype));
  }
#endif
  LEAVE_FUNC;
}

static void CallAfter(ProcessNode *node) {
  int i, dc_sp, org_sp;
  char *dc_puttype;
  char *dc_window;
  gboolean has_window;
  ENTER_FUNC;
#ifdef DEBUG
  fprintf(stderr, "---- callafter 1 ----\n");
  for (i = 0; i < node->w.sp; i++) {
    fprintf(stderr, "%02d %-16s %s\n", i, node->w.s[i].window,
            strputtype(node->w.s[i].puttype));
  }
  fprintf(stderr, "---- callafter 1 end ----\n");
#endif
  dc_window =
      ValueStringPointer(GetItemLongName(node->mcprec->value, "dc.window"));
  dc_puttype =
      ValueStringPointer(GetItemLongName(node->mcprec->value, "dc.puttype"));
  if (!strcasecmp(dc_puttype, "CURRENT")) {
    node->puttype = SCREEN_CURRENT_WINDOW;
  } else if (!strcasecmp(dc_puttype, "NEW")) {
    node->puttype = SCREEN_NEW_WINDOW;
  } else if (!strcasecmp(dc_puttype, "CLOSE")) {
    node->puttype = SCREEN_CLOSE_WINDOW;
  } else if (!strcasecmp(dc_puttype, "CHANGE")) {
    node->puttype = SCREEN_CHANGE_WINDOW;
  } else if (!strcasecmp(dc_puttype, "JOIN")) {
    node->puttype = SCREEN_JOIN_WINDOW;
  } else {
    node->puttype = SCREEN_CURRENT_WINDOW;
  }

#ifdef DEBUG
  fprintf(stderr, "[%s] -> [%s] [%s]\n", node->window, dc_window,
          strputtype(node->puttype));
#endif

  /* org window sp*/
  org_sp = -1;
  for (i = 0; i < node->w.sp; i++) {
    if (!strcmp(node->window, node->w.s[i].window)) {
      org_sp = i;
      break;
    }
  }

  /* windowがあるかどうかチェック */
  has_window = FALSE;
  for (i = 0; i < node->w.sp; i++) {
    if (!strcmp(dc_window, node->w.s[i].window)) {
      dc_sp = i;
      has_window = TRUE;
      node->w.s[i].puttype = node->puttype;
      break;
    }
  }
  if (!has_window) {
    /* なければadd */
    if (node->w.sp > WINDOW_STACK_SIZE) {
      Warning("window stack size over[%d],can't add [%s]", WINDOW_STACK_SIZE,
              dc_window);
      Warning("---- window stack ----");
      for (i = 0; i < node->w.sp; i++) {
        Warning("[%02d] [%s] [%d]", i, node->w.s[i].window,
                node->w.s[i].puttype);
      }
      Warning("---- window stack end ----");
      return;
    }
    strncpy(node->w.s[node->w.sp].window, dc_window, SIZE_NAME);
    node->w.s[node->w.sp].puttype = node->puttype;
    dc_sp = node->w.sp;
    node->w.sp++;
  }

  switch (node->puttype) {
  case SCREEN_CLOSE_WINDOW:
    /* 前のリスト要素をCLOSEに  */
    node->w.s[dc_sp].puttype = SCREEN_CLOSE_WINDOW;
    break;
  case SCREEN_CURRENT_WINDOW:
  case SCREEN_NEW_WINDOW:
    /* do nothing  */
    break;
  case SCREEN_CHANGE_WINDOW:
    /* ひとつ前のウィンドウをCLOSE  */
    if (org_sp != -1) {
      node->w.s[org_sp].puttype = SCREEN_CLOSE_WINDOW;
    }
    node->w.s[dc_sp].puttype = SCREEN_CURRENT_WINDOW;
    break;
  case SCREEN_JOIN_WINDOW:
    if (has_window) {
      for (i = dc_sp + 1; i < node->w.sp; i++) {
        node->w.s[i].puttype = SCREEN_CLOSE_WINDOW;
      }
    } else {
      for (i = 0; i < dc_sp; i++) {
        node->w.s[i].puttype = SCREEN_CLOSE_WINDOW;
      }
    }
    node->w.s[dc_sp].puttype = SCREEN_CURRENT_WINDOW;
    break;
  }
#ifdef DEBUG
  fprintf(stderr, "---- callafter 2 ----\n");
  for (i = 0; i < node->w.sp; i++) {
    fprintf(stderr, "%02d %-16s %s\n", i, node->w.s[i].window,
            strputtype(node->w.s[i].puttype));
  }
  fprintf(stderr, "---- callafter 2 end ----\n");
#endif
  LEAVE_FUNC;
}

extern void ExecuteProcess(ProcessNode *node) {
  WindowBind *bind;
  MessageHandler *handler;
  char *window;
  long start, end;

  ENTER_FUNC;
  window =
      ValueStringPointer(GetItemLongName(node->mcprec->value, "dc.window"));
  dbgprintf("window [%s]", window);
  bind = (WindowBind *)g_hash_table_lookup(ThisLD->bhash, window);
  if (bind == NULL) {
    Warning("invalid event request [%s]", window);
    exit(2);
  }
  dbgprintf("calling [%s] widget [%s] event [%s]", bind->module, node->widget,
            node->event);
  handler = bind->handler;
  if (((MessageHandlerClass *)bind->handler)->ExecuteDC == NULL) {
    Warning("No such handler [%s] widget [%s] event [%s]", bind->module,
            node->widget, node->event);
    exit(2);
  }

  CallBefore(bind, node);
  start = GetNowTime();
  /* _ExecuteProcess */
  if (!(handler->klass->ExecuteDC(handler, node))) {
    Warning("Illegular execution [%s] widget [%s] event [%s]", bind->module,
            node->widget, node->event);
    exit(2);
  }
  end = GetNowTime();
  TimerPrintf(start, end, "aps %s %s:%s:%s:%s\n", node->user, bind->module,
              window, node->widget, node->event);
  CallAfter(node);
  LEAVE_FUNC;
}

static void StopHandlerDC(MessageHandler *handler) {
  ENTER_FUNC;
  if (((handler->fInit & INIT_READYDC) != 0) &&
      ((handler->fInit & INIT_STOPDC) == 0)) {
    handler->fInit |= INIT_STOPDC;
    if (handler->klass->StopDC != NULL) {
      handler->klass->StopDC(handler);
    }
  }
  LEAVE_FUNC;
}

static void _StopDC(char *name, WindowBind *bind, void *dummy) {
  ENTER_FUNC;
  dbgprintf("name = [%s]", name);
  StopHandlerDC(bind->handler);
  LEAVE_FUNC;
}

extern void StopDC(void) {
  ENTER_FUNC;
  g_hash_table_foreach(ThisLD->bhash, (GHFunc)_StopDC, NULL);
  LEAVE_FUNC;
}

static void CleanUpHandlerDC(MessageHandler *handler) {
  if (((handler->fInit & INIT_READYDC) != 0) &&
      ((handler->fInit & INIT_CLEANDC) == 0)) {
    handler->fInit |= INIT_CLEANDC;
    if (handler->klass->CleanUpDC != NULL) {
      handler->klass->CleanUpDC(handler);
    }
  }
}

static void _CleanUpOnlineDC(char *name, WindowBind *bind, void *dummy) {
  CleanUpHandlerDC(bind->handler);
}

extern void CleanUpOnlineDC(void) {
  g_hash_table_foreach(ThisLD->bhash, (GHFunc)_CleanUpOnlineDC, NULL);
}

extern void CleanUpHandlerDB(MessageHandler *handler) {
  if (((handler->fInit & INIT_READYDB) != 0) &&
      ((handler->fInit & INIT_CLEANDB) == 0)) {
    handler->fInit |= INIT_CLEANDB;
    if (handler->klass->CleanUpDB != NULL) {
      handler->klass->CleanUpDB(handler);
    }
  }
}

static void _CleanUpOnlineDB(char *name, WindowBind *bind, void *dummy) {
  CleanUpHandlerDB(bind->handler);
}

extern void CleanUpOnlineDB(void) {
  g_hash_table_foreach(ThisLD->bhash, (GHFunc)_CleanUpOnlineDB, NULL);
}

extern void StopHandlerDB(MessageHandler *handler) {
  ENTER_FUNC;
  if (((handler->fInit & INIT_READYDB) != 0) &&
      ((handler->fInit & INIT_STOPDB) == 0)) {
    handler->fInit |= INIT_STOPDB;
    if (handler->klass->StopDB != NULL) {
      handler->klass->StopDB(handler);
    }
  }
  LEAVE_FUNC;
}

static void _StopOnlineDB(char *name, WindowBind *bind, void *dummy) {
  ENTER_FUNC;
  StopHandlerDB(bind->handler);
  LEAVE_FUNC;
}

extern void StopOnlineDB(void) {
  ENTER_FUNC;
  CloseDB(NULL);
  g_hash_table_foreach(ThisLD->bhash, (GHFunc)_StopOnlineDB, NULL);
  LEAVE_FUNC;
}

#if 0
static	void
CleanUpDC(
	MessageHandler	*handler)
{
	if		(	(  ( handler->fInit & INIT_READYDC  )  !=  0  )
			&&	(  ( handler->fInit & INIT_CLEANDC )  ==  0  ) ) {
		handler->fInit |= INIT_CLEANDC;
		if		(  handler->klass->CleanUpDC  !=  NULL  ) {
			handler->klass->CleanUpDC(handler);
		}
	}
}
#endif

extern int StartBatch(char *name, char *para) {
  MessageHandler *handler;
  BatchBind *bind;
  int rc;

  ENTER_FUNC;
  dbgprintf("calling [%s]", name);
  if ((bind = g_hash_table_lookup(ThisBD->BatchTable, name)) == NULL) {
    Error("%s application is not in BD.\n", name);
  }
  handler = bind->handler;
  if (handler->klass->ReadyExecute != NULL) {
    handler->klass->ReadyExecute(handler, ThisBD->loadpath);
  }
  if (handler->klass->ExecuteBatch != NULL) {
    rc = handler->klass->ExecuteBatch(handler, name, para);
  } else {
    rc = -1;
    Warning("%s is handler not support batch.\n", name);
  }
  LEAVE_FUNC;
  return (rc);
}

/*
 *	handler misc functions
 */
extern void ExpandStart(char *line, char *start, char *path, char *module,
                        char *param) {
  char *p, *q;

  p = start;
  q = line;

  while (*p != 0) {
    if (*p == '%') {
      p++;
      switch (*p) {
      case 'm':
        q += sprintf(q, "%s", module);
        break;
      case 'p':
        q += sprintf(q, "%s", path);
        break;
      case 'a':
        q += sprintf(q, "%s", param);
        break;
      default:
        *q++ = *p;
        break;
      }
    } else {
      *q++ = *p;
    }
    p++;
  }
  *q = 0;
}

extern void ReadyExecuteCommon(MessageHandler *handler) {}
