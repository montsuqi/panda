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
*/
#define	TIMER
#define	DEBUG
#define	TRACE

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

#include	"defaults.h"
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"DDparser.h"
#include	"dbgroup.h"
#include	"blobreq.h"
#include	"debug.h"

extern	void
InitDB_Process(
	NETFILE	*fp)
{
ENTER_FUNC;
	fpBlob = fp;
LEAVE_FUNC;
}

typedef	void	(*DB_FUNC2)(DBG_Instance *, DBCOMM_CTRL *);

typedef	struct {
	char			*fname;
	DBG_Instance	*dbg;
}	_ExecDBFuncArg;

static	void
_ExecDBFunc(
	char	*rname,
	RecordStruct	*rec,
	_ExecDBFuncArg	*arg)
{
	DB_Struct	*db = RecordDB(rec);
	DBG_Instance	*dbg = arg->dbg;
	int				ix;
	DB_Operation	*op;

ENTER_FUNC;
	if		(  ( ix = (int)(long)g_hash_table_lookup(db->opHash,arg->fname) )  >  0  ) {
		if		(  ( op = db->ops[ix-1] )  !=  NULL  ) {
			if		(  dbg->class->func->primitive->record  !=  NULL  ) {
				if		(  !(*dbg->class->func->primitive->record)(dbg,arg->fname,rec)  ) {
					Warning("function not found [%s]\n",arg->fname);
				}
			}
		}
	}
LEAVE_FUNC;
}

static	int
ExecFunction(
	DBG_Instance	*dbg,
	char			*fname,
	Bool			fAll)
{
	DBCOMM_CTRL	ctrl;
	DB_FUNC2	func;
	_ExecDBFuncArg	arg;

ENTER_FUNC;
	dbgprintf("func  = [%s]",fname);
	if		(  dbg  !=  NULL  ) {
		dbgprintf("name  = [%s]",dbg->class->name);
	}
	dbgprintf("fAll = [%s]",fAll?"TRUE":"FALSE");
	ctrl.rc = 0;
	if		(  dbg  ==  NULL  ) {
		fprintf(stderr,"DBG_Instance is NULL.\n");
		exit(1);
	} else {
		if		(	(  fAll  )
				||	(  dbg->class->dbt  !=  NULL  ) ) { 
			if		(  ( func = (DB_FUNC2)g_hash_table_lookup(dbg->class->func->table,fname) )
					   !=  NULL  ) {
				(*func)(dbg,&ctrl);
				dbgprintf("ctrl.rc   = [%d]",ctrl.rc);
				if		(  dbg->class->dbt  !=  NULL  ) {
					arg.fname = fname;
					arg.dbg = dbg;
					g_hash_table_foreach(dbg->class->dbt,(GHFunc)_ExecDBFunc,&arg);
				}
			} else {
				Warning("function not found [%s]\n",fname);
				ctrl.rc = MCP_BAD_FUNC;
			}
		}
	}
LEAVE_FUNC;
	return	(ctrl.rc); 
}

extern	int
ExecDBOP(
	DBG_Instance	*dbg,
	char			*sql,
	int				usage)
{
	int		rc;

	rc = dbg->class->func->primitive->exec(dbg, sql, TRUE, usage);
	return	(rc);
}

extern	int
ExecRedirectDBOP(
	DBG_Instance	*dbg,
	char			*sql,
	int				usage)
{
	int		rc;

	rc = dbg->class->func->primitive->exec(dbg,sql,FALSE, usage);
	return	(rc);
}

extern	ValueStruct	*
ExecDB_Process(
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args,
	DB_Environment	*env)
{
	DB_FUNC	func;
	DBG_Class		*dbg;
	ValueStruct		*ret;
#ifdef	TIMER
	struct	timeval	tv;
	long	ever
		,	now;
#endif

ENTER_FUNC;
	dbgprintf("func = [%s] %s",ctrl->func,(rec == NULL)?"NULL":"rec");
	ctrl->rc = 0;
#ifdef	TIMER
	gettimeofday(&tv,NULL);
	ever = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
#endif
	ret = NULL;
	if		(  rec  ==  NULL  ) {
		fprintf(stderr,"RecordStruct is NULL.\n");
		exit(1);
	} else {
		dbgprintf("rec->name = [%s]",rec->name);
		dbg = RecordDB(rec)->dbg;
		dbgprintf("dbg->name = [%s]",dbg->name);
		if		(  ( func = g_hash_table_lookup(dbg->func->table,ctrl->func) )
				   ==  NULL  ) {
			ret = (*dbg->func->primitive->access)(env->entry[dbg->id],ctrl->func,ctrl,rec,args);
		} else {
			ret = (*func)(env->entry[dbg->id],ctrl,rec,args);
		}
		if		(  ctrl->rc  <  0  ) {
			Warning("bad function [%s] rc = %d\n",ctrl->func, ctrl->rc);
		}
	}
#ifdef	TIMER
	gettimeofday(&tv,NULL);
	now = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
	if		(  rec  !=  NULL  ) {
		dbgprintf("DB  %s:%s process time %6ld(ms)",
			   ctrl->func,rec->name,(now - ever));
	} else {
		dbgprintf("DB  %s    process time %6ld(ms)",
			   ctrl->func,(now - ever));
	}
#endif
LEAVE_FUNC;
	return	(ret);
}

static	int
ExecDBG_Operation(
	DBG_Instance	*dbg,
	char			*name)
{
	return ExecFunction(dbg,name,FALSE);
}

extern	int
TransactionStart(
	DBG_Instance *dbg)
{
	int		rc;

ENTER_FUNC;
	rc = ExecDBG_Operation(dbg,"DBSTART");
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionAllStart(
	DB_Environment	*env)
{
	int		i;
	int		rc;

ENTER_FUNC;
	rc = 0;
	for	( i = 0 ; i < env->nNode ; i ++ ) {
		rc += ExecDBG_Operation(env->entry[i],"DBSTART");
	}
	env->dbstatus = DB_STATUS_START;
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionEnd(
	DBG_Instance *dbg)
{
	int		rc;

ENTER_FUNC;
	rc = ExecDBG_Operation(dbg,"DBCOMMIT");
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionAllEnd(
	DB_Environment	*env)
{
	int		i;
	int		rc;
ENTER_FUNC;
	rc = 0;
	for	( i = 0 ; i < env->nNode ; i ++ ) {
		rc += ExecDBG_Operation(env->entry[i],"DBCOMMIT");
	}
	env->dbstatus = DB_STATUS_CONNECT;
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionRollback(
	DBG_Instance *dbg)
{
	int		rc;

ENTER_FUNC;
	rc = ExecDBG_Operation(dbg,"DBROLLBACK");
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionAllRollback(
	DB_Environment	*env)
{
	int		i;
	int		rc;
ENTER_FUNC;
	rc = 0;
	for	( i = 0 ; i < env->nNode ; i ++ ) {
		rc += ExecDBG_Operation(env->entry[i],"DBROLLBACK");
	}
	env->dbstatus = DB_STATUS_CONNECT;
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionPrepare(
	DBG_Instance *dbg)
{
	int		rc;

ENTER_FUNC;
	rc = ExecDBG_Operation(dbg,"DBPREPARE");
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionAllPrepare(
	DB_Environment	*env)
{
	int		i;
	int		rc;
ENTER_FUNC;
	rc = 0;
	for	( i = 0 ; i < env->nNode ; i ++ ) {
		rc += ExecDBG_Operation(env->entry[i],"DBPREPARE");
	}
	env->dbstatus = DB_STATUS_PREPARE;
LEAVE_FUNC;
	return	(rc);
}

extern	int
GetDBStatus(
	DB_Environment	*env)
{
	DBG_Instance	*dbg;
	int		i;
	int     rc;
ENTER_FUNC;
	rc = DB_STATUS_NOCONNECT; 
	for	( i = 0 ; i < env->nNode ; i ++ ) {
		dbg = env->entry[i];
		if		(  rc  <  dbg->update.dbstatus  )	{
			rc = dbg->update.dbstatus;
		}
	}
LEAVE_FUNC;
	return rc;
}

extern	void
RedirectError(void)
{
	DBG_Instance	*dbg;
	int		i;
ENTER_FUNC;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisDB_Environment->entry[i];
		CloseDB_RedirectPort(dbg);
	}
LEAVE_FUNC;
}

extern	DBG_Instance	*
NewDBG_Instance(
	DBG_Class		*class,
	DB_Environment	*env)
{
	DBG_Instance	*dbg;

ENTER_FUNC;
	dbg = New(DBG_Instance);
	dbg->class = class;
	dbg->fpLog = NULL;
	dbg->redirectData = NULL;
	dbg->checkData = NULL;

	dbg->update.dbstatus = DB_STATUS_NOCONNECT;
	dbg->update.conn = NULL;
	dbg->readonly.dbstatus = DB_STATUS_NOCONNECT;
	dbg->readonly.conn = NULL;
	dbg->env = env;
LEAVE_FUNC;

	return	(dbg);
}

extern	DBG_Instance	*
OpenDB(
	DBG_Class 		*class,
	DB_Environment	*env)
{
	DBG_Instance	*dbg;

	dbg = NewDBG_Instance(class,env);
	if		(  ExecDBG_Operation(dbg,"DBOPEN")  !=  0  ) {
		xfree(dbg);
		dbg = NULL;
	}
	return	(dbg);
}

extern	DB_Environment	*
NewDB_Environment(
	int		n)
{
	DB_Environment	*env;

	env = (DB_Environment *)xmalloc(sizeof(DB_Environment) + sizeof(DBG_Instance *) * n);
	env->nNode = n;
	env->dbstatus = DB_STATUS_NOCONNECT;

	return	(env);
}

extern	DB_Environment	*
OpenAllDB(void)
{
	int		i;
	DB_Environment	*env;

	env = NewDB_Environment(ThisEnv->cDBG);
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbgprintf("i = %d id = %d",i,ThisEnv->DBG[i]->id);
		env->entry[ThisEnv->DBG[i]->id] =  OpenDB(ThisEnv->DBG[i],env);
	}
	return	(env);
}

extern	int
OpenRedirectDB(
	DBG_Instance *dbg)
{
	return ExecFunction(dbg,"DBOPEN",TRUE);
}

extern	int
CloseRedirectDB(
	DBG_Instance	*dbg)
{
	return ExecFunction(dbg,"DBDISCONNECT",TRUE);
}

extern	int
TransactionRedirectStart(
	DBG_Instance	*dbg)
{
	return ExecFunction(dbg,"DBSTART",TRUE);
}

extern	int
TransactionRedirectEnd(
	DBG_Instance	*dbg)
{
	int rc;

	rc = ExecFunction(dbg,"DBCOMMIT",TRUE);

	return rc;
}

extern	int
TransactionRedirectRollback(
	DBG_Instance	*dbg)
{
	int rc;

	rc = ExecFunction(dbg,"DBROLLBACK",TRUE);

	return rc;
}

extern	int
TransactionRedirectPrepare(
	DBG_Instance	*dbg)
{
	int rc;

	rc = ExecFunction(dbg,"DBPREPARE",TRUE);

	return rc;
}

extern	int
CloseDB(
	DBG_Instance	*dbg)
{
	return ExecDBG_Operation(dbg,"DBDISCONNECT");
}

extern	int
CloseAllDB(
	DB_Environment	*env)
{
	int		i;
	int		rc;

ENTER_FUNC;
	rc = 0;
	if		(  env  !=  NULL  ) {
		for	( i = 0 ; i < env->nNode ; i ++ ) {
			rc += CloseDB(env->entry[i]);
		}
	}
	xfree(env);
LEAVE_FUNC;
	return	(rc);
}

/*	utility	*/

extern	Port	*
GetDB_Port(
	DBG_Class	*dbg,
	int			usage)
{
	Port	*port;
	int		i;

ENTER_FUNC;
	for	( i = 0 ; i < dbg->nServer ; i ++ ) {
		dbgprintf("usage = %d",dbg->server[i].usage);
		if		(	(  dbg->server[i].usage  ==  usage  )
				&&	(  dbg->server[i].port  !=  NULL  ) )	break;
	}
	dbgprintf("i = %d",i);
	if		(  i  ==  dbg->nServer  ) {
		dbgmsg("port null");
		port = NULL;
	} else {
		port = dbg->server[i].port;
	}
LEAVE_FUNC;
	return (port);
}

extern	char	*
GetDB_Host(
	DBG_Class	*dbg,
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
	DBG_Class	*dbg,
	int			usage)
{
	char	*port;

ENTER_FUNC;
	if		(  DB_Port  !=  NULL  ) {
		port = DB_Port;
	} else {
		port = IP_PORT(GetDB_Port(dbg,usage));
	}
LEAVE_FUNC;
	return (port);
}

extern	char	*
GetDB_DBname(
	DBG_Class	*dbg,
	int			usage)
{
	char	*name;
	int		i;

	if		(  DB_Name  !=  NULL  ) {
		name = DB_Name;
	} else {
		for	( i = 0 ; i < dbg->nServer ; i ++ ) {
			if		(	(  dbg->server[i].usage  ==  usage  )
					&&	(  dbg->server[i].dbname !=  NULL  ) )	break;
		}
		if		(  i  ==  dbg->nServer  ) {
			name = NULL;
		} else {
			name = dbg->server[i].dbname;
		}
	}
	return	(name);
}

extern	char	*
GetDB_User(
	DBG_Class	*dbg,
	int			usage)
{
	char	*name;
	int		i;

	if		(  DB_User  !=  NULL  ) {
		name = DB_User;
	} else {
		for	( i = 0 ; i < dbg->nServer ; i ++ ) {
			if		(	(  dbg->server[i].usage  ==  usage  )
					&&	(  dbg->server[i].user   !=  NULL   ) )	break;
		}
		if		(  i  ==  dbg->nServer  ) {
			name = NULL;
		} else {
			name = dbg->server[i].user;
		}
	}
	return	(name);
}

extern	char	*
GetDB_Pass(
	DBG_Class	*dbg,
	int			usage)
{
	char	*pass;
	int		i;

	if		(  DB_Pass  !=  NULL  ) {
		pass = DB_Pass;
	} else {
		for	( i = 0 ; i < dbg->nServer ; i ++ ) {
			if		(	(  dbg->server[i].usage  ==  usage  )
					&&	(  dbg->server[i].pass   !=  NULL   ) )	break;
		}
		if		(  i  ==  dbg->nServer  ) {
			pass = NULL;
		} else {
			pass = dbg->server[i].pass;
		}
	}
	return	(pass);
}

extern	char	*
GetDB_Sslmode(
	DBG_Class	*dbg,
	int			usage)
{
	char	*sslmode;
	int		i;

	if		(  DB_Sslmode  !=  NULL  ) {
		sslmode = DB_Sslmode;
	} else {
		for	( i = 0 ; i < dbg->nServer ; i ++ ) {
			if		(	(  dbg->server[i].usage  ==  usage  )
					&&	(  dbg->server[i].sslmode   !=  NULL   ) )	break;
		}
		if		(  i  ==  dbg->nServer  ) {
			sslmode = NULL;
		} else {
			sslmode = dbg->server[i].sslmode;
		}
	}
	return	(sslmode);
}

extern	void
MakeCTRL(
	DBCOMM_CTRL	*ctrl,
	ValueStruct	*mcp)
{
	strcpy(ctrl->func,ValueStringPointer(GetItemLongName(mcp,"func")));
	ctrl->rc = ValueInteger(GetItemLongName(mcp,"rc"));
	ctrl->blocks = ValueInteger(GetItemLongName(mcp,"db.path.blocks"));
	ctrl->rno = ValueInteger(GetItemLongName(mcp,"db.path.rname"));
	ctrl->pno = ValueInteger(GetItemLongName(mcp,"db.path.pname"));
	ctrl->count = ValueInteger(GetItemLongName(mcp,"db.rcount"));
#if	0
	ctrl->limit = ValueInteger(GetItemLongName(mcp,"db.limit"));
#else
	if		(  ValueInteger(GetItemLongName(mcp,"version"))  >=  2  ) {
		ctrl->limit = ValueInteger(GetItemLongName(mcp,"db.limit"));
	} else {
		ctrl->limit = 1;
	}
	if		(  ValueInteger(GetItemLongName(mcp,"version"))  >=  3  ) {
		ctrl->offset = ValueInteger(GetItemLongName(mcp,"db.offset"));
	} else {
		ctrl->offset = 0;
	}
#endif
#ifdef	DEBUG
	DumpDB_Node(ctrl);
#endif
}

extern	ValueStruct	*
_GetDB_Argument(
	RecordStruct	*rec,
	char			*pname,
	char			*func,
	int				*apno)
{
	ValueStruct	*value;
	PathStruct		*path;
	DB_Operation	*op;
	int		pno
		,	ono;

ENTER_FUNC;
	value = rec->value;
	if		(	(  pname  !=  NULL  )
			&&	(  ( pno = (int)(long)g_hash_table_lookup(RecordDB(rec)->paths,
														  pname) )  !=  0  ) ) {
		pno --;
		path = RecordDB(rec)->path[pno];
		value = ( path->args != NULL ) ? path->args : value;
		if		(	(  func  !=  NULL  )
				&&	( ( ono = (int)(long)g_hash_table_lookup(path->opHash,func) )  !=  0  ) ) {
			op = path->ops[ono-1];
			value = ( op->args != NULL ) ? op->args : value;
		}
	} else {
		pno = 0;
	}
	if		(  apno  !=  NULL  ) {
		*apno = pno;
	}
LEAVE_FUNC;
	return	(value);
}

extern	RecordStruct	*
MakeCTRLbyName(
	ValueStruct		**value,
	DBCOMM_CTRL	*rctrl,
	char	*rname,
	char	*pname,
	char	*func)
{
	DBCOMM_CTRL		ctrl;
	RecordStruct	*rec;
	int			rno;

ENTER_FUNC;
	ctrl.rno = 0;
	ctrl.pno = 0;
	ctrl.blocks = 0;
	ctrl.count = rctrl->count;
	ctrl.limit = rctrl->limit;
	ctrl.offset = rctrl->offset;

	*value = NULL;
	if		(	(  rname  !=  NULL  )
			&&	(  ( rno = (int)(long)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) ) {
		ctrl.rno = rno - 1;
		rec = ThisDB[rno-1];
		*value = _GetDB_Argument(rec,pname,func,&ctrl.pno);
	} else {
		rec = NULL;
	}
	if		(  rctrl  !=  NULL  ) {
		strcpy(ctrl.func,func);
		*rctrl = ctrl;
	}
#ifdef	DEBUG
	DumpDB_Node(&ctrl);
#endif
LEAVE_FUNC;
	return	(rec);
}

extern	void
MakeMCP(
	ValueStruct	*mcp,
	DBCOMM_CTRL	*ctrl)
{
	strcpy(ValueStringPointer(GetItemLongName(mcp,"func")),ctrl->func);
	ValueInteger(GetItemLongName(mcp,"rc")) = ctrl->rc;
	ValueInteger(GetItemLongName(mcp,"db.path.blocks")) = ctrl->blocks;
	ValueInteger(GetItemLongName(mcp,"db.path.rname")) = ctrl->rno;
	ValueInteger(GetItemLongName(mcp,"db.path.pname")) = ctrl->pno;
	ValueInteger(GetItemLongName(mcp,"db.rcount")) = ctrl->count;
	ValueInteger(GetItemLongName(mcp,"db.limit")) = ctrl->limit;
	ValueInteger(GetItemLongName(mcp,"db.offset")) = ctrl->offset;
}

#ifdef	DEBUG
extern	void
DumpDB_Node(
	DBCOMM_CTRL	*ctrl)
{
	printf("func   = [%s]\n",ctrl->func);
	printf("blocks = %d\n",ctrl->blocks);
	printf("rno    = %d\n",ctrl->rno);
	printf("pno    = %d\n",ctrl->pno);
	printf("count  = %d\n",ctrl->count);
	printf("limit  = %d\n",ctrl->limit);
	printf("offset = %d\n",ctrl->offset);
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
	p += sprintf(p,		"offset int;");
	p += sprintf(p,		"func	varchar(%d);",SIZE_FUNC);
	p += sprintf(p,		"rname	varchar(%d);",SIZE_NAME);
	p += sprintf(p,		"pname	varchar(%d);",SIZE_NAME);
	p += sprintf(p,	"};");
	rec = ParseRecordMem(buff);

	return	(rec);
}

