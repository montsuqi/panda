/*
PANDA -- a simple transaction monitor
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include	"libmondai.h"
#include	"RecParser.h"
#include	"DDparser.h"
#include	"LDparser.h"
#include	"DIparser.h"
#include	"DBparser.h"
#include	"BDparser.h"
#include	"DBDparser.h"
#include	"directory.h"
#include	"debug.h"

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
			fprintf(stderr,"[%s]\n",gname);
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
	if		(  di->DBG_Table  !=  NULL  ) {
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
	}
dbgmsg("<AssignDBG");
}

extern	void
SetUpDirectory(
	char	*name,
	char	*ld,
	char	*bd,
	char	*db,
	Bool    parse_ld)
{
	DI_Struct	*di;
dbgmsg(">SetUpDirectory");
	di = DI_Parser(name,ld,bd,db,parse_ld);
	if ( parse_ld ) {
		AssignDBG(di); 
	}
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

