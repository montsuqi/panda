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

#define		_D_PARSER
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	"types.h"
#include	"libmondai.h"
#include	"mhandler.h"
#include	"DDparser.h"
#include	"Dlex.h"
#include	"LDparser.h"
#include	"DBparser.h"
#include	"directory.h"
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
	{	"wfc"		,T_WFC		},

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

static	GHashTable	*Windows;

static	WindowBind	*
AddWindow(
	LD_Struct	*ld,
	char	*name)
{
	WindowBind	*bind;

	bind = New(WindowBind);
	bind->name = StrDup(name);
	bind->ix = -1;
	g_hash_table_insert(ld->whash,bind->name,bind);
	return	(bind);
}

static	RecordStruct	*
GetWindow(
	char		*name)
{
	RecordStruct	*rec;
	char		*wname;

dbgmsg(">GetWindow");
	dbgprintf("GetWindow(%s)",name);
	if		(  name  !=  NULL  ) {
		if		(  ( rec = 
					 (RecordStruct *)g_hash_table_lookup(Windows,name) )
				   ==  NULL  ) {
			wname = StrDup(name);
			rec = ReadRecordDefine(name);
			g_hash_table_insert(Windows,wname,rec);
		}
	} else {
		rec = NULL;
	}
dbgmsg("<GetWindow");
	return	(rec);
}

static	void
ParWindow(
	LD_Struct	*ld)
{
	WindowBind	*bind;
	int			ix;
	RecordStruct	*rec;
	WindowBind	**wnb;
	char		wname[SIZE_NAME+1];

dbgmsg(">ParWindow");
	if		(  GetSymbol  !=  '{'  ) { 
		Error("syntax error");
	} else {
		while	(  GetName  !=  '}'  ) {
			if		(  ComToken  ==  T_SYMBOL  ) {
				strcpy(wname,ComSymbol);
				if		(  ( bind = (WindowBind *)g_hash_table_lookup(ld->whash,wname) )
						   ==  NULL  ) {
					bind = AddWindow(ld,wname);
					bind->handler = NULL;
					bind->module = NULL;
				}
				bind->rec = GetWindow(wname);
				bind->ix = ld->cWindow;
				wnb = (WindowBind **)xmalloc(sizeof(WindowBind *) * ( ld->cWindow + 1));
				if		(  ld->cWindow  >  0  ) {
					memcpy(wnb,ld->window,sizeof(WindowBind *) * ld->cWindow);
					xfree(ld->window);
				}
				ld->window = wnb;
				ld->window[ld->cWindow] = bind;
				ld->cWindow ++;
				if		(  g_hash_table_lookup(LD_Table,wname)  ==  NULL  ) {
					g_hash_table_insert(LD_Table,ld->name,ld);
				} else {
					Error("same window name appier");
				}
			} else {
				Error("record name not found");
			}
			if		(  GetSymbol  !=  ';'  ) {
				Error("Window ; missing");
			}
		}
	}
dbgmsg("<ParWindow");
}

static	void
ParDB(
	LD_Struct	*ld,
	char		*gname)
{
	RecordStruct	*db;
	RecordStruct	**rtmp;
	char			name[SIZE_BUFF];
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
						if		(  g_hash_table_lookup(ld->DB_Table,ComSymbol)  ==  NULL  ) {
							rtmp = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * ( ld->cDB + 1));
							memcpy(rtmp,ld->db,sizeof(RecordStruct *) * ld->cDB);
							xfree(ld->db);
							if		(  db->opt.db->dbg  ==  NULL  ) {
								db->opt.db->dbg = (DBG_Struct *)StrDup(gname);
							}
							ld->db = rtmp;
							ld->db[ld->cDB] = db;
							ld->cDB ++;
							g_hash_table_insert(ld->DB_Table,StrDup(ComSymbol),(void *)ld->cDB);
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
ParDATA(
	LD_Struct	*ld)
{
dbgmsg(">ParDATA");
	if		(  GetSymbol  ==  '{'  ) {
		while	(  GetSymbol  !=  '}'  ) {
			switch	(ComToken) {
			  case	T_SPA:
				GetName;
				if		(  ComToken   ==  T_SYMBOL  ) {
					if		(  ( ld->sparec = ReadRecordDefine(ComSymbol) )
							   ==  NULL  ) {
						Error("spa record not found");
					}
				} else {
					Error("spa must be integer");
				}
				break;
			  case	T_WINDOW:
				ParWindow(ld);
				break;
			  default:
				Error("syntax error 1");
				break;
			}
			if		(  GetSymbol  !=  ';'  ) {
				Error("DATA ; missing");
			}
		}
	} else {
		Error("DATA syntax error");
	}
dbgmsg("<ParDATA");
}

static	void
ParBIND(
	LD_Struct	*ret)
{
	WindowBind	*bind;
	int			ix;

dbgmsg(">ParBIND");
	if		(	(  GetSymbol  ==  T_SCONST  )
			||	(  ComToken    ==  T_SYMBOL  ) ) {
		if		(  ( ix = (int)g_hash_table_lookup(ret->whash,ComSymbol) )  ==  0  ) {
			bind = AddWindow(ret,ComSymbol);
			bind->rec = NULL;
		} else {
			bind = ret->window[ix-1];
		}
		if		(	(  GetSymbol  ==  T_SCONST  )
				||	(  ComToken    ==  T_SYMBOL  ) ) {
			bind->handler = (void *)StrDup(ComSymbol);
		} else {
			Error("handler name error");
		}
		if		(	(  GetSymbol  ==  T_SCONST  )
				||	(  ComToken    ==  T_SYMBOL  ) ) {
			bind->module = StrDup(ComSymbol);
		} else {
			Error("module name error");
		}
	} else {
		Error("window name error");
	}
dbgmsg("<ParBIND");
}

static	LD_Struct	*
ParLD(void)
{
	LD_Struct	*ret;
	char		*gname;

dbgmsg(">ParLD");
	ret = NULL;
	while	(  GetSymbol  !=  T_EOF  ) {
		switch	(ComToken) {
		  case	T_NAME:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("no name");
			} else {
				ret = New(LD_Struct);
				ret->name = StrDup(ComSymbol);
				ret->group = "";
				ret->ports = NULL;
				ret->whash = NewNameHash();
				ret->nCache = 0;
				ret->cDB = 1;
				ret->db = (RecordStruct **)xmalloc(sizeof(RecordStruct *));
				ret->db[0] = NULL;
				ret->cWindow = 0;
				ret->arraysize = SIZE_DEFAULT_ARRAY_SIZE;
				ret->textsize = SIZE_DEFAULT_TEXT_SIZE;
				ret->DB_Table = NewNameHash();
				ret->home = NULL;
				ret->wfc = NULL;
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
		  case	T_CACHE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ret->nCache = ComInt;
			} else {
				Error("invalid cache size");
			}
			break;
		  case	T_MGROUP:
			GetName;
			ret->group = StrDup(ComSymbol);
			break;
		  case	T_DB:
			if		(	(  GetSymbol  ==  T_SCONST  )
					||	(  ComToken    ==  T_SYMBOL   ) ) {
				gname = StrDup(ComSymbol);
				if		(  GetSymbol  !=  '{'  ) {
					Error("DB { missing");
				}
			} else
			if		(  ComToken  ==  '{'  ) {
				gname = StrDup("");
			} else {
				gname = NULL;
				Error("DB error");
			}
			ParDB(ret,gname);
			break;
		  case	T_DATA:
			ParDATA(ret);
			break;
		  case	T_HOME:
			if		(  GetSymbol  ==  T_SCONST  ) {
				ret->home = StrDup(ExpandPath(ComSymbol,ThisEnv->BaseDir));
			} else {
				Error("home directory invalid");
			}
			break;
		  case	T_WFC:
			if		(  GetSymbol  ==  T_SCONST  ) {
				ret->wfc = ParPort(ComSymbol,PORT_WFC_APS);
			} else {
				Error("wfc invalid");
			}
			break;
		  case	T_BIND:
			ParBIND(ret);
			break;
		  case	T_HANDLER:
			ParHANDLER();
			break;
		  default:
			Error("invalid directive");
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("; missing");
		}
	}
dbgmsg("<ParLD");
	return	(ret);
}

static	void
_BindHandler(
	char	*name,
	WindowBind	*bind,
	void		*dummy)
{
	MessageHandler	*handler;

	BindMessageHandlerCommon(&bind->handler);
}

static	void
BindHandler(
	LD_Struct	*ld)
{
	g_hash_table_foreach(ld->whash,(GHFunc)_BindHandler,NULL);
}

extern	LD_Struct	*
LD_Parser(
	char	*name)
{
	LD_Struct	*ret;
	struct	stat	stbuf;

dbgmsg(">LD_Parser");
dbgmsg(name); 
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  PushLexInfo(name,D_Dir,Reserved)  !=  NULL  ) {
			ret = ParLD();
			DropLexInfo();
			BindHandler(ret);
		} else {
			printf("[%s]\n",name);
			Error("LD file not found");
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
dbgmsg("<LD_Parser");
	return	(ret);
}

extern	void
LD_ParserInit(void)
{
	LexInit();
	Reserved = MakeReservedTable(tokentable);

	LD_Table = NewNameHash();
	Windows = NewNameHash();
	MessageHandlerInit();
}
