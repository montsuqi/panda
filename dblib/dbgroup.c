/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<dlfcn.h>
#include	<glib.h>

#include	"defaults.h"
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"load.h"
#include	"dbgroup.h"
#include	"debug.h"


static	GHashTable	*DBMS_Table;
static	char		*MONDB_LoadPath;

extern	DB_Func	*
NewDB_Func(void)
{
	DB_Func	*ret;

	ret = New(DB_Func);
	ret->exec = NULL;
	ret->access = NULL;
	ret->table = NewNameHash();
	return	(ret);
}

extern	DB_Func	*
EnterDB_Function(
	char	*name,
	DB_OPS	*ops,
	char	*commentStart,
	char	*commentEnd)
{
	DB_Func	*func;
	int		i;

dbgmsg(">EnterDB_Function");
#ifdef	DEBUG
	printf("Enter [%s]\n",name); 
#endif
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
dbgmsg("<EnterDB_Function");
	return	(func); 
}

static	void
InitDBG(void)
{
dbgmsg(">InitDBG");
	DBMS_Table = NewNameHash();
	if		(  ( MONDB_LoadPath = getenv("MONDB_LOAD_PATH") )  ==  NULL  ) {
		MONDB_LoadPath = MONTSUQI_LIBRARY_PATH;
	}
dbgmsg("<InitDBG");
}

static	void
SetUpDBG(void)
{
	DBG_Struct	*dbg;
	int		i;
	DB_Func	*func;
	char		funcname[SIZE_BUFF]
	,			filename[SIZE_BUFF];
	void		*handle;
	DB_Func		*(*f_init)(void);

dbgmsg(">SetUpDBG");
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
#ifdef	DEBUG
		printf("Entering [%s]\n",dbg->type);
#endif
		if		(  ( func = (DB_Func *)g_hash_table_lookup(DBMS_Table,dbg->type) )
				   ==  NULL  ) {
			sprintf(filename,"%s.so",dbg->type);
			dbgprintf("[%s][%s]",MONDB_LoadPath,filename);
			if		(  ( handle = LoadFile(MONDB_LoadPath,filename) )  !=  NULL  ) {
				sprintf(funcname,"Init%s",dbg->type);
				if		(  ( f_init = dlsym(handle,funcname) )
						   !=  NULL  ) {
					func = (*f_init)();
				} else {
					fprintf(stderr,
							"DB group type [%s] not found.(can not initialize)\n",
							dbg->type);
					exit(1);
				}
			} else {
				fprintf(stderr,
						"DB group type [%s] not found.(module not exist)\n",
						dbg->type);
				exit(1);
			}
		}
		dbg->func = func;
	}
dbgmsg("<SetUpDBG");
}

extern	void
InitDB_Process(void)
{
dbgmsg(">InitDB_Process");
	InitDBG();
	SetUpDBG();
dbgmsg("<InitDB_Process");
}

typedef	void	(*DB_FUNC2)(DBG_Struct *, DBCOMM_CTRL *);

static	int
ExecFunction(
	DBG_Struct	*dbg,
	char		*gname,
	char		*name)
{
	DBCOMM_CTRL	ctrl;
	DB_FUNC2	func;
	int			i;

dbgmsg(">ExecFunction");
#ifdef	DEBUG
	printf("group = [%s]\n",gname);
	printf("func  = [%s]\n",name);
	printf("name  = [%s]\n",dbg->name);
#endif
	if		(  dbg  ==  NULL  ) {
		for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
			dbg = ThisEnv->DBG[i];
			ctrl.rc += ExecFunction(dbg,dbg->name,name);
		}
	} else
	if		(  dbg->dbt  !=  NULL  ) { 
		if		(  ( func = (DB_FUNC2)g_hash_table_lookup(dbg->func->table,name) )
				   !=  NULL  ) {
			(*func)(dbg,&ctrl);
		} else {
			printf("function not found [%s]\n",name);
			ctrl.rc = MCP_BAD_FUNC;
		}
	} else {
		ctrl.rc = MCP_BAD_OTHER;
	}
dbgmsg("<ExecFunction");
	return	(ctrl.rc); 
}

extern	void
ExecDBG_Operation(
	DBG_Struct	*dbg,
	char		*name)
{
	ExecFunction(dbg,NULL,name);
}

extern	void
ExecDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	dbg->func->exec(dbg,sql);
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
			ctrl->rc += ExecFunction(dbg,dbg->name,ctrl->func);
		}
	} else {
		dbg = rec->opt.db->dbg;
		if		(  ( func = g_hash_table_lookup(dbg->func->table,ctrl->func) )
				   ==  NULL  ) {
			if		(  !(*dbg->func->access)(dbg,ctrl->func,ctrl,rec)  ) {
				printf("function not found [%s]\n",ctrl->func);
				ctrl->rc = MCP_BAD_FUNC;
			}
		} else {
			(*func)(dbg,ctrl,rec);
		}
	}
dbgmsg("<ExecDB_Process");
}
