/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2004 Ogochan & JMA.

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

#define	_DI_PARSER

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<sys/stat.h>	/*	for stbuf	*/
#include	<unistd.h>
#include	<libgen.h>
#include	"types.h"
#include	"const.h"
#include	"dirs.h"
#include	"libmondai.h"
#include	"DDparser.h"
#include	"Lex.h"
#include	"LDparser.h"
#include	"BDparser.h"
#include	"DBDparser.h"
#include	"DIparser.h"
#include	"dirs.h"
#define		_DIRECTORY
#include	"directory.h"
#include	"debug.h"

#define	T_NAME			(T_YYBASE +1)
#define	T_LD			(T_YYBASE +2)
#define	T_LINKSIZE		(T_YYBASE +3)
#define	T_MULTI			(T_YYBASE +4)
#define	T_BD			(T_YYBASE +5)
#define	T_DBGROUP		(T_YYBASE +6)
#define	T_PORT			(T_YYBASE +7)
#define	T_USER			(T_YYBASE +8)
#define	T_PASS			(T_YYBASE +9)
#define	T_TYPE			(T_YYBASE +10)
#define	T_FILE			(T_YYBASE +11)
#define	T_REDIRECT		(T_YYBASE +12)
#define	T_REDIRECTPORT	(T_YYBASE +13)
#define	T_RECDIR		(T_YYBASE +16)
#define	T_BASEDIR		(T_YYBASE +17)
#define	T_PRIORITY		(T_YYBASE +18)
#define	T_LINKAGE		(T_YYBASE +19)
#define	T_STACKSIZE		(T_YYBASE +20)
#define	T_DB			(T_YYBASE +21)
#define	T_WFC			(T_YYBASE +23)
#define	T_EXIT			(T_YYBASE +24)
#define	T_ENCODING		(T_YYBASE +25)
#define	T_TERMPORT		(T_YYBASE +26)
#define	T_CONTROL		(T_YYBASE +27)
#define	T_DDIR			(T_YYBASE +28)
#define	T_BLOB			(T_YYBASE +29)
#define	T_AUTH			(T_YYBASE +30)
#define	T_SPACE			(T_YYBASE +31)

#undef	Error
#define	Error(msg)		{CURR->fError=TRUE;_Error((msg),CURR->fn,CURR->cLine);}

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
}

static	TokenTable	tokentable[] = {
	{	"ld"				,T_LD		},
	{	"bd"				,T_BD		},
	{	"db"				,T_DB		},
	{	"name"				,T_NAME 	},
	{	"linksize"			,T_LINKSIZE	},
	{	"multiplex_level"	,T_MULTI	},
	{	"db_group"			,T_DBGROUP	},
	{	"port"				,T_PORT		},
	{	"user"				,T_USER		},
	{	"password"			,T_PASS		},
	{	"type"				,T_TYPE		},
	{	"file"				,T_FILE		},
	{	"redirect"			,T_REDIRECT	},
	{	"redirect_port"		,T_REDIRECTPORT	},
	{	"ddir"				,T_DDIR		},
	{	"record"			,T_RECDIR	},
	{	"base"				,T_BASEDIR	},
	{	"priority"			,T_PRIORITY	},
	{	"linkage"			,T_LINKAGE	},
	{	"stacksize"			,T_STACKSIZE	},
	{	"wfc"				,T_WFC		},
	{	"exit"				,T_EXIT		},
	{	"locale"			,T_ENCODING	},
	{	"encoding"			,T_ENCODING	},
	{	"termport"			,T_TERMPORT	},
	{	"control"			,T_CONTROL	},
	{	"blob"				,T_BLOB		},
	{	"auth"				,T_AUTH		},
	{	"space"				,T_SPACE	},
	{	""					,0			}
};

static	GHashTable	*Reserved;

static	void
ParWFC(void)
{
ENTER_FUNC;
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_PORT:
			switch	(GetSymbol) {
			  case	T_ICONST:
				ThisEnv->WfcApsPort = NewIP_Port(NULL,IntStrDup(ComInt));
				break;
			  case	T_SCONST:
				ThisEnv->WfcApsPort = ParPort(ComSymbol,PORT_WFC_APS);
				break;
			  default:
				Error("invalid port number");
				break;
			}
			GetSymbol;
			break;
		  case	T_TERMPORT:
			switch	(GetSymbol) {
			  case	T_ICONST:
				ThisEnv->TermPort = NewIP_Port(NULL,IntStrDup(ComInt));
				break;
			  case	T_SCONST:
				ThisEnv->TermPort = ParPort(ComSymbol,PORT_WFC);
				break;
			  default:
				Error("invalid port number");
				break;
			}
			GetSymbol;
			break;
		  default:
			Error("wfc keyword error");
			break;
		}
		if		(  ComToken  !=  ';'  ) {		
			Error("missing ; in wfc directive");
		}
	}
LEAVE_FUNC;
}

static	void
ParCONTROL(void)
{
ENTER_FUNC;
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_PORT:
			switch	(GetSymbol) {
			  case	T_ICONST:
				ThisEnv->ControlPort = NewIP_Port(NULL,IntStrDup(ComInt));
				GetSymbol;
				break;
			  case	T_SCONST:
				ThisEnv->ControlPort = ParPort(ComSymbol,PORT_WFC_CONTROL);
				GetSymbol;
				break;
			  case	';':
				ThisEnv->ControlPort = NULL;
				break;
			  default:
				Error("invalid port number");
				break;
			}
			break;
		  default:
			Error("control keyword error");
			break;
		}
		if		(  ComToken  !=  ';'  ) {		
			Error("missing ; in control directive");
		}
	}
LEAVE_FUNC;
}

extern	LD_Struct	*
LD_DummyParser(void)
{
	LD_Struct	*ret;
    ret = New(LD_Struct);
    ret->name = StrDup(ComSymbol);
	return	(ret);
}

static	BLOB_Struct	*
ParBLOB(void)
{
	BLOB_Struct	*blob;
	URL			*auth;
	char		*file;

ENTER_FUNC;
	blob = New(BLOB_Struct);
	blob->port = NULL;
	blob->auth = NULL;
	blob->dir = NULL;
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_AUTH:
			switch	(GetSymbol) {
			  case	T_SCONST:
				auth = New(URL);
				ParseURL(auth,ComSymbol,"file");
				if		(  !stricmp(auth->protocol,"file")  ) {
					file = auth->file;
					auth->file = StrDup(ExpandPath(file,ThisEnv->BaseDir));
					if		(  file  !=  NULL  ) {
						xfree(file);
					}
				}
				blob->auth = auth;
				GetSymbol;
				break;
			  case	';':
				blob->auth = NULL;
				break;
			  default:
				Error("invalid auth URL");
				break;
			}
			break;
		  case	T_PORT:
			switch	(GetSymbol) {
			  case	T_ICONST:
				blob->port = NewIP_Port(NULL,IntStrDup(ComInt));
				GetSymbol;
				break;
			  case	T_SCONST:
				blob->port = ParPort(ComSymbol,PORT_BLOB);
				GetSymbol;
				break;
			  case	';':
				blob->port = NULL;
				break;
			  default:
				Error("invalid port number");
				break;
			}
			break;
		  case	T_SPACE:
			switch	(GetSymbol) {
			  case	T_SCONST:
				blob->dir = StrDup(ExpandPath(ComSymbol,ThisEnv->BaseDir));
				GetSymbol;
				break;
			  case	';':
				blob->dir = NULL;
				break;
			  default:
				Error("invalid blob space");
				break;
			}
			break;
		  default:
			Error("blob keyword error");
			break;
		}
		if		(  ComToken  !=  ';'  ) {		
			Error("missing ; in blob directive");
		}
	}
LEAVE_FUNC;
	return	(blob);
}

static	void
ParLD_Elements(
	Bool    parse_ld)
{
	char		buff[SIZE_BUFF];
	char		name[SIZE_BUFF];
	char		*p
	,			*q;
	LD_Struct	**tmp;
	int			i
	,			n;
	Port		**tports;
	LD_Struct	*ld;

ENTER_FUNC;
	strcpy(buff,ThisEnv->D_Dir);
	p = buff;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s.ld",p,ComSymbol);
		if ( parse_ld ) {
			ld = LD_Parser(name); 
		} else {
			ld = LD_DummyParser(); 
		}
		if		(  ld !=  NULL  ) {
			if		(  g_hash_table_lookup(ThisEnv->LD_Table,ComSymbol)
					   !=  NULL  ) {
				Error("same ld appier");
			}
			tmp = (LD_Struct **)xmalloc(sizeof(LD_Struct *) * ( ThisEnv->cLD + 1));
			if		(  ThisEnv->cLD  >  0  ) {
				memcpy(tmp,ThisEnv->ld,sizeof(LD_Struct *) * ThisEnv->cLD);
				xfree(ThisEnv->ld);
			}
			ThisEnv->ld = tmp;
			ThisEnv->ld[ThisEnv->cLD] = ld;
			ThisEnv->cLD ++;
			g_hash_table_insert(ThisEnv->LD_Table, StrDup(ComSymbol),ld);
			if		(  GetSymbol  ==  T_SCONST  ) {
				ld->nports = 0;
				while	(  ComToken  ==  T_SCONST  ) {
					strcpy(buff,ComSymbol);
					n = 0;
					switch	(GetSymbol) {
					  case	',':
						n = 1;
						break;
					  case	'*':
						if		(  GetSymbol  ==  T_ICONST  ) {
							n = ComInt;
							GetSymbol;
						} else {
							Error("invalid number");
						}
						break;
					  case	';':
						n = 1;
						break;
					  default:
						Error("invalid operator");
						break;
					}
					tports = (Port **)xmalloc(sizeof(Port *) * ( ld->nports + n));
					if		(  ld->nports  >  0  ) {
						memcpy(tports,ld->ports,sizeof(Port *) * ld->nports);
						xfree(ld->ports);
					}
					ld->ports = tports;
					for	( i = 0 ; i < n ; i ++ ) {
						ld->ports[ld->nports] = ParPort(buff,PORT_WFC);
						ld->nports ++;
					}
					if		(  ComToken  ==  ','  ) {
						GetSymbol;
					}
				}
			} else
			if		(  ComToken  ==  T_ICONST  ) {
				ld->nports = ComInt;
				ld->ports = (Port **)xmalloc(sizeof(Port *) * ld->nports);
				for	( i = 0 ; i < ld->nports ; i ++ ) {
					ld->ports[i] = NULL;
				}
				GetSymbol;
			} else {
				ld->ports = (Port **)xmalloc(sizeof(Port *));
				ld->ports[0] = ParPort("localhost",NULL);
				ld->nports = 1;
			}
		}
		p = q + 1;
	}	while	(	(  q   !=  NULL  )
				&&	(  ld  ==  NULL  ) );
	if		(  ld  ==  NULL  ) {
		Error("ld not found");
	}
LEAVE_FUNC;
}

static	void
SkipLD(void)
{
ENTER_FUNC;
	while	(  ComToken  !=  ';'  ) {
		switch	(ComToken) {
		  case	T_SCONST:
			switch	(GetSymbol)	{
			  case	',':
				break;
			  case	'*':
				if		(  GetSymbol  ==  T_ICONST  ) {
					GetSymbol;
				} else {
					Error("invalid number");
				}
				break;
			  case	T_ICONST:
				GetSymbol;
				break;
			}
			if		(  ComToken  ==  ','  ) {
				GetSymbol;
			}
			break;
		  case	T_ICONST:
			GetSymbol;
			break;
		  default:
			break;
		}
	}
LEAVE_FUNC;
}

static	void
ParLD(
	char	*dname,
	Bool    parse_ld)
{
ENTER_FUNC;
	while	(  GetSymbol  !=  '}'  ) {
		if		(	(  ComToken  ==  T_SYMBOL  )
				||	(  ComToken  ==  T_SCONST  )
				||	(  ComToken  ==  T_EXIT    ) ) {
			if		(  ThisEnv->D_Dir  ==  NULL  ) {
				if		(	(  GetSymbol  ==  T_SCONST  )
						||	(  ComToken   ==  T_ICONST  ) ) {
					SkipLD();
				}
			} else {
				if		(	(  dname  ==  NULL  )
						||	(  !strcmp(ComSymbol,dname)  ) ) {
					ParLD_Elements(parse_ld);
				} else {
					if		(	(  GetSymbol  ==  T_SCONST  )
							||	(  ComToken   ==  T_ICONST  ) ) {
						SkipLD();
					}
				}
			}
			if		(  ComToken  !=  ';'  ) {
				printf("[%s]\n",ComSymbol);
				Error("syntax error 2");
			}
		}
	}
LEAVE_FUNC;
}

static	void
ParBD(
	char	*dname,
	Bool    parse_ld)
{
	char		name[SIZE_BUFF];
	BD_Struct	*bd;
	BD_Struct	**btmp;
	char		buff[SIZE_BUFF];
	char		*p
	,			*q;

ENTER_FUNC;
	bd = NULL;
	while	(  GetSymbol  !=  '}'  ) {
		if		(	(  ComToken  ==  T_SYMBOL  )
				||	(  ComToken  ==  T_SCONST  ) ) {
			if		(  ThisEnv->D_Dir  !=  NULL  ) {
				if		(	(  dname  ==  NULL  )
						||	(  !strcmp(dname,ComSymbol)  ) ) {
					strcpy(buff,ThisEnv->D_Dir);
					p = buff;
					do {
						if		(  ( q = strchr(p,':') )  !=  NULL  ) {
							*q = 0;
						}

						sprintf(name,"%s/%s.bd",p,ComSymbol);

						if ( parse_ld ) {
							bd = BD_Parser(name);
							if		(  bd  ==  NULL  ) {
									Error("bd not found");
							} else {
									if		(  g_hash_table_lookup(ThisEnv->BD_Table,ComSymbol)  ==  NULL  ) {
											btmp = (BD_Struct **)xmalloc(sizeof(BD_Struct *)
																		 * ( ThisEnv->cBD + 1));
											if		(  ThisEnv->cBD  >  0  ) {
													memcpy(btmp,ThisEnv->bd,sizeof(BD_Struct *)
														   * ThisEnv->cBD);
													xfree(ThisEnv->bd);
											}
											ThisEnv->bd = btmp;
											ThisEnv->bd[ThisEnv->cBD] = bd;
											ThisEnv->cBD ++;
											g_hash_table_insert(ThisEnv->BD_Table,
																StrDup(ComSymbol),bd);
									} else {
											Error("same bd appier");
									}
							}
							p = q + 1;
						}
					}	while	(	(  q   !=  NULL  )
								&&	(  bd  ==  NULL  ) );
				}
			}
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("syntax error 1");
		}
	}
LEAVE_FUNC;
}

static	void
ParDBD(
	char	*dname)
{
	char		name[SIZE_BUFF];
	DBD_Struct	*dbd;
	DBD_Struct	**btmp;
	char		buff[SIZE_BUFF];
	char		*p
	,			*q;

ENTER_FUNC;
	while	(  GetSymbol  !=  '}'  ) {
		if		(	(  ComToken  ==  T_SYMBOL  )
				||	(  ComToken  ==  T_SCONST  ) ) {
			if		(  ThisEnv->D_Dir  !=  NULL  ) {
				if		(	(  dname  ==  NULL  )
						||	(  !strcmp(dname,ComSymbol)  ) ) {
					strcpy(buff,ThisEnv->D_Dir);
					p = buff;
					do {
						if		(  ( q = strchr(p,':') )  !=  NULL  ) {
							*q = 0;
						}
						sprintf(name,"%s/%s.dbd",p,ComSymbol);
						if		(  (  dbd = DBD_Parser(name) )  !=  NULL  ) {
							if		(  g_hash_table_lookup(ThisEnv->DBD_Table,ComSymbol)  ==  NULL  ) {
								btmp = (DBD_Struct **)xmalloc(sizeof(DBD_Struct *)
															 * ( ThisEnv->cDBD + 1));
								if		(  ThisEnv->cDBD  >  0  ) {
									memcpy(btmp,ThisEnv->db,sizeof(DBD_Struct *)
										   * ThisEnv->cDBD);
									xfree(ThisEnv->db);
								}
								ThisEnv->db = btmp;
								ThisEnv->db[ThisEnv->cDBD] = dbd;
								ThisEnv->cDBD ++;
								g_hash_table_insert(ThisEnv->DBD_Table,
													StrDup(ComSymbol),dbd);
							} else {
								Error("same db appier");
							}
						}
						p = q + 1;
					}	while	(	(  q   !=  NULL  )
								&&	(  dbd  ==  NULL  ) );
					if		(  dbd  ==  NULL  ) {
						Error("dbd not found");
					}
				}
			}
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("syntax error 1");
		}
	}
LEAVE_FUNC;
}

static	void
AppendDBG(
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
	ThisEnv->cDBG ++;
	g_hash_table_insert(ThisEnv->DBG_Table,dbg->name,dbg);
LEAVE_FUNC;
}

static	void
ParDBGROUP(
	char	*name)
{
	DBG_Struct	*dbg;
	char		*env;

ENTER_FUNC;
	if		(  g_hash_table_lookup(ThisEnv->DBG_Table,name)  !=  NULL  ) {
		Error("DB group name duplicate");
	}
	dbg = New(DBG_Struct);
	dbg->name = name;
	dbg->id = 0;
	dbg->type = NULL;
	dbg->func = NULL;
	dbg->user = NULL;
	dbg->dbname = NULL;
	dbg->port = NULL;
	dbg->pass = NULL;
	dbg->file = NULL;
	dbg->redirect = NULL;
	dbg->redirectPort = NULL;
	dbg->fpLog = NULL;
	dbg->dbt = NULL;
	dbg->priority = 50;
	if		(  ( env = getenv("MONDB_LOCALE") )  ==  NULL  ) {
		dbg->coding = DB_LOCALE;
	} else
	if		(  stricmp(env,"UTF8")  ==  0  ) {
		dbg->coding = NULL;
	}
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_TYPE:
			GetSymbol;
			if		(	(  ComToken  ==  T_SYMBOL  )
					||	(  ComToken  ==  T_SCONST  ) ) {
				dbg->type = StrDup(ComSymbol);
			} else {
				Error("invalid DBMS type");
			}
			break;
		  case	T_PORT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->port = ParPort(ComSymbol,NULL);
			} else {
				Error("invalid port");
			}
			break;
		  case	T_REDIRECTPORT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->redirectPort = ParPort(ComSymbol,PORT_REDIRECT);
			} else {
				Error("invalid port");
			}
			break;
		  case	T_NAME:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->dbname = StrDup(ComSymbol);
			} else {
				Error("invalid DB name");
			}
			break;
		  case	T_USER:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->user = StrDup(ComSymbol);
			} else {
				Error("invalid DB user");
			}
			break;
		  case	T_PASS:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->pass = StrDup(ComSymbol);
			} else {
				Error("invalid DB password");
			}
			break;
		  case	T_FILE:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->file = StrDup(ComSymbol);
			} else {
				Error("invalid logging file name");
			}
			break;
		  case	T_ENCODING:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  stricmp(ComSymbol,"utf8")  ==  0  ) {
					dbg->coding = NULL;
				} else {
					dbg->coding = StrDup(ComSymbol);
				}
			} else {
				Error("invalid logging file name");
			}
			break;
		  case	T_REDIRECT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->redirect = (DBG_Struct *)StrDup(ComSymbol);
			} else {
				Error("invalid redirect group");
			}
			break;
		  case	T_PRIORITY:
			if		(  GetSymbol  ==  T_ICONST  ) {
				dbg->priority = atoi(ComSymbol);
			} else {
				Error("priority invalid");
			}
			break;
		  default:
			Error("other syntax error in db_group");
			printf("[%s]\n",ComSymbol);
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("; not found in db_group");
		}
	}
	AppendDBG(dbg);
LEAVE_FUNC;
}

static	void
_AssignDBG(
	char	*name,
	DBG_Struct	*dbg,
	void	*dummy)
{
	DBG_Struct	*red;

ENTER_FUNC;
	if		(  dbg->redirect  !=  NULL  ) {
		red = g_hash_table_lookup(ThisEnv->DBG_Table,(char *)dbg->redirect);
		if		(  red  ==  NULL  ) {
			Error("redirect DB group not found");
		}
		xfree(dbg->redirect);
		dbg->redirect = red;
	}
LEAVE_FUNC;
}

static	int
DBGcomp(
	DBG_Struct	**x,
	DBG_Struct	**y)
{
	return	(  (*x)->priority  -  (*y)->priority  );
}

static	void
AssignDBG(void)
{
	int		i;
	DBG_Struct	*dbg;

ENTER_FUNC;
	qsort(ThisEnv->DBG,ThisEnv->cDBG,sizeof(DBG_Struct *),
		  (int (*)(const void *,const void *))DBGcomp);
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		dbgprintf("%d DB group name = [%s]\n",dbg->priority,dbg->name);
		_AssignDBG(dbg->name,dbg,NULL);
	}
LEAVE_FUNC;
}

static	RecordStruct	*
BuildMcpArea(
	size_t	stacksize)
{
	RecordStruct	*rec;
	char	buff[SIZE_BUFF];
	char	*p;

	p = buff;
	p += sprintf(p,	"mcparea	{");
	p += sprintf(p,		"func varchar(%d);",SIZE_FUNC);
	p += sprintf(p,		"rc int;");
	p += sprintf(p,		"dc	{");
	p += sprintf(p,			"window	 varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"widget	 varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"event	 varchar(%d);",SIZE_EVENT);
	p += sprintf(p,			"module	 varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"fromwin varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"status	 varchar(%d);",SIZE_STATUS);
	p += sprintf(p,			"puttype varchar(%d);",SIZE_PUTTYPE);
	p += sprintf(p,			"term	 varchar(%d);",SIZE_TERM);
	p += sprintf(p,			"user	 varchar(%d);",SIZE_USER);
	p += sprintf(p,		"};");
	p += sprintf(p,		"db	{");
	p += sprintf(p,			"path	{");
	p += sprintf(p,				"blocks	int;");
	p += sprintf(p,				"rname	int;");
	p += sprintf(p,				"pname	int;");
	p += sprintf(p,			"};");
	p += sprintf(p,			"table      varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"pathname   varchar(%d);",SIZE_NAME);
	p += sprintf(p,		"};");
	p += sprintf(p,		"private	{");
	p += sprintf(p,			"count	int;");
	p += sprintf(p,			"swindow	char(%d)[%d];",SIZE_NAME,stacksize);
	p += sprintf(p,			"state		char(1)[%d];",stacksize);
	p += sprintf(p,			"index		int[%d];",stacksize);
	p += sprintf(p,			"pstatus	char(1);");
	p += sprintf(p,			"pputtype 	char(1);");
	p += sprintf(p,			"prc		char(1);");
	p += sprintf(p,		"};");
	p += sprintf(p,	"};");
	rec = ParseRecordMem(buff);

	return	(rec);
}

static	DI_Struct	*
ParDI(
	char	*ld,
	char	*bd,
	char	*db,
	Bool    parse_ld)
{
	char	*gname;
	char	buff[SIZE_LONGNAME+1];

ENTER_FUNC;
	ThisEnv = NULL;
	while	(  GetSymbol  !=  T_EOF  ) {
		switch	(ComToken) {
		  case	T_NAME:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("no name");
			} else {
				ThisEnv = New(DI_Struct);
				ThisEnv->name = StrDup(ComSymbol);
				ThisEnv->BaseDir = BaseDir;
				ThisEnv->D_Dir = D_Dir;
				ThisEnv->RecordDir = RecordDir;
				ThisEnv->WfcApsPort = ParPort("localhost",PORT_WFC_APS);
				ThisEnv->TermPort = ParPort("localhost",PORT_WFC);
				ThisEnv->ControlPort = ParPort(CONTROL_PORT,PORT_WFC_CONTROL);
				ThisEnv->cLD = 0;
				ThisEnv->cBD = 0;
				ThisEnv->cDBD = 0;
				ThisEnv->stacksize = SIZE_STACK;
				ThisEnv->LD_Table = NewNameHash();
				ThisEnv->BD_Table = NewNameHash();
				ThisEnv->DBD_Table = NewNameHash();
				ThisEnv->mlevel = MULTI_NO;
				ThisEnv->cDBG = 0;
				ThisEnv->DBG = NULL;
				ThisEnv->DBG_Table = NewNameHash();
				ThisEnv->blob = NULL;
			}
			break;
		  case	T_STACKSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				if		(  ComInt  >  SIZE_STACK  ) {
					ThisEnv->stacksize = ComInt;
				}
			} else {
				Error("stacksize must be integer");
			}
			break;
		  case	T_LINKSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ThisEnv->linksize = ComInt;
			} else {
				Error("linksize must be integer");
			}
			break;
		  case	T_LINKAGE:
			if		(  GetSymbol   ==  T_SYMBOL  ) {
				if ( parse_ld ) {
					sprintf(buff,"%s.rec",ComSymbol);
					ThisEnv->linkrec = ReadRecordDefine(buff);
				} else {
					break;
				}
				if		(  ThisEnv->linkrec ==  NULL  ) {
					Error("linkage record not found");
				} else {
					ThisEnv->linksize = NativeSizeValue(NULL,ThisEnv->linkrec->value);
				}
			} else {
				Error("linkage invalid");
			}
			break;
		  case	T_BASEDIR:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->BaseDir  ==  NULL  ) {
					if		(  !strcmp(ComSymbol,".")  ) {
						ThisEnv->BaseDir = ".";
					} else {
						ThisEnv->BaseDir = StrDup(ExpandPath(ComSymbol
															,NULL));
					}
				}
			} else {
				Error("base directory invalid");
			}
			break;
		  case	T_RECDIR:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->RecordDir  ==  NULL  ) {
					if		(  !strcmp(ComSymbol,".")  ) {
						ThisEnv->RecordDir = StrDup(ThisEnv->BaseDir);
					} else {
						ThisEnv->RecordDir = StrDup(ExpandPath(ComSymbol
															,ThisEnv->BaseDir));
					}
					RecordDir = ThisEnv->RecordDir;
				}
			} else {
				Error("recored directory invalid");
			}
			break;
		  case	T_DDIR:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->D_Dir  ==  NULL  ) {
					if		(  !strcmp(ComSymbol,".")  ) {
						ThisEnv->D_Dir = StrDup(ThisEnv->BaseDir);
					} else {
						ThisEnv->D_Dir = StrDup(ExpandPath(ComSymbol
															,ThisEnv->BaseDir));
					}
				}
			} else {
				Error("DDIR directory invalid");
			}
			break;
		  case	T_MULTI:
			GetName;
			if		(  stricmp(ComSymbol,"no")  ==  0  ) {
				ThisEnv->mlevel = MULTI_NO;
			} else
			if		(  stricmp(ComSymbol,"db")  ==  0  ) {
				ThisEnv->mlevel = MULTI_DB;
			} else
			if		(  stricmp(ComSymbol,"ld")  ==  0  ) {
				ThisEnv->mlevel = MULTI_LD;
			} else
			if		(  stricmp(ComSymbol,"id")  ==  0  ) {
				ThisEnv->mlevel = MULTI_ID;
			} else
			if		(  stricmp(ComSymbol,"aps")  ==  0  ) {
				ThisEnv->mlevel = MULTI_APS;
			} else {
				Error("invalid multiplex level");
			}
			break;
		  case	T_BLOB:
			if		(  GetSymbol  ==  '{'  ) {
				ThisEnv->blob = ParBLOB();
			} else {
				Error("syntax error in blob directive");
			}
			break;
		  case	T_CONTROL:
			if		(  GetSymbol  ==  '{'  ) {
				ParCONTROL();
			} else {
				Error("syntax error in control directive");
			}
			break;
		  case	T_WFC:
			if		(  GetSymbol  ==  '{'  ) {
				ParWFC();
			} else {
				Error("syntax error in wfc directive");
			}
			break;
		  case	T_LD:
			if		(  GetSymbol  ==  '{'  ) {
				ParLD(ld, parse_ld);
			} else {
				Error("syntax error in ld directive");
			}
			break;
		  case	T_BD:
			if		(  GetSymbol  ==  '{'  ) {
				ParBD(bd, parse_ld);
			} else {
				Error("syntax error in bd directive");
			}
			break;
		  case	T_DB:
			if		(  GetSymbol  ==  '{'  ) {
				ParDBD(db);
			} else {
				Error("syntax error in db directive");
			}
			break;
		  case	T_DBGROUP:
			if		(  GetSymbol  ==  T_SCONST  ) {
				gname = StrDup(ComSymbol);
				if		(  GetSymbol  !=  '{'  ) {
					Error("syntax error in db names");
				}
			} else
			if		(  ComToken  ==  '{'  ) {
				gname = "";
			} else {
				gname = NULL;
				Error("syntax error dbgroup directive");
			}
			ParDBGROUP(gname);
			break;
		  default:
			printf("[%X][%s]\n",ComToken,ComSymbol);
			Error("misc syntax error");
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("; missing");
		}
	}
	ThisEnv->mcprec = BuildMcpArea(ThisEnv->stacksize);
	AssignDBG();
LEAVE_FUNC;
	return	(ThisEnv);
}

extern	DI_Struct	*
DI_Parser(
	char	*name,
	char	*ld,
	char	*bd,
	char	*db,
	Bool    parse_ld)
{
	struct	stat	stbuf;
	DI_Struct		*ret;

ENTER_FUNC;
	DirectoryDir = dirname(StrDup(name));
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  PushLexInfo(name,DirectoryDir,Reserved)  !=  NULL  ) {
			ret = ParDI(ld,bd,db,parse_ld);
			DropLexInfo();
		} else {
			ret = NULL;
		}
	} else {
        fprintf(stderr, "DI file not found\n");
        exit(1);
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
DI_ParserInit(void)
{
	LexInit();
	Reserved = MakeReservedTable(tokentable);
}
