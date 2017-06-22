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
#include	"libmondai.h"
#include	"RecParser.h"
#include	"DBparser.h"
#include	"dirs.h"
#include	"const.h"
#include	"enum.h"
#include	"directory.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	Bool	fCreate;
static	Bool	fDrop;
static	Bool	fInsert;

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

ENTER_FUNC;
	for	( i = alevel - 1 ; i >= 0 ; i -- ) {
		if		(  Dim[i]  ==  0  ) {
			printf("[]");
		} else {
			printf("[%d]",Dim[i]);
		}
	}
LEAVE_FUNC;
}

static	void
PutItemName(void)
{
	int		j;

ENTER_FUNC;
	PutTab(1);
	if		(  level  >  1  ) {
		for	( j = 0 ; j < level - 1 ; j ++ ) {
			printf("%s_",rname[j]);
		}
	}
	printf("%s",rname[level-1]);
	printf("\t");
LEAVE_FUNC;
}

static	void
PutItemNameEx(void)
{
	int		j;

ENTER_FUNC;
	PutTab(1);
	if		(  level  >  1  ) {
		for	( j = 0 ; j < level - 1 ; j ++ ) {
			printf("%s_",rname[j]);
		}
	}
	printf("%s",rname[level-1]);
LEAVE_FUNC;
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

ENTER_FUNC;
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
		printf("char(%d)",(int)ValueStringLength(val));
		PutDim();
		break;
	  case	GL_TYPE_VARCHAR:
		PutItemName();
		printf("varchar(%d)",(int)ValueStringLength(val));
		PutDim();
		break;
	  case	GL_TYPE_NUMBER:
		PutItemName();
		printf("numeric(%d,%d)",
			   (int)ValueFixedLength(val),
			   (int)ValueFixedSlen(val));
		PutDim();
		break;
	  case	GL_TYPE_TIMESTAMP:
		PutItemName();
		printf("timestamp");
		PutDim();
		break;
	  case	GL_TYPE_DATE:
		PutItemName();
		printf("date");
		PutDim();
		break;
	  case	GL_TYPE_TIME:
		PutItemName();
		printf("time");
		PutDim();
		break;
	  case	GL_TYPE_TEXT:
		PutItemName();
		printf("text");
		PutDim();
		break;
	  case	GL_TYPE_BINARY:
		PutItemName();
		printf("bytea");
		PutDim();
		break;
	  case	GL_TYPE_OBJECT:
		PutItemName();
		printf("oid");
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
LEAVE_FUNC;
}

static	void
MakeCreate(
	RecordStruct	*rec)
{
	char	***item
	,		**pk;
	KeyStruct	*key;

ENTER_FUNC;
	if		(  ( ValueAttribute(rec->value) & GL_ATTR_VIRTUAL )  ==  0  ) {
		printf("create\ttable\t%s\t(\n",rec->name);
		level = 0;
		alevel = 0;
		TableBody(rec->value,ArraySize,TextSize);
		if		(	(  rec->type  ==  RECORD_DB  )
				&&	(  ( key = RecordDB(rec)->pkey )  !=  NULL  ) ) {
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
LEAVE_FUNC;
}

static	void
MakeDrop(
	RecordStruct	*rec)
{
ENTER_FUNC;
	if		(  ( ValueAttribute(rec->value) & GL_ATTR_VIRTUAL )  ==  0  ) {
		printf("drop\ttable\t%s;\n",rec->name);
	}
LEAVE_FUNC;
}

static void
TableInsert(ValueStruct *val, size_t arraysize, size_t textsize, int type)
{
    int i;
    ValueStruct *tmp;
    Bool    fComm;

    if (val == NULL) return;

    switch (ValueType(val)) {
      case  GL_TYPE_INT:
        if (type)
            PutItemNameEx();
        else
            printf("\t0");
        break;
      case  GL_TYPE_BOOL:
        if( type ) PutItemNameEx();
        else printf("\ttrue");
        break;
      case  GL_TYPE_BYTE:
      case  GL_TYPE_CHAR:
        if (type)
            PutItemNameEx();
        else
            printf("\t''");
        break;
      case  GL_TYPE_VARCHAR:
        if (type)
            PutItemNameEx();
        else
            printf("\t''");
        break;
      case  GL_TYPE_NUMBER:
        if (type)
            PutItemNameEx();
        else
            printf("\t0");
        break;
      case  GL_TYPE_TEXT:
        if (type)
            PutItemNameEx();
        else
            printf("\t''");
        break;
      case  GL_TYPE_RECORD:
        level ++;
        fComm = FALSE;
        for ( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
            tmp = ValueRecordItem(val,i);
            if      (   (  !IS_VALUE_VIRTUAL(tmp)  )
                        &&  (  !IS_VALUE_ALIAS(tmp)    ) ) {
                if      (  fComm  ) {
                    printf(",\n");
                }
                fComm = TRUE;
                rname[level-1] = ValueRecordName(val,i);
                TableInsert(tmp,arraysize,textsize, type);
            }
        }
        level --;
        break;
      default:
        if (type == 0)
            fprintf(stderr, "対応していない型です。: %s\n", rname[level-1]);
        break;
    }
}

static void
MakeInsert(
	RecordStruct	*rec)
{
	if		(  ( ValueAttribute(rec->value) & GL_ATTR_VIRTUAL )  ==  0  ) {
		printf("insert into %s (\n",rec->name);
    }

    level = 0;
    alevel = 0;
    TableInsert(rec->value,ArraySize,TextSize, 1);

    printf(")\nvalues (\n");

    level = 0;
    alevel = 0;
    TableInsert(rec->value,ArraySize,TextSize, 0);

    printf(");\n");
}

#if 0
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
	  case	GL_TYPE_BOOL:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_NUMBER:
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_TIMESTAMP:
	  case	GL_TYPE_DATE:
	  case	GL_TYPE_TIME:
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
#endif

static	ARG_TABLE	option[] = {
	{	"create",	BOOLEAN,	TRUE,	(void*)&fCreate,
		"generate 'create table'"						},
	{	"drop",		BOOLEAN,	TRUE,	(void*)&fDrop,
		"generate 'drop table'"							},

	{	"insert",	BOOLEAN,	TRUE,	(void*)&fInsert,
		"generate insert script"						},

	{	"textsize",	INTEGER,	TRUE,	(void*)&TextSize,
		"'text' length(for COBOL)"						},
	{	"arraysize",INTEGER,	TRUE,	(void*)&ArraySize,
		"'array' size (for COBOL)"						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"record directory"								},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	fCreate = FALSE;
	fInsert = FALSE;
	fDrop = FALSE;
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
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("dbgen",NULL);

	if		(  fl  !=  NULL  ) {
		RecParserInit();
		DB_ParserInit();
		if		( fDrop ) {
			if		(  ( rec = DB_Parser(fl->name,NULL,FALSE) )  !=  NULL  ) {
				MakeDrop(rec);
			}
		}
		if		(  fCreate  ) {
			if		(  ( rec = DB_Parser(fl->name,NULL,FALSE) )  !=  NULL  ) {
				MakeCreate(rec);
			}
		}
		if		( fInsert ) {
			if		(  ( rec = DB_Parser(fl->name,NULL,FALSE) )  !=  NULL  ) {
				MakeInsert(rec);
			}
		}
	}

	return	(0);
}
