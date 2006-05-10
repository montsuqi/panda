/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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

#include	"defaults.h"
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
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
	DB_Struct	*db = rec->opt.db;
	DBG_Struct	*dbg = db->dbg;
	int			ix;
	DB_Operation	*op;

ENTER_FUNC;
	if		(  ( ix = (int)g_hash_table_lookup(db->opHash,fname) )  >  0  ) {
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
	if		(  dbg  ==  NULL  ) {
		ctrl.rc = 0;
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

extern	void
ExecDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	dbg->func->primitive->exec(dbg,sql,TRUE);
}

extern	int
ExecRedirectDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	return dbg->func->primitive->exec(dbg,sql,FALSE);
}

extern	void
ExecDB_Process(
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DB_FUNC	func;
	DBG_Struct		*dbg;
	int				i;

ENTER_FUNC;
#ifdef	DEBUG	
	printf("func = [%s] %s\n",ctrl->func,(rec == NULL)?"NULL":"rec");
#endif
	if		(  rec  ==  NULL  ) { 
		ctrl->rc = 0;
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
			ctrl->rc += ExecFunction(dbg,ctrl->func,FALSE);
		}
	} else {
		dbg = rec->opt.db->dbg;
		if		(  ( func = g_hash_table_lookup(dbg->func->table,ctrl->func) )
				   ==  NULL  ) {
			if		(  !(*dbg->func->primitive->access)(dbg,ctrl->func,ctrl,rec,args)  ) {
				Warning("function not found [%s]\n",ctrl->func);
				ctrl->rc = MCP_BAD_FUNC;
			}
		} else {
			(*func)(dbg,ctrl,rec,args);
		}
	}
LEAVE_FUNC;
}

static	int
ExecDBG_Operation(
	DBG_Struct	*dbg,
	char		*name)
{
	return ExecFunction(dbg,name,FALSE);
}

extern	void
TransactionStart(
	DBG_Struct *dbg)
{
	int		i;

ENTER_FUNC;
	NewPool("Transaction");
	if		(  dbg  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		}
	}
	ExecDBG_Operation(dbg,"DBSTART");
LEAVE_FUNC;
}

extern	void
TransactionEnd(
	DBG_Struct *dbg)
{
	int		i;
ENTER_FUNC;
	ExecDBG_Operation(dbg,"DBCOMMIT");
	if		(  dbg  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
		}
	}
	ReleasePoolByName("Transaction");
LEAVE_FUNC;
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

