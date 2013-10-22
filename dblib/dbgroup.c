/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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
#define	TIMER
#define	DEBUG
#define	TRACE
*/

#define	DBGROUP
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<glib.h>
#include	<sys/time.h>
#include	<assert.h>

#include	"defaults.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"DDparser.h"
#include	"dbgroup.h"
#include	"blobreq.h"
#include	"debug.h"

extern	void
InitializeCTRL(
	DBCOMM_CTRL	*ctrl)
{
	*ctrl->func = '\0';
	*ctrl->rname = '\0';
	*ctrl->pname = '\0';
	ctrl->rno = 0;
	ctrl->pno = 0;
	ctrl->rcount = 0;
	ctrl->blocks = 0;
	ctrl->rc = 0;
	ctrl->limit = 1;
	ctrl->redirect = 1;
	ctrl->usage = 1;
	ctrl->time = 0;
	ctrl->rec = NULL;
	ctrl->value = NULL;
	ctrl->src = NULL;
}

static void
SetApplicationName(
	DBG_Struct	*dbg,
	char		*appname)
{
	if (appname != NULL){
		dbg->appname = appname;
	}
}

extern	void
InitDB_Process(
	char *appname)
{
	int i;
ENTER_FUNC;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		SetApplicationName(ThisEnv->DBG[i], appname);
	}
LEAVE_FUNC;
}

static  DB_FUNC
LookupFUNC(
	DBG_Struct	*dbg,
	char		*funcname)
{
	DB_FUNC	func;

	assert(dbg);
	assert(dbg->func);

	func = (DB_FUNC)g_hash_table_lookup(dbg->func->table,funcname);
	return func;
}

static	void
SetDBAudit(
	DBG_Struct		*dbg)
{
	ValueStruct	*audit;
	LargeByteString	*lbs;

	if		( dbg->auditlog > 0 ) {
		if ( ThisEnv->auditrec->value != NULL){
			audit = ThisEnv->auditrec->value;
			SetValueInteger(GetItemLongName(audit,"ticket_id"), dbg->ticket_id);
			lbs = dbg->last_query;
			if ((lbs != NULL) && (LBS_Size(lbs) > 0)){
				SetValueString(GetItemLongName(audit,"exec_query"),LBS_Body(lbs), dbg->coding);
			}
		}
	}
}

static	void
_DBRecordFunc(
	char	*rname,
	RecordStruct	*rec,
	char	*fname)
{
	DB_Struct	*db = RecordDB(rec);
	DBG_Struct	*dbg = db->dbg;
	int			ix;
	DB_Operation	*op;

ENTER_FUNC;
	if		(  ( ix = (int)(long)g_hash_table_lookup(db->opHash,fname) )  >  0  ) {
		if		(  ( op = db->ops[ix-1] )  !=  NULL  ) {
			if		(  !(*dbg->func->primitive->record)(dbg,fname,rec)  ) {
				Warning("function not found [%s]\n",fname);
			}
		}
	}
LEAVE_FUNC;
}

static	int
_ExecFunction(
	DBG_Struct	*dbg,
	char		*funcname,
	DBCOMM_CTRL	*ctrl,
	Bool		fAll)
{
	typedef	void (*Operation_FUNC)(DBG_Struct *, DBCOMM_CTRL *);
	Operation_FUNC	opfunc;

	if		(	(  !fAll  ) && (  dbg->dbt  ==  NULL  ) ) {
		return MCP_OK;
	}
	opfunc = (Operation_FUNC)LookupFUNC(dbg,funcname);
	if		(  opfunc == NULL ) {
		Warning("function not found [%s]\n",funcname);
		return MCP_BAD_FUNC;
	}
	/* _DBOPEN,_DBDISCONNECT,_DBSTART,_DBCOMMIT... */
	(*opfunc)(dbg,ctrl);
	dbgprintf("ctrl.rc   = [%d]",ctrl->rc);
	if		(   ( dbg->func->primitive->record  !=  NULL )  &&
						( dbg->dbt  !=  NULL  )	) {
		/* _DBRECORD */
		g_hash_table_foreach(dbg->dbt,(GHFunc)_DBRecordFunc,funcname);
	}
	return ctrl->rc;
}

static	int
ExecFunction(
	DBG_Struct	*dbg,
	char		*funcname,
	Bool		fAll)
{
	DBCOMM_CTRL	ctrl;
	int			i;

ENTER_FUNC;
	InitializeCTRL(&ctrl);
	dbgprintf("func  = [%s]",funcname);
	dbgprintf("fAll = [%s]",fAll?"TRUE":"FALSE");
	if		(  dbg  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			ctrl.rc += _ExecFunction(ThisEnv->DBG[i],funcname,&ctrl,fAll);
		}
	} else {
		dbgprintf("name  = [%s]",dbg->name);
		ctrl.rc = _ExecFunction(dbg,funcname,&ctrl,fAll);
	}
LEAVE_FUNC;
	return	(ctrl.rc);
}

extern	int
ExecDBOP(
	DBG_Struct	*dbg,
	char		*sql,
	int			usage)
{
	int		rc;
	rc = dbg->func->primitive->exec(dbg,sql,TRUE, usage);
	return	(rc);
}

extern	int
ExecRedirectDBOP(
	DBG_Struct	*dbg,
	char		*sql,
	int			usage)
{
	int		rc;

	rc = dbg->func->primitive->exec(dbg,sql,FALSE, usage);
	return	(rc);
}

extern	ValueStruct	*
ExecDB_Process(
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DB_FUNC	func;
	DBG_Struct		*dbg;
	ValueStruct		*ret;
	long	start
		,	end;

ENTER_FUNC;
	ret = NULL;
	ctrl->rc = 0;
	start = GetNowTime();
	if		(  ctrl->fDBOperation  ) {
		ctrl->rc = ExecFunction(NULL,ctrl->func, FALSE);
	} else {
		if (rec == NULL) {
			ctrl->rc = MCP_BAD_FUNC;
		} else {
			dbg = RecordDB(rec)->dbg;
			func = LookupFUNC(dbg,ctrl->func);
			if		(  func ==  NULL  ) {
				/* _DBACCESS */
				func = (DB_FUNC)dbg->func->primitive->access;
			}
			ret = (*func)(dbg,ctrl,rec,args);
			SetDBAudit(dbg);
		}
		if		(  ctrl->rc  <  0  ) {
			Warning("bad function [%s:%s:%s] rc = %d\n",ctrl->func, ctrl->rname,ctrl->pname,ctrl->rc);
		}
	}
	end = GetNowTime();
	ctrl->time = (end - start);
	TimerPrintf(start,end, "%s:%s:%s\n",ctrl->func,ctrl->rname,ctrl->pname);
LEAVE_FUNC;
	return	(ret);
}

static	int
ExecDBG_Operation(
	DBG_Struct	*dbg,
	char		*funcname)
{
	int rc;
	rc = ExecFunction(dbg,funcname,FALSE);
	return rc;
}

extern	int
TransactionStart(
	DBG_Struct *dbg)
{
	int		rc;

ENTER_FUNC;
	rc = ExecDBG_Operation(dbg,"DBSTART");
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionEnd(
	DBG_Struct *dbg)
{
	int		rc;

ENTER_FUNC;
	rc = ExecDBG_Operation(dbg,"DBCOMMIT");
LEAVE_FUNC;
	return	(rc);
}

extern	int
GetDBRedirectStatus(int newstatus)
{
	int dbstatus;
	DBG_Struct	*dbg, *rdbg;
	int		i;
ENTER_FUNC;
    dbstatus = DB_STATUS_NOCONNECT;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		if		( dbg->redirect != NULL ) {
			rdbg = dbg->redirect;
			if		( dbstatus  <  rdbg->process[PROCESS_UPDATE].dbstatus )	{
				dbstatus = rdbg->process[PROCESS_UPDATE].dbstatus;
			}
		}
	}
LEAVE_FUNC;
	return dbstatus;
}

extern	void
RedirectError(void)
{
	DBG_Struct	*dbg;
	int		i;
ENTER_FUNC;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		CloseDB_RedirectPort(dbg);
	}
LEAVE_FUNC;
}

extern	int
OpenDB(
	DBG_Struct *dbg)
{
	return ExecDBG_Operation(dbg,"DBOPEN");
}

extern	int
OpenRedirectDB(
	DBG_Struct *dbg)
{
	return ExecFunction(dbg,"DBOPEN",TRUE);
}

extern	int
CloseRedirectDB(
	DBG_Struct *dbg)
{
	return ExecFunction(dbg,"DBDISCONNECT",TRUE);
}

extern	int
TransactionRedirectStart(
	DBG_Struct *dbg)
{
	return ExecFunction(dbg,"DBSTART",FALSE);
}

extern	int
TransactionRedirectEnd(
	DBG_Struct *dbg)
{
	int rc;

	rc = ExecFunction(dbg,"DBCOMMIT",FALSE);

	return rc;
}

extern	int
CloseDB(
	DBG_Struct *dbg)
{
	return ExecDBG_Operation(dbg,"DBDISCONNECT");
}

/*	utility	*/
static int
UsageNum(
	DBG_Struct	*dbg,
	int			usage)
{
	int i;
	for	( i = 0 ; i < dbg->nServer ; i ++ ) {
		if		(  dbg->server[i].usage  ==  usage  )
			break;
	}
	return i;
};
extern	Port	*
GetDB_Port(
	DBG_Struct	*dbg,
	int			usage)
{
	Port	*port;
	int		num;

ENTER_FUNC;
	num = UsageNum(dbg, usage);
	if		(  num  ==  dbg->nServer  ) {
		port = NULL;
	} else {
		port = dbg->server[num].port;
	}
LEAVE_FUNC;
	return (port);
}

extern	char	*
GetDB_Host(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*host;

ENTER_FUNC;
	if		(  DB_Host  !=  NULL  ) {
		host = DB_Host;
	} else {
		host = IP_HOST((GetDB_Port(dbg,usage)));
	}
LEAVE_FUNC;
	return (host);
}

extern	char	*
GetDB_PortName(
	DBG_Struct	*dbg,
	int			usage)
{
	Port	*port;
	char	*portname;

ENTER_FUNC;
	if		(  DB_Port  !=  NULL  ) {
		portname = DB_Port;
	} else {
		port = GetDB_Port(dbg,usage);
		if ( (port != NULL) && (port->type == PORT_IP)) {
			portname = IP_PORT(port);
		} else {
			portname = NULL;
		}
	}
LEAVE_FUNC;
	return (portname);
}

extern	int
GetDB_PortMode(
	DBG_Struct	*dbg,
	int			usage)
{
	Port	*port;
	int	mode;

ENTER_FUNC;
	port = GetDB_Port(dbg,usage);
	if ( (port != NULL) && port->type == PORT_UNIX) {
		mode = UNIX_MODE(port);
	} else {
		mode = 0;
	}
LEAVE_FUNC;
	return (mode);
}

extern	char	*
GetDB_DBname(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*name = NULL;
	int		num;
	if		(  DB_Name  !=  NULL  ) {
		name = DB_Name;
	} else {
		num = UsageNum(dbg, usage);
		if		(  num  <  dbg->nServer  ) {
			name = dbg->server[num].dbname;
		}
	}
	return	(name);
}

extern	char	*
GetDB_User(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*name = NULL;
	int		num;

	if		(  DB_User  !=  NULL  ) {
		name = DB_User;
	} else {
		num = UsageNum(dbg, usage);
		if		(  num  <  dbg->nServer  ) {
			name = dbg->server[num].user;
		}
	}
	return	(name);
}

extern	char	*
GetDB_Pass(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*pass = NULL;
	int		num;

	if		(  DB_Pass  !=  NULL  ) {
		pass = DB_Pass;
	} else {
		num = UsageNum(dbg, usage);
		if		(  num  <  dbg->nServer  ) {
			pass = dbg->server[num].pass;
		}
	}
	return	(pass);
}

extern	char	*
GetDB_Sslmode(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*sslmode = NULL;
	int		num;

	num = UsageNum(dbg, usage);
	if		(  num  <  dbg->nServer  ) {
		sslmode = dbg->server[num].sslmode;
	}
	return	(sslmode);
}

extern	char	*
GetDB_Sslcert(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*sslcert = NULL;
	int		num;

	num = UsageNum(dbg, usage);
	if		(  num  <  dbg->nServer  ) {
		sslcert = dbg->server[num].sslcert;
	}
	return	(sslcert);
}

extern	char	*
GetDB_Sslkey(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*sslkey = NULL;
	int		num;

	num = UsageNum(dbg, usage);
	if		(  num  <  dbg->nServer  ) {
		sslkey = dbg->server[num].sslkey;
	}
	return	(sslkey);
}

extern	char	*
GetDB_Sslrootcert(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*sslrootcert = NULL;
	int		num;

	num = UsageNum(dbg, usage);
	if		(  num  <  dbg->nServer  ) {
		sslrootcert = dbg->server[num].sslrootcert;
	}
	return	(sslrootcert);
}

extern	char	*
GetDB_Sslcrl(
	DBG_Struct	*dbg,
	int			usage)
{
	char	*sslcrl = NULL;
	int		num;

	num = UsageNum(dbg, usage);
	if		(  num  <  dbg->nServer  ) {
		sslcrl = dbg->server[num].sslcrl;
	}
	return	(sslcrl);
}

extern	Bool
IsDBOperation(
	char	*func)
{
	Bool ret;
	if ( !strcmp(func,"DBOPEN") ||
			 !strcmp(func,"DBCLOSE") ||
			 !strcmp(func,"DBSTART") ||
			 !strcmp(func,"DBCOMMIT") ||
			 !strcmp(func,"DBDISCONNECT") ) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}
	return ret;
}

extern Bool
IsDBUpdateFunc(char *func)
{
	if (   (strncmp(func, "DBFETCH", 7) == 0)
		|| (strncmp(func, "DBCLOSECURSOR", 13) == 0) ){
		return FALSE;
	}
	return TRUE;
}

static	ValueStruct	*
GetTableFuncValue(
	RecordStruct	*rec,
	DBCOMM_CTRL		*ctrl)
{
	ValueStruct	*value;
	PathStruct		*path;
	DB_Operation	*op;
	int		pno
		,	ono;

ENTER_FUNC;
	value = rec->value;
	if		(	(  strlen(ctrl->pname) > 0 )
			&&	(  ( pno = (int)(long)g_hash_table_lookup(RecordDB(rec)->paths,
														  ctrl->pname) )  !=  0  ) ) {
		pno --;
		path = RecordDB(rec)->path[pno];
		ctrl->usage = path->usage;
		value = ( path->args != NULL ) ? path->args : value;
		if		(	(  strlen(ctrl->func) > 0  )
				&&	( ( ono = (int)(long)g_hash_table_lookup(path->opHash,ctrl->func) )  !=  0  ) ) {
			op = path->ops[ono-1];
			value = ( op->args != NULL ) ? op->args : value;
		}
	} else {
		pno = 0;
	}
	ctrl->pno = pno;
LEAVE_FUNC;
	return	(value);
}

extern	int
GetTableFuncData(
	RecordStruct	**rec,
	ValueStruct		**value,
	DBCOMM_CTRL		*ctrl)
{
	int	rno;

ENTER_FUNC;
	*value = NULL;
	*rec = NULL;

	ctrl->fDBOperation = FALSE;
	if (strlen(ctrl->rname) <= 0) {
		return 0;
	}

	if ((rno = (int)(long)g_hash_table_lookup(DB_Table,ctrl->rname)) != 0) {
		ctrl->rno = rno -1;
		*rec = ThisDB[rno-1];
		*value = GetTableFuncValue(*rec,ctrl);
		return 1;
	}
	return 0;
LEAVE_FUNC;
}

extern	Bool
SetDBCTRLValue(
	DBCOMM_CTRL		*ctrl,
	char 	*pname)
{
	int		pno
		,	ono;
	ValueStruct	*value;
	PathStruct		*path;
	DB_Operation	*op;
	
ENTER_FUNC;
	if ( !pname ) {
		Warning("path name not set.\n");
		return FALSE;
	}
	strncpy(ctrl->pname,pname,SIZE_NAME);
	if (ctrl->rec == NULL) {
		return FALSE;
	}
	value = ctrl->rec->value;
	if		(  (pno = (int)(long)g_hash_table_lookup(RecordDB(ctrl->rec)->paths,pname) )  !=  0 ){
		pno --;
		path = RecordDB(ctrl->rec)->path[pno];
		ctrl->usage = path->usage;
		value = ( path->args != NULL ) ? path->args : value;
		if		(	(  strlen(ctrl->func) > 0  )
				&&	( ( ono = (int)(long)g_hash_table_lookup(path->opHash,ctrl->func) )  !=  0  ) ) {
			op = path->ops[ono-1];
			ctrl->src = path->ops[ono-1]->proc;
			value = ( op->args != NULL ) ? op->args : value;
		}
	}	else {
		pno = 0;
	}
	ctrl->pno = pno;
	ctrl->value = value;
LEAVE_FUNC;
	return TRUE;
}

extern	Bool
SetDBCTRLRecord(
	DBCOMM_CTRL		*ctrl,
	char *rname)
{
	int	rno;
	Bool	rc = FALSE;
ENTER_FUNC;
	if ( !rname ) {
		Warning("table name not set.\n");
		return rc;
	}
	strncpy(ctrl->rname,rname,SIZE_NAME);
	ctrl->fDBOperation = FALSE;
	if ((rno = (int)(long)g_hash_table_lookup(DB_Table,ctrl->rname)) != 0) {
		ctrl->rno = rno -1;
		ctrl->rec = ThisDB[rno-1];
		rc = TRUE;
	} else {
		Warning("The table name of [%s] is not found.\n", rname);
	}
LEAVE_FUNC;
	return rc;
}


#ifdef	DEBUG
extern	void
DumpDB_Node(
	DBCOMM_CTRL	*ctrl)
{
	printf("func   = [%s]\n",ctrl->func);
	printf("rname  = [%s]\n",ctrl->rname);
	printf("pname  = [%s]\n",ctrl->pname);
	printf("blocks = %d\n",ctrl->blocks);
	printf("rno    = %d\n",ctrl->rno);
	printf("pno    = %d\n",ctrl->pno);
	printf("count  = %d\n",ctrl->rcount);
	printf("limit  = %d\n",ctrl->limit);
}
#endif

extern	RecordStruct	*
BuildDBCTRL(void)
{
	RecordStruct	*rec;
	char			*buff
		,			*p;

	buff = (char *)xmalloc(SIZE_BUFF);
	p = buff;
	p += sprintf(p,	"dbctrl	{");
	p += sprintf(p,		"rc int;");
	p += sprintf(p,		"count int;");
	p += sprintf(p,		"limit int;");
	p += sprintf(p,		"func	varchar(%d);",SIZE_FUNC);
	p += sprintf(p,		"rname	varchar(%d);",SIZE_NAME);
	p += sprintf(p,		"pname	varchar(%d);",SIZE_NAME);
	p += sprintf(p,	"};");
	rec = ParseRecordMem(buff);

	return	(rec);
}

static	void
CopyValuebyName(
	ValueStruct	*to,
	char			*to_name,
	ValueStruct	*from,
	char			*from_name)
{
	CopyValue(GetItemLongName(to, to_name), GetItemLongName(from, from_name));
}

extern		RecordStruct		*
SetAuditRec(
	ValueStruct		*mcp,
	RecordStruct		*rec)
{
	time_t now;
	struct	tm	tm_now;
	ValueStruct	*audit;

	audit = rec->value;
	now = time(NULL);
	localtime_r(&now, &tm_now);
	SetValueDateTime(GetItemLongName(audit, "exec_date"), tm_now);
	CopyValuebyName(audit, "func", mcp, "func");
	CopyValuebyName(audit, "result", mcp, "rc");
	CopyValuebyName(audit, "username", mcp, "dc.user");
	CopyValuebyName(audit, "term", mcp, "dc.term");
	CopyValuebyName(audit, "windowname", mcp, "dc.window");
	CopyValuebyName(audit, "widget", mcp, "dc.widget");
	CopyValuebyName(audit, "event", mcp, "dc.event");
	CopyValuebyName(audit, "comment", mcp, "db.logcomment");
	return (rec);
}

extern	void
AuditLog(
	ValueStruct		*mcp)
{
	int	i;
	DBG_Struct	*dbg;
	ValueStruct *ret;
	DB_FUNC	func;
	RecordStruct		*rec;
ENTER_FUNC;
	rec = ThisEnv->auditrec;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		if		( dbg->auditlog > 0 ) {
			rec = SetAuditRec(mcp, rec);
			if		(  (func = LookupFUNC(dbg,"DBAUDITLOG")) != NULL){
				ret = (*func)(dbg,NULL,rec,rec->value);
				if ( ret != NULL ){
					FreeValueStruct(ret);
				}
			}
		}
	}
LEAVE_FUNC;
}

