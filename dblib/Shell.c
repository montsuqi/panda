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

static	ValueStruct	*
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	char 		**cmdv;
ENTER_FUNC;
	dbg->count = 0;
	cmdv = dbg->process[PROCESS_UPDATE].conn;
	cmdv[dbg->count] = NULL;
	unsetenv("MCP_BATCH_NAME");
	unsetenv("MCP_BATCH_COMMENT");

	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	int
DoShell(
	char	**cmdv)
{
	char	**cmd;
	char	*argv[256];
	char	*sh;
	pid_t	pgid;
	int		pid;
	int		rc;
	int 	i;
	extern	char	**environ;

ENTER_FUNC;
#ifdef	DEBUG
	printf("command --------------------------------\n");
	for (cmd = cmdv; *cmd; cmd++) {
		printf("command: %s\n", *cmd);
	}
	printf("----------------------------------------\n");
#endif
	if		(  *cmdv  !=  NULL  ) {
		if		(  ( pid = fork() )  ==  0  )	{
			setpgid(0,0);
			sh = BIN_DIR "/monbatch";
			pgid = getpgrp();
			printf("****************************************** %d\n", pgid);
			argv[0] = sh;
			i = 1;
			for (cmd = cmdv; *cmd; cmd++) {
				argv[i++] = *cmd;
			}
			argv[i] = NULL;
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
	int			i;
	int			rc;
	char 		**cmdv;

ENTER_FUNC;
	CheckDB_Redirect(dbg);
	cmdv = (char **)dbg->process[PROCESS_UPDATE].conn;
	rc = DoShell(cmdv);
	CommitDB_Redirect(dbg);
	for (i=0; i< dbg->count; i++) {
		xfree(cmdv[i]);
	}
	dbg->count = 0;
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
	char	buff[SIZE_OTHER];
	char *str;

	if		(  val  ==  NULL  ) {
	} else
	switch	(val->type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		LBS_EmitChar(lbs,'"');
		LBS_EmitString(lbs, ValueToString(val,dbg->coding));
		LBS_EmitChar(lbs,'"');
		break;
	  case	GL_TYPE_NUMBER:
		nv = FixedToNumeric(&ValueFixed(val));
		str = NumericOutput(nv);
		LBS_EmitString(lbs,str);
		xfree(str);
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
RegistShell(
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
	char 		**cmdv;

ENTER_FUNC;
	ret = NULL;
	dbg =  rec->opt.db->dbg;
	cmdv = (char **)dbg->process[PROCESS_UPDATE].conn;
	if	(  src  ==  NULL )	{
		Error("function \"%s\" is not found.",ctrl->func);
	}
	RewindLBS(src);
	lbs = NewLBS();
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
				LBS_EmitEnd(lbs);
				cmdv[dbg->count] = StrDup(LBS_Body(lbs));
				dbg->count++;
				RewindLBS(lbs);
				break;
			  default:
				break;
			}
		}
	}
	FreeLBS(lbs);
	cmdv[dbg->count] = NULL;
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_DBACCESS(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	char *name, *comment;
	DB_Struct	*db;
	PathStruct	*path;
	LargeByteString	*src;
	int		ix;
	Bool	rc;
	ValueStruct	*ret;

ENTER_FUNC;
#ifdef	TRACE
	printf("[%s]\n",ctrl->func);
#endif
	ret = NULL;
	name = ValueToString(GetItemLongName(args,"name"),dbg->coding);
	setenv("MCP_BATCH_NAME", name, 0);
	comment = ValueToString(GetItemLongName(args,"comment"),dbg->coding);
	setenv("MCP_BATCH_COMMENT", comment, 0);

	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
		rc = TRUE;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		if		(  ( ix = (int)(long)g_hash_table_lookup(path->opHash,ctrl->func) )  ==  0  ) {
			rc = FALSE;
		} else {
			src = path->ops[ix-1]->proc;
			if		(  src  !=  NULL  ) {
				ret = RegistShell(ctrl,rec,src,args);
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
LEAVE_FUNC;
}

extern	DB_Func	*
InitShell(void)
{
	struct sigaction sa;
	DB_Func	*ret;
ENTER_FUNC;
	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset (&sa.sa_mask);
	sa.sa_flags |= SA_RESTART;
	sa.sa_handler = (void *)OnChildExit;
	if (sigaction(SIGCHLD, &sa, NULL) != 0) {
		fprintf(stderr,"sigaction(2) failure\n");
	}

	ret = EnterDB_Function("Shell",Operations,DB_PARSER_SQL,&Core,"# ","\n");
LEAVE_FUNC;
	return	(ret);
}

