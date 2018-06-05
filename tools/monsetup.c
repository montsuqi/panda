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
#include "dbutils.h"
#include "monsys.h"
#include "bytea.h"
#include "monpushevent.h"
#include "gettext.h"
#include "option.h"
#include "enum.h"
#include "message.h"
#include "debug.h"

#define DefaultOutput "JSON"

static char *Directory;
static Bool fRECREATE = FALSE;
static int DelDays;

static ARG_TABLE option[] = {
    {"dir", STRING, TRUE, (void *)&Directory, N_("directory file name")},
    {"recreate", BOOLEAN, TRUE, (void *)&fRECREATE, "table re-create"},
    {"deldays", INTEGER, TRUE, (void *)&DelDays,
     "delete logs older than the specified number of days"},
    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  Directory = "./directory";
  fRECREATE = FALSE;
  DelDays = 50;
}

static void InitSystem(void) {
  char *dir, *env;
  int days;

  InitMessage("monsetup", NULL);
  if ((dir = getenv("MON_DIRECTORY_PATH")) != NULL) {
    Directory = dir;
  }
  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_NONE);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
  if ((env = getenv("MONBATCH_LOG_EXPIRE")) != NULL) {
    days = atoi(env);
    if (days > 0) {
      DelDays = days;
    }
  }
}

static Bool create_clog(DBG_Struct *dbg) {
  Bool rc;
  char *sql, *p;

  sql = (char *)xmalloc(SIZE_BUFF);
  p = sql;
  p += sprintf(p, "CREATE TABLE %s (", BATCH_CLOG_TABLE);
  p += sprintf(p, "       id    varchar(37),");
  p += sprintf(p, "       num   int,");
  p += sprintf(p, "       clog text,");
  p += sprintf(p, "PRIMARY KEY(id, num)");
  p += sprintf(p, ");");
  rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
  xfree(sql);
  return rc;
}

static Bool create_clog_setup(DBG_Struct *dbg) {
  Bool exist;
  int rc;
  char sql[SIZE_SQL + 1];

  TransactionStart(dbg);
  exist = table_exist(dbg, BATCH_CLOG_TABLE);
  if (!exist) {
    create_clog(dbg);
  } else if (exist && fRECREATE) {
    snprintf(sql, SIZE_SQL, "DROP TABLE %s;", BATCH_CLOG_TABLE);
    ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
    create_clog(dbg);
  }
  rc = TransactionEnd(dbg);
  return (rc == MCP_OK);
}

static Bool check_monbatch_log(DBG_Struct *dbg) {
  Bool rc;
  char *sql, *p;

  if (column_exist(dbg, BATCH_LOG_TABLE, "groupname")) {
    return TRUE;
  }
  sql = (char *)xmalloc(SIZE_BUFF);
  p = sql;
  p += sprintf(p, "ALTER TABLE %s ADD", BATCH_LOG_TABLE);
  p += sprintf(p, "       groupname    varchar(64);");
  rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
  xfree(sql);
  return rc;
}

static Bool create_monbatch_log(DBG_Struct *dbg) {
  Bool rc;
  char *sql, *p;
  sql = (char *)xmalloc(SIZE_BUFF);
  p = sql;
  p += sprintf(p, "CREATE TABLE %s (", BATCH_LOG_TABLE);
  p += sprintf(p, "       id    varchar(37)  primary key,");
  p += sprintf(p, "       pgid   int,");
  p += sprintf(p, "       rc     int,");
  p += sprintf(p, "       message varchar(512),");
  p += sprintf(p, "       starttime  timestamp  with time zone,");
  p += sprintf(p, "       endtime  timestamp  with time zone,");
  p += sprintf(p, "       tenant int,");
  p += sprintf(p, "       name varchar(256),");
  p += sprintf(p, "       groupname varchar(64),");
  p += sprintf(p, "       comment varchar(512),");
  p += sprintf(p, "       extra varchar(512),");
  p += sprintf(p, "       exwindow varchar(256),");
  p += sprintf(p, "       exwidget varchar(256),");
  p += sprintf(p, "       exevent varchar(256),");
  p += sprintf(p, "       exterm varchar(256),");
  p += sprintf(p, "       exuser varchar(256),");
  p += sprintf(p, "       exhost varchar(256),");
  p += sprintf(p, "       exec_record text");
  p += sprintf(p, ");");
  p += sprintf(p, "CREATE INDEX %s_pgid ON %s (pgid);", BATCH_LOG_TABLE,
               BATCH_LOG_TABLE);
  p += sprintf(p, "CREATE INDEX %s_start ON %s (starttime);", BATCH_LOG_TABLE,
               BATCH_LOG_TABLE);
  rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
  xfree(sql);
  return rc;
}

static Bool monbatch_log_setup(DBG_Struct *dbg) {
  Bool exist;
  int rc;
  char sql[SIZE_SQL + 1];

  TransactionStart(dbg);
  exist = table_exist(dbg, BATCH_LOG_TABLE);
  if (!exist) {
    create_monbatch_log(dbg);
  } else if (exist && fRECREATE) {
    snprintf(sql, SIZE_SQL, "DROP TABLE %s;", BATCH_LOG_TABLE);
    ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
    create_monbatch_log(dbg);
  } else if (exist && !fRECREATE) {
    check_monbatch_log(dbg);
  }
  rc = TransactionEnd(dbg);
  return (rc == MCP_OK);
}

static Bool delete_monbatch_log(DBG_Struct *dbg, int days) {
  int rc;
  char nowtime[50];
  char sql[SIZE_SQL + 1];
  timestamp(nowtime, sizeof(nowtime));

  TransactionStart(dbg);
  snprintf(sql, SIZE_SQL,
           "DELETE FROM %s AS c USING %s AS l WHERE c.id = l.id AND "
           "l.starttime + '%d days' < '%s';",
           BATCH_CLOG_TABLE, BATCH_LOG_TABLE, days, nowtime);
  snprintf(sql, SIZE_SQL, "DELETE FROM %s WHERE starttime + '%d days' < '%s';",
           BATCH_LOG_TABLE, days, nowtime);
  ExecDBOP(dbg, sql, TRUE, DB_UPDATE);

  rc = TransactionEnd(dbg);
  return (rc == MCP_OK);
}

static Bool create_monbatch(DBG_Struct *dbg) {
  Bool rc;
  char *sql, *p;

  sql = (char *)xmalloc(SIZE_BUFF);
  p = sql;
  p += sprintf(p, "CREATE TABLE %s (", BATCH_TABLE);
  p += sprintf(p, "       id    varchar(37) primary key,");
  p += sprintf(p, "       pgid   int,");
  p += sprintf(p, "       starttime  timestamp with time zone,");
  p += sprintf(p, "       tenant int,");
  p += sprintf(p, "       name varchar(256),");
  p += sprintf(p, "       groupname varchar(64),");
  p += sprintf(p, "       comment varchar(512),");
  p += sprintf(p, "       extra varchar(512),");
  p += sprintf(p, "       exwindow varchar(256),");
  p += sprintf(p, "       exwidget varchar(256),");
  p += sprintf(p, "       exevent varchar(256),");
  p += sprintf(p, "       exterm varchar(256),");
  p += sprintf(p, "       exuser varchar(256),");
  p += sprintf(p, "       exhost varchar(256)");
  p += sprintf(p, ");");
  p += sprintf(p, "CREATE INDEX %s_pgid ON %s (pgid);", BATCH_TABLE,
               BATCH_TABLE);
  p += sprintf(p, "CREATE INDEX %s_start ON %s (starttime);", BATCH_TABLE,
               BATCH_TABLE);
  rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
  xfree(sql);

  return rc;
}

static Bool check_monbatch(DBG_Struct *dbg) {
  Bool rc;
  char *sql, *p;

  if (column_exist(dbg, BATCH_TABLE, "groupname")) {
    return TRUE;
  }
  sql = (char *)xmalloc(SIZE_BUFF);
  p = sql;
  p += sprintf(p, "ALTER TABLE %s ADD", BATCH_TABLE);
  p += sprintf(p, "       groupname    varchar(64);");
  rc = ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
  xfree(sql);
  return rc;
}

static Bool monbatch_setup(DBG_Struct *dbg) {
  Bool exist;
  int rc;
  char sql[SIZE_SQL + 1];

  TransactionStart(dbg);
  exist = table_exist(dbg, BATCH_TABLE);
  if (!exist) {
    create_monbatch(dbg);
  } else if (exist && fRECREATE) {
    snprintf(sql, SIZE_SQL, "DROP TABLE %s;", BATCH_TABLE);
    ExecDBOP(dbg, sql, TRUE, DB_UPDATE);
    create_monbatch(dbg);
  } else if (exist && !fRECREATE) {
    check_monbatch(dbg);
  }
  rc = TransactionEnd(dbg);
  return (rc == MCP_OK);
}

static Bool monsysdb_setup(DBG_Struct *dbg) {
  Bool rc;

  if (OpenDB(dbg) != MCP_OK) {
    return FALSE;
  }
  rc = monbatch_setup(dbg);
  if (rc) {
    rc = monbatch_log_setup(dbg);
  }
  if (rc) {
    rc = create_clog_setup(dbg);
  }
  if (rc) {
    rc = delete_monbatch_log(dbg, DelDays);
  }
  if (rc) {
    rc = monblob_setup(dbg, fRECREATE);
  }
  if (rc) {
    rc = monpushevent_setup(dbg);
  }
  if (CloseDB(dbg) != MCP_OK) {
    return FALSE;
  }
  return rc;
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

  if (!monsysdb_setup(dbg)) {
    Error("monsetup failure");
  }
  return 0;
}
