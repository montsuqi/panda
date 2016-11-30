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
#include	<uuid/uuid.h>
#include 	<errno.h>
#include	<string.h>

#include	"libmondai.h"
#include	"enum.h"
#include	"SQLparser.h"
#include	"dbgroup.h"
#include	"monsys.h"
#include	"redirect.h"
#include	"debug.h"

#include	"directory.h"

static	void
OnChildExit(
	int		ec)
{
ENTER_FUNC;
	while( waitpid(-1, NULL, WNOHANG) > 0 );
LEAVE_FUNC;
}

static	void
SetSigChild(void)
{
	struct sigaction sa, orgsa;

	memset(&orgsa, 0, sizeof(struct sigaction));
	sigaction(SIGCHLD, NULL, &orgsa);
	if (orgsa.sa_handler == SIG_DFL) {
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset (&sa.sa_mask);
		sa.sa_flags |= SA_RESTART;
		sa.sa_handler = (void *)OnChildExit;
		if (sigaction(SIGCHLD, &sa, NULL) != 0) {
			fprintf(stderr,"sigaction(2) failure\n");
		}
	}
}

static	void
SetSigChildDFL(void)
{
	struct sigaction sa, orgsa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigaction(SIGCHLD, NULL, &orgsa);
	if (orgsa.sa_handler != SIG_DFL) {
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset (&sa.sa_mask);
		sa.sa_handler = SIG_DFL;
		sa.sa_flags |= SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) != 0) {
			fprintf(stderr,"sigaction(2) failure\n");
		}
	}
}

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
_QUERY(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed,
	int			usage)
{
	return NULL;
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
	unsetenv("MON_BATCH_ID");
	unsetenv("MON_BATCH_NAME");
	unsetenv("MON_BATCH_COMMENT");
	unsetenv("MON_BATCH_EXTRA");
	unsetenv("MON_BATCH_GROUPNAME");
	unsetenv("GINBEE_CUSTOM_BATCH_REPOS_NAME");

	if(dbg->transaction_id) {
		xfree(dbg->transaction_id);
	}
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
	pid_t	pid;
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
			if (setpgid(0,0) != 0) {
				perror("setpgid");
			}
			sh = BIN_DIR "/monbatch";
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
			Warning("DoShell fork error:%s",strerror(errno));
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
	SetSigChild();
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

static int
ExSystem(
	char	*command)
{
	int		pid;
	int		rc;
	char	*sh;
	char	*argv[3];
	extern	char	**environ;

	SetSigChildDFL();
	if		(  command  !=  NULL  ) {
		if		(  ( pid = fork() )  <  0  )	{
			rc = -1;
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
			sh = BIN_DIR "/monbatch";
			argv[0] = sh;
			argv[1] = command;
			argv[2] = NULL;
			execve(sh, argv, environ);
			rc = MCP_BAD_OTHER;
		} else
		if		(  pid   >  0  )	{	/*	parent	*/
			while(waitpid(pid, &rc, 0) <0){
				if (errno != EINTR){
					rc = -1;
					break;
				}
			}
		}
	} else {
		rc = -1;
	}
	return (rc);
}

static	int
ExCommand(
	RecordStruct	*rec,
	LargeByteString	*src)
{
	int		c;
	int		rc;
	ValueStruct	*val;
	DBG_Struct	*dbg;
	LargeByteString	*lbs;
	char *command;

	dbg =  rec->opt.db->dbg;
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
				RewindLBS(lbs);
				break;
			  default:
				break;
			}
		}
	}
	command = StrDup(LBS_Body(lbs));
	FreeLBS(lbs);
	rc = ExSystem(command);
	xfree(command);
	return	(rc);
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
	if ((val = GetItemLongName(args, "id")) != NULL) {
		SetValueString(val, dbg->transaction_id, dbg->coding);
	}
	ret = DuplicateValue(args,TRUE);
	cmdv[dbg->count] = NULL;
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
SetRetvalue(
	ValueStruct		*mondbg_value,
	ValueStruct		*shell_value)
{
	int i;
	char *name;
	ValueStruct		*val;

	for	( i = 0 ; i < ValueRecordSize(mondbg_value) ; i ++ ) {
		name = ValueRecordName(mondbg_value,i);
		val = GetItemLongName(shell_value, name);
		if (val) {
			CopyValue(val, ValueRecordItem(mondbg_value,i));
		}
	}
	return shell_value;
}

static	void
SetBatchEnv(
	DBG_Struct		*dbg,
	ValueStruct		*args)
{
	char *name, *comment, *extra, *groupname, *repos_name;

	name = ValueToString(GetItemLongName(args,"name"),dbg->coding);
	setenv("MON_BATCH_NAME", name, 1);
	comment = ValueToString(GetItemLongName(args,"comment"),dbg->coding);
	setenv("MON_BATCH_COMMENT", comment, 1);
	extra = ValueToString(GetItemLongName(args,"extra"),dbg->coding);
	setenv("MON_BATCH_EXTRA", extra, 1);
	groupname = ValueToString(GetItemLongName(args,"groupname"),dbg->coding);
	setenv("MON_BATCH_GROUPNAME", groupname, 1);
	repos_name = ValueToString(GetItemLongName(args,"repos_name"),dbg->coding);
	setenv("GINBEE_CUSTOM_BATCH_REPOS_NAME", repos_name, 1);
}

static	ValueStruct	*
_DBACCESS(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	uuid_t	u;
	DB_Struct	*db;
	PathStruct	*path;
	LargeByteString	*src;
	int		ix;
	Bool	rc;
	ValueStruct	*ret;

ENTER_FUNC;
	dbgprintf("[%s]\n",ctrl->func);
	ret = NULL;

	if (dbg->count == 0) {
		SetBatchEnv(dbg, args);
		dbg->transaction_id = xmalloc(SIZE_TERM+1);
		uuid_generate(u);
		uuid_unparse(u, dbg->transaction_id);
		setenv("MON_BATCH_ID", dbg->transaction_id, 1);
	}
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

Bool
KeyValueToWhere(
	LargeByteString	*lbs,
	DBG_Struct		*dbg,
	Bool first,
	char *key,
	char *value)
{
	char	buff[SIZE_OTHER];

	if ((value != NULL) && (strlen(value) > 0)) {
		if ( first ) {
			LBS_EmitString(lbs," WHERE ");
		} else {
			LBS_EmitString(lbs," AND ");
		}
		sprintf(buff,"%s = '%s'",key, Escape_monsys(dbg, value));
		LBS_EmitString(lbs,buff);
		first = FALSE;
	}
	return first;
}

static	char *
ValueToWhere(
	DBG_Struct		*dbg,
	ValueStruct		*value)
{
	char *keys[] = {"id", "tenant", "name", "comment", "extra", "groupname", NULL};
	char pgid_s[10];
	char *where, *k, *v;
	Bool first = TRUE;
	LargeByteString	*lbs;
	int i, pgid;

	lbs = NewLBS();
	i = 0;
	pgid = ValueToInteger(GetItemLongName(value,"pgid"));
	if ((pgid > 0) && (pgid < 99999) ){
		snprintf(pgid_s, 10, "%d", pgid);
		first = KeyValueToWhere(lbs, dbg, first, "pgid", pgid_s);
	}
	while ((k = keys[i]) != NULL) {
		v = ValueToString(GetItemLongName(value,k),NULL);
		first = KeyValueToWhere(lbs, dbg, first, k, v);
		i++;
	}
	LBS_EmitEnd(lbs);
	where = StrDup((char *)LBS_Body(lbs));
	FreeLBS(lbs);
	return where;
}

static	ValueStruct	*
_EXCOMMAND(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	uuid_t	u;
	DB_Struct	*db;
	PathStruct	*path;
	LargeByteString	*src;
	ValueStruct		*ret;
	int		ix;
	int		rc;
	char *uuid;

ENTER_FUNC;
	SetBatchEnv(dbg, args);
	uuid = xmalloc(SIZE_TERM+1);
	uuid_generate(u);
	uuid_unparse(u, uuid);
	setenv("MON_BATCH_ID", uuid, 1);

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
				rc = ExCommand(rec,src);
			} else {
				rc = FALSE;
			}
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
	xfree(uuid);
	ret = DuplicateValue(args,TRUE);
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_DBSELECT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	char *where;
	DBG_Struct		*mondbg;
	ValueStruct	*ret = NULL;
	size_t sql_len  = SIZE_SQL;
	char *sql;

	mondbg = GetDBG_monsys();
	where = ValueToWhere(mondbg, args);
	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len, "DECLARE %s_csr CURSOR FOR SELECT * FROM %s %s ORDER BY groupname, starttime;",
			 BATCH_TABLE, BATCH_TABLE, where);
	ret = ExecDBQuery(mondbg, sql, FALSE, DB_UPDATE);
	xfree(where);
	xfree(sql);
	return ret;
}

static	ValueStruct	*
_DBFETCH(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DBG_Struct		*mondbg;
	ValueStruct	*ret = NULL;
	ValueStruct	*mondbg_val;
	size_t sql_len  = SIZE_SQL;
	char *sql;

	ctrl->rcount = 0;
	mondbg = GetDBG_monsys();
	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len, "FETCH 1 FROM %s_csr;",
			 BATCH_TABLE);
	mondbg_val = ExecDBQuery(mondbg, sql, FALSE, DB_UPDATE);
	xfree(sql);
	if (mondbg_val) {
		ctrl->rc = MCP_OK;
		ctrl->rcount = 1;
		ret = SetRetvalue(mondbg_val, DuplicateValue(args,TRUE));
	} else {
		ctrl->rc = MCP_EOF;
	}
	FreeValueStruct(mondbg_val);
	return ret;
}

static	ValueStruct	*
_DBDELETE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret = NULL;
	int pgid;

	CheckBatchPg();

	pgid = ValueToInteger(GetItemLongName(args,"pgid"));
	ctrl->rc = killpg(pgid, SIGHUP);

	return ret;
}

static	ValueStruct	*
_DBCLOSECURSOR(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DBG_Struct		*mondbg;
	size_t sql_len = SIZE_SQL;
	char *sql;

	ValueStruct	*ret;

	ret = NULL;
	ctrl->rc = MCP_OK;
	ctrl->rcount = 0;
	mondbg = GetDBG_monsys();

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len, "CLOSE %s_csr;",
			 BATCH_TABLE);
	ExecDBOP(mondbg, sql, FALSE, DB_UPDATE);
	xfree(sql);

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
	{	"EXCOMMAND",	_EXCOMMAND },
	{	"DBSELECT",		_DBSELECT },
	{	"DBFETCH",		_DBFETCH },
	{	"DBUPDATE",		_DBERROR },
	{	"DBDELETE",		_DBDELETE },
	{	"DBINSERT",		_DBERROR },
	{	"DBCLOSECURSOR",_DBCLOSECURSOR },

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	_QUERY,
	NULL,
};

extern	DB_Func	*
InitShell(void)
{
	DB_Func	*ret;
ENTER_FUNC;
	ret = EnterDB_Function("Shell",Operations,DB_PARSER_SQL,&Core,"# ","\n");
LEAVE_FUNC;
	return	(ret);
}
