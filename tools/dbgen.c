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
#include	"dirs.h"
#include	"misc.h"
#include	"const.h"
#include	"enum.h"
#include	"directory.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	Bool	fCreate;
static	int		TextSize;
static	int		ArraySize;

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

	switch	(val->type) {
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
		printf("char(%d)",val->body.CharData.len);
		PutDim();
		break;
	  case	GL_TYPE_VARCHAR:
		PutItemName();
		printf("varchar(%d)",val->body.CharData.len);
		PutDim();
		break;
	  case	GL_TYPE_NUMBER:
		PutItemName();
		printf("numeric(%d,%d)",
			   val->body.FixedData.flen,
			   val->body.FixedData.slen);
		PutDim();
		break;
	  case	GL_TYPE_TEXT:
		PutItemName();
		printf("text");
		PutDim();
		break;
	  case	GL_TYPE_ARRAY:
		tmp = val->body.ArrayData.item[0];
		Dim[alevel] = val->body.ArrayData.count;
		alevel ++;
		TableBody(tmp,arraysize,textsize);
		alevel --;
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < val->body.RecordData.count ; i ++ ) {
			tmp = val->body.RecordData.item[i];
			if		(  ( tmp->attr & GL_ATTR_VIRTUAL )  !=  GL_ATTR_VIRTUAL  ) {
				if		(  fComm  ) {
					printf(",\n");
				}
				fComm = TRUE;
				rname[level-1] = val->body.RecordData.names[i];
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

	printf("create\ttable\t%s\t(\n",rec->name);
	level = 0;
	alevel = 0;
	TableBody(rec->rec,ArraySize,TextSize);
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

static	ARG_TABLE	option[] = {
	{	"create",	BOOLEAN,	TRUE,	(void*)&fCreate,
		"create tableを作る"							},
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
	ArraySize = SIZE_DEFAULT_ARRAY_SIZE;
	TextSize = SIZE_DEFAULT_TEXT_SIZE;
	RecordDir = ".";
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
	InitMessage();

	if		(  fl  !=  NULL  ) {
		if		(  fCreate  ) {
			DD_ParserInit();
			if		(  ( rec = DD_ParserDataDefines(fl->name) )  !=  NULL  ) {
				MakeCreate(rec);
			}
		} else {
		}
	}

	return	(0);
}
