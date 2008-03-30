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
#include	<libmondai.h>
#include	<RecParser.h>
#include	"DDparser.h"
#include	"Lex.h"
#include	"LDparser.h"
#include	"BDparser.h"
#include	"DBDparser.h"
#include	"DIparser.h"
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
#define	T_APSPATH		(T_YYBASE +32)
#define	T_WFCPATH		(T_YYBASE +33)
#define	T_REDPATH		(T_YYBASE +34)
#define	T_DBPATH		(T_YYBASE +35)
#define	T_UPDATE		(T_YYBASE +36)
#define	T_READONLY		(T_YYBASE +37)

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
	{	"apspath"			,T_APSPATH	},
	{	"wfcpath"			,T_WFCPATH	},
	{	"redpath"			,T_REDPATH	},
	{	"dbpath"			,T_DBPATH	},
	{	"update"			,T_UPDATE	},
	{	"readonly"			,T_READONLY	},
	{	""					,0			}
};

static	GHashTable	*Reserved;

static	void
ParWFC(
	CURFILE	*in)
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
				ParError("invalid port number");
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
				ParError("invalid port number");
				break;
			}
			GetSymbol;
			break;
		  default:
			ParError("wfc keyword error");
			break;
		}
		if		(  ComToken  !=  ';'  ) {		
			ParError("missing ; in wfc directive");
		}
		ERROR_BREAK;
	}
LEAVE_FUNC;
}

static	void
ParCONTROL(
	CURFILE	*in)
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
				ParError("invalid port number");
				break;
			}
			break;
		  default:
			ParError("control keyword error");
			break;
		}
		if		(  ComToken  !=  ';'  ) {		
			ParError("missing ; in control directive");
		}
		ERROR_BREAK;
	}
LEAVE_FUNC;
}

extern	LD_Struct	*
LD_DummyParser(
	CURFILE	*in)
{
	LD_Struct	*ret;

ENTER_FUNC;
    ret = New(LD_Struct);
    ret->name = StrDup(ComSymbol);
LEAVE_FUNC;
	return	(ret);
}

static	BLOB_Struct	*
ParBLOB(
	CURFILE	*in)
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
				auth = NewURL();
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
				ParError("invalid auth URL");
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
				ParError("invalid port number");
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
				ParError("invalid blob space");
				break;
			}
			break;
		  default:
			ParError("blob keyword error");
			break;
		}
		if		(  ComToken  !=  ';'  ) {		
			ParError("missing ; in blob directive");
		}
		ERROR_BREAK;
	}
LEAVE_FUNC;
	return	(blob);
}

static	void
ParLD_Elements(
	CURFILE	*in,
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
		dbgprintf("[%s]\n",name);
		if ( parse_ld ) {
			ld = LD_Parser(name); 
		} else {
			ld = LD_DummyParser(in);
		}
		if		(  ld !=  NULL  ) {
			if		(  g_hash_table_lookup(ThisEnv->LD_Table,ComSymbol)
					   !=  NULL  ) {
				ParError("same ld appier");
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
			switch	(ComToken)	{
			  case	T_SYMBOL:
			  case	T_SCONST:
				dbgprintf("[%s]",ComSymbol);
				dbgmsg("symbol");
				ld->nports = 0;
				while	(	(  ComToken  ==  T_SCONST  )
						||	(  ComToken  ==  T_SYMBOL  ) ) {
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
							ParError("invalid ld multiplicity number");
						}
						break;
					  case	T_ICONST:
						n = ComInt;
						GetSymbol;
						break;
					  case	';':
						n = 1;
						break;
					  default:
						ParError("invalid operator(ld multiplicity)");
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
				break;
			  case	T_ICONST:
				dbgmsg("iconst");
				ld->nports = ComInt;
				ld->ports = (Port **)xmalloc(sizeof(Port *) * ld->nports);
				for	( i = 0 ; i < ld->nports ; i ++ ) {
					ld->ports[i] = NULL;
				}
				GetSymbol;
				break;
			  default:
				dbgmsg("else");
				ld->ports = (Port **)xmalloc(sizeof(Port *));
				ld->ports[0] = ParPort("localhost",NULL);
				ld->nports = 1;
			}
		}
		p = q + 1;
		ERROR_BREAK;
	}	while	(	(  q   !=  NULL  )
				&&	(  ld  ==  NULL  ) );
	if		(  ld  ==  NULL  ) {
		ParError("ld not found");
	}
LEAVE_FUNC;
}

static	void
SkipLD(
	CURFILE	*in)
{
ENTER_FUNC;
	while	(  ComToken  !=  ';'  ) {
		switch	(ComToken) {
		  case	T_SYMBOL:
		  case	T_SCONST:
			switch	(GetSymbol)	{
			  case	',':
				break;
			  case	'*':
				if		(  GetSymbol  ==  T_ICONST  ) {
					GetSymbol;
				} else {
					ParError("invalid number");
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
		ERROR_BREAK;
	}
LEAVE_FUNC;
}

static	void
ParLD(
	CURFILE	*in,
	char	*dname,
	Bool    parse_ld)
{
ENTER_FUNC;
	while	(  GetSymbol  !=  '}'  ) {
		if		(	(  ComToken  ==  T_SYMBOL  )
				||	(  ComToken  ==  T_SCONST  )
				||	(  ComToken  ==  T_EXIT    ) ) {
			if		(  ThisEnv->D_Dir  ==  NULL  ) {
				SkipLD(in);
			} else {
				if		(	(  dname  ==  NULL  )
						||	(  !strcmp(ComSymbol,dname)  ) ) {
					ParLD_Elements(in,parse_ld);
				} else {
					SkipLD(in);
				}
			}
			if		(  ComToken  !=  ';'  ) {
				Message("[%c]\n",ComToken);
				Message("[%s]\n",ComSymbol);
				ParError("syntax error 2");
			}
		}
		ERROR_BREAK;
	}
LEAVE_FUNC;
}

static	void
ParBD(
	CURFILE	*in,
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
							if		(  bd  !=  NULL  ) {
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
									ParError("same bd appier");
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
			ParError("syntax error 1");
		}
		ERROR_BREAK;
	}
LEAVE_FUNC;
}

static	void
ParDBD(
	CURFILE	*in,
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
								ParError("same db appier");
							}
						}
						p = q + 1;
					}	while	(	(  q   !=  NULL  )
								&&	(  dbd  ==  NULL  ) );
					if		(  dbd  ==  NULL  ) {
						ParError("dbd not found");
					}
				}
			}
		}
		if		(  GetSymbol  !=  ';'  ) {
			ParError("syntax error 1");
		}
		ERROR_BREAK;
	}
LEAVE_FUNC;
}

static	void
AddDB_Server(
	DBG_Struct	*dbg,
	DB_Server	*server)
{
	DB_Server	*tmp;

ENTER_FUNC;
	tmp = (DB_Server *)xmalloc(sizeof(DB_Server) * ( dbg->nServer + 1 ));
	if		(  dbg->server  !=  NULL  ) {
		memcpy(tmp,dbg->server,(sizeof(DB_Server) * dbg->nServer));
	}
	if		(  server  !=  NULL  ) {
		memcpy(&tmp[dbg->nServer],server,sizeof(DB_Server));
	} else {
		memclear(&tmp[dbg->nServer],sizeof(DB_Server));
	}
	dbg->nServer ++;
	xfree(dbg->server);
	dbg->server = tmp;
LEAVE_FUNC;
}

static	DBG_Struct	*
NewDBG_Struct(
	char	*name)
{
	DBG_Struct	*dbg;
	char		*env;

	dbg = New(DBG_Struct);
	dbg->name = StrDup(name);
	dbg->id = 0;
	dbg->type = NULL;
	dbg->func = NULL;
	dbg->nServer = 0;
	dbg->server = NULL;
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

	return	(dbg);
}

static	void
ParDB_Server(
	int			usage,
	DBG_Struct	*dbg,
	CURFILE	*in)
{
	DB_Server	server;

	server.usage = usage;
	server.port = NULL;
	server.dbname = NULL;
	server.user = NULL;
	server.pass = NULL;
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_PORT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.port = ParPort(ComSymbol,NULL);
			} else {
				ParError("invalid port");
			}
			break;
		  case	T_NAME:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.dbname = StrDup(ComSymbol);
			} else {
				ParError("invalid DB name");
			}
			break;
		  case	T_USER:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.user = StrDup(ComSymbol);
			} else {
				ParError("invalid DB user");
			}
			break;
		  case	T_PASS:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.pass = StrDup(ComSymbol);
			} else {
				ParError("invalid DB password");
			}
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			ParError("; not found in db_group");
		}
		ERROR_BREAK;
	}
	AddDB_Server(dbg,&server);
}

static	void
ParDBGROUP(
	CURFILE	*in,
	char	*name)
{
	DBG_Struct	*dbg;
	DB_Server	server;
	int		i;

ENTER_FUNC;
	if		(  g_hash_table_lookup(ThisEnv->DBG_Table,name)  !=  NULL  ) {
		ParError("DB group name duplicate");
	}
	dbg = NewDBG_Struct(name);
	memclear(&server,sizeof(server));
	server.usage = DB_UPDATE;
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_UPDATE:
			if		(  GetSymbol  ==  '{'  ) {
				ParDB_Server(DB_UPDATE,dbg,in);
			} else {
				ParError("{ not found");
			}
			break;
		  case	T_READONLY:
			if		(  GetSymbol  ==  '{'  ) {
				ParDB_Server(DB_READONLY,dbg,in);
			} else {
				ParError("{ not found");
			}
			break;
		  case	T_TYPE:
			GetSymbol;
			if		(	(  ComToken  ==  T_SYMBOL  )
					||	(  ComToken  ==  T_SCONST  ) ) {
				dbg->type = StrDup(ComSymbol);
			} else {
				ParError("invalid DBMS type");
			}
			break;
		  case	T_REDIRECTPORT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->redirectPort = ParPort(ComSymbol,PORT_REDIRECT);
			} else {
				ParError("invalid port");
			}
			break;
		  case	T_FILE:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->file = StrDup(ComSymbol);
			} else {
				ParError("invalid logging file name");
			}
			break;
		  case	T_ENCODING:
			if		(  GetSymbol  ==  T_SCONST  ) {
#if	1
				dbg->coding = StrDup(ComSymbol);
#else
				if		(	(  stricmp(ComSymbol,"utf8")   ==  0  )
						||	(  stricmp(ComSymbol,"utf-8")  ==  0  ) ) {
					dbg->coding = NULL;
				} else {
					dbg->coding = StrDup(ComSymbol);
				}
#endif
			} else {
				ParError("invalid logging file name");
			}
			break;
		  case	T_REDIRECT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->redirect = (DBG_Struct *)StrDup(ComSymbol);
			} else {
				ParError("invalid redirect group");
			}
			break;
		  case	T_PRIORITY:
			if		(  GetSymbol  ==  T_ICONST  ) {
				dbg->priority = atoi(ComSymbol);
			} else {
				ParError("priority invalid");
			}
			break;
		  case	T_PORT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.port = ParPort(ComSymbol,NULL);
			} else {
				ParError("invalid port");
			}
			break;
		  case	T_NAME:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.dbname = StrDup(ComSymbol);
			} else {
				ParError("invalid DB name");
			}
			break;
		  case	T_USER:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.user = StrDup(ComSymbol);
			} else {
				ParError("invalid DB user");
			}
			break;
		  case	T_PASS:
			if		(  GetSymbol  ==  T_SCONST  ) {
				server.pass = StrDup(ComSymbol);
			} else {
				ParError("invalid DB password");
			}
			break;
		  default:
			ParErrorPrintf("other syntax error in db_group [%s]\n",ComSymbol);
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			ParError("; not found in db_group");
		}
		ERROR_BREAK;
	}
	if		(  server.dbname  !=  NULL  ) {
		if		(  dbg->server  ==  NULL  ) {
			AddDB_Server(dbg,&server);
		} else {
			AddDB_Server(dbg,NULL);
			for	( i = dbg->nServer - 1  ; i > 0 ; i -- ) {
				dbg->server[i] = dbg->server[i-1];
			}
			dbg->server[0] = server;
		}
	}
	RegistDBG(dbg);
LEAVE_FUNC;
}

static	void
_AssignDBG(
	CURFILE	*in,
	char	*name,
	DBG_Struct	*dbg)
{
	DBG_Struct	*red;

ENTER_FUNC;
	if		(  dbg->redirect  !=  NULL  ) {
		red = g_hash_table_lookup(ThisEnv->DBG_Table,(char *)dbg->redirect);
		if		(  red  ==  NULL  ) {
			ParError("redirect DB group not found");
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
AssignDBG(
	CURFILE	*in)
{
	int		i;
	DBG_Struct	*dbg;

ENTER_FUNC;
	qsort(ThisEnv->DBG,ThisEnv->cDBG,sizeof(DBG_Struct *),
		  (int (*)(const void *,const void *))DBGcomp);
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		dbgprintf("%d DB group name = [%s]\n",dbg->priority,dbg->name);
		_AssignDBG(in,dbg->name,dbg);
	}
LEAVE_FUNC;
}

static	RecordStruct	*
BuildMcpArea(
	size_t	stacksize)
{
	RecordStruct	*rec;
	char    *buff
		,	*p;

	buff = (char *)xmalloc(SIZE_BUFF);
	p = buff;
	p += sprintf(p,	"mcparea	{");
	p += sprintf(p,		"version int;");
	p += sprintf(p,		"func varchar(%d);",SIZE_FUNC);
	p += sprintf(p,		"rc int;");
	p += sprintf(p,		"dc	{");
	p += sprintf(p,			"window	 varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"widget	 varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"event	 varchar(%d);",SIZE_EVENT);
	p += sprintf(p,			"module	 varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"fromwin varchar(%d);",SIZE_NAME);
	p += sprintf(p,			"status	 varchar(%d);",SIZE_STATUS);
	p += sprintf(p,			"dbstatus varchar(%d);",SIZE_STATUS);
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
	p += sprintf(p,			"limit      int;");
	p += sprintf(p,			"rcount     int;");
	p += sprintf(p,		"};");
	p += sprintf(p,		"private	{");
	p += sprintf(p,			"count	int;");
	p += sprintf(p,			"swindow	char(%d)[%d];",SIZE_NAME,(int)stacksize);
	p += sprintf(p,			"state		char(1)[%d];",(int)stacksize);
	p += sprintf(p,			"pstatus	char(1);");
	p += sprintf(p,			"pputtype 	int;");
	p += sprintf(p,			"prc		char(1);");
	p += sprintf(p,		"};");
	p += sprintf(p,	"};");
	rec = ParseRecordMem(buff);

	return	(rec);
}

static	DI_Struct	*
NewEnv(
	char	*name)
{
	char	buff[SIZE_LONGNAME+1];
	DI_Struct	*env;

	env = New(DI_Struct);
	env->name = StrDup(name);
	env->BaseDir = BaseDir;
	env->D_Dir = D_Dir;
	env->RecordDir = RecordDir;
	sprintf(buff,"/tmp/wfc.%s",name);
	env->WfcApsPort = ParPort(buff,NULL);
	sprintf(buff,"/tmp/wfc.term");
	env->TermPort = ParPort(buff,NULL);
	sprintf(buff,"/tmp/wfcc.%s",name);
	env->ControlPort = ParPort(buff,NULL);
	env->cLD = 0;
	env->cBD = 0;
	env->cDBD = 0;
	env->stacksize = SIZE_STACK;
	env->LD_Table = NewNameHash();
	env->BD_Table = NewNameHash();
	env->DBD_Table = NewNameHash();
	env->mlevel = MULTI_NO;
	env->cDBG = 0;
	env->DBG = NULL;
	env->DBG_Table = NewNameHash();
	env->blob = NULL;
	env->ApsPath = NULL;
	env->WfcPath = NULL;
	env->RedPath = NULL;
	env->DbPath = NULL;
	env->linkrec = NULL;

	return	(env);
}

static	DI_Struct	*
ParDI(
	CURFILE	*in,
	char	*ld,
	char	*bd,
	char	*db,
	Bool    parse_ld)
{
	char	gname[SIZE_LONGNAME+1]
		,	buff[SIZE_LONGNAME+1];

ENTER_FUNC;
	ThisEnv = NULL;
	while	(  GetSymbol  !=  T_EOF  ) {
		switch	(ComToken) {
		  case	T_NAME:
			if		(	(  GetName   !=  T_SYMBOL  )
					&&	(  ComToken  !=  T_SCONST  ) ) {
				ParError("no name");
			} else {
				ThisEnv = NewEnv(ComSymbol);
			}
			break;
		  case	T_STACKSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				if		(  ComInt  >  SIZE_STACK  ) {
					ThisEnv->stacksize = ComInt;
				}
			} else {
				ParError("stacksize must be integer");
			}
			break;
		  case	T_LINKSIZE:
			ParError("this feature is obsolete. use linkage directive");
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
					ParError("linkage record not found");
				}
			} else {
				ParError("linkage invalid");
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
				ParError("base directory invalid");
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
				ParError("recored directory invalid");
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
				ParError("DDIR directory invalid");
			}
			break;
		  case	T_APSPATH:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->ApsPath  ==  NULL  ) {
					ThisEnv->ApsPath = StrDup(ExpandPath(ComSymbol
														 ,ThisEnv->BaseDir));
				} else {
					ParError("APS path definision duplicate");
				}
			} else {
				ParError("APS path invalid");
			}
			break;
		  case	T_WFCPATH:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->WfcPath  ==  NULL  ) {
					ThisEnv->WfcPath = StrDup(ExpandPath(ComSymbol
														 ,ThisEnv->BaseDir));
				} else {
					ParError("WFC path definision duplicate");
				}
			} else {
				ParError("WFC path invalid");
			}
			break;
		  case	T_REDPATH:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->RedPath  ==  NULL  ) {
					ThisEnv->RedPath = StrDup(ExpandPath(ComSymbol
														 ,ThisEnv->BaseDir));
				} else {
					ParError("DBREDIRECTOR path definision duplicate");
				}
			} else {
				ParError("DBREDIRECTOR path invalid");
			}
			break;
		  case	T_DBPATH:
			if		(  GetSymbol  ==  T_SCONST  ) {
				ThisEnv->DbPath = StrDup(ExpandPath(ComSymbol
													,ThisEnv->BaseDir));
				SetDBGPath(ThisEnv->DbPath);
			} else {
				ParError("db handler load path invalid");
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
				ParError("invalid multiplex level");
			}
			break;
		  case	T_BLOB:
			if		(  GetSymbol  ==  '{'  ) {
				ThisEnv->blob = ParBLOB(in);
			} else {
				ParError("syntax error in blob directive");
			}
			break;
		  case	T_CONTROL:
			if		(  GetSymbol  ==  '{'  ) {
				ParCONTROL(in);
			} else {
				ParError("syntax error in control directive");
			}
			break;
		  case	T_WFC:
			if		(  GetSymbol  ==  '{'  ) {
				ParWFC(in);
			} else {
				ParError("syntax error in wfc directive");
			}
			break;
		  case	T_LD:
			if		(  GetSymbol  ==  '{'  ) {
				ParLD(in,ld, parse_ld);
			} else {
				ParError("syntax error in ld directive");
			}
			break;
		  case	T_BD:
			if		(  GetSymbol  ==  '{'  ) {
				ParBD(in,bd, parse_ld);
			} else {
				ParError("syntax error in bd directive");
			}
			break;
		  case	T_DB:
			if		(  GetSymbol  ==  '{'  ) {
				ParDBD(in,db);
			} else {
				ParError("syntax error in db directive");
			}
			break;
		  case	T_DBGROUP:
			if		(  GetSymbol  ==  T_SCONST  ) {
				strcpy(gname,ComSymbol);
				if		(  GetSymbol  !=  '{'  ) {
					ParError("syntax error in db names");
				}
			} else
			if		(  ComToken  ==  '{'  ) {
				strcpy(gname,"");
			} else {
				ParError("syntax error dbgroup directive");
			}
			ParDBGROUP(in,gname);
			break;
		  default:
			ParErrorPrintf("misc syntax error [%X][%s]\n",ComToken,ComSymbol);
			break;
		}
		if		(  GetSymbol  !=  ';'  ) {
			ParError("; missing");
		}
		ERROR_BREAK;
	}
	if		(  ThisEnv  !=  NULL  )	{
		ThisEnv->mcprec = BuildMcpArea(ThisEnv->stacksize);
		AssignDBG(in);
	}
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
	CURFILE		*in
		,		root;

ENTER_FUNC;
	ret = NULL;
	root.next = NULL;
	DirectoryDir = dirname(StrDup(name));
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  ( in = PushLexInfo(&root,name,DirectoryDir,Reserved) )  !=  NULL  ) {
			ret = ParDI(in,ld,bd,db,parse_ld);
			DropLexInfo(&in);
		}
	} else {
		Error("DI file not found %s\n", name);
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
