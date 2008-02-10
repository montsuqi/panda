/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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
#include	"RecParser.h"
#include	"front.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	Bool	fLD;
static	Bool	fBD;
static	Bool	fDBD;
static	Bool	fDBG;
static	int		nTab;
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
_DumpItems(
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
		printf("char(%d)",(int)ValueStringLength(value));
		break;
	  case	GL_TYPE_VARCHAR:
		printf("varchar(%d)",(int)ValueStringLength(value));
		break;
	  case	GL_TYPE_DBCODE:
		printf("dbcode(%d)",(int)ValueStringLength(value));
		break;
	  case	GL_TYPE_NUMBER:
		if		(  ValueFixedSlen(value)  ==  0  ) {
			printf("number(%d)",(int)ValueFixedLength(value));
		} else {
			printf("number(%d,%d)",
				   (int)ValueFixedLength(value),
				   (int)ValueFixedSlen(value));
		}
		break;
	  case	GL_TYPE_TEXT:
		printf("text");
		break;
	  case	GL_TYPE_ARRAY:
		_DumpItems(ValueArrayItem(value,0));
		printf("[%d]",(int)ValueArraySize(value));
		break;
	  case	GL_TYPE_RECORD:
		printf("{\n");
		nTab ++;
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			PutTab(nTab);
			printf("%s\t",ValueRecordName(value,i));
			_DumpItems(ValueRecordItem(value,i));
			printf(";\n");
		}
		nTab --;
		PutTab(nTab);
		printf("}");
		break;
	  default:
		break;
	}
	fflush(stdout);
}

static	void
DumpItems(
	int			n,
	ValueStruct	*value)
{
	nTab = n;
	_DumpItems(value);
}

static	void
DumpKey(
	KeyStruct	*pkey)
{
	char	***item
	,		**pk;

ENTER_FUNC;
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
LEAVE_FUNC;
}

#include	"SQLparser.h"
static	void
_DumpOps(
	LargeByteString	*sql)
{
	int		c;
	int		n;
	Bool	fIntoAster;

ENTER_FUNC;
	RewindLBS(sql);
	printf("\t\t\t\tlength = %d\n",(int)LBS_Size(sql));
	fIntoAster = FALSE;
	while	(  ( c = LBS_FetchByte(sql) )  >=  0  ) {
		if		(  c  !=  SQL_OP_ESC  ) {
			printf("%04d ",(int)LBS_GetPos(sql)-1);
			do {
				printf("%c",c);
			}	while	(	(  ( c = LBS_FetchByte(sql) )  >=  0  )
						&&	(  c  !=  SQL_OP_ESC  ) );
			printf("\n");
		}
		printf("%04d ",(int)LBS_GetPos(sql)-1);
		switch	(c = LBS_FetchByte(sql)) {
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
		  case	SQL_OP_LIMIT:
			printf(" $limit");fflush(stdout);
			break;
		  default:
			dbgprintf("[%X]",c);
			break;
		}
		printf("\n");
	}
LEAVE_FUNC;
}

static	void
DumpOps(
	char	*name,
	int		ops,
	PathStruct	*path)
{
	DB_Operation	*op;

	printf("\t\t\top = [%s]\n",name);
	if		(  ( op = path->ops[ops-1] )  !=  NULL  ) {
		if		(  op->args  !=  NULL  ) {
			printf("** args\n\t\t\t");
			DumpItems(3,op->args);
			printf("\n");
		}
		if		(  (int)(unsigned long)op->proc  >=  SIZE_DBOP  ) {
			_DumpOps(op->proc);
		}
	} else {
		printf("default operation.\n");
	}
	printf("\t\t\t-----\n");
}
static	void
DumpPath(
	PathStruct	*path)
{
ENTER_FUNC;
	printf("\t\tname     = [%s]\n",path->name);
	if		(  path->args  !=  NULL  ) {
		printf("** args\n\t\t");
		DumpItems(2,path->args);
		printf("\n");
	}
	g_hash_table_foreach(path->opHash,(GHFunc)DumpOps,path);
LEAVE_FUNC;
}

static	void
DumpDB(
	DB_Struct	*db)
{
	int		i;

ENTER_FUNC;
	printf("\t\tDB group = [%s]\n",((DBG_Struct *)db->dbg)->name);
	DumpKey(db->pkey);
	if		(  db->pcount  >  0  ) {
		printf("\t\tpath ------\n");
		for	( i = 1 ; i < db->pcount ; i ++ ) {
			DumpPath(db->path[i]);
		}
	}
LEAVE_FUNC;
}

static	void
DumpRecord(
	RecordStruct	*db)
{
ENTER_FUNC;
	DumpDB(db->opt.db);
	nTab = 2;
	printf("\t\t%s\t",db->name);
	DumpItems(2,db->value);
	printf(";\n");
LEAVE_FUNC;
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
		printf("\t\tmodule    \"%s\";\n",(char *)bind->module);
		printf("\t\tclass     \"%s\";\n",(char *)handler->klass);
		printf("\t\tselialize \"%s\";\n",(char *)handler->serialize);
		printf("\t\tlocale    \"%s\";\n",ConvCodeset(handler->conv));
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
_DumpBind(
	char	*name,
	WindowBind	*bind,
	void	*dummy)
{
	MessageHandler		*handler;

	handler = bind->handler;

	printf("\tbind\t\"%s\"",name);
	printf("\t\"%s\"",handler->name);
	printf("\t\"%s\"",(char *)bind->module);
	printf(";\n");
}

static	void
DumpLD(
	LD_Struct	*ld)
{
	int		i;

ENTER_FUNC;
	printf("name      = [%s]\n",ld->name);
	printf("\tgroup     = [%s]\n",ld->group);
	printf("\tarraysize = %d\n",(int)ld->arraysize);
	printf("\ttextsize  = %d\n",(int)ld->textsize);

	printf("ld->cBind = %d\n",(int)ld->cBind);

	g_hash_table_foreach(ld->bhash,(GHFunc)_DumpHandler,NULL);
	g_hash_table_foreach(ld->bhash,(GHFunc)_DumpBind,NULL);

	printf("\t%s\t",ld->sparec->name);
	DumpItems(1,ld->sparec->value);
	printf(";\n");
	for	( i = 0 ; i < ld->cWindow ; i ++ ) {
		if		(  ld->windows[i]  !=  NULL  ) {
			printf("\t%s\t",ld->windows[i]->name);
			if		(  ld->windows[i]  !=  NULL  ) {
				DumpItems(1,ld->windows[i]->value);
			} else {
				printf("{}");
			}
			printf(";\n");
		}
	}
	printf("\tcDB       = %d\n",(int)ld->cDB);
	for	( i = 1 ; i < ld->cDB ; i ++ ) {
		DumpRecord(ld->db[i]);
	}
LEAVE_FUNC;
}

static	void
DumpBD(
	BD_Struct	*bd)
{
	int		i;

	printf("name      = [%s]\n",bd->name);
	printf("\tarraysize = %d\n",(int)bd->arraysize);
	printf("\ttextsize  = %d\n",(int)bd->textsize);

	g_hash_table_foreach(bd->BatchTable,(GHFunc)_DumpHandler,NULL);

	printf("\tcDB       = %d\n",(int)bd->cDB);
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
	printf("\tarraysize = %d\n",(int)dbd->arraysize);
	printf("\ttextsize  = %d\n",(int)dbd->textsize);
	printf("\tcDB       = %d\n",(int)dbd->cDB);
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
	if		(  dbg->port  !=  NULL  ) {
		printf("\thost     = [%s]\n",IP_HOST(dbg->port));
		printf("\tport     = [%s]\n"  ,IP_PORT(dbg->port));
	}
	printf("\tDB name  = [%s]\n",dbg->dbname);
	printf("\tDB user  = [%s]\n",dbg->user);
	printf("\tDB pass  = [%s]\n",dbg->pass);
	printf("\tDB locale= [%s]\n",dbg->coding);
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

ENTER_FUNC;
	InitDirectory();
dbgmsg("*");
	SetUpDirectory(Directory,NULL,NULL,NULL,TRUE);
dbgmsg("*");

	printf("name     = [%s]\n",ThisEnv->name);
	printf("mlevel   = %d\n"  ,ThisEnv->mlevel);
	printf("cLD      = %d\n"  ,(int)ThisEnv->cLD);
	printf("cBD      = %d\n"  ,(int)ThisEnv->cBD);
	printf("cDBD     = %d\n"  ,(int)ThisEnv->cDBD);
#if	0
	printf("LINK ---------\n");
	DumpRecord(ThisEnv->linkrec);
#endif
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
		"print LD infomation(s)"						},
	{	"bd",		BOOLEAN,	TRUE,		(void*)&fBD,
		"print BD infomation(s)"						},
	{	"dbd",		BOOLEAN,	TRUE,		(void*)&fDBD,
		"print DBD infomation(s)"						},
	{	"dbg",		BOOLEAN,	TRUE,		(void*)&fDBG,
		"print DB group infomation(s)"					},

	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"directory file name"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"record directory"								},
	{	"ddir",		STRING,		TRUE,	(void*)&D_Dir,
		"defines directory"			 					},
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
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("checkdir",NULL);

	DumpDirectory();

	return	(0);
}
