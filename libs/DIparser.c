/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA.

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
#define	T_LDDIR			(T_YYBASE +14)
#define	T_BDDIR			(T_YYBASE +15)
#define	T_RECDIR		(T_YYBASE +16)
#define	T_BASEDIR		(T_YYBASE +17)
#define	T_PRIORITY		(T_YYBASE +18)
#define	T_LINKAGE		(T_YYBASE +19)
#define	T_STACKSIZE		(T_YYBASE +20)
#define	T_DB			(T_YYBASE +21)
#define	T_DBDDIR		(T_YYBASE +22)
#define	T_WFC			(T_YYBASE +23)
#define	T_EXIT			(T_YYBASE +24)
#define	T_LOCALE		(T_YYBASE +25)
#define	T_TERMPORT		(T_YYBASE +26)
#define	T_CONTROL		(T_YYBASE +27)

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
	{	"lddir"				,T_LDDIR	},
	{	"bddir"				,T_BDDIR	},
	{	"dbddir"			,T_DBDDIR	},
	{	"record"			,T_RECDIR	},
	{	"base"				,T_BASEDIR	},
	{	"priority"			,T_PRIORITY	},
	{	"linkage"			,T_LINKAGE	},
	{	"stacksize"			,T_STACKSIZE	},
	{	"wfc"				,T_WFC		},
	{	"exit"				,T_EXIT		},
	{	"locale"			,T_LOCALE	},
	{	"termport"			,T_TERMPORT	},
	{	"control"			,T_CONTROL	},
	{	""					,0			}
};

static	GHashTable	*Reserved;

static	void
ParWFC(void)
{
dbgmsg(">ParWFC");
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
dbgmsg("<ParWFC");
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

static	void
ParLD_Elements(void)
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
		if		(  (  ld = LD_Parser(name) )  !=  NULL  ) {
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
						ld->ports[ld->nports] = ParPort(buff,NULL);
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
dbgmsg(">SkipLD");
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
dbgmsg("<SkipLD");
}

static	void
ParLD(
	char	*dname)
{
dbgmsg(">ParLD");
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
					ParLD_Elements();
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
dbgmsg("<ParLD");
}

static	void
ParBD(
	char	*dname)
{
	char		name[SIZE_BUFF];
	BD_Struct	*bd;
	BD_Struct	**btmp;
	char		buff[SIZE_BUFF];
	char		*p
	,			*q;

dbgmsg(">ParBD");
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
						if		(  (  bd = BD_Parser(name) )  !=  NULL  ) {
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
					}	while	(	(  q   !=  NULL  )
								&&	(  bd  ==  NULL  ) );
					if		(  bd  ==  NULL  ) {
						Error("bd not found");
					}
				}
			}
		}
		if		(  GetSymbol  !=  ';'  ) {
			Error("syntax error 1");
		}
	}
dbgmsg("<ParBD");
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

dbgmsg(">ParDBD");
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
dbgmsg("<ParDBD");
}

static	void
AppendDBG(
	DBG_Struct	*dbg)
{
	DBG_Struct	**dbga;

dbgmsg(">AppendDBG");
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
dbgmsg("<AppendDBG");
}

static	void
ParDBGROUP(
	char	*name)
{
	DBG_Struct	*dbg;
	char		*env;

dbgmsg(">ParDBGROUP");
	if		(  g_hash_table_lookup(ThisEnv->DBG_Table,name)  !=  NULL  ) {
		Error("DB group name duplicate");
	}
	dbg = New(DBG_Struct);
	dbg->name = name;
	dbg->id = 0;
	dbg->type = "PostgreSQL";
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
		  case	T_LOCALE:
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
dbgmsg("<ParDBGROUP");
}

static	void
_AssignDBG(
	char	*name,
	DBG_Struct	*dbg,
	void	*dummy)
{
	DBG_Struct	*red;

dbgmsg(">_AssignDBG");
	if		(  dbg->redirect  !=  NULL  ) {
		red = g_hash_table_lookup(ThisEnv->DBG_Table,(char *)dbg->redirect);
		if		(  red  ==  NULL  ) {
			Error("redirect DB group not found");
		}
		xfree(dbg->redirect);
		dbg->redirect = red;
	}
dbgmsg("<_AssignDBG");
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

dbgmsg(">AssignDBG");
	qsort(ThisEnv->DBG,ThisEnv->cDBG,sizeof(DBG_Struct *),
		  (int (*)(const void *,const void *))DBGcomp);
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		//		printf("%d DB group name = [%s]\n",dbg->priority,dbg->name);
		_AssignDBG(dbg->name,dbg,NULL);
	}
dbgmsg("<AssignDBG");
}

static	RecordStruct	*
BuildMcpArea(
	size_t	stacksize)
{
	FILE	*fp;
	char	name[SIZE_LONGNAME+1];
	RecordStruct	*rec;

	sprintf(name,"/tmp/mcparea%d.rec",(int)getpid());
	if		(  ( fp = fopen(name,"w") )  ==  NULL  ) {
		fprintf(stderr,"tempfile can not make.\n");
		exit(1);
	}
	fprintf(fp,	"mcparea	{");
	fprintf(fp,		"func varchar(%d);",SIZE_FUNC);
	fprintf(fp,		"rc int;");
	fprintf(fp,		"dc	{");
	fprintf(fp,			"window	 varchar(%d);",SIZE_NAME);
	fprintf(fp,			"widget	 varchar(%d);",SIZE_NAME);
	fprintf(fp,			"event	 varchar(%d);",SIZE_EVENT);
	fprintf(fp,			"module	 varchar(%d);",SIZE_NAME);
	fprintf(fp,			"fromwin varchar(%d);",SIZE_NAME);
	fprintf(fp,			"status	 varchar(%d);",SIZE_STATUS);
	fprintf(fp,			"puttype varchar(%d);",SIZE_PUTTYPE);
	fprintf(fp,			"term	 varchar(%d);",SIZE_TERM);
	fprintf(fp,			"user	 varchar(%d);",SIZE_USER);
	fprintf(fp,		"};");
	fprintf(fp,		"db	{");
	fprintf(fp,			"path	{");
	fprintf(fp,				"blocks	int;");
	fprintf(fp,				"rname	int;");
	fprintf(fp,				"pname	int;");
	fprintf(fp,			"};");
	fprintf(fp,		"};");
	fprintf(fp,		"private	{");
	fprintf(fp,			"count	int;");
	fprintf(fp,			"swindow	char(%d)[%d];",SIZE_NAME,stacksize);
	fprintf(fp,			"state		char(1)[%d];",stacksize);
	fprintf(fp,			"index		int[%d];",stacksize);
	fprintf(fp,			"pstatus	char(1);");
	fprintf(fp,			"pputtype 	char(1);");
	fprintf(fp,			"prc		char(1);");
	fprintf(fp,		"};");
	fprintf(fp,	"};");
	fclose(fp);

	rec = ParseRecordFile(name);
	remove(name);

	return	(rec);
}

static	DI_Struct	*
ParDI(
	char	*ld,
	char	*bd,
	char	*db)
{
	char	*gname;

dbgmsg(">ParDI");
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
				if		(  ( ThisEnv->linkrec = ReadRecordDefine(ComSymbol) )
						   ==  NULL  ) {
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
		  case	T_LDDIR:
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
				Error("LD directory invalid");
			}
			break;
		  case	T_BDDIR:
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
				Error("BD directory invalid");
			}
			break;
		  case	T_DBDDIR:
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
				Error("DBD directory invalid");
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
				ParLD(ld);
			} else {
				Error("syntax error in ld directive");
			}
			break;
		  case	T_BD:
			if		(  GetSymbol  ==  '{'  ) {
				ParBD(bd);
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
dbgmsg("<ParDI");
	return	(ThisEnv);
}

extern	DI_Struct	*
DI_Parser(
	char	*name,
	char	*ld,
	char	*bd,
	char	*db)
{
	struct	stat	stbuf;
	DI_Struct	*ret;

dbgmsg(">DI_Parser");
	DirectoryDir = dirname(StrDup(name));
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  PushLexInfo(name,DirectoryDir,Reserved)  !=  NULL  ) {
			ret = ParDI(ld,bd,db);
			DropLexInfo();
		} else {
			ret = NULL;
		}
	} else {
        fprintf(stderr, "DI file not found\n");
        exit(1);
	}
dbgmsg("<DI_Parser");
	return	(ret);
}

extern	void
DI_ParserInit(void)
{
	LexInit();
	Reserved = MakeReservedTable(tokentable);
}
