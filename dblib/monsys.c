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

#include <uuid/uuid.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "directory.h"
#include "dbgroup.h"
#include "monsys.h"
#include "debug.h"

extern void CheckBatchExist(DBG_Struct *dbg, int pgid) {
  size_t sql_len = SIZE_SQL;
  char *sql;

  if (killpg(pgid, 0) < 0) {
    Warning("Shell: not exist [%d]", pgid);
    sql = (char *)xmalloc(sql_len);
    snprintf(sql, sql_len, "DELETE FROM %s WHERE pgid='%d';", BATCH_TABLE,
             pgid);
    ExecDBOP(dbg, sql);
    xfree(sql);
  }
}

extern void CheckBatchPg(void) {
  DBG_Struct *mondbg;
  size_t sql_len = SIZE_SQL;
  char *sql;
  ValueStruct *ret, *value;
  int i, pgid;

  mondbg = GetDBG_monsys();
  sql = (char *)xmalloc(sql_len);
  snprintf(sql, sql_len, "SELECT pgid FROM %s ;", BATCH_TABLE);
  ret = ExecDBQuery(mondbg, sql);
  xfree(sql);
  if (!ret) {
    return;
  }

  if (ValueType(ret) == GL_TYPE_ARRAY) {
    for (i = 0; i < ValueArraySize(ret); i++) {
      value = ValueArrayItem(ret, i);
      pgid = ValueToInteger(GetItemLongName(value, "pgid"));
      CheckBatchExist(mondbg, pgid);
    }
  } else {
    pgid = ValueToInteger(GetItemLongName(ret, "pgid"));
    CheckBatchExist(mondbg, pgid);
  }
  FreeValueStruct(ret);
}

extern DBG_Struct *GetDBG_monsys(void) {
  int i;
  DBG_Struct *dbg = NULL;

  if (ThisEnv) {
    for (i = 0; i < ThisEnv->cDBG; i++) {
      dbg = ThisEnv->DBG[i];
      if (!(strcmp(dbg->name, "")) && (!strcmp(dbg->type, "PostgreSQL"))) {
        break;
      }
    }
  }
  return dbg;
}

extern char *Coding_monsys(DBG_Struct *dbg, const char *src) {
  char *dest;
  ValueStruct *value;

  value = NewValue(GL_TYPE_CHAR);
  SetValueStringWithLength(value, src, strlen(src), NULL);
  dest = StrDup(ValueToString(value, dbg->coding));
  FreeValueStruct(value);

  return dest;
}

extern char *Escape_monsys(DBG_Struct *dbg, const char *src) {
  char *dest = NULL;
  ValueStruct *value, *ret, *recval, *retval;
  DBCOMM_CTRL *ctrl = NULL;
  RecordStruct *rec = NULL;

  value = NewValue(GL_TYPE_CHAR);
  SetValueStringWithLength(value, src, strlen(src), NULL);

  recval = NewValue(GL_TYPE_RECORD);
  ValueAddRecordItem(recval, "dbescapestring", value);

  retval = ExecDBESCAPE(dbg, ctrl, rec, recval);
  if ((ret = GetItemLongName(retval, "dbescapestring")) != NULL) {
    dest = StrDup(ValueToString(ret, dbg->coding));
  }
  FreeValueStruct(retval);
  FreeValueStruct(recval);
  return dest;
}

extern Bool BatchIDExist(DBG_Struct *dbg, char *id) {
  size_t sql_len = SIZE_SQL;
  char *sql;
  ValueStruct *ret;
  Bool rc;

  sql = (char *)xmalloc(sql_len);
  snprintf(sql, sql_len, "SELECT 1 FROM %s WHERE id='%s';", BATCH_LOG_TABLE,
           id);
  ret = ExecDBQuery(dbg, sql);
  xfree(sql);
  if (ret) {
    rc = TRUE;
    FreeValueStruct(ret);
  } else {
    rc = FALSE;
  }
  return rc;
}

extern char *GenUUID(void) {
  uuid_t u;
  static char *uuid;
  uuid = xmalloc(SIZE_TERM + 1);
  uuid_generate(u);
  uuid_unparse(u, uuid);
  return uuid;
}
