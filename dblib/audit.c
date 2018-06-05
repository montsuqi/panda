/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_POSTGRES
#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

#include "enum.h"
#include "libmondai.h"
#include "dbgroup.h"
#include "directory.h"
#include "redirect.h"
#include "debug.h"
#include "PostgreSQLlib.h"
#include "audit.h"

static char *QueryAuditSequence(void) {
  char *buff;

  buff = (char *)xmalloc(SIZE_SQL);
  snprintf(buff, SIZE_SQL, "CREATE SEQUENCE %s_seq START 1; ", AUDITLOG_TABLE);
  return buff;
}

static char *QueryAuditTable(void) {
  char *buff, *p;

  buff = (char *)xmalloc(SIZE_SQL);
  p = buff;
  p += sprintf(p, "CREATE TABLE %s                (  ", AUDITLOG_TABLE);
  p += sprintf(p, "    id bigint PRIMARY KEY,        ");
  p += sprintf(p, "    exec_date timestamp,          ");
  p += sprintf(p, "    username varchar(%d),         ", SIZE_USER);
  p += sprintf(p, "    term varchar(%d),             ", SIZE_TERM);
  p += sprintf(p, "    windowname varchar(%d),       ", SIZE_NAME);
  p += sprintf(p, "    widget varchar(%d),           ", SIZE_NAME);
  p += sprintf(p, "    event character varying(%d),  ", SIZE_NAME);
  p += sprintf(p, "    comment text,                 ");
  p += sprintf(p, "    func varchar(%d),             ", SIZE_FUNC);
  p += sprintf(p, "    result integer,               ");
  p += sprintf(p, "    ticket_id integer,            ");
  p += sprintf(p, "    exec_query text               ");
  p += sprintf(p, ");                                ");
  return buff;
}

static Bool CreateCommand(DBG_Struct *dbg, char *sql) {
  Bool ret = FALSE;
  PGresult *res;
  res = PQexec(PGCONN(dbg, DB_UPDATE), sql);
  if ((res != NULL) && (PQresultStatus(res) == PGRES_COMMAND_OK)) {
    ret = TRUE;
  } else {
    ret = FALSE;
    Warning("PostgreSQL: %s", PQerrorMessage(PGCONN(dbg, DB_UPDATE)));
  }
  PQclear(res);
  return ret;
}

extern void CheckAuditTable(DBG_Struct *dbg) {
  PGresult *res;
  char *buff;
  Bool rc;
  int ntuples;

  buff = (char *)xmalloc(SIZE_SQL);
  snprintf(buff, SIZE_SQL,
           "SELECT tablename FROM pg_tables WHERE tablename ='%s';",
           AUDITLOG_TABLE);
  res = PQexec(PGCONN(dbg, DB_UPDATE), buff);
  xfree(buff);
  if ((res == NULL) || (PQresultStatus(res) != PGRES_TUPLES_OK)) {
    Warning("PostgreSQL: %s", PQerrorMessage(PGCONN(dbg, DB_UPDATE)));
    PQclear(res);
    return;
  }
  ntuples = PQntuples(res);
  PQclear(res);
  if (ntuples > 0) {
    return;
  }
  buff = QueryAuditTable();
  rc = CreateCommand(dbg, buff);
  xfree(buff);
  if (rc) {
    buff = QueryAuditSequence();
    CreateCommand(dbg, buff);
    xfree(buff);
  }
}

#endif /* #ifdef HAVE_POSTGRES */
