/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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

#define	MAIN
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
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"misc.h"
#include	"directory.h"
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	Bool	fLD;
static	Bool	fBD;
static	Bool	fDBD;
static	Bool	fDBG;
static	char	*Directory;

static	void
PutTab(
	int		n)
{
	for	( ; n > 0 ; n -- ) {
		printf("\t");
	}
}

static	void
DumpKey(
	KeyStruct	*pkey)
{
	char	***item
	,		**pk;

dbgmsg(">DumpKey");
	if		(  pkey  !=  NULL  ) {
		item = pkey->item;
		PutTab(2);
		printf("pkey = ");
		while	(  *item  !=  NULL  ) {
			pk = *item;
			while	(  *pk  !=  NULL  ) {
				printf("%s",*pk);
				pk ++;
				if		(  *pk  !=  NULL  ) {
					printf(".");
				}
			}
			item ++;
			if		(  *item  !=  NULL  ) {
				printf(",");
			}
		}
	}
	printf("\n");
dbgmsg("<DumpKey");
}

#include	"SQLparser.h"
static	void
_DumpOps(
	LargeByteString	*sql)
{
	int		c;
	int		n;
	Bool	fIntoAster;
	ValueStruct	*val;

	RewindLBS(sql);
	printf("\t\t\t\tlength = %d\n",LBS_Size(sql));
	fIntoAster = FALSE;
	while	(  ( c = LBS_FetchByte(sql) )  >=  0  ) {
		if		(  c  < 0x80  ) {
			printf("%04d ",LBS_GetPos(sql)-1);
			do {
				printf("%c",c);
			}	while	(	(  ( c = LBS_FetchByte(sql) )  >=  0  )
						&&	(  c  <  0x80  ) );
			printf("\n");
		}
		printf("%04d ",LBS_GetPos(sql)-1);
		switch	(c) {
		  case	SQL_OP_INTO:
			n = LBS_FetchInt(sql);
			printf("INTO\t%d",n);
			if		(  n  >  0  ) {
				fIntoAster = FALSE;
			} else {
				fIntoAster = TRUE;
			}
			break;
		  case	SQL_OP_STO:
			if		(  !fIntoAster  ) {
				printf("STO\t%p",
					   (ValueStruct *)LBS_FetchPointer(sql));
			} else {
				printf("STO\t*");
			}
			break;
		  case	SQL_OP_REF:
			printf("REF\t%p",
				   (ValueStruct *)LBS_FetchPointer(sql));
			break;
		  case	SQL_OP_VCHAR:
			printf("VCHAR");fflush(stdout);
			break;
		  case	SQL_OP_EOL:
			printf("EOL");fflush(stdout);
			break;
		  default:
			dbgprintf("[%X]",c);
			break;
		}
		printf("\n");
	}
}

static	void
DumpOps(
	char	*name,
	int		ops,
	PathStruct	*path)
{
	LargeByteString	*sql;

	printf("\t\t\top = [%s]\n",name);
	if		(  ( sql = path->ops[ops-1] )  !=  NULL  ) {
		_DumpOps(sql);
	} else {
		printf("default operation.\n");
	}
	printf("\t\t\t-----\n");
}
static	void
DumpPath(
	PathStruct	*path)
{
	printf("\t\tname     = [%s]\n",path->name);
	g_hash_table_foreach(path->opHash,(GHFunc)DumpOps,path);
}

static	void
DumpDB(
	DB_Struct	*db)
{
	int		i;

dbgmsg(">DumpDB");
	printf("\t\tDB group = [%s]\n",((DBG_Struct *)db->dbg)->name);
	DumpKey(db->pkey);
	if		(  db->pcount  >  0  ) {
		printf("\t\tpath ------\n");
		for	( i = 1 ; i < db->pcount ; i ++ ) {
			DumpPath(db->path[i]);
		}
	}
dbgmsg("<DumpDB");
}

static	int		nTab;
static	void
DumpItems(
	ValueStruct	*value)
{
	int		i;

	if		(  value  ==  NULL  )	return;
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		printf("int");
		break;
	  case	GL_TYPE_BOOL:
		printf("bool");
		break;
	  case	GL_TYPE_BYTE:
		printf("byte");
		break;
	  case	GL_TYPE_CHAR:
		printf("char(%d)",ValueStringLength(value));
		break;
	  case	GL_TYPE_VARCHAR:
		printf("varchar(%d)",ValueStringLength(value));
		break;
	  case	GL_TYPE_DBCODE:
		printf("dbcode(%d)",ValueStringLength(value));
		break;
	  case	GL_TYPE_NUMBER:
		if		(  ValueFixedSlen(value)  ==  0  ) {
			printf("number(%d)",ValueFixedLength(value));
		} else {
			printf("number(%d,%d)",
				   ValueFixedLength(value),
				   ValueFixedSlen(value));
		}
		break;
	  case	GL_TYPE_TEXT:
		printf("text");
		break;
	  case	GL_TYPE_ARRAY:
		DumpItems(ValueArrayItem(value,0));
		printf("[%d]",ValueArraySize(value));
		break;
	  case	GL_TYPE_RECORD:
		printf("{\n");
		nTab ++;
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			PutTab(nTab);
			printf("%s\t",ValueRecordName(value,i));
			DumpItems(ValueRecordItem(value,i));
			printf(";\n");
		}
		nTab --;
		PutTab(nTab);
		printf("}");
		break;
	  default:
		break;
	}
}

static	void
DumpRecord(
	RecordStruct	*db)
{
dbgmsg(">DumpRecord");
	DumpDB(db->opt.db);
	nTab = 2;
	printf("\t\t%s\t",db->name);
	DumpItems(db->value);
	printf(";\n");
dbgmsg("<DumpRecord");
}

static	void
_DumpHandler(
	char	*name,
	WindowBind	*bind,
	void	*dummy)
{
	MessageHandler		*handler;

	handler = bind->handler;

	if		(  ( handler->fInit & INIT_LOAD )  ==  0  ) {
		handler->fInit |= INIT_LOAD;
		printf("\thandler\t\"%s\"\t{\n",handler->name);
		printf("\t\tclass     \"%s\";\n",(char *)handler->klass);
		printf("\t\tselialize \"%s\";\n",(char *)handler->serialize);
		printf("\t\tlocale    \"%s\";\n",handler->conv->locale);
		printf("\t\tstart     \"%s\";\n",handler->start);
		printf("\t\tencoding  ");
		switch	(handler->conv->encode) {
		  case	STRING_ENCODING_URL:
			printf("\"URL\";\n");
			break;
		  case	STRING_ENCODING_BASE64:
			printf("\"BASE64\";\n");
			break;
		  default:
			printf("\"NULL\";\n");
			break;
		}
			
		printf("\t};\n");
	}
}

static	void
DumpLD(
	LD_Struct	*ld)
{
	int		i;

dbgmsg(">DumpLD");
	printf("name      = [%s]\n",ld->name);
	printf("\tgroup     = [%s]\n",ld->group);
	printf("\tarraysize = %d\n",ld->arraysize);
	printf("\ttextsize  = %d\n",ld->textsize);

	g_hash_table_foreach(ld->whash,(GHFunc)_DumpHandler,NULL);

	for	( i = 0 ; i < ld->cWindow ; i ++ ) {
		printf("\tbind\t\"%s\"\t\"%s\"\t\"%s\";\n",
			   ld->window[i]->name,
			   ld->window[i]->handler->name,
			   ld->window[i]->module);
	}
	printf("\tcDB       = %d\n",ld->cDB);
	for	( i = 1 ; i < ld->cDB ; i ++ ) {
		DumpRecord(ld->db[i]);
	}
dbgmsg("<DumpLD");
}

static	void
DumpBD(
	BD_Struct	*bd)
{
	int		i;

	printf("name      = [%s]\n",bd->name);
	printf("\tarraysize = %d\n",bd->arraysize);
	printf("\ttextsize  = %d\n",bd->textsize);

	g_hash_table_foreach(bd->BatchTable,(GHFunc)_DumpHandler,NULL);

	printf("\tcDB       = %d\n",bd->cDB);
	for	( i = 1 ; i < bd->cDB ; i ++ ) {
		DumpRecord(bd->db[i]);
	}
}

static	void
DumpDBD(
	DBD_Struct	*dbd)
{
	int		i;

	printf("name      = [%s]\n",dbd->name);
	printf("\tarraysize = %d\n",dbd->arraysize);
	printf("\ttextsize  = %d\n",dbd->textsize);
	printf("\tcDB       = %d\n",dbd->cDB);
	for	( i = 1 ; i < dbd->cDB ; i ++ ) {
		DumpRecord(dbd->db[i]);
	}
}

static	void
DumpDBG(
	char		*name,
	DBG_Struct	*dbg,
	void		*dummy)
{
	printf("name     = [%s]\n",dbg->name);
	printf("\ttype     = [%s]\n",dbg->type);
	printf("\thost     = [%s]\n",dbg->port->host);
	printf("\tport     = [%s]\n"  ,dbg->port->port);
	printf("\tDB name  = [%s]\n",dbg->dbname);
	printf("\tDB user  = [%s]\n",dbg->user);
	printf("\tDB pass  = [%s]\n",dbg->pass);
	if		(  dbg->file  !=  NULL  ) {
		printf("\tlog file = [%s]\n",dbg->file);
	}
	if		(  dbg->redirect  !=  NULL  ) {
		while	(  dbg->redirect  !=  NULL  ) {
			dbg = dbg->redirect;
		}
		printf("\tredirect = [%s]\n",dbg->name);
	}
}

static	void
DumpDirectory(void)
{
	int		i;

dbgmsg(">DumpDirectory");
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL);

	printf("name     = [%s]\n",ThisEnv->name);
	printf("mlevel   = %d\n"  ,ThisEnv->mlevel);
	printf("linksize = %d\n"  ,ThisEnv->linksize);
	printf("cLD      = %d\n"  ,ThisEnv->cLD);
	printf("cBD      = %d\n"  ,ThisEnv->cBD);
	printf("cDBD     = %d\n"  ,ThisEnv->cDBD);
	if		(  fLD  ) {
		printf("LD ----------\n");
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			DumpLD(ThisEnv->ld[i]);
		}
	}
	if		(  fBD  ) {
		printf("BD ----------\n");
		for	( i = 0 ; i < ThisEnv->cBD ; i ++ ) {
			DumpBD(ThisEnv->bd[i]);
		}
	}
	if		(  fDBD  ) {
		printf("DBD ----------\n");
		for	( i = 0 ; i < ThisEnv->cDBD ; i ++ ) {
			DumpDBD(ThisEnv->db[i]);
		}
	}
	if		(  fDBG  ) {
		printf("DBG ---------\n");
		g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)DumpDBG,NULL);
	}
dbgmsg("<DumpDirectory");
}

static	ARG_TABLE	option[] = {
	{	"ld",		BOOLEAN,	TRUE,		(void*)&fLD,
		"LD情報を出力する"								},
	{	"bd",		BOOLEAN,	TRUE,		(void*)&fBD,
		"BD情報を出力する"								},
	{	"dbd",		BOOLEAN,	TRUE,		(void*)&fDBD,
		"DBD情報を出力する"								},
	{	"dbg",		BOOLEAN,	TRUE,		(void*)&fDBG,
		"DB group情報を出力する"						},

	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"レコードのあるディレクトリ"					},
	{	"ddir",		STRING,		TRUE,	(void*)&D_Dir,
		"定義格納ディレクトリ"		 					},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	D_Dir = NULL;
	RecordDir = NULL;

	Directory = "./directory";
	fLD = FALSE;
	fBD = FALSE;
	fDBD = FALSE;
	fDBG = FALSE;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("checkdir",NULL);

	DumpDirectory();

	return	(0);
}
