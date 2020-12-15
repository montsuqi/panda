/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan.
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
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <unistd.h>

#include <uuid/uuid.h>

#include "const.h"
#include "enum.h"
#include "libmondai.h"
#include "directory.h"
#include "wfcdata.h"
#include "dbgroup.h"
#include "monsys.h"
#include "bytea.h"
#include "dbops.h"
#include "debug.h"

static ValueStruct *_NewBLOB(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                             RecordStruct *rec, ValueStruct *args) {
  ValueStruct *ret, *val;
  char *sql;
  int rc;
  size_t sql_len = SIZE_SQL;
  DBG_Struct *mondbg;
  monblob_struct *monblob;
  mondbg = GetDBG_monsys();
  monblob = new_monblob_struct(mondbg, NULL);
  sql = xmalloc(sql_len);
  snprintf(sql, sql_len,
           "INSERT INTO %s (id, blobid, status) VALUES('%s', '%u', '%d');",
           MONBLOB, monblob->id, 1, 503);
  rc = ExecDBOP(mondbg, sql);
  xfree(sql);
  if ((val = GetItemLongName(args, "id")) != NULL) {
    SetValueString(val, monblob->id, dbg->coding);
  }
  free_monblob_struct(monblob);
  if (ctrl != NULL) {
    ctrl->rc = rc;
  }
  ret = DuplicateValue(args, TRUE);
  return (ret);
}

static ValueStruct *_ImportBLOB(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                RecordStruct *rec, ValueStruct *args) {
  ValueStruct *ret;
  ValueStruct *val;
  DBG_Struct *mondbg;
  char *id = NULL;
  char *rid = NULL;
  int persist = 0;
  char *filename = NULL;
  char *content_type = NULL;
  int rc;
  int _lifetype;
  unsigned int lifetype = 1;

  mondbg = GetDBG_monsys();
  if ((val = GetItemLongName(args, "id")) != NULL) {
    id = ValueToString(val, dbg->coding);
  }
  if ((val = GetItemLongName(args, "persist")) != NULL) {
    persist = ValueToInteger(val);
  }
  if ((val = GetItemLongName(args, "filename")) != NULL) {
    filename = ValueToString(val, dbg->coding);
  }
  if ((val = GetItemLongName(args, "content_type")) != NULL) {
    content_type = ValueToString(val, dbg->coding);
  }
  if ((val = GetItemLongName(args, "lifetype")) != NULL) {
    _lifetype = ValueToInteger(val);
    if (_lifetype < 0) {
      lifetype = 1;
    } else {
      lifetype = _lifetype;
    }
  }
  rid = monblob_import(mondbg, id, persist, filename, content_type, lifetype);
  if ((rid != NULL) && (val = GetItemLongName(args, "id")) != NULL) {
    SetValueString(val, rid, dbg->coding);
    xfree(rid);
    rc = MCP_OK;
  } else {
    rc = MCP_BAD_OTHER;
  }
  if (ctrl != NULL) {
    ctrl->rc = rc;
  }
  ret = DuplicateValue(args, TRUE);
  return (ret);
}

static ValueStruct *_ExportBLOB(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                RecordStruct *rec, ValueStruct *args) {
  ValueStruct *ret;
  ValueStruct *val;
  DBG_Struct *mondbg;
  char *id = NULL;
  char *filename = NULL;
  int rc;

  mondbg = GetDBG_monsys();
  if ((val = GetItemLongName(args, "id")) != NULL) {
    id = ValueToString(val, dbg->coding);
  }
  if ((val = GetItemLongName(args, "filename")) != NULL) {
    filename = ValueToString(val, dbg->coding);
  }
  if (monblob_export_file(mondbg, id, filename)) {
    rc = MCP_OK;
  } else {
    rc = MCP_BAD_OTHER;
  }
  if (ctrl != NULL) {
    ctrl->rc = rc;
  }
  ret = DuplicateValue(args, TRUE);
  return (ret);
}

static ValueStruct *_PersistBLOB(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                 RecordStruct *rec, ValueStruct *args) {
  ValueStruct *ret;
  ValueStruct *val;
  DBG_Struct *mondbg;
  char *id = NULL;
  char *filename = NULL;
  char *content_type = NULL;
  int rc;

  mondbg = GetDBG_monsys();
  rc = MCP_BAD_ARG;
  if ((val = GetItemLongName(args, "id")) != NULL) {
    id = ValueToString(val, dbg->coding);
    if ((val = GetItemLongName(args, "filename")) != NULL) {
      filename = ValueToString(val, dbg->coding);
    }
    if ((val = GetItemLongName(args, "content_type")) != NULL) {
      content_type = ValueToString(val, dbg->coding);
    }
    if (monblob_persist(mondbg, id, filename, content_type, 1)) {
      rc = MCP_OK;
    } else {
      rc = MCP_BAD_OTHER;
    }
  }
  if (ctrl != NULL) {
    ctrl->rc = rc;
  }
  ret = DuplicateValue(args, TRUE);
  return (ret);
}

static ValueStruct *_DestroyBLOB(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                 RecordStruct *rec, ValueStruct *args) {
  DBG_Struct *mondbg;
  ValueStruct *ret;
  ValueStruct *val;
  char *id;
  int rc;

  mondbg = GetDBG_monsys();
  rc = MCP_BAD_ARG;
  if ((val = GetItemLongName(args, "id")) != NULL) {
    id = ValueToString(val, dbg->coding);
    if (monblob_delete(mondbg, id)) {
      rc = MCP_OK;
    } else {
      rc = MCP_BAD_OTHER;
    }
  }
  if (ctrl != NULL) {
    ctrl->rc = rc;
  }
  ret = NULL;
  return (ret);
}

static DB_OPS Operations[] = {
    /*	DB operations		*/
    {"DBOPEN", (DB_FUNC)_DBOPEN},
    {"DBDISCONNECT", (DB_FUNC)_DBDISCONNECT},
    {"DBSTART", (DB_FUNC)_DBSTART},
    {"DBCOMMIT", (DB_FUNC)_DBCOMMIT},
    /*	table operations	*/
    {"MONBLOBNEW", _NewBLOB},
    {"MONBLOBIMPORT", _ImportBLOB},
    {"MONBLOBEXPORT", _ExportBLOB},
    {"MONBLOBPERSIST", _PersistBLOB},
    {"MONBLOBDESTROY", _DestroyBLOB},
    {NULL, NULL}};

static DB_Primitives Core = {
    _EXEC,
    _DBACCESS,
    NULL,
};

extern DB_Func *InitMONBLOB(void) {
  return (
      EnterDB_Function("MONBLOB", Operations, DB_PARSER_NULL, &Core, "", ""));
}
