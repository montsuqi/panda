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
#include	<unistd.h>
#include	<libgen.h>
#include	"types.h"
#include	"const.h"
#include	"dirs.h"
#include	"libmondai.h"
#include	"misc.h"
#include	"DDparser.h"
#include	"DIlex.h"
#include	"LDparser.h"
#include	"DBparser.h"
#include	"DIparser.h"
#include	"dirs.h"
#define	_DIRECTORY
#include	"directory.h"
#include	"debug.h"

static	Bool	fError;

#define	GetSymbol	(DI_Token = DI_Lex(FALSE))
#define	GetName		(DI_Token = DI_Lex(TRUE))
#undef	Error
#define	Error(msg)	{fError=TRUE;_Error((msg),DI_FileName,DI_cLine);}

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
}

static	void
ParWFC(void)
{
dbgmsg(">ParWFC");
	while	(  GetSymbol  !=  '}'  ) {
		switch	(DI_Token) {
		  case	T_PORT:
			switch	(GetSymbol) {
			  case	T_ICONST:
				ThisEnv->WfcApsPort = New(Port);
				ThisEnv->WfcApsPort->host = "localhost";
				ThisEnv->WfcApsPort->port = IntStrDup(DI_ComInt);
				break;
			  case	T_SCONST:
				ThisEnv->WfcApsPort = ParPort(DI_ComSymbol,PORT_WFC_APS);
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
		if		(  DI_Token  !=  ';'  ) {		
			Error("syntax error 1");
		}
	}
dbgmsg("<ParWFC");
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

	strcpy(buff,ThisEnv->LD_Dir);
	p = buff;
	do {
dbgmsg("-4");
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s.ld",p,DI_ComSymbol);
dbgmsg("-3");
		if		(  (  ld = LD_Parser(name) )  !=  NULL  ) {
			if		(  g_hash_table_lookup(ThisEnv->LD_Table,DI_ComSymbol)
					   !=  NULL  ) {
				Error("same ld appier");
			}
			tmp = (LD_Struct **)xmalloc(sizeof(LD_Struct *) * ( ThisEnv->cLD + 1));
dbgmsg("-2");
			if		(  ThisEnv->cLD  >  0  ) {
				memcpy(tmp,ThisEnv->ld,sizeof(LD_Struct *) * ThisEnv->cLD);
				xfree(ThisEnv->ld);
			}
			ThisEnv->ld = tmp;
			ThisEnv->ld[ThisEnv->cLD] = ld;
			ThisEnv->cLD ++;
dbgmsg("-1");
			g_hash_table_insert(ThisEnv->LD_Table, StrDup(DI_ComSymbol),ld);
			if		(  GetSymbol  ==  T_SCONST  ) {
				ld->nports = 0;
dbgmsg("0");
				while	(  DI_Token  ==  T_SCONST  ) {
					strcpy(buff,DI_ComSymbol);
					n = 0;
					switch	(GetSymbol) {
					  case	',':
						n = 1;
						break;
					  case	'*':
						if		(  GetSymbol  ==  T_ICONST  ) {
							n = DI_ComInt;
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
dbgmsg("1");
					tports = (Port **)xmalloc(sizeof(Port *) * ( ld->nports + n));
					if		(  ld->nports  >  0  ) {
						memcpy(tports,ld->ports,sizeof(Port *) * ld->nports);
						xfree(ld->ports);
					}
dbgmsg("2");
					ld->ports = tports;
					for	( i = 0 ; i < n ; i ++ ) {
						ld->ports[ld->nports] = ParPort(buff,PORT_APS_BASE);
						ld->nports ++;
					}
dbgmsg("3");
					if		(  DI_Token  ==  ','  ) {
						GetSymbol;
					}
				}
			} else
			if		(  DI_Token  ==  T_ICONST  ) {
dbgmsg("4");
				ld->nports = DI_ComInt;
				ld->ports = (Port **)xmalloc(sizeof(Port *) * ld->nports);
				for	( i = 0 ; i < ld->nports ; i ++ ) {
					ld->ports[i] = NULL;
				}
dbgmsg("5");
				GetSymbol;
			} else {
dbgmsg("6");
				ld->ports = (Port **)xmalloc(sizeof(Port *));
				ld->ports[0] = ParPort("localhost",PORT_APS_BASE);
				ld->nports = 1;
dbgmsg("7");
			}
		}
		p = q + 1;
dbgmsg("8");
	}	while	(	(  q   !=  NULL  )
				&&	(  ld  ==  NULL  ) );
	if		(  ld  ==  NULL  ) {
		Error("ld not found");
	}
}

static	void
SkipLD(void)
{
dbgmsg(">SkipLD");
	while	(  DI_Token  ==  T_SCONST  ) {
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
		if		(  DI_Token  ==  ','  ) {
			GetSymbol;
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
		if		(	(  DI_Token  ==  T_SYMBOL  )
				||	(  DI_Token  ==  T_SCONST  )
				||	(  DI_Token  ==  T_EXIT    ) ) {
			if		(  ThisEnv->LD_Dir  ==  NULL  ) {
				if		(  GetSymbol  ==  T_SCONST  ) {
					SkipLD();
				}
			} else {
				if		(	(  dname  ==  NULL  )
						||	(  !strcmp(DI_ComSymbol,dname)  ) ) {
					ParLD_Elements();
				} else {
					if		(  GetSymbol  ==  T_SCONST  ) {
						SkipLD();
					}
				}
			}
			if		(  DI_Token  !=  ';'  ) {
				printf("[%s]\n",DI_ComSymbol);
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
		if		(	(  DI_Token  ==  T_SYMBOL  )
				||	(  DI_Token  ==  T_SCONST  ) ) {
			if		(  ThisEnv->BD_Dir  !=  NULL  ) {
				if		(	(  dname  ==  NULL  )
						||	(  !strcmp(dname,DI_ComSymbol)  ) ) {
					strcpy(buff,ThisEnv->BD_Dir);
					p = buff;
					do {
						if		(  ( q = strchr(p,':') )  !=  NULL  ) {
							*q = 0;
						}
						sprintf(name,"%s/%s.bd",p,DI_ComSymbol);
						if		(  (  bd = BD_Parser(name) )  !=  NULL  ) {
							if		(  g_hash_table_lookup(ThisEnv->BD_Table,DI_ComSymbol)  ==  NULL  ) {
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
													StrDup(DI_ComSymbol),bd);
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
		if		(	(  DI_Token  ==  T_SYMBOL  )
				||	(  DI_Token  ==  T_SCONST  ) ) {
			if		(  ThisEnv->DBD_Dir  !=  NULL  ) {
				if		(	(  dname  ==  NULL  )
						||	(  !strcmp(dname,DI_ComSymbol)  ) ) {
					strcpy(buff,ThisEnv->DBD_Dir);
					p = buff;
					do {
						if		(  ( q = strchr(p,':') )  !=  NULL  ) {
							*q = 0;
						}
						sprintf(name,"%s/%s.dbd",p,DI_ComSymbol);
						if		(  (  dbd = DB_Parser(name) )  !=  NULL  ) {
							if		(  g_hash_table_lookup(ThisEnv->DBD_Table,DI_ComSymbol)  ==  NULL  ) {
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
													StrDup(DI_ComSymbol),dbd);
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

dbgmsg(">ParDBGROUP");
	if		(  g_hash_table_lookup(ThisEnv->DBG_Table,name)  !=  NULL  ) {
		Error("DB group name duplicate");
	}
	dbg = New(DBG_Struct);
	dbg->name = name;
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
	while	(  GetSymbol  !=  '}'  ) {
		switch	(DI_Token) {
		  case	T_TYPE:
			GetSymbol;
			if		(	(  DI_Token  ==  T_SYMBOL  )
					||	(  DI_Token  ==  T_SCONST  ) ) {
				dbg->type = StrDup(DI_ComSymbol);
			} else {
				Error("invalid DBMS type");
			}
			break;
		  case	T_PORT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->port = ParPort(DI_ComSymbol,-1);
			} else {
				Error("invalid port");
			}
			break;
		  case	T_REDIRECTPORT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->redirectPort = ParPort(DI_ComSymbol,PORT_REDIRECT);
			} else {
				Error("invalid port");
			}
			break;
		  case	T_NAME:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->dbname = StrDup(DI_ComSymbol);
			} else {
				Error("invalid DB name");
			}
			break;
		  case	T_USER:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->user = StrDup(DI_ComSymbol);
			} else {
				Error("invalid DB user");
			}
			break;
		  case	T_PASS:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->pass = StrDup(DI_ComSymbol);
			} else {
				Error("invalid DB password");
			}
			break;
		  case	T_FILE:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->file = StrDup(DI_ComSymbol);
			} else {
				Error("invalid logging file name");
			}
			break;
		  case	T_REDIRECT:
			if		(  GetSymbol  ==  T_SCONST  ) {
				dbg->redirect = (DBG_Struct *)StrDup(DI_ComSymbol);
			} else {
				Error("invalid redirect group");
			}
			break;
		  case	T_PRIORITY:
			if		(  GetSymbol  ==  T_ICONST  ) {
				dbg->priority = atoi(DI_ComSymbol);
			} else {
				Error("priority invalid");
			}
			break;
		  default:
			Error("other syntax error in db_group");
			printf("[%s]\n",DI_ComSymbol);
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

static	ValueStruct	*
BuildMcpArea(
	size_t	stacksize)
{
	RecordStruct	*rec;
	ValueStruct		*val;
	FILE			*fp;

	if		(  ( fp = tmpfile() )  ==  NULL  ) {
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
	rewind(fp);
	rec = DD_Parse(fp,"");
	fclose(fp);

	val = rec->rec;
	xfree(rec);
	return	(val);
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
		switch	(DI_Token) {
		  case	T_NAME:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("no name");
			} else {
				ThisEnv = New(DI_Struct);
				ThisEnv->name = StrDup(DI_ComSymbol);
				ThisEnv->BaseDir = BaseDir;
				ThisEnv->LD_Dir = LD_Dir;
				ThisEnv->BD_Dir = BD_Dir;
				ThisEnv->DBD_Dir = DBD_Dir;
				ThisEnv->RecordDir = RecordDir;
				ThisEnv->WfcApsPort = ParPort("localhost",PORT_WFC_APS);
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
				if		(  DI_ComInt  >  SIZE_STACK  ) {
					ThisEnv->stacksize = DI_ComInt;
				}
			} else {
				Error("stacksize must be integer");
			}
			break;
		  case	T_LINKSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ThisEnv->linksize = DI_ComInt;
			} else {
				Error("linksize must be integer");
			}
			break;
		  case	T_LINKAGE:
			if		(  GetSymbol   ==  T_SYMBOL  ) {
				if		(  ( ThisEnv->linkrec = ReadRecordDefine(DI_ComSymbol) )
						   ==  NULL  ) {
					Error("linkage record not found");
				} else {
					ThisEnv->linksize = NativeSizeValue(NULL,ThisEnv->linkrec);
				}
			} else {
				Error("linkage invalid");
			}
			break;
		  case	T_BASEDIR:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->BaseDir  ==  NULL  ) {
					if		(  !strcmp(DI_ComSymbol,".")  ) {
						ThisEnv->BaseDir = ".";
					} else {
						ThisEnv->BaseDir = StrDup(ExpandPath(DI_ComSymbol
															,NULL));
					}
				}
			} else {
				Error("base directory invalid");
			}
			break;
		  case	T_LDDIR:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->LD_Dir  ==  NULL  ) {
					if		(  !strcmp(DI_ComSymbol,".")  ) {
						ThisEnv->LD_Dir = StrDup(ThisEnv->BaseDir);
					} else {
						ThisEnv->LD_Dir = StrDup(ExpandPath(DI_ComSymbol
															,ThisEnv->BaseDir));
					}
				}
			} else {
				Error("LD directory invalid");
			}
			break;
		  case	T_BDDIR:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->BD_Dir  ==  NULL  ) {
					if		(  !strcmp(DI_ComSymbol,".")  ) {
						ThisEnv->BD_Dir = StrDup(ThisEnv->BaseDir);
					} else {
						ThisEnv->BD_Dir = StrDup(ExpandPath(DI_ComSymbol
															,ThisEnv->BaseDir));
					}
				}
			} else {
				Error("BD directory invalid");
			}
			break;
		  case	T_DBDDIR:
			if		(  GetSymbol  ==  T_SCONST  ) {
				if		(  ThisEnv->DBD_Dir  ==  NULL  ) {
					if		(  !strcmp(DI_ComSymbol,".")  ) {
						ThisEnv->DBD_Dir = StrDup(ThisEnv->BaseDir);
					} else {
						ThisEnv->DBD_Dir = StrDup(ExpandPath(DI_ComSymbol
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
					if		(  !strcmp(DI_ComSymbol,".")  ) {
						ThisEnv->RecordDir = StrDup(ThisEnv->BaseDir);
					} else {
						ThisEnv->RecordDir = StrDup(ExpandPath(DI_ComSymbol
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
			if		(  stricmp(DI_ComSymbol,"no")  ==  0  ) {
				ThisEnv->mlevel = MULTI_NO;
			} else
			if		(  stricmp(DI_ComSymbol,"db")  ==  0  ) {
				ThisEnv->mlevel = MULTI_DB;
			} else
			if		(  stricmp(DI_ComSymbol,"ld")  ==  0  ) {
				ThisEnv->mlevel = MULTI_LD;
			} else
			if		(  stricmp(DI_ComSymbol,"id")  ==  0  ) {
				ThisEnv->mlevel = MULTI_ID;
			} else
			if		(  stricmp(DI_ComSymbol,"aps")  ==  0  ) {
				ThisEnv->mlevel = MULTI_APS;
			} else {
				Error("invalid multiplex level");
			}
			break;
		  case	T_WFC:
			if		(  GetSymbol  ==  '{'  ) {
				ParWFC();
			} else {
				Error("syntax error 2");
			}
			break;
		  case	T_LD:
			if		(  GetSymbol  ==  '{'  ) {
				ParLD(ld);
			} else {
				Error("syntax error 2");
			}
			break;
		  case	T_BD:
			if		(  GetSymbol  ==  '{'  ) {
				ParBD(bd);
			} else {
				Error("syntax error 2");
			}
			break;
		  case	T_DB:
			if		(  GetSymbol  ==  '{'  ) {
				ParDBD(db);
			} else {
				Error("syntax error 2");
			}
			break;
		  case	T_DBGROUP:
			if		(  GetSymbol  ==  T_SCONST  ) {
				gname = StrDup(DI_ComSymbol);
				if		(  GetSymbol  !=  '{'  ) {
					Error("syntax error 3");
				}
			} else
			if		(  DI_Token  ==  '{'  ) {
				gname = "";
			} else {
				gname = NULL;
				Error("syntax error 4");
			}
			ParDBGROUP(gname);
			break;
		  default:
			Error("syntax error 5");
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
	FILE	*fp;
	DI_Struct	*ret;

dbgmsg(">DI_Parser");
	if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
		fError = FALSE;
		DirectoryDir = dirname(StrDup(name));
		DI_FileName = StrDup(name);
		DI_cLine = 1;
		DI_File = fp;
		ret = ParDI(ld,bd,db);
		fclose(DI_File);
		if		(  fError  ) {
			ret = NULL;
		}
	} else {
		Error("DI file not found");
		ret = NULL;
	}
dbgmsg("<DI_Parser");
	return	(ret);
}

extern	void
DI_ParserInit(void)
{
	DI_LexInit();
}
