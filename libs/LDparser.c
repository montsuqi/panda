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

#define	_LD_PARSER

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	"types.h"
#include	"libmondai.h"
#include	"misc.h"
#include	"DDparser.h"
#include	"LDlex.h"
#include	"LDparser.h"
#include	"DBparser.h"
#include	"directory.h"
#include	"dirs.h"
#include	"debug.h"

static	Bool	fError;
static	GHashTable	*Windows;
static	GHashTable	*Handler;

#define	GetSymbol	(LD_Token = LD_Lex(FALSE))
#define	GetName		(LD_Token = LD_Lex(TRUE))
#undef	Error
#define	Error(msg)	{fError=TRUE;_Error((msg),LD_FileName,LD_cLine);}

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
}

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
			if		(  LD_Token  ==  T_SYMBOL  ) {
				strcpy(wname,LD_ComSymbol);
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
		if		(	(  LD_Token  ==  T_SYMBOL  )
				||	(  LD_Token  ==  T_SCONST  ) ) {
			if		(  stricmp(LD_ComSymbol,"metadb")  ) {
				strcpy(buff,RecordDir);
				p = buff;
				do {
					if		(  ( q = strchr(p,':') )  !=  NULL  ) {
						*q = 0;
					}
					sprintf(name,"%s/%s.db",p,LD_ComSymbol);
					if		(  (  db = DB_Parser(name) )  !=  NULL  ) {
						if		(  g_hash_table_lookup(ld->DB_Table,LD_ComSymbol)  ==  NULL  ) {
							rtmp = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * ( ld->cDB + 1));
							memcpy(rtmp,ld->db,sizeof(RecordStruct *) * ld->cDB);
							xfree(ld->db);
							if		(  db->opt.db->dbg  ==  NULL  ) {
								db->opt.db->dbg = (DBG_Struct *)StrDup(gname);
							}
							ld->db = rtmp;
							ld->db[ld->cDB] = db;
							ld->cDB ++;
							g_hash_table_insert(ld->DB_Table,StrDup(LD_ComSymbol),(void *)ld->cDB);
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
			switch	(LD_Token) {
			  case	T_SPA:
				GetName;
				if		(  LD_Token   ==  T_SYMBOL  ) {
					if		(  ( ld->sparec = ReadRecordDefine(LD_ComSymbol) )
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

static	MessageHandler	*
NewMessageHandler(
	char	*name,
	char	*klass)
{
	MessageHandler	*handler;

	handler = New(MessageHandler);
	handler->name = StrDup(name);
	handler->klass = (MessageHandlerClass *)klass;
	handler->serialize = NULL;
	handler->conv = New(CONVOPT);
	handler->conv->encode = STRING_ENCODING_URL;
	handler->start = NULL;
	handler->fInit = 0;
	handler->loadpath = NULL;
	handler->private = NULL;
	g_hash_table_insert(Handler,handler->name,handler);

	return	(handler);
}

static	void
ParHANDLER(void)
{
	MessageHandler	*handler;

ENTER_FUNC;
	GetSymbol; 
	if		(	(  LD_Token  ==  T_SYMBOL  )
			||	(  LD_Token  ==  T_SCONST  ) ) {
		if		(  g_hash_table_lookup(Handler,LD_ComSymbol)  ==  NULL  ) {
			handler = NewMessageHandler(LD_ComSymbol,NULL);
		} else {
			Error("handler name duplicate");
		}
		if		(  GetSymbol  ==  '{'  ) {
			while	(  GetSymbol  !=  '}'  ) {
				switch	(LD_Token) {
				  case	T_CLASS:
					if		(  GetName   ==  T_SCONST  ) {
						handler->klass = (MessageHandlerClass *)StrDup(LD_ComSymbol);
					} else {
						Error("class must be string.");
					}
					break;
				  case	T_SERIALIZE:
					if		(  GetName   ==  T_SCONST  ) {
						handler->serialize = (ConvFuncs *)StrDup(LD_ComSymbol);
					} else {
						Error("serialize method must be string.");
					}
					break;
				  case	T_START:
					if		(  GetName   ==  T_SCONST  ) {
						handler->start = StrDup(LD_ComSymbol);
					} else {
						Error("start parameter must be string.");
					}
					break;
				  case	T_LOCALE:
					if		(  GetName   ==  T_SCONST  ) {
						handler->conv->locale = StrDup(LD_ComSymbol);
					} else {
						Error("locale name must be string.");
					}
					break;
				  case	T_LOADPATH:
					if		(  GetName   ==  T_SCONST  ) {
						handler->loadpath = StrDup(LD_ComSymbol);
					} else {
						Error("load path must be string.");
					}
					break;
				  case	T_ENCODING:
					if		(  GetName   ==  T_SCONST  ) {
						if		(  !stricmp(LD_ComSymbol,"URL")  ) {
							handler->conv->encode = STRING_ENCODING_URL;
						} else
						if		(  !stricmp(LD_ComSymbol,"BASE64")  ) {
							handler->conv->encode = STRING_ENCODING_BASE64;
						} else {
							Error("unsupported string encoding");
						}
					} else {
						Error("string encoding must be string.");
					}
					break;
				  default:
					Error("handler parameter(s)");
					break;
				}
				if		(  GetSymbol  !=  ';'  ) {
					Error("parameter ; missing");
				}
			}
		} else {
			Error("invalid char");
		}
	} else {
		Error("invalid handler name");
	}
LEAVE_FUNC;
}

static	void
ParBIND(
	LD_Struct	*ret)
{
	WindowBind	*bind;
	int			ix;

dbgmsg(">ParBIND");
	if		(	(  GetSymbol  ==  T_SCONST  )
			||	(  LD_Token   ==  T_SYMBOL  ) ) {
		if		(  ( ix = (int)g_hash_table_lookup(ret->whash,LD_ComSymbol) )  ==  0  ) {
			bind = AddWindow(ret,LD_ComSymbol);
			bind->rec = NULL;
		} else {
			bind = ret->window[ix-1];
		}
		if		(	(  GetSymbol  ==  T_SCONST  )
				||	(  LD_Token   ==  T_SYMBOL  ) ) {
			bind->handler = (void *)StrDup(LD_ComSymbol);
		} else {
			Error("handler name error");
		}
		if		(	(  GetSymbol  ==  T_SCONST  )
				||	(  LD_Token   ==  T_SYMBOL  ) ) {
			bind->module = StrDup(LD_ComSymbol);
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
		switch	(LD_Token) {
		  case	T_NAME:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("no name");
			} else {
				ret = New(LD_Struct);
				ret->name = StrDup(LD_ComSymbol);
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
				ret->arraysize = LD_ComInt;
			} else {
				Error("invalid array size");
			}
			break;
		  case	T_TEXTSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ret->textsize = LD_ComInt;
			} else {
				Error("invalid text size");
			}
			break;
		  case	T_CACHE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ret->nCache = LD_ComInt;
			} else {
				Error("invalid cache size");
			}
			break;
		  case	T_MGROUP:
			GetName;
			ret->group = StrDup(LD_ComSymbol);
			break;
		  case	T_DB:
			if		(	(  GetSymbol  ==  T_SCONST  )
					||	(  LD_Token  ==  T_SYMBOL   ) ) {
				gname = StrDup(LD_ComSymbol);
				if		(  GetSymbol  !=  '{'  ) {
					Error("DB { missing");
				}
			} else
			if		(  LD_Token  ==  '{'  ) {
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
				ret->home = StrDup(ExpandPath(LD_ComSymbol,ThisEnv->BaseDir));
			} else {
				Error("home directory invalid");
			}
			break;
		  case	T_WFC:
			if		(  GetSymbol  ==  T_SCONST  ) {
				ret->wfc = ParPort(LD_ComSymbol,PORT_WFC_APS);
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

	if		(  ( handler = (MessageHandler *)g_hash_table_lookup(Handler,
																 (char *)bind->handler) )  !=  NULL  ) {
		bind->handler = handler;
	} else {
		Error("invalid handler name");
	}
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
	FILE	*fp;
	LD_Struct	*ret;
	struct	stat	stbuf;

dbgmsg(">LD_Parser");
dbgmsg(name); 
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
			fError = FALSE;
			LD_FileName = name;
			LD_cLine = 1;
			LD_File = fp;
			ret = ParLD();
			fclose(LD_File);
			if		(  fError  ) {
				ret = NULL;
			} else {
				BindHandler(ret);
			}
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

static	void
EnterDefaultHandler(void)
{
	MessageHandler	*handler;

	handler = NewMessageHandler("OpenCOBOL","OpenCOBOL");
	handler->serialize = (ConvFuncs *)"OpenCOBOL";
	handler->conv->locale = "euc-jp";
	handler->start = "";

	handler = NewMessageHandler("dotCOBOL","dotCOBOL");
	handler->serialize = (ConvFuncs *)"dotCOBOL";
	handler->conv->locale = "euc-jp";
	handler->start = "";

	handler = NewMessageHandler("C","C");
	handler->serialize = NULL;
	handler->conv->locale = "";
	handler->start = "";

	handler = NewMessageHandler("Exec","Exec");
	handler->serialize = (ConvFuncs *)"CGI";
	handler->conv->locale = "euc-jp";
	handler->start = "%m";
}

extern	void
LD_ParserInit(void)
{
	LD_LexInit();
	LD_Table = NewNameHash();
	Windows = NewNameHash();
	Handler = NewNameHash();
	EnterDefaultHandler();
}
