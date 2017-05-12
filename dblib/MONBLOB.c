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
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<signal.h>

#include	<uuid/uuid.h>

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
#include	"monsys.h"
#include	"comm.h"
#include	"comms.h"
#include	"redirect.h"
#include	"debug.h"

#define 	MONBLOB	"monblobapi"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->process[PROCESS_UPDATE].conn)

static void
SendMONBLOBValue(
	ValueStruct	*val,
	char *tempsocket)
{
	Port	*port;
	int		fd, _fd;
	NETFILE	*fp;
	char *buf;

	port = ParPortName(tempsocket);
	_fd =InitServerPort(port,1);
	if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
		Error("INET Domain Accept");
	}
	fp = SocketToNet(fd);
	buf = xmalloc(JSON_SizeValue(NULL,val));
	JSON_PackValue(NULL,buf,val);
	SendString(fp, buf);
	xfree(buf);
	close(_fd);
	CloseNet(fp);
	CleanUNIX_Socket(port);
}

static	ValueStruct	*
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	OpenDB_RedirectPort(dbg);
	dbg->process[PROCESS_UPDATE].conn = xmalloc((SIZE_ARG)*sizeof(char *));
	dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;
	dbg->process[PROCESS_READONLY].dbstatus = DB_STATUS_NOCONNECT;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  dbg->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT ) {
		xfree(dbg->process[PROCESS_UPDATE].conn);
		CloseDB_RedirectPort(dbg);
		dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_DISCONNECT;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	BeginDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	CheckDB_Redirect(dbg);
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_NewBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret, *val;
	uuid_t	u;
	char *id;
	char *sql;
	size_t	sql_len = SIZE_SQL;
	DBG_Struct		*mondbg;
ENTER_FUNC;
	id = xmalloc(SIZE_TERM+1);
	uuid_generate(u);
	uuid_unparse(u, id);
	mondbg = GetDBG_monsys();
	sql = xmalloc(sql_len);
	snprintf(sql, sql_len, "INSERT INTO monblobapi (id, status) VALUES('%s', '%d');", id , 503);
	ExecDBOP(mondbg, sql, FALSE, DB_UPDATE);
	xfree(sql);

	if ((val = GetItemLongName(args, "id")) != NULL) {
		SetValueString(val, id, dbg->coding);
	}
	xfree(id);
	ret = DuplicateValue(args,TRUE);
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_ImportBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;
	ValueStruct	*val;
	char *id = NULL;
	char *filename = NULL;
	pid_t	pid;
	char tempdir[PATH_MAX];
	char tempsocket[PATH_MAX];
	char *cmd;

ENTER_FUNC;
	if ((val = GetItemLongName(args, "id")) != NULL) {
		id = ValueToString(val,dbg->coding);
	}
	if ((val = GetItemLongName(args, "filename")) != NULL) {
		filename = ValueToString(val,dbg->coding);
	}
	snprintf(tempdir, PATH_MAX, "/tmp/blobapi_XXXXXX");
	if (!mkdtemp(tempdir)){
		Error("mkdtemp: %s", strerror(errno));
	}
	snprintf(tempsocket, PATH_MAX, "%s/%s", tempdir, "blobapi");
	cmd = BIN_DIR "/" MONBLOB;
	if ( ( pid = fork() ) <0 ) {
		Error("fork: %s", strerror(errno));
	}
	if (pid == 0){
		/* child */
		if (execl(cmd,MONBLOB,"-importid", id,"-import", filename, "-socket", tempsocket, NULL) < 0) {
			Error("execl: %s:%s", strerror(errno), cmd);
		}
	}
	/* parent */
	SendMONBLOBValue(val, tempsocket);
	rmdir(tempdir);
LEAVE_FUNC;
	return	(ret);
}

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed,
	int			usage)
{
	return	(MCP_OK);
}

static	ValueStruct	*
_DBACCESS(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(ret);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)_DBCOMMIT },
	/*	table operations	*/
	{	"MONBLOBNEW",		_NewBLOB		},
	{	"MONBLOBIMPORT",	_ImportBLOB		},

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL,
};

extern	DB_Func	*
InitMONBLOB(void)
{
	return	(EnterDB_Function("MONBLOB",Operations,DB_PARSER_NULL,&Core,"",""));
}

