/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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

#define	_D_PARSER

#include	"const.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	"types.h"
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"DBparser.h"
#include	"DBDparser.h"
#include	"Dlex.h"
#include	"dirs.h"
#include	"debug.h"

static	TokenTable	tokentable[] = {
	{	"data"		,T_DATA 	},
	{	"host"		,T_HOST		},
	{	"name"		,T_NAME		},
	{	"home"		,T_HOME		},
	{	"port"		,T_PORT		},
	{	"spa"		,T_SPA		},
	{	"window"	,T_WINDOW	},
	{	"cache"		,T_CACHE	},
	{	"arraysize"	,T_ARRAYSIZE},
	{	"textsize"	,T_TEXTSIZE	},
	{	"db"		,T_DB		},
	{	"multiplex_group"	,T_MGROUP		},
	{	"bind"		,T_BIND		},

	{	"handler"	,T_HANDLER	},
	{	"class"		,T_CLASS	},
	{	"serialize"	,T_SERIALIZE},
	{	"start"		,T_START	},
	{	"locale"	,T_LOCALE	},
	{	"encoding"	,T_ENCODING	},
	{	"loadpath"	,T_LOADPATH	},

	{	""			,0	}
};

static	GHashTable	*Reserved;

static	void
ParDBDB(
	DBD_Struct	*dbd,
	char		*gname)
{
	RecordStruct	*db;
	RecordStruct	**rtmp;
	char		name[SIZE_BUFF];
	char		buff[SIZE_BUFF];
	char		*p
	,			*q;

dbgmsg(">ParDBDB");
	while	(  GetSymbol  !=  '}'  ) {
		if		(	(  ComToken  ==  T_SYMBOL  )
				||	(  ComToken  ==  T_SCONST  ) ) {
			if		(  stricmp(ComSymbol,"metadb")  ) {
				strcpy(buff,RecordDir);
				p = buff;
				do {
					if		(  ( q = strchr(p,':') )  !=  NULL  ) {
						*q = 0;
					}
					sprintf(name,"%s/%s.db",p,ComSymbol);
					if		(  (  db = DB_Parser(name) )  !=  NULL  ) {
						if		(  g_hash_table_lookup(dbd->DBD_Table,ComSymbol)  ==  NULL  ) {
							rtmp = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * ( dbd->cDB + 1));
							memcpy(rtmp,dbd->db,sizeof(RecordStruct *) * dbd->cDB);
							xfree(dbd->db);
							if		(  db->opt.db->dbg  ==  NULL  ) {
								db->opt.db->dbg = (DBG_Struct *)StrDup(gname);
							}
							dbd->db = rtmp;
							dbd->db[dbd->cDB] = db;
							dbd->cDB ++;
							g_hash_table_insert(dbd->DBD_Table, StrDup(ComSymbol),(void *)dbd->cDB);
						} else {
							Error("same db appier");
						}
					}
					p = q + 1;
				}	while	(	(  q   !=  NULL  )
								&&	(  db  ==  NULL  ) );
				if		(  db  ==  NULL  ) {
					Error("db not found");
				}
			}
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("DB ; missing");
		}
	}
	xfree(gname);
dbgmsg("<ParDBDB");
}

static	DBD_Struct	*
ParDB(void)
{
	DBD_Struct	*ret;
	char		*gname;

dbgmsg(">ParDB");
	ret = NULL;
	while	(  GetSymbol  !=  T_EOF  ) {
		switch	(ComToken) {
		  case	T_NAME:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("no name");
			} else {
				ret = New(DBD_Struct);
				ret->name = StrDup(ComSymbol);
				ret->cDB = 1;
				ret->db = (RecordStruct **)xmalloc(sizeof(RecordStruct *));
				ret->db[0] = NULL;
				ret->arraysize = SIZE_DEFAULT_ARRAY_SIZE;
				ret->textsize = SIZE_DEFAULT_TEXT_SIZE;
				ret->DBD_Table = NewNameHash();
			}
			break;
		  case	T_ARRAYSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ret->arraysize = ComInt;
			} else {
				Error("invalid array size");
			}
			break;
		  case	T_TEXTSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ret->textsize = ComInt;
			} else {
				Error("invalid text size");
			}
			break;
		  case	T_DB:
			if		(  GetSymbol  ==  T_SCONST  ) {
				gname = StrDup(ComSymbol);
				if		(  GetSymbol  !=  '{'  ) {
					Error("syntax error 3");
				}
			} else
			if		(  ComToken  ==  '{'  ) {
				gname = StrDup("");
			} else {
				gname = NULL;
				Error("syntax error 4");
			}
			ParDBDB(ret,gname);
			break;
		  default:
			Error("syntax error 3");
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("; missing");
		}
	}
dbgmsg("<ParDB");
	return	(ret);
}

extern	DBD_Struct	*
DBD_Parser(
	char	*name)
{
	DBD_Struct	*ret;
	struct	stat	stbuf;

dbgmsg(">DBD_Parser");
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  PushLexInfo(name,D_Dir,Reserved)  !=  NULL  ) {
			ret = ParDB();
			DropLexInfo();
		} else {
			Error("DBD file not found");
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
dbgmsg("<DBD_Parser");
	return	(ret);
}

extern	void
DBD_ParserInit(void)
{
	LexInit();
	Reserved = MakeReservedTable(tokentable);

	DBD_Table = NewNameHash();
}
