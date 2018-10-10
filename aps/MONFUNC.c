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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#include "const.h"
#include "libmondai.h"
#include "dblib.h"
#include "dbgroup.h"
#include "handler.h"
#include "directory.h"
#include "MONFUNC.h"
#include "message.h"
#include "debug.h"

static MONFUNC_PackFunc PackFunc;
static MONFUNC_UnPackFunc UnPackFunc;
static MONFUNC_SizeFunc SizeFunc;
static CONVOPT *Conv;

static void ReturnBackMCP(ValueStruct *mcp, ValueStruct *mcporg) {
  CopyValue(GetItemLongName(mcp, "dc.module"),
            GetItemLongName(mcporg, "dc.module"));
  CopyValue(GetItemLongName(mcp, "dc.user"),
            GetItemLongName(mcporg, "dc.user"));
  CopyValue(GetItemLongName(mcp, "dc.term"),
            GetItemLongName(mcporg, "dc.term"));
}

static ValueStruct *UnPackMCP(char *mcpdata) {
  ValueStruct *mcp;
  ValueStruct *mcporg;

  mcp = ThisEnv->mcprec->value;
  mcporg = DuplicateValue(mcp, TRUE);
  UnPackFunc(Conv, mcpdata, mcp);
  ReturnBackMCP(mcp, mcporg);
  FreeValueStruct(mcporg);
  return mcp;
}

static void CTRLtoMCP(ValueStruct *mcp, DBCOMM_CTRL *ctrl) {
  strcpy(ValueStringPointer(GetItemLongName(mcp, "func")), ctrl->func);
  strcpy(ValueStringPointer(GetItemLongName(mcp, "db.table")), ctrl->rname);
  strcpy(ValueStringPointer(GetItemLongName(mcp, "db.pathname")), ctrl->pname);
  SetValueInteger(GetItemLongName(mcp, "rc"), ctrl->rc);
  SetValueInteger(GetItemLongName(mcp, "db.rcount"), ctrl->rcount);
  SetValueInteger(GetItemLongName(mcp, "db.limit"), ctrl->limit);
}

static void MCPtoCTRL(DBCOMM_CTRL *ctrl, ValueStruct *mcp) {
  char *rname, *pname;

  strncpy(ctrl->func, ValueStringPointer(GetItemLongName(mcp, "func")),
          SIZE_FUNC);
  strncpy(ctrl->user, ValueStringPointer(GetItemLongName(mcp, "dc.user")),
          SIZE_USER);
  strncpy(ctrl->term, ValueStringPointer(GetItemLongName(mcp, "dc.term")),
          SIZE_TERM);
  if (ValueInteger(GetItemLongName(mcp, "version")) == 2) {
    ctrl->limit = ValueInteger(GetItemLongName(mcp, "db.limit"));
    ctrl->redirect = ValueInteger(GetItemLongName(mcp, "db.redirect"));
  }
  if (!IsDBOperation(ctrl->func)) {
    rname = ValueStringPointer(GetItemLongName(mcp, "db.table"));
    SetDBCTRLRecord(ctrl, rname);
    pname = ValueStringPointer(GetItemLongName(mcp, "db.pathname"));
    SetDBCTRLValue(ctrl, pname);
  }
}

static int MonDBOperation(ValueStruct *mcp, char *func) {
  DBCOMM_CTRL ctrl;
  InitializeCTRL(&ctrl);
  MCPtoCTRL(&ctrl, mcp);
  ctrl.fDBOperation = TRUE;
  ExecDB_Process(&ctrl, ctrl.rec, ctrl.value);
  CTRLtoMCP(mcp, &ctrl);
  return ctrl.rc;
}

static int MonDBFunc(ValueStruct *mcp, char *func, char *data,
                     ValueStruct **retval) {
  DBCOMM_CTRL ctrl;
  *retval = NULL;
  InitializeCTRL(&ctrl);

  MCPtoCTRL(&ctrl, mcp);
  if (IsDBUpdateFunc(ctrl.func)) {
    UnPackFunc(Conv, data, ctrl.value);
  }
  *retval = ExecDB_Process(&ctrl, ctrl.rec, ctrl.value);
  CTRLtoMCP(mcp, &ctrl);
  return ctrl.rc;
}

static int MonLOGFunc(ValueStruct *mcp) {
  SetValueInteger(GetItemLongName(mcp, "rc"), 0);
  return 0;
}

static int MonGLFunc(ValueStruct *mcp) {
  strcpy(ValueStringPointer(GetItemLongName(mcp, "dc.status")), "PUTG");
  SetValueInteger(GetItemLongName(mcp, "rc"), 0);
  return 0;
}

static int _MONFUNC(char *mcpdata, char *data, ValueStruct **retval) {
  ValueStruct *mcp;
  ValueStruct *audit;
  char *func;
  int ret;

  *retval = NULL;
  mcp = UnPackMCP(mcpdata);

  audit = ThisEnv->auditrec->value;
  InitializeValue(audit);
  func = ValueStringPointer(GetItemLongName(mcp, "func"));
  if (!strcmp(func, "PUTWINDOW")) {
    ret = MonGLFunc(mcp);
  } else if (!strcmp(func, "AUDITLOG")) {
    ret = MonLOGFunc(mcp);
  } else if (IsDBOperation(func)) {
    ret = MonDBOperation(mcp, func);
  } else {
    ret = MonDBFunc(mcp, func, data, retval);
  }
  if (ValueInteger(GetItemLongName(mcp, "db.logflag")) > 0) {
    AuditLog(mcp);
    SetValueInteger(GetItemLongName(mcp, "db.logflag"),0);
  }
#ifdef DEBUG
  DumpValueStruct(mcp);
#endif
  return ret;
}

extern void InitMONFUNC(CONVOPT *conv, MONFUNC_PackFunc packfunc,
                        MONFUNC_UnPackFunc unpackfunc,
                        MONFUNC_SizeFunc sizefunc) {
  Conv = conv;
  PackFunc = packfunc;
  UnPackFunc = unpackfunc;
  SizeFunc = sizefunc;
}

extern int MONFUNC(char *mcpdata, char *data) {
  int rc;
  ValueStruct *retval;
  ValueStruct *mcp;

  rc = _MONFUNC(mcpdata, data, &retval);
  mcp = ThisEnv->mcprec->value;
  PackFunc(Conv, mcpdata, mcp);
  if (retval != NULL) {
    if (rc == MCP_OK) {
      PackFunc(Conv, data, retval);
    }
    FreeValueStruct(retval);
  }
  return rc;
}
