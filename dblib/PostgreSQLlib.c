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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_POSTGRES
#include	<libpq-fe.h>
#include	<libpq/libpq-fs.h>
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"PostgreSQLlib.h"
#include	"debug.h"

extern void
AddConninfo(
	LargeByteString *conninfo,
	char *item,
	char *value)
{
ENTER_FUNC;
	if		(  value  !=  NULL  )	{
		dbgprintf("%s = '%s'",item,value);
		LBS_EmitString(conninfo, item);
		LBS_EmitString(conninfo, "='");
		LBS_EmitString(conninfo, value);
		LBS_EmitString(conninfo, "'");
		LBS_EmitSpace(conninfo);
	}
LEAVE_FUNC;
}

static LargeByteString	*
_CreateConninfo(
	LargeByteString *conninfo,
	DBG_Struct	*dbg,
	int			usage)
{
	int portnum;
	char buff[SIZE_OTHER];
	
	AddConninfo(conninfo, "host", GetDB_Host(dbg,usage));
	if ( GetDB_PortName(dbg,usage) != NULL) {
		AddConninfo(conninfo, "port",GetDB_PortName(dbg,usage));
	} else {
		/* for PostgreSQL
			 db_group{
			   port "/var/run/postgresql/:5432";
			 }
			 host='/var/run/postgresql/'
			 mode='5432'
			 socket = /var/run/postgresql/.s.PGSQL.5432 */
		portnum = GetDB_PortMode(dbg,usage);
		if ( portnum != 0 ){
			snprintf(buff, SIZE_OTHER, "%d", portnum);
			AddConninfo(conninfo, "port", buff);
		}
	}
	AddConninfo(conninfo, "user", GetDB_User(dbg,usage));
	AddConninfo(conninfo, "password", GetDB_Pass(dbg,usage));
	AddConninfo(conninfo, "sslmode", GetDB_Sslmode(dbg,usage));
	return conninfo;
}
	
extern LargeByteString	*
CreateConninfo(
	DBG_Struct	*dbg,
	int			usage)
{
	LargeByteString *conninfo;

ENTER_FUNC;
	conninfo = NewLBS();
	conninfo = _CreateConninfo(conninfo, dbg, usage);
	AddConninfo(conninfo, "dbname", GetDB_DBname(dbg,usage));
	LBS_EmitEnd(conninfo);
LEAVE_FUNC;

	return conninfo;
}

extern LargeByteString	*
Template1Conninfo(
	DBG_Struct	*dbg,
	int			usage)
{
	LargeByteString *conninfo;
ENTER_FUNC;
	conninfo = NewLBS();
	conninfo = _CreateConninfo(conninfo, dbg, usage);
	AddConninfo(conninfo, "dbname", "template1");
	LBS_EmitEnd(conninfo);
LEAVE_FUNC;
	return conninfo;	
}

extern	PGconn	*
PGCONN(
	DBG_Struct	*dbg,
	int			usage)
{
	int		ix;

	ix = IsUsageUpdate(usage) ? PROCESS_UPDATE : PROCESS_READONLY;
	return	((PGconn *)dbg->process[ix].conn);
}

extern PGconn	*
PgConnect(
	DBG_Struct	*dbg,
	int			usage)
{
	LargeByteString *conninfo;
	PGconn	*conn;

	conninfo = CreateConninfo(dbg,usage);
	conn = PQconnectdb(LBS_Body(conninfo));
	FreeLBS(conninfo);
		
	if		(  PQstatus(conn)  !=  CONNECTION_OK  ) {
		Message("Connection to database \"%s\" failed.",
				GetDB_DBname(dbg,usage));
		Message("%s", PQerrorMessage(conn));
		PQfinish(conn);
		conn = NULL;
	}
	return conn;
}

extern  char	*
GetPGencoding(
	PGconn	*conn)
{
	PGresult	*res;
	char		*encoding = NULL;	
	char		*sql = "SELECT pg_encoding_to_char(encoding) " \
				" FROM pg_database " \
				"WHERE datname = current_database();";
	res = PQexec(conn, sql);
	if ( (res == NULL) || (PQresultStatus(res) != PGRES_TUPLES_OK) ) {
		Warning("PostgreSQL: %s",PQerrorMessage(conn));
	} else {
		encoding = StrDup((char *)PQgetvalue(res,0,0));
	}
	PQclear(res);
	return encoding;
}

#endif /* #ifdef HAVE_POSTGRES */
