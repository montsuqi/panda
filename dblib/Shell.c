/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2003 Ogochan.

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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
	Bool		fRedirect)
{
	int			rc;

	rc = MCP_OK;
	return	(rc);
}

static	void
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	dbg->conn = NewLBS();
	OpenDB_RedirectPort(dbg);
	dbg->fConnect = TRUE;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
}

static	void
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  dbg->fConnect  ) { 
		CloseDB_RedirectPort(dbg);
		FreeLBS(dbg->conn);
		dbg->fConnect = FALSE;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
}

static	void
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	LBS_EmitStart(dbg->conn); 
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
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

static	void
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int			rc;
	char		*p
	,			*q;

ENTER_FUNC;
	CheckDB_Redirect(dbg);
	LBS_EmitEnd(dbg->conn);
	RewindLBS(dbg->conn);
	p = (char *)LBS_Body(dbg->conn);
	rc = 0;
	while	(  ( q = strchr(p,0xFF) )  !=  NULL  ) {
		*q = 0;
		rc += DoShell(p);
		p = q + 1;
	}
	rc += DoShell(p);
	LBS_Clear(dbg->conn);
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
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

static	void
ExecShell(
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	LargeByteString	*src,
	ValueStruct		*args)
{
	int		c;
	ValueStruct	*val;
	DBG_Struct	*dbg;

ENTER_FUNC;
	dbg =  rec->opt.db->dbg;
	if	(  src  ==  NULL )	{
		Error("function \"%s\" is not found.",ctrl->func);
	}
	RewindLBS(src);
	while	(  ( c = LBS_FetchByte(src) )  >=  0  ) {
		if		(  c  !=  SQL_OP_ESC  ) {
			LBS_EmitChar(dbg->conn,c);
		} else {
			c = LBS_FetchByte(src);
			switch	(c) {
			  case	SQL_OP_REF:
				val = (ValueStruct *)LBS_FetchPointer(src);
				InsertValue(dbg,dbg->conn,val);
				break;
			  case	SQL_OP_EOL:
			  case	0:
#if	1
				LBS_EmitChar(dbg->conn,';');
#else
				LBS_EmitChar(dbg->conn,0xFF);
#endif
				break;
			  default:
				break;
			}
		}
	}
LEAVE_FUNC;
}

static	Bool
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

ENTER_FUNC;
#ifdef	TRACE
	printf("[%s]\n",name); 
#endif
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
		rc = TRUE;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		if		(  ( ix = (int)g_hash_table_lookup(path->opHash,name) )  ==  0  ) {
			rc = FALSE;
		} else {
			src = path->ops[ix-1]->proc;
			if		(  src  !=  NULL  ) {
				ExecShell(ctrl,rec,src,args);
				rc = TRUE;
			} else {
				rc = FALSE;
			}
		}
	}
LEAVE_FUNC;
	return	(rc);
}

static	void
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
	_DBACCESS
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

	ret = EnterDB_Function("Shell",Operations,&Core,"# ","\n");
LEAVE_FUNC;
	return	(ret); 
}

