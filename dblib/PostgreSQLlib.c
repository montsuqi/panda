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

extern	void
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
	DBG_Class	*dbg,
	int			usage)
{
	AddConninfo(conninfo, "host", GetDB_Host(dbg,usage));
	AddConninfo(conninfo, "port", GetDB_PortName(dbg,usage));
	AddConninfo(conninfo, "user", GetDB_User(dbg,usage));
	AddConninfo(conninfo, "password", GetDB_Pass(dbg,usage));
	AddConninfo(conninfo, "sslmode", GetDB_Sslmode(dbg,usage));
	return conninfo;
}
	
extern LargeByteString	*
CreateConninfo(
	DBG_Class	*dbg,
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
	DBG_Class	*dbg,
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
	DBG_Instance	*dbg,
	int				usage)
{
	DB_Process	process;

	process = IsUsageUpdate(usage) ? dbg->update : dbg->readonly;
	return	((PGconn *)process.conn);
}

extern PGconn	*
PgConnect(
	DBG_Instance	*dbg,
	int				usage)
{
	DB_Process		*process;
	LargeByteString *conninfo;
	PGconn	*conn;
	
	process = IsUsageUpdate(usage) ? &dbg->update : &dbg->readonly;
	conninfo = CreateConninfo(dbg->class,usage);
	conn = PQconnectdb(LBS_Body(conninfo));
	FreeLBS(conninfo);

	if		(  PQstatus(conn)  ==  CONNECTION_OK  ) {
		Message("Connection to database \"%s\".", GetDB_DBname(dbg->class,usage));
		process->conn = (void *)conn;
		process->dbstatus = DB_STATUS_CONNECT;
	} else {
		Message("Connection to database \"%s\" failed.", GetDB_DBname(dbg->class,usage));
		Message("%s", PQerrorMessage(conn));
		PQfinish(conn);
		conn = NULL;
	}
	return conn;
}

#endif /* #ifdef HAVE_POSTGRES */
