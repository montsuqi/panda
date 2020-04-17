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

#define MAIN
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
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "libmondai.h"
#include "directory.h"
#include "dbgroup.h"
#include "monsys.h"
#include "gettext.h"
#include "option.h"
#include "enum.h"
#include "message.h"
#include "debug.h"

static char *Directory;

static ARG_TABLE option[] = {
    {"dir", STRING, TRUE, (void *)&Directory, N_("directory file name")},
    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) { Directory = "./directory"; }

static void InitSystem(void) {
  char *dir;
  InitMessage("monbatch_ps", NULL);
  if ((dir = getenv("MON_DIRECTORY_PATH")) != NULL) {
    Directory = dir;
  }
  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_NONE);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
}
static void _ViewInfo(ValueStruct *value) {
  printf("|%-5.5s|%-8.8s|%-23.23s|%-40.40s\n",
         ValueToString(GetItemLongName(value, "pgid"), NULL),
         ValueToString(GetItemLongName(value, "time"), NULL),
         ValueToString(GetItemLongName(value, "name"), NULL),
         ValueToString(GetItemLongName(value, "comment"), NULL));
}

static void ViewInfo(ValueStruct *values) {
  int i;
  ValueStruct *value;

  printf("|%-5.5s|%-8.8s|%-23.23s|%-40.40s\n", "PGID", "  TIME", " NAME",
         " COMMENT");

  if (ValueType(values) == GL_TYPE_ARRAY) {
    for (i = 0; i < ValueArraySize(values); i++) {
      value = ValueArrayItem(values, i);
      _ViewInfo(value);
    }
  } else {
    _ViewInfo(values);
  }
}

static void Batch_view(DBG_Struct *dbg) {
  size_t sql_len = SIZE_SQL;
  char *sql;
  ValueStruct *ret;

  CheckBatchPg();

  TransactionStart(dbg);

  printf("Active Batch\n");
  sql = (char *)xmalloc(sql_len);
  snprintf(sql, sql_len,
           "SELECT pgid,  date_trunc('seconds', now() - starttime) as time, "
           "name, comment FROM %s ORDER BY starttime;",
           BATCH_TABLE);
  ret = ExecDBQuery(dbg, sql);
  xfree(sql);
  if (ret) {
    ViewInfo(ret);
  }
  TransactionEnd(dbg);
}

extern int main(int argc, char **argv) {
  DBG_Struct *dbg;

  SetDefault();
  GetOption(option, argc, argv, NULL);
  InitSystem();

  dbg = GetDBG_monsys();

  if (!dbg) {
    Error("DBG does not found.");
  }

  dbg->dbt = NewNameHash();
  if (OpenDB(dbg) == MCP_OK) {
    Batch_view(dbg);
  }
  CloseDB(dbg);

  return 0;
}
