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
#include	"blobreq.h"
#include	"debug.h"

typedef	struct {
	MonObjectType	obj;
	char	*fname;
	Bool	fImport;
}	BLOBDir;

static	GHashTable	*DBMS_Table;
static	char		*MONDB_LoadPath;

extern	DB_Func	*
NewDB_Func(void)
{
	DB_Func	*ret;

	ret = New(DB_Func);
	ret->primitive = NULL;
	ret->table = NewNameHash();
	return	(ret);
}

extern	DB_Func	*
EnterDB_Function(
	char	*name,
	DB_OPS	*ops,
	DB_Primitives	*primitive,
	char	*commentStart,
	char	*commentEnd)
{
	DB_Func	*func;
	int		i;

dbgmsg(">EnterDB_Function");
	dbgprintf("Enter [%s]\n",name); 
	if		(  ( func = (DB_Func *)g_hash_table_lookup(DBMS_Table,name) )
			   ==  NULL  ) {
		func = NewDB_Func();
		g_hash_table_insert(DBMS_Table,name,func);
		func->commentStart = commentStart;
		func->commentEnd = commentEnd;
		func->primitive = primitive;
	}
	for	( i = 0 ; ops[i].name != NULL ; i ++ ) {
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

static	char	*
TempName(
	char	*buff,
	size_t	size)
{
	int		fd;
	char	*ret;

	if		(  ( fd = mkstemp(buff) )  >  0  ) {
		ret = buff;
		close(fd);
	} else {
		ret = NULL;
	}
	return	(ret);
}

extern	char	*
FindBlobPool(
	DBG_Struct	*dbg,
	ValueStruct	*value)
{
	char	*name
		,	*fname;
	char	buff[SIZE_NAME+1];
	BLOBDir	*bd;

	if		(  ( bd = (BLOBDir *)g_hash_table_lookup(dbg->loPool,ValueObject(value)) )
			   ==  NULL  ) {
		strcpy(buff,"/tmp/XXXXXX");
		fname = TempName(buff,SIZE_NAME+1);
		if		(  RequestExportBLOB(fpBlob,ValueObject(value),fname)  ) {
			bd = New(BLOBDir);
			bd->obj = *ValueObject(value);
			fname = StrDup(fname);
			bd->fname = fname;
			bd->fImport = FALSE;
			g_hash_table_insert(dbg->loPool,&bd->obj,bd);
		} else
		if		(  RequestImportBLOB(fpBlob,ValueObject(value),fname)  ) {
			bd = New(BLOBDir);
			bd->obj = *ValueObject(value);
			fname = StrDup(fname);
			bd->fname = fname;
			bd->fImport = TRUE;
			g_hash_table_insert(dbg->loPool,&bd->obj,bd);
		} else {
			fname = NULL;
		}
	}
	return	(fname);
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
		dbg->id = i;
		dbgprintf("Entering [%s]\n",dbg->type);
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
InitDB_Process(
	NETFILE	*fp)
{
ENTER_FUNC;
	fpBlob = fp;
	InitDBG();
	SetUpDBG();
LEAVE_FUNC;
}

typedef	void	(*DB_FUNC2)(DBG_Struct *, DBCOMM_CTRL *);

static	int
ExecFunction(
	DBG_Struct	*dbg,
	char		*name,
	Bool		fAll)
{
	DBCOMM_CTRL	ctrl;
	DB_FUNC2	func;
	int			i;

dbgmsg(">ExecFunction");
#ifdef	DEBUG
	printf("func  = [%s]\n",name);
	if		(  dbg  !=  NULL  ) {
		printf("name  = [%s]\n",dbg->name);
	}
	printf("fAll = [%s]\n",fAll?"TRUE":"FALSE");
#endif
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
			} else {
				printf("function not found [%s]\n",name);
				ctrl.rc = MCP_BAD_FUNC;
			}
		}
	}
dbgmsg("<ExecFunction");
	return	(ctrl.rc); 
}

extern	void
ExecDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	dbg->func->primitive->exec(dbg,sql,TRUE);
}

extern	void
ExecRedirectDBOP(
	DBG_Struct	*dbg,
	char		*sql)
{
	char	*p;

	dbg->func->primitive->exec(dbg,sql,FALSE);
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

dbgmsg(">ExecDB_Process");
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
				printf("function not found [%s]\n",ctrl->func);
				ctrl->rc = MCP_BAD_FUNC;
			}
		} else {
			(*func)(dbg,ctrl,rec,args);
		}
	}
dbgmsg("<ExecDB_Process");
}

static	void
ExecDBG_Operation(
	DBG_Struct	*dbg,
	char		*name)
{
	ExecFunction(dbg,name,FALSE);
}

static	guint
ObjHash(
	MonObjectType	*obj)
{
	guint	ret;
	int		i;

	ret = obj->source;
	for	( i = 0 ; i < SIZE_OID / sizeof(unsigned int) ; i ++ ) {
		ret += obj->id.el[i];
	}
	return	(ret);
}

static	gint
ObjCompare(
	MonObjectType	*s1,
	MonObjectType	*s2)
{
	return	(!memcmp(s1,s2,sizeof(MonObjectType)));
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
			ThisEnv->DBG[i]->loPool = g_hash_table_new((GHashFunc)ObjHash,
													   (GCompareFunc)ObjCompare);
		}
	}
	ExecDBG_Operation(dbg,"DBSTART");
LEAVE_FUNC;
}

static	void
_ReleaseBLOB(
	char		*name,
	BLOBDir		*bd,
	DBG_Struct *dbg)
{
	if		(  bd->fImport  ) {
		RequestImportBLOB(fpBlob,&bd->obj,bd->fname);
	}
	unlink(bd->fname);
	xfree(bd->fname);
	xfree(bd);
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
			if		(  dbg->loPool  !=  NULL  ) {
				g_hash_table_foreach(dbg->loPool,(GHFunc)_ReleaseBLOB,dbg);
				g_hash_table_destroy(dbg->loPool);
				dbg->loPool = NULL;
			}
		}
	}
	ReleasePoolByName("Transaction");
LEAVE_FUNC;
}

extern	void
OpenDB(
	DBG_Struct *dbg)
{
	ExecDBG_Operation(dbg,"DBOPEN");
}

extern	void
OpenRedirectDB(
	DBG_Struct *dbg)
{
	ExecFunction(dbg,"DBOPEN",TRUE);
}

extern	void
TransactionRedirectStart(
	DBG_Struct *dbg)
{
	NewPool("Transaction");
	ExecFunction(dbg,"DBSTART",TRUE);
}

extern	void
TransactionRedirectEnd(
	DBG_Struct *dbg)
{
	ExecFunction(dbg,"DBCOMMIT",TRUE);
	ReleasePoolByName("Transaction");
}

extern	void
CloseDB(
	DBG_Struct *dbg)
{
	ExecDBG_Operation(dbg,"DBDISCONNECT");
}

