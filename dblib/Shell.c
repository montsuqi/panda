/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2008 Ogochan & JMA (Japan Medical Association).
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
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<glib.h>
#include	<signal.h>

#include	"types.h"
#include	"libmondai.h"
#include	"enum.h"
#include	"SQLparser.h"
#include	"dbgroup.h"
#include	"redirect.h"
#include	"debug.h"

#include	"directory.h"

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRedirect,
	int			usage)
{
	int			rc;

	rc = MCP_OK;
	return	(rc);
}

static	ValueStruct	*
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	OpenDB_RedirectPort(dbg);
	dbg->process[PROCESS_UPDATE].conn = (void *)NewLBS();
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
		CloseDB_RedirectPort(dbg);
		FreeLBS((LargeByteString *)dbg->process[PROCESS_UPDATE].conn);
		dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_DISCONNECT;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	LBS_EmitStart((LargeByteString *)dbg->process[PROCESS_UPDATE].conn);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	int
DoShell(
	char	*command)
{
	char	*argv[4];
	char	*sh;
	int		pid;
	int		rc;
	extern	char	**environ;

ENTER_FUNC;
#ifdef	DEBUG
	printf("command --------------------------------\n");
	printf("%s\n",command);
	printf("----------------------------------------\n");
#endif
	if		(  *command  !=  0  ) {
		if		(  ( pid = fork() )  ==  0  )	{
			if		(  ( sh = getenv("SHELL") )  ==  NULL  ) {
				sh = "/bin/sh";
			}
			argv[0] = sh;
			argv[1] = "-c";
			argv[2] = command;
			argv[3] = NULL;
			execve(sh, argv, environ);
			rc = MCP_BAD_OTHER;
		} else
		if		(  pid  <  0  ) {
			rc = MCP_BAD_OTHER;
		} else {
			rc = MCP_OK;
		}
	} else {
		rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(rc);
}

static	ValueStruct	*
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int			rc;
	char		*p
	,			*q;
	LargeByteString	*lbs;

ENTER_FUNC;
	CheckDB_Redirect(dbg);
	lbs = (LargeByteString *)dbg->process[PROCESS_UPDATE].conn;
	LBS_EmitEnd(lbs);
	RewindLBS(lbs);
	p = (char *)LBS_Body(lbs);
	rc = 0;
	while	(  ( q = strchr(p,0xFF) )  !=  NULL  ) {
		*q = 0;
		rc += DoShell(p);
		p = q + 1;
	}
	rc += DoShell(p);
	LBS_String(dbg->last_query,p);
	LBS_Clear(lbs);
	LastQuery(dbg, ctrl);
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	void
InsertValue(
	DBG_Struct		*dbg,
	LargeByteString	*lbs,
	ValueStruct		*val)
{
	Numeric	nv;
	char	buff[SIZE_BUFF];

	if		(  val  ==  NULL  ) {
	} else
	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		LBS_EmitChar(lbs,'"');
		LBS_EmitString(lbs,ValueToString(val,dbg->coding));
		LBS_EmitChar(lbs,'"');
		break;
	  case	GL_TYPE_NUMBER:
		nv = FixedToNumeric(&ValueFixed(val));
		LBS_EmitString(lbs,NumericOutput(nv));
		NumericFree(nv);
		break;
	  case	GL_TYPE_INT:
		sprintf(buff,"%d",ValueInteger(val));
		LBS_EmitString(lbs,buff);
		break;
	  case	GL_TYPE_FLOAT:
		sprintf(buff,"%g",ValueFloat(val));
		LBS_EmitString(lbs,buff);
		break;
	  case	GL_TYPE_BOOL:
		sprintf(buff,"%s",ValueBool(val) ? "true" : "false");
		LBS_EmitString(lbs,buff);
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_ARRAY:
	  case	GL_TYPE_RECORD:
		break;
	  default:
		break;
	}
}

static	ValueStruct	*
ExecShell(
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	LargeByteString	*src,
	ValueStruct		*args)
{
	int		c;
	ValueStruct	*val;
	DBG_Struct	*dbg;
	ValueStruct	*ret;
	LargeByteString	*lbs;

ENTER_FUNC;
	ret = NULL;
	dbg =  rec->opt.db->dbg;
	lbs = (LargeByteString *)dbg->process[PROCESS_UPDATE].conn;
	if	(  src  ==  NULL )	{
		Error("function \"%s\" is not found.",ctrl->func);
	}
	RewindLBS(src);
	while	(  ( c = LBS_FetchByte(src) )  >=  0  ) {
		if		(  c  !=  SQL_OP_ESC  ) {
			LBS_EmitChar(lbs,c);
		} else {
			c = LBS_FetchByte(src);
			switch	(c) {
			  case	SQL_OP_REF:
				val = (ValueStruct *)LBS_FetchPointer(src);
				InsertValue(dbg,lbs,val);
				break;
			  case	SQL_OP_EOL:
			  case	0:
				LBS_EmitChar(lbs,';');
				break;
			  default:
				break;
			}
		}
	}
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_DBACCESS(
	DBG_Struct		*dbg,
	char			*name,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DB_Struct	*db;
	PathStruct	*path;
	LargeByteString	*src;
	int		ix;
	Bool	rc;
	ValueStruct	*ret;

ENTER_FUNC;
#ifdef	TRACE
	printf("[%s]\n",name); 
#endif
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
		rc = TRUE;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		if		(  ( ix = (int)(long)g_hash_table_lookup(path->opHash,name) )  ==  0  ) {
			rc = FALSE;
		} else {
			src = path->ops[ix-1]->proc;
			if		(  src  !=  NULL  ) {
				ret = ExecShell(ctrl,rec,src,args);
				rc = TRUE;
			} else {
				rc = FALSE;
			}
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc ? MCP_OK : MCP_BAD_FUNC;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_DBERROR(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)_DBCOMMIT },
	/*	table operations	*/
	{	"DBSELECT",		_DBERROR },
	{	"DBFETCH",		_DBERROR },
	{	"DBUPDATE",		_DBERROR },
	{	"DBDELETE",		_DBERROR },
	{	"DBINSERT",		_DBERROR },

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL,
};

static	void
OnChildExit(
	int		ec)
{
ENTER_FUNC;
	while( waitpid(-1, NULL, WNOHANG) > 0 );
	(void)signal(SIGCHLD, (void *)OnChildExit);
LEAVE_FUNC;
}

extern	DB_Func	*
InitShell(void)
{
	DB_Func	*ret;
ENTER_FUNC;
	(void)signal(SIGCHLD, (void *)OnChildExit);

	ret = EnterDB_Function("Shell",Operations,DB_PARSER_SQL,&Core,"# ","\n");
LEAVE_FUNC;
	return	(ret); 
}

