/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/socket.h>	/*	for SOCK_STREAM	*/
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"value.h"
#include	"misc.h"
#include	"dbgroup.h"
#include	"Postgres.h"
#include	"debug.h"

#include	"directory.h"

static	GHashTable	*DBMS_Table;

extern	void
EnterDB_Function(
	char	*name,
	DB_OPS	*ops,
	char	*commentStart,
	char	*commentEnd)
{
	DB_Func	*func;
	int		i;

	if		(  ( func = (DB_Func *)g_hash_table_lookup(DBMS_Table,name) )
			   ==  NULL  ) {
		func = NewDB_Func();
		g_hash_table_insert(DBMS_Table,name,func);
		func->commentStart = commentStart;
		func->commentEnd = commentEnd;
	}
	for	( i = 0 ; ops[i].name != NULL ; i ++ ) {
		if		(  !strcmp(ops[i].name,"access")  ) {
			func->access = (DB_FUNC_NAME)ops[i].func;
		} else	
		if		(  !strcmp(ops[i].name,"exec")  ) {
			func->exec   = (DB_EXEC)ops[i].func;
		}
		if		(  g_hash_table_lookup(func->table,ops[i].name)  ==  NULL  ) {
			g_hash_table_insert(func->table,ops[i].name,ops[i].func);
		}
	}
}

extern	void
InitDBG(void)
{
dbgmsg(">InitDBG");
	DBMS_Table = NewNameHash();
dbgmsg("<InitDBG");
}

extern	void
SetUpDBG(void)
{
	DBG_Struct	*dbg;
	int		i;
dbgmsg(">SetUpDBG");
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		dbg->func = (DB_Func *)g_hash_table_lookup(DBMS_Table,dbg->type);
	}
dbgmsg("<SetUpDBG");
}

typedef	void	(*DB_FUNC2)(DBCOMM_CTRL *, DBG_Struct *);

static	int
ExecFunction(
	char	*gname,
	DBG_Struct	*dbg,
	char	*name)
{
	DBCOMM_CTRL	ctrl;
	DB_FUNC2	func;

dbgmsg(">ExecFunction");
#ifdef	DEBUG
	printf("group = [%s]\n",gname);
	printf("func  = [%s]\n",name);
	printf("name  = [%s]\n",dbg->name);
#endif
	if		(  dbg->dbt  !=  NULL  ) { 
		if		(  ( func = (DB_FUNC2)g_hash_table_lookup(dbg->func->table,name) )
				   !=  NULL  ) {
			(*func)(&ctrl,dbg);
		} else {
			printf("function not found [%s]\n",name);
		}
	}
dbgmsg("<ExecFunction");
	return	(ctrl.rc); 
}

extern	void
ExecDBG_Operation(
	DBG_Struct	*dbg,
	char		*name)
{
	ExecFunction(NULL,dbg,name);
}

extern	void
ExecDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	dbg->func->exec(dbg,sql);
}

extern	int
ExecDB_Function(
	char	*name,
	char	*tname,
	RecordStruct	*rec)
{
	DB_FUNC	func;
	DBCOMM_CTRL	ctrl;
	DBG_Struct		*dbg;
	int				i;

dbgmsg(">ExecDB_Function");
	if		(  tname  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
			ExecFunction(dbg->name,dbg,name);
		}
	} else {
		dbg = rec->opt.db->dbg;
		if		(  ( func = g_hash_table_lookup(dbg->func->table,ctrl.func) )
				   ==  NULL  ) {
			if		(  !(*dbg->func->access)(name,&ctrl,rec)  ) {
				printf("function not found [%s]\n",name);
			}
		} else {
			(*func)(&ctrl,rec);
		}
	}
dbgmsg("<ExecDB_Function");
	return	(ctrl.rc);
}

extern	void
ExecDB_Process(
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec)
{
	DB_FUNC	func;
	DBG_Struct		*dbg;
	int				i;

dbgmsg(">ExecDB_Process");
#ifdef	DEBUG	
	printf("func = [%s] %s\n",ctrl->func,(rec == NULL)?"NULL":"rec");
#endif
	if		(  rec  ==  NULL  ) { 
		ctrl->rc = 0;
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
			ctrl->rc += ExecFunction(dbg->name,dbg,ctrl->func);
		}
	} else {
		dbg = rec->opt.db->dbg;
		if		(  ( func = g_hash_table_lookup(dbg->func->table,ctrl->func) )
				   ==  NULL  ) {
			if		(  !(*dbg->func->access)(ctrl->func,ctrl,rec)  ) {
				printf("function not found [%s]\n",ctrl->func);
			}
		} else {
			(*func)(ctrl,rec);
		}
	}
dbgmsg("<ExecDB_Process");
}
