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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_OPENCOBOL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "const.h"
#include "libmondai.h"
#include "comm.h"
#include "directory.h"
#include "handler.h"
#include "defaults.h"
#include "enum.h"
#include "dblib.h"
#include "queue.h"
#include "libcob.h"
#define _OPENCOBOL
#include "OpenCOBOL.h"
#include "MONFUNC.h"
#include "message.h"
#include "debug.h"

static void *McpData;
static void *LinkData;
static void *SpaData;
static void *ScrData;

static char *ModuleLoadPath;

static void PutApplication(ProcessNode *node) {
  int i;
  char *scr;
  size_t size;
  long start, end;

  ENTER_FUNC;
  start = GetNowTime();
  if (node->mcprec != NULL) {
    OpenCOBOL_PackValue(OpenCOBOL_Conv, McpData, node->mcprec->value);
  }
  if (node->linkrec != NULL) {
    OpenCOBOL_PackValue(OpenCOBOL_Conv, LinkData, node->linkrec->value);
  }
  if (node->sparec != NULL) {
    OpenCOBOL_PackValue(OpenCOBOL_Conv, SpaData, node->sparec->value);
  }
  for (i = 0, scr = (char *)ScrData; i < node->cWindow; i++) {
    if (node->scrrec[i] != NULL) {
      size = OpenCOBOL_PackValue(OpenCOBOL_Conv, scr, node->scrrec[i]->value);
      scr += size;
    }
  }
  end = GetNowTime();
  TimerPrintf(start, end, "PutApplication\n");
  LEAVE_FUNC;
}

static void GetApplication(ProcessNode *node) {
  char *scr;
  int i;
  long start, end;

  ENTER_FUNC;
  start = GetNowTime();
  if (node->mcprec != NULL) {
    OpenCOBOL_UnPackValue(OpenCOBOL_Conv, McpData, node->mcprec->value);
  }
  if (node->linkrec != NULL) {
    OpenCOBOL_UnPackValue(OpenCOBOL_Conv, LinkData, node->linkrec->value);
  }
  if (node->sparec != NULL) {
    OpenCOBOL_UnPackValue(OpenCOBOL_Conv, SpaData, node->sparec->value);
  }
  for (i = 0, scr = (char *)ScrData; i < node->cWindow; i++) {
    if (node->scrrec[i] != NULL) {
      scr += OpenCOBOL_UnPackValue(OpenCOBOL_Conv, scr, node->scrrec[i]->value);
    }
  }
  end = GetNowTime();
  TimerPrintf(start, end, "GetApplication\n");
  LEAVE_FUNC;
}

static Bool _ExecuteProcess(MessageHandler *handler, ProcessNode *node) {
  int (*apl)(char *, char *, char *, char *);
  char *module;
  long start, end;
  Bool rc;

  ENTER_FUNC;
  module =
      ValueStringPointer(GetItemLongName(node->mcprec->value, "dc.module"));
  if ((apl = cob_resolve(module)) != NULL) {
    PutApplication(node);

    start = GetNowTime();
    (void)apl(McpData, SpaData, LinkData, ScrData);
    end = GetNowTime();
    TimerPrintf(start, end, "OpenCOBOL %s:%s:%s\n", module, node->widget,
                node->event);

    GetApplication(node);
    if (ValueInteger(GetItemLongName(node->mcprec->value, "rc")) < 0) {
      rc = FALSE;
    } else {
      rc = TRUE;
    }
  } else {
    Warning("%s - %s is not found.", cob_resolve_error(), module);
    rc = FALSE;
  }
  LEAVE_FUNC;
  return (rc);
}

static void _ReadyDC(MessageHandler *handler) {
  int i;
  size_t scrsize;

  ENTER_FUNC;
  OpenCOBOL_Conv = NewConvOpt();
  ConvSetSize(OpenCOBOL_Conv, ThisLD->textsize, ThisLD->arraysize);
  ConvSetCodeset(OpenCOBOL_Conv, ConvCodeset(handler->conv));
  if (handler->conv != NULL) {
    OpenCOBOL_Conv->fBigEndian = handler->conv->fBigEndian;
  }

  InitMONFUNC(OpenCOBOL_Conv, OpenCOBOL_PackValue, OpenCOBOL_UnPackValue,
              OpenCOBOL_SizeValue);

  if (ThisEnv->mcprec != NULL) {
    McpData =
        xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv, ThisEnv->mcprec->value));
    OpenCOBOL_PackValue(OpenCOBOL_Conv, McpData, ThisEnv->mcprec->value);
  } else {
    McpData = NULL;
  }
  if (ThisEnv->linkrec != NULL) {
    LinkData =
        xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv, ThisEnv->linkrec->value));
  } else {
    LinkData = NULL;
  }
  if (ThisLD->sparec != NULL) {
    SpaData =
        xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv, ThisLD->sparec->value));
  }
  scrsize = 0;
  for (i = 0; i < ThisLD->cWindow; i++) {
    if (ThisLD->windows[i] != NULL) {
      scrsize += OpenCOBOL_SizeValue(OpenCOBOL_Conv, ThisLD->windows[i]->value);
    }
  }
  ScrData = xmalloc(scrsize);
  LEAVE_FUNC;
}

static void _StopDC(MessageHandler *handler) {
  xfree(McpData);
  xfree(LinkData);
  xfree(SpaData);
  xfree(ScrData);
}

static void _StopDB(MessageHandler *handler) {
  ENTER_FUNC;
  LEAVE_FUNC;
}

static void _ReadyDB(MessageHandler *handler) {
  ENTER_FUNC;
  LEAVE_FUNC;
}

static int _StartBatch(MessageHandler *handler, char *name, char *param) {
  int (*apl)(char *);
  int rc;
  ValueStruct *val;
  char *arg;

  ENTER_FUNC;
  OpenCOBOL_Conv = NewConvOpt();
  ConvSetSize(OpenCOBOL_Conv, ThisBD->textsize, ThisBD->arraysize);
  ConvSetCodeset(OpenCOBOL_Conv, ConvCodeset(handler->conv));
  if (handler->conv != NULL) {
    OpenCOBOL_Conv->fBigEndian = handler->conv->fBigEndian;
  }

  InitMONFUNC(OpenCOBOL_Conv, OpenCOBOL_PackValue, OpenCOBOL_UnPackValue,
              OpenCOBOL_SizeValue);

#ifdef DEBUG
  printf("starting [%s][%s]\n", name, param);
#endif
  if ((apl = cob_resolve(name)) != NULL) {
    val = NewValue(GL_TYPE_CHAR);
    SetValueStringWithLength(val, param, ThisBD->textsize, NULL);
    arg = StrnDup(ValueToString(val, "euc-jisx0213"), ThisBD->textsize);
    StringC2Cobol(arg, ThisBD->textsize);
    rc = apl(arg);
    FreeValueStruct(val);
    xfree(arg);
  } else {
    Warning("%s - %s is not found.", cob_resolve_error(), name);
    rc = -1;
  }
  LEAVE_FUNC;
  return (rc);
}

static void _ReadyExecute(MessageHandler *handler, char *loadpath) {
  ENTER_FUNC;

  if (LibPath == NULL) {
    if ((ModuleLoadPath = getenv("COB_LIBRARY_PATH")) == NULL) {
      if (loadpath != NULL) {
        ModuleLoadPath = loadpath;
      } else {
        ModuleLoadPath = MONTSUQI_LOAD_PATH;
      }
    }
  } else {
    ModuleLoadPath = LibPath;
  }
  if (handler->loadpath == NULL) {
    handler->loadpath = ModuleLoadPath;
  }
  cob_init(0, NULL);
  LEAVE_FUNC;
}

static MessageHandlerClass Handler = {
    "OpenCOBOL", _ReadyExecute, _ExecuteProcess, _StartBatch, _ReadyDC,
    _StopDC,     NULL,          _ReadyDB,        _StopDB,     NULL};

extern MessageHandlerClass *OpenCOBOL(void) {
  ENTER_FUNC;
  LEAVE_FUNC;
  return (&Handler);
}
#endif
