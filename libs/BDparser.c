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
#define	MAIN
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
#include	"mhandler.h"
#include	"DBparser.h"
#include	"BDparser.h"
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
ParDB(
	BD_Struct	*bd,
	char		*gname)
{
	RecordStruct	*db;
	RecordStruct	**rtmp;
	char		name[SIZE_BUFF];
	char		buff[SIZE_BUFF];
	char		*p
	,			*q;

dbgmsg(">ParDB");
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
						if		(  g_hash_table_lookup(bd->DB_Table,ComSymbol)  ==  NULL  ) {
							rtmp = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * ( bd->cDB + 1));
							memcpy(rtmp,bd->db,sizeof(RecordStruct *) * bd->cDB);
							xfree(bd->db);
							if		(  db->opt.db->dbg  ==  NULL  ) {
								db->opt.db->dbg = (DBG_Struct *)StrDup(gname);
							}
							bd->db = rtmp;
							bd->db[bd->cDB] = db;
							bd->cDB ++;
							g_hash_table_insert(bd->DB_Table, StrDup(ComSymbol),(void *)bd->cDB);
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
dbgmsg("<ParDB");
}

static	void
ParBIND(
	BD_Struct	*ret)
{
	BatchBind	*bind;

dbgmsg(">ParBIND");
	if		(	(  GetSymbol  ==  T_SCONST  )
			||	(  ComToken   ==  T_SYMBOL  ) ) {
		if		(  ( bind = g_hash_table_lookup(ret->BatchTable,ComSymbol) )  ==  NULL  ) {
			bind = New(BatchBind);
			bind->module = StrDup(ComSymbol);;
			g_hash_table_insert(ret->BatchTable,bind->module,bind);
		}
		if		(	(  GetSymbol  ==  T_SCONST  )
				||	(  ComToken   ==  T_SYMBOL  ) ) {
			bind->handler = (void *)StrDup(ComSymbol);
		} else {
			Error("handler name error");
		}
	} else {
		Error("module name error");
	}
dbgmsg("<ParBIND");
}

static	BD_Struct	*
ParBD(void)
{
	BD_Struct	*ret;
	char		*gname;

dbgmsg(">ParBD");
	ret = NULL;
	while	(  GetSymbol  !=  T_EOF  ) {
		switch	(ComToken) {
		  case	T_NAME:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("no name");
			} else {
				ret = New(BD_Struct);
				ret->name = StrDup(ComSymbol);
				ret->cDB = 1;
				ret->db = (RecordStruct **)xmalloc(sizeof(RecordStruct *));
				ret->db[0] = NULL;
				ret->arraysize = SIZE_DEFAULT_ARRAY_SIZE;
				ret->textsize = SIZE_DEFAULT_TEXT_SIZE;
				ret->DB_Table = NewNameHash();
				ret->BatchTable = NewNameHash();
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
			ParDB(ret,gname);
			break;
		  case	T_BIND:
			ParBIND(ret);
			break;
		  case	T_HANDLER:
			ParHANDLER();
			break;
		  default:
			Error("syntax error 3");
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("; missing");
		}
	}
dbgmsg("<ParBD");
	return	(ret);
}

static	void
_BindHandler(
	char	*name,
	BatchBind	*bind,
	void		*dummy)
{
	MessageHandler	*handler;

	BindMessageHandlerCommon(&bind->handler);
}

static	void
BindHandler(
	BD_Struct	*bd)
{
	g_hash_table_foreach(bd->BatchTable,(GHFunc)_BindHandler,NULL);
}

extern	BD_Struct	*
BD_Parser(
	char	*name)
{
	BD_Struct	*ret;
	struct	stat	stbuf;

dbgmsg(">BD_Parser");
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  PushLexInfo(name,D_Dir,Reserved)  !=  NULL  ) {
			ret = ParBD();
			DropLexInfo();
			BindHandler(ret);
		} else {
			Error("BD file not found");
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
dbgmsg("<BD_Parser");
	return	(ret);
}

extern	void
BD_ParserInit(void)
{
	LexInit();
	Reserved = MakeReservedTable(tokentable);

	BD_Table = NewNameHash();
	MessageHandlerInit();
}

