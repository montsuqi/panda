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
#define	MAIN
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define	_DB_PARSER

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	"types.h"
#include	"value.h"
#include	"misc.h"
#include	"DDparser.h"
#include	"DBparser.h"
#include	"DBlex.h"
#include	"driver.h"
#include	"dirs.h"
#include	"debug.h"

static	Bool	fError;

#define	GetSymbol	(DB_Token = DB_Lex(FALSE))
#define	GetName		(DB_Token = DB_Lex(TRUE))
#undef	Error
#define	Error(msg)	{fError=TRUE;_Error((msg),DB_FileName,DB_cLine);}

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
}

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
		if		(	(  DB_Token  ==  T_SYMBOL  )
				||	(  DB_Token  ==  T_SCONST  ) ) {
			if		(  stricmp(DB_ComSymbol,"metadb")  ) {
				strcpy(buff,RecordDir);
				p = buff;
				do {
					if		(  ( q = strchr(p,':') )  !=  NULL  ) {
						*q = 0;
					}
					sprintf(name,"%s/%s.db",p,DB_ComSymbol);
					if		(  (  db = DD_ParserDataDefines(name) )  !=  NULL  ) {
						if		(  g_hash_table_lookup(dbd->DB_Table,DB_ComSymbol)  ==  NULL  ) {
							rtmp = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * ( dbd->cDB + 1));
							memcpy(rtmp,dbd->db,sizeof(RecordStruct *) * dbd->cDB);
							xfree(dbd->db);
							if		(  db->opt.db->dbg  ==  NULL  ) {
								db->opt.db->dbg = (DBG_Struct *)StrDup(gname);
							}
							dbd->db = rtmp;
							dbd->db[dbd->cDB] = db;
							dbd->cDB ++;
							g_hash_table_insert(dbd->DB_Table, StrDup(DB_ComSymbol),(void *)dbd->cDB);
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
		switch	(DB_Token) {
		  case	T_NAME:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("no name");
			} else {
				ret = New(DBD_Struct);
				ret->name = StrDup(DB_ComSymbol);
				ret->cDB = 1;
				ret->db = (RecordStruct **)xmalloc(sizeof(RecordStruct *));
				ret->db[0] = NULL;
				ret->arraysize = 0;
				ret->textsize = 0;
				ret->DB_Table = NewNameHash();
			}
			break;
		  case	T_ARRAYSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ret->arraysize = DB_ComInt;
			} else {
				Error("invalid array size");
			}
			break;
		  case	T_TEXTSIZE:
			if		(  GetSymbol  ==  T_ICONST  ) {
				ret->textsize = DB_ComInt;
			} else {
				Error("invalid text size");
			}
			break;
		  case	T_DB:
			if		(  GetSymbol  ==  T_SCONST  ) {
				gname = StrDup(DB_ComSymbol);
				if		(  GetSymbol  !=  '{'  ) {
					Error("syntax error 3");
				}
			} else
			if		(  DB_Token  ==  '{'  ) {
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
DB_Parser(
	char	*name)
{
	FILE	*fp;
	DBD_Struct	*ret;
	struct	stat	stbuf;

dbgmsg(">DB_Parser");
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
			fError = FALSE;
			DB_FileName = name;
			DB_cLine = 1;
			DB_File = fp;
			ret = ParDB();
			fclose(DB_File);
			if		(  fError  ) {
				ret = NULL;
			}
		} else {
			Error("DB file not found");
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
dbgmsg("<DB_Parser");
	return	(ret);
}

extern	void
DB_ParserInit(void)
{
	DB_LexInit();
	DBD_Table = NewNameHash();
}

#ifdef	MAIN
char	*RecordDir;
#include	"DDparser.h"
extern	int
main(
	int		argc,
	char	**argv)
{
	DB_Struct	*ret;
	int			i;

	ThisScreen = NewScreenData();
	RecordDir = "../record";

	DD_ParserInit();
	DB_ParserInit();
	ret = DB_Parser("../lddef/demo.db");

	printf("name = [%s]\n",ret->name);
	printf("cmd  = [%s]\n",ret->cmd);
	printf("spa  = [%d]\n",ret->spasize);
	for	( i = 0 ; i < ret->cWindow ; i ++ ) {
		printf("-----\n");
		DumpValueStruct(ret->data[i]);
	}
		
	return	(0);
}
#endif
