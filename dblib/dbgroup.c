/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
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

typedef	void	(*DB_FUNC2)(DBG_Struct *, DBCOMM_CTRL *);

static	void
_ExecDBFunc(
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
			if		(  dbg->func->primitive->record  !=  NULL  ) {
				if		(  !(*dbg->func->primitive->record)(dbg,fname,rec)  ) {
					Warning("function not found [%s]\n",fname);
				}
			}
		}
	}
LEAVE_FUNC;
}

static	int
ExecFunction(
	DBG_Struct	*dbg,
	char		*name,
	Bool		fAll)
{
	DBCOMM_CTRL	ctrl;
	DB_FUNC2	func;
	int			i;

ENTER_FUNC;
#ifdef	DEBUG
	printf("func  = [%s]\n",name);
	if		(  dbg  !=  NULL  ) {
		printf("name  = [%s]\n",dbg->name);
	}
	printf("fAll = [%s]\n",fAll?"TRUE":"FALSE");
#endif
	ctrl.rc = 0;
	if		(  dbg  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
			ctrl.rc += ExecFunction(dbg,name,fAll);
		}
	} else {
		if		(	(  fAll  )
				||	(  dbg->dbt  !=  NULL  ) ) { 
			if		(  ( func = (DB_FUNC2)g_hash_table_lookup(dbg->func->table,name) )
					   !=  NULL  ) {
				(*func)(dbg,&ctrl);
				dbgprintf("ctrl.rc   = [%d]",ctrl.rc);
				if		(  dbg->dbt  !=  NULL  ) {
					g_hash_table_foreach(dbg->dbt,(GHFunc)_ExecDBFunc,name);
				}
			} else {
				Warning("function not found [%s]\n",name);
				ctrl.rc = MCP_BAD_FUNC;
			}
		}
	}
LEAVE_FUNC;
	return	(ctrl.rc); 
}

extern	int
ExecDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	int		rc;

	rc = dbg->func->primitive->exec(dbg,sql,TRUE);
	return	(rc);
}

extern	int
ExecRedirectDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	int		rc;

	rc = dbg->func->primitive->exec(dbg,sql,FALSE);
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
	int				i;
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
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
			ctrl->rc += ExecFunction(dbg,ctrl->func,FALSE);
			dbgprintf("dbg->name = [%s]",dbg->name);
			dbgprintf("ctrl->rc  = [%d]",ctrl->rc);
		}
	} else {
		dbgprintf("rec->name = [%s]",rec->name);
		dbg = RecordDB(rec)->dbg;
		dbgprintf("dbg->name = [%s]",dbg->name);
		if		(  ( func = g_hash_table_lookup(dbg->func->table,ctrl->func) )
				   ==  NULL  ) {
			ret = (*dbg->func->primitive->access)(dbg,ctrl->func,ctrl,rec,args);
		} else {
			ret = (*func)(dbg,ctrl,rec,args);
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
	DBG_Struct	*dbg,
	char		*name)
{
	return ExecFunction(dbg,name,FALSE);
}

extern	int
TransactionStart(
	DBG_Struct *dbg)
{
	int		i;
	int		rc;

ENTER_FUNC;
	NewPool("Transaction");
	if		(  dbg  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		}
	}
	rc = ExecDBG_Operation(dbg,"DBSTART");
LEAVE_FUNC;
	return	(rc);
}

extern	int
TransactionEnd(
	DBG_Struct *dbg)
{
	int		i;
	int		rc;

ENTER_FUNC;
	rc = ExecDBG_Operation(dbg,"DBCOMMIT");
	if		(  dbg  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
		}
	}
	ReleasePoolByName("Transaction");
LEAVE_FUNC;
	return	(rc);
}

extern	int
GetDBStatus(void)
{
	DBG_Struct	*dbg;
	int		i;
	int     rc = 0;
ENTER_FUNC;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		if ( rc < dbg->dbstatus ){
			rc = dbg->dbstatus;
		}
	}
LEAVE_FUNC;
	return rc;
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
	NewPool("Transaction");
	return ExecFunction(dbg,"DBSTART",TRUE);
}

extern	int
TransactionRedirectEnd(
	DBG_Struct *dbg)
{
	int rc;

	rc = ExecFunction(dbg,"DBCOMMIT",TRUE);
	ReleasePoolByName("Transaction");

	return rc;
}

extern	int
CloseDB(
	DBG_Struct *dbg)
{
	return ExecDBG_Operation(dbg,"DBDISCONNECT");
}

/*	utility	*/

extern	char	*
GetDB_Host(
	DBG_Struct	*dbg)
{
	char	*host;

	if		(  DB_Host  !=  NULL  ) {
		host = DB_Host;
	} else {
		if		(  dbg->port  ==  NULL  ) {
			host = NULL;
		} else {
			host = IP_HOST(dbg->port);
		}
	}
	return (host);
}

extern	char	*
GetDB_Port(
	DBG_Struct	*dbg)
{
	char	*port;

	if		(  DB_Port  !=  NULL  ) {
		port = DB_Port;
	} else {
		if		(  dbg->port  ==  NULL  ) {
			port = NULL;
		} else {
			port = IP_PORT(dbg->port);
		}
	}
	return (port);
}

extern	char	*
GetDB_DBname(
	DBG_Struct	*dbg)
{
	return (( DB_Name != NULL ) ? DB_Name : dbg->dbname);
}

extern	char	*
GetDB_User(
	DBG_Struct	*dbg)
{
	return (( DB_User != NULL ) ? DB_User : dbg->user);
}

extern	char	*
GetDB_Pass(
	DBG_Struct	*dbg)
{
	return (( DB_Pass != NULL ) ? DB_Pass : dbg->pass);
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
	ctrl->limit = ValueInteger(GetItemLongName(mcp,"db.limit"));
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

