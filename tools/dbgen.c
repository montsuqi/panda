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
#include	"DBparser.h"
#include	"dirs.h"
#include	"const.h"
#include	"enum.h"
#include	"directory.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	Bool	fCreate;
static	Bool	fInsert;

static	int		TextSize;
static	int		ArraySize;
static	char	*DB_Name;

static	int		level;
static	char	*rname[SIZE_RNAME];
static	int		alevel;
static	int		Dim[SIZE_RNAME];

static	void
PutTab(
	int		n)
{
	for	( ; n > 0 ; n -- ) {
		fputc('\t',stdout);
	}
}

static	void
PutDim(void)
{
	int		i;

	for	( i = alevel - 1 ; i >= 0 ; i -- ) {
		if		(  Dim[i]  ==  0  ) {
			printf("[]");
		} else {
			printf("[%d]",Dim[i]);
		}
	}
}

static	void
PutItemName(void)
{
	int		j;

	PutTab(1);
	if		(  level  >  1  ) {
		for	( j = 0 ; j < level - 1 ; j ++ ) {
			printf("%s_",rname[j]);
		}
	}
	printf("%s",rname[level-1]);
	printf("\t");
}


static	void
TableBody(
	ValueStruct	*val,
	size_t		arraysize,
	size_t		textsize)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;

	if		(  val  ==  NULL  )	return;

	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
		PutItemName();
		printf("integer");
		PutDim();
		break;
	  case	GL_TYPE_BOOL:
		PutItemName();
		printf("char");
		PutDim();
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
		PutItemName();
		printf("char(%d)",ValueStringLength(val));
		PutDim();
		break;
	  case	GL_TYPE_VARCHAR:
		PutItemName();
		printf("varchar(%d)",ValueStringLength(val));
		PutDim();
		break;
	  case	GL_TYPE_NUMBER:
		PutItemName();
		printf("numeric(%d,%d)",
			   ValueFixedLength(val),
			   ValueFixedSlen(val));
		PutDim();
		break;
	  case	GL_TYPE_TEXT:
		PutItemName();
		printf("text");
		PutDim();
		break;
	  case	GL_TYPE_ARRAY:
		tmp = ValueArrayItem(val,0);
		Dim[alevel] = ValueArraySize(val);
		alevel ++;
		TableBody(tmp,arraysize,textsize);
		alevel --;
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			if		(	(  !IS_VALUE_VIRTUAL(tmp)  )
					&&	(  !IS_VALUE_ALIAS(tmp)    ) ) {
				if		(  fComm  ) {
					printf(",\n");
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				TableBody(tmp,arraysize,textsize);
			}
		}
		level --;
		break;
	  case	GL_TYPE_DBCODE:
	  default:
		break;
	}
}

static	void
MakeCreate(
	RecordStruct	*rec)
{
	char	***item
	,		**pk;
	KeyStruct	*key;

	if		(  ( ValueAttribute(rec->value) & GL_ATTR_VIRTUAL )  ==  0  ) {
		printf("create\ttable\t%s\t(\n",rec->name);
		level = 0;
		alevel = 0;
		TableBody(rec->value,ArraySize,TextSize);
		if		(  ( key = rec->opt.db->pkey )  !=  NULL  ) {
			item = key->item;
			printf(",\n\tprimary\tkey(\n");
			while	(  *item  !=  NULL  ) {
				pk = *item;
				printf("\t\t");
				while	(  *pk  !=  NULL  ) {
					printf("%s",*pk);
					pk ++;
					if		(  *pk  !=  NULL  ) {
						printf("_");
					}
				}
				item ++;
				if		(  *item  !=  NULL  ) {
					printf(",");
				}
				printf("\n");
			}
			printf("\t)\n);\n");
		} else {
			printf("\n);\n");
		}
	}
}

static	void
PutName(void)
{
	int		j;

	if		(  level  >  1  ) {
		for	( j = 0 ; j < level - 1 ; j ++ ) {
			printf("%s_",rname[j]);
		}
	}
	printf("%s",rname[level-1]);
}


static	void
PutItemNames(
	ValueStruct	*val)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;

	if		(  val  ==  NULL  )	return;

	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
		PutName();
		break;
	  case	GL_TYPE_BOOL:
		PutName();
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
		PutName();
		break;
	  case	GL_TYPE_VARCHAR:
		PutName();
		break;
	  case	GL_TYPE_NUMBER:
		PutName();
		break;
	  case	GL_TYPE_TEXT:
		PutName();
		break;
	  case	GL_TYPE_ARRAY:
		tmp = ValueArrayItem(val,0);
		Dim[alevel] = ValueArraySize(val);
		alevel ++;
		PutItemNames(tmp);
		alevel --;
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			if		(  !IS_VALUE_VIRTUAL(tmp)  )	{
				if		(  fComm  ) {
					printf(",");
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				PutItemNames(tmp);
			}
		}
		level --;
		break;
	  default:
		break;
	}
}

static	ARG_TABLE	option[] = {
	{	"create",	BOOLEAN,	TRUE,	(void*)&fCreate,
		"create tableを作る"							},

	{	"insert",	BOOLEAN,	TRUE,	(void*)&fInsert,
		"insert用スクリプトを作る"						},

	{	"textsize",	INTEGER,	TRUE,	(void*)&TextSize,
		"textの最大長"									},
	{	"arraysize",INTEGER,	TRUE,	(void*)&ArraySize,
		"可変要素配列の最大繰り返し数"					},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"レコードのあるディレクトリ"					},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	fCreate = FALSE;
	fInsert = FALSE;
	ArraySize = SIZE_DEFAULT_ARRAY_SIZE;
	TextSize = SIZE_DEFAULT_TEXT_SIZE;
	RecordDir = ".";
	DB_Name = "";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	RecordStruct	*rec;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("dbgen",NULL);

	if		(  fl  !=  NULL  ) {
		DD_ParserInit();
		DB_ParserInit();
		if		(  fCreate  ) {
			if		(  ( rec = DB_Parser(fl->name) )  !=  NULL  ) {
				MakeCreate(rec);
			}
		}
	}

	return	(0);
}
