/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
 *
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
#include	<unistd.h>
#include	<dlfcn.h>
#include	<glib.h>
#include	"defaults.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"DDparser.h"
#include	"LDparser.h"
#include	"DIparser.h"
#include	"DBparser.h"
#include	"BDparser.h"
#include	"DBDparser.h"
#include	"directory.h"
#include	"load.h"
#include	"debug.h"

static	GHashTable	*DBMS_Table;
static	char		*MONDB_LoadPath;

typedef struct {
	char *name;
	char *type;
} REQUIRED_DBG;

REQUIRED_DBG RDBG[] = {
	{"system", "System"},
	{"monblob", "MONBLOB"},
	{NULL,NULL}
};

static	DB_Func	*
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
	int		type,
	DB_Primitives	*primitive,
	char	*commentStart,
	char	*commentEnd)
{
	DB_Func	*func;
	int		i;

ENTER_FUNC;
	dbgprintf("Enter [%s]\n",name);
	if		(  ( func = (DB_Func *)g_hash_table_lookup(DBMS_Table,name) )
			   ==  NULL  ) {
		func = NewDB_Func();
		func->commentStart = commentStart;
		func->commentEnd = commentEnd;
		func->primitive = primitive;
		func->type = type;
		for	( i = 0 ; ops[i].name != NULL ; i ++ ) {
			if		(  g_hash_table_lookup(func->table,ops[i].name)  ==  NULL  ) {
				g_hash_table_insert(func->table,ops[i].name,ops[i].func);
			}
		}
	}
LEAVE_FUNC;
	return	(func);
}

extern	void
InitDirectory(void)
{
ENTER_FUNC;
	InitPool();
	RecParserInit();
	DB_ParserInit();
	LD_ParserInit();
	BD_ParserInit();
	DBD_ParserInit();
	DI_ParserInit();
LEAVE_FUNC;
}

extern	DBG_Struct	*
GetDBG(
	char	*name)
{
	DBG_Struct	*dbg;

	dbg = (DBG_Struct *)g_hash_table_lookup(ThisEnv->DBG_Table,name);
	return	(dbg);
}

extern	void
LoadDBG(
	DBG_Struct	*dbg)
{
	DB_Func	*func;
	char		funcname[SIZE_LONGNAME+1]
	,			filename[SIZE_LONGNAME+1];
	void		*handle;
	DB_Func		*(*f_init)(void);

ENTER_FUNC;
	dbgprintf("Entering [%s]\n",dbg->type);
	if		(  dbg->func  ==  NULL  ) {
		if		(  ( func = (DB_Func *)g_hash_table_lookup(DBMS_Table,dbg->type) )
				   ==  NULL  ) {
			sprintf(filename,"%s." SO_SUFFIX,dbg->type);
			dbgprintf("[%s][%s]",MONDB_LoadPath,filename);
			if		(  ( handle = LoadFile(MONDB_LoadPath,filename) )  !=  NULL  ) {
				sprintf(funcname,"Init%s",dbg->type);
				if		(  ( f_init = dlsym(handle,funcname) )
						   !=  NULL  ) {
					func = (*f_init)();
					g_hash_table_insert(DBMS_Table,dbg->type,func);
				} else {
					Error("DB group type [%s] not found.(can not initialize)\n",
						  dbg->type);
				}
			} else {
				perror("LoadFile");
				Error("DB group type [%s] not found.(module not exist)\n", dbg->type);
			}
		}
		dbg->func = func;
	}
LEAVE_FUNC;
}

extern	void
RegistDBG(
	DBG_Struct	*dbg)
{
	DBG_Struct	**dbga;

ENTER_FUNC;
	dbga = (DBG_Struct **)xmalloc(sizeof(DBG_Struct *) *
										  ( ThisEnv->cDBG + 1 ));
	if		(  ThisEnv->cDBG  >  0  ) {
		memcpy(dbga,ThisEnv->DBG,sizeof(DBG_Struct *) * ThisEnv->cDBG);
		xfree(ThisEnv->DBG);
	}
	ThisEnv->DBG = dbga;
	ThisEnv->DBG[ThisEnv->cDBG] = dbg;
	dbg->id = ThisEnv->cDBG;
	ThisEnv->cDBG ++;
	dbgprintf("dbg = [%s]\n",dbg->name);
	LoadDBG(dbg);
	g_hash_table_insert(ThisEnv->DBG_Table,dbg->name,dbg);
LEAVE_FUNC;
}

static	void
_AssignDBG(
	RecordStruct	**db,
	int				n)
{
	int		i;
	char	*gname;
	DBG_Struct	*dbg;

ENTER_FUNC;
	for	( i = 1 ; i < n ; i ++ ) {
		if		(  ( gname = RecordDB(db[i])->gname )  !=  NULL  ) {
			if		(  ( dbg = GetDBG(gname) )  ==  NULL  )	{
				Error("DB group [%s] not found", gname);
			}
			RecordDB(db[i])->dbg = dbg;
			xfree(gname);
			RecordDB(db[i])->gname = NULL;
		} else {
			dbg = RecordDB(db[i])->dbg;
		}
		if		(  dbg->dbt  ==  NULL  ) {
			dbg->dbt = NewNameHash();
		}
		if		(  g_hash_table_lookup(dbg->dbt,db[i]->name)  ==  NULL  ) {
			g_hash_table_insert(dbg->dbt,db[i]->name,db[i]);
		}
	}
LEAVE_FUNC;
}

static	void
AssignDBG(
	DI_Struct	*di)
{
	int		i;

ENTER_FUNC;
	if		(  di->DBG_Table  !=  NULL  ) {
		for	( i = 0 ; i < di->cLD ; i ++ ) {
			if		(  di->ld[i]->db  !=  NULL  ) {
				_AssignDBG(di->ld[i]->db,di->ld[i]->cDB);
			}
		}
		for	( i = 0 ; i < di->cBD ; i ++ ) {
			if		(  di->bd[i]->db  !=  NULL  ) {
				_AssignDBG(di->bd[i]->db,di->bd[i]->cDB);
			}
		}
		for	( i = 0 ; i < di->cDBD ; i ++ ) {
			if		(  di->db[i]->db  !=  NULL  ) {
				_AssignDBG(di->db[i]->db,di->db[i]->cDB);
			}
		}
	}
LEAVE_FUNC;
}

extern	void
SetDBGPath(
	char	*path)
{
	MONDB_LoadPath = path;
}

extern	void
InitDBG(void)
{
ENTER_FUNC;
	DBMS_Table = NewNameHash();
	if		(  ( MONDB_LoadPath = getenv("MONDB_LOAD_PATH") )  ==  NULL  ) {
		MONDB_LoadPath = MONTSUQI_LIBRARY_PATH;
	}
LEAVE_FUNC;
}

static void
RequiredDBGROUP(void)
{
	int i;
	DBG_Struct	*dbg;
ENTER_FUNC;
	for (i=0; RDBG[i].name != NULL; i++){
		if	( g_hash_table_lookup(ThisEnv->DBG_Table,RDBG[i].name) == NULL ) {
			dbg = NewDBG_Struct(RDBG[i].name);
			dbg->type = StrDup(RDBG[i].type);
			RegistDBG(dbg);
		}
	}
LEAVE_FUNC;
}

extern	void
SetUpDirectory(
	char	*name,
	char	*ld,
	char	*bd,
	char	*db,
	int		parse_type)
{
	DI_Struct	*di;
ENTER_FUNC;
	InitDBG();
	di = DI_Parser(name,ld,bd,db,parse_type);
	RequiredDBGROUP();
	if ( (parse_type >= P_ALL) && di ) {
		AssignDBG(di);
	}
LEAVE_FUNC;
}

extern	LD_Struct	*
GetLD(
	char	*name)
{
	LD_Struct	*ret = NULL;
	if (ThisEnv) {
		ret = g_hash_table_lookup(ThisEnv->LD_Table,name);
	}
	return	(ret);
}

extern	BD_Struct	*
GetBD(
	char	*name)
{
	BD_Struct	*ret = NULL;
	if (ThisEnv) {
		ret = g_hash_table_lookup(ThisEnv->BD_Table,name);
	}
	return	(ret);
}

extern	DBD_Struct	*
GetDBD(
	char	*name)
{
	DBD_Struct	*ret = NULL;
	if (ThisEnv) {
		ret = g_hash_table_lookup(ThisEnv->DBD_Table,name);
	}
	return	(ret);
}

extern	RecordStruct	*
GetTableDBG(
	char	*gname,
	char	*tname)
{
	DBG_Struct	*dbg;
	RecordStruct	*db;

	if		(  ( dbg = GetDBG(gname) )  !=  NULL  ) {
		db = g_hash_table_lookup(dbg->dbt,tname);
	} else {
		db = NULL;
	}
	return	(db);
}
