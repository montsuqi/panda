/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<unistd.h>
#include	<glib.h>
#include	"types.h"
#include	"value.h"
#include	"DDparser.h"
#include	"LDparser.h"
#include	"DIparser.h"
#include	"DBparser.h"
#include	"driver.h"
#include	"directory.h"
#include	"misc.h"
#include	"debug.h"

extern	void
InitDirectory(
	Bool	fScreen)
{
dbgmsg(">InitDirectory");
	if		(  fScreen  )	{
		ThisScreen = NewScreenData();
	}
	DD_ParserInit();
	LD_ParserInit();
	BD_ParserInit();
	DB_ParserInit();
	DI_ParserInit();
dbgmsg("<InitDirectory");
}

static	void
_AssignDBG(
	RecordStruct	**db,
	int				n,
	GHashTable		*DBG_Table)
{
	int		i;
	char	*gname;
	DBG_Struct	*dbg;

dbgmsg(">_AssignDBG");
	for	( i = 1 ; i < n ; i ++ ) {
		gname = (char *)db[i]->opt.db->dbg;
		if		(  ( dbg = (DBG_Struct *)g_hash_table_lookup(DBG_Table,gname) )
				   ==  NULL  ) {
			Error("DB group not found");
		}
		if		(  dbg->dbt  ==  NULL  ) {
			dbg->dbt = NewNameHash();
		}
		db[i]->opt.db->dbg = dbg;
		if		(  g_hash_table_lookup(dbg->dbt,db[i]->name)  ==  NULL  ) {
			g_hash_table_insert(dbg->dbt,db[i]->name,db[i]);
		}
		xfree(gname);
	}
dbgmsg("<_AssignDBG");
}

static	void
AssignDBG(
	DI_Struct	*di)
{
	int		i;

dbgmsg(">AssignDBG");
	for	( i = 0 ; i < di->cLD ; i ++ ) {
		if		(  di->ld[i]->db  !=  NULL  ) {
			_AssignDBG(di->ld[i]->db,di->ld[i]->cDB,di->DBG_Table);
		}
	}
	for	( i = 0 ; i < di->cBD ; i ++ ) {
		if		(  di->bd[i]->db  !=  NULL  ) {
			_AssignDBG(di->bd[i]->db,di->bd[i]->cDB,di->DBG_Table);
		}
	}
	for	( i = 0 ; i < di->cDBD ; i ++ ) {
		if		(  di->db[i]->db  !=  NULL  ) {
			_AssignDBG(di->db[i]->db,di->db[i]->cDB,di->DBG_Table);
		}
	}
dbgmsg("<AssignDBG");
}

extern	void
SetUpDirectory(
	char	*name,
	char	*ld,
	char	*bd,
	char	*db)
{
	DI_Struct	*di;
dbgmsg(">SetUpDirectory");
	di = DI_Parser(name,ld,bd,db);
	AssignDBG(di);
dbgmsg("<SetUpDirectory");
}

extern	LD_Struct	*
SearchWindowToLD(
	char	*wname)
{
	LD_Struct	*ret;

	ret = g_hash_table_lookup(LD_Table,wname);
	return	(ret);
}

extern	LD_Struct	*
GetLD(
	char	*name)
{
	LD_Struct	*ret;

	ret = g_hash_table_lookup(ThisEnv->LD_Table,name);
	return	(ret);
}

extern	BD_Struct	*
GetBD(
	char	*name)
{
	BD_Struct	*ret;

	ret = g_hash_table_lookup(ThisEnv->BD_Table,name);
	return	(ret);
}

extern	DBD_Struct	*
GetDBD(
	char	*name)
{
	DBD_Struct	*ret;

	ret = g_hash_table_lookup(ThisEnv->DBD_Table,name);
	return	(ret);
}

