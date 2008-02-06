/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2007-2008 Ogochan.
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
#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"DBparser.h"
#include	"dbgroup.h"
#include	"dirs.h"
#include	"directory.h"
#include	"gettext.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*Directory;
static	DBD_Struct	*ThisDBD;

static	Bool	fCreate;
static	Bool	fInsert;

static	int		TextSize;
static	int		ArraySize;
static	char	*InLang;
static	char	*InCode;
static	char	*InEncode;
static	char	*DBD_Name;
static	char	*DBG_Name;

static	int		level;
static	char	*rname[SIZE_RNAME];
static	int		alevel;
static	int		Dim[SIZE_RNAME];

static	void
PutTab(
	LargeByteString	*lbs,
	int		n)
{
	for	( ; n > 0 ; n -- ) {
		LBS_EmitString(lbs,"\t");
	}
}

static	void
PutDim(
	LargeByteString	*lbs)
{
	char	buff[SIZE_LONGNAME+1];
	int		i;

	for	( i = alevel - 1 ; i >= 0 ; i -- ) {
		if		(  Dim[i]  ==  0  ) {
			LBS_EmitString(lbs,"[]");
		} else {
			sprintf(buff,"[%d]",Dim[i]);
			LBS_EmitString(lbs,buff);
		}
	}
}

static	void
PutItemName(
	LargeByteString	*lbs)
{
	int		j;

	PutTab(lbs,1);
	if		(  level  >  1  ) {
		for	( j = 0 ; j < level - 1 ; j ++ ) {
			LBS_EmitString(lbs,rname[j]);
			LBS_EmitString(lbs,"_");
		}
	}
	LBS_EmitString(lbs,rname[level-1]);
	LBS_EmitString(lbs,"\t");
}

static	void
TableBody(
	LargeByteString	*lbs,
	ValueStruct	*val,
	size_t		arraysize,
	size_t		textsize)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;
	char	buff[SIZE_LONGNAME+1];

	if		(  val  ==  NULL  )	return;

	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
		PutItemName(lbs);
		LBS_EmitString(lbs,"integer");
		PutDim(lbs);
		break;
	  case	GL_TYPE_BOOL:
		PutItemName(lbs);
		LBS_EmitString(lbs,"char");
		PutDim(lbs);
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
		PutItemName(lbs);
		sprintf(buff,"char(%d)",(int)ValueStringLength(val));
		LBS_EmitString(lbs,buff);
		PutDim(lbs);
		break;
	  case	GL_TYPE_VARCHAR:
		PutItemName(lbs);
		sprintf(buff,"varchar(%d)",(int)ValueStringLength(val));
		LBS_EmitString(lbs,buff);
		PutDim(lbs);
		break;
	  case	GL_TYPE_NUMBER:
		PutItemName(lbs);
		sprintf(buff,"numeric(%d,%d)",
				(int)ValueFixedLength(val),
				(int)ValueFixedSlen(val));
		LBS_EmitString(lbs,buff);
		PutDim(lbs);
		break;
	  case	GL_TYPE_TEXT:
		PutItemName(lbs);
		LBS_EmitString(lbs,"text");
		PutDim(lbs);
		break;
	  case	GL_TYPE_BINARY:
		PutItemName(lbs);
		LBS_EmitString(lbs,"bytea");
		PutDim(lbs);
		break;
	  case	GL_TYPE_OBJECT:
		PutItemName(lbs);
		LBS_EmitString(lbs,"oid");
		PutDim(lbs);
		break;
	  case	GL_TYPE_TIMESTAMP:
		PutItemName(lbs);
		LBS_EmitString(lbs,"timestamp");
		PutDim(lbs);
		break;
	  case	GL_TYPE_DATE:
		PutItemName(lbs);
		LBS_EmitString(lbs,"date");
		PutDim(lbs);
		break;
	  case	GL_TYPE_TIME:
		PutItemName(lbs);
		LBS_EmitString(lbs,"time");
		PutDim(lbs);
		break;
	  case	GL_TYPE_ARRAY:
		tmp = ValueArrayItem(val,0);
		Dim[alevel] = ValueArraySize(val);
		alevel ++;
		TableBody(lbs,tmp,arraysize,textsize);
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
					LBS_EmitString(lbs,",\n");
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				TableBody(lbs,tmp,arraysize,textsize);
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
	DBG_Struct		*dbg,
	RecordStruct	*rec)
{
	char	***item
	,		**pk;
	KeyStruct	*key;
	LargeByteString	*lbs;

ENTER_FUNC;
	if		(  ( ValueAttribute(rec->value) & GL_ATTR_VIRTUAL )  ==  0  ) {
		lbs = NewLBS();
		LBS_EmitString(lbs,"create table ");
		LBS_EmitString(lbs,rec->name);
		LBS_EmitString(lbs,"(\n");
		level = 0;
		alevel = 0;
		TableBody(lbs,rec->value,ArraySize,TextSize);
		if		(	(  rec->type  ==  RECORD_DB  )
				&&	(  ( key = RecordDB(rec)->pkey )  !=  NULL  ) ) {
			item = key->item;
			LBS_EmitString(lbs,",\n\tprimary key(\n");
			while	(  *item  !=  NULL  ) {
				pk = *item;
				LBS_EmitString(lbs,"\t\t");
				while	(  *pk  !=  NULL  ) {
					LBS_EmitString(lbs,*pk);
					pk ++;
					if		(  *pk  !=  NULL  ) {
						LBS_EmitString(lbs,"_");
					}
				}
				item ++;
				if		(  *item  !=  NULL  ) {
					LBS_EmitString(lbs,",");
				}
				LBS_EmitString(lbs,"\n");
			}
			LBS_EmitString(lbs,"\t)\n);\n");
		} else {
			LBS_EmitString(lbs,"\n);\n");
		}
		LBS_EmitEnd(lbs);
#ifdef	DEBUG
		printf("[%s]",(char *)LBS_Body(lbs));
#endif
		if		(  ExecDBOP(dbg,(char *)LBS_Body(lbs))  !=  MCP_OK  ) {
			fprintf(stderr,N_("create table error on %s\n"),rec->name);
		}
		FreeLBS(lbs);
	}
LEAVE_FUNC;
}

static	void
_MakeCreate(
	char	*name,
	RecordStruct	*rec,
	DBG_Struct	*dbg)
{
	MakeCreate(dbg,rec);
}

static	void
CreateTables(
	char	*gname,
	char	*tname)
{
	DBG_Struct	*dbg;
	RecordStruct	*rec;

ENTER_FUNC;
	if		( ( dbg = GetDBG(gname) )  !=  NULL  ) {
		OpenDB(dbg);
		if		(	(  tname  !=  NULL  )
				&&	(  ( rec = GetTableDBG(gname,tname) )  !=  NULL  ) ) {
			MakeCreate(dbg,rec);
		} else
		if		(  tname  ==  NULL  ) {
			if		(  dbg->dbt  !=  NULL  ) {
				g_hash_table_foreach(dbg->dbt,(GHFunc)_MakeCreate,dbg);
			}
		} else {
			fprintf(stderr,N_("table %s not found.\n"),tname);
		}
		CloseDB(dbg);
	} else {
		fprintf(stderr,N_("db group %s not found.\n"),gname);
	}
LEAVE_FUNC;
}

static	void
Insert(
	char	*gname,
	char	*rname,
	char	*pname)
{
	ConvFuncs	*infunc;
	CONVOPT		*inopt;
	int			type;
	struct	stat	sb;
	ssize_t		left;
	size_t		size;
	byte		*p;
	RecordStruct	*rec;
	DBG_Struct	*dbg;
	ValueStruct	*value;
	DBCOMM_CTRL	ctrl;

ENTER_FUNC;
	type = 0;
	if		(  !strlicmp(InLang,"xml")  ) {
		if		(  !strlicmp(InLang,"xml2")  ) {
			type = XML_TYPE2;
		} else {
			type = XML_TYPE1;
		}
		InLang = "XML";
	}
	if		(  ( infunc = GetConvFunc(InLang) )  ==  NULL  ) {
		fprintf(stderr,N_("can not found %s convert rule\n"),InLang);
		exit(1);
	} else {
		inopt = NewConvOpt();
		ConvSetCodeset(inopt,InCode);
		ConvSetSize(inopt,TextSize,ArraySize);
		if		(  !stricmp(InEncode,"url")  ) {
			ConvSetEncoding(inopt,STRING_ENCODING_URL);
		} else
		if		(  !stricmp(InEncode,"cgi")  ) {
			ConvSetEncoding(inopt,STRING_ENCODING_BASE64);
		} else {
			ConvSetEncoding(inopt,STRING_ENCODING_NULL);
		}
		if		(  !stricmp(InLang,"xml")  ) {
			ConvSetXmlType(inopt,type);
			ConvSetIndent(inopt,TRUE);
			ConvSetType(inopt,TRUE);
		}
	}
	if		( ( dbg = GetDBG(gname) )  !=  NULL  ) {
		OpenDB(dbg);
		fstat(fileno(stdin),&sb);
		if		(  ( p = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fileno(stdin),0) )
				   !=  NULL  ) {
			left = sb.st_size;
			while	(  left  >  0  ) {
				if		(  ( rec = MakeCTRLbyName(&value,&ctrl,rname,pname,"DBINSERT") )  !=  NULL  ) {
					size = infunc->UnPackValue(inopt,p,value);
#ifdef	TRACE
					DumpValueStruct(value);
#endif
					ExecDB_Process(&ctrl,rec,value);
					left -= size + strlen(infunc->bsep);
					p += size + strlen(infunc->bsep);
				} else
					break;
			}
			munmap(p,sb.st_size);
		}
		CloseDB(dbg);
	} else {
		fprintf(stderr,N_("db group %s not found.\n"),gname);
	}
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	"dbgroup",	STRING,		TRUE,	(void*)&DBG_Name,
		N_("dbgroup name")		 						},
	{	"dbd",		STRING,		TRUE,	(void*)&DBD_Name,
		N_("DBD name")			 						},

	{	"create",	BOOLEAN,	TRUE,	(void*)&fCreate,
		N_("execute create table")						},
	{	"insert",	BOOLEAN,	TRUE,	(void*)&fInsert,
		N_("execute insert")							},

	{	"textsize",	INTEGER,	TRUE,	(void*)&TextSize,
		N_("max of text length")						},
	{	"arraysize",INTEGER,	TRUE,	(void*)&ArraySize,
		N_("max of array")								},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		N_("record folder")								},
	{	"inlang",	STRING,		TRUE,	(void*)&InLang	,
		N_("input data language type")					},
	{	"incode",	STRING,		TRUE,	(void*)&InCode	,
		N_("input data character encoding")				},
	{	"inencode",	STRING,		TRUE,	(void*)&InEncode,
		N_("input data string encoding type") 			},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Directory = "./directory";

	fCreate = FALSE;
	fInsert = FALSE;
	ArraySize = SIZE_DEFAULT_ARRAY_SIZE;
	TextSize = SIZE_DEFAULT_TEXT_SIZE;
	RecordDir = NULL;

	InLang = "CSVE";
	InCode = "utf-8";
	InEncode = "null";

	DBG_Name = NULL;
	DBD_Name = NULL;
}

static	void
InitSystem(
	char	*name)
{
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,name,TRUE);
	if		(  ( ThisDBD = GetDBD(name) )  ==  NULL  ) {
		fprintf(stderr,N_("DBD \"%s\" not found.\n"),name);
		exit(1);
	}

	ThisDB = ThisDBD->db;
	DB_Table = ThisDBD->DBD_Table;
	CurrentProcess = NULL;

	if		(  ThisDBD->cDB  >  0  ) {
		InitDB_Process(NULL);
	}
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*table
		,		*path;

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("pandadb",NULL);
	InitSystem(DBD_Name);
	if		(  fl  !=  NULL  ) {
		if		(  fCreate  ) {
			if		(  fl  !=  NULL  ) {
				table = fl->name;
				CreateTables(DBG_Name,table);
			}
		} else
		if		(  fInsert  ) {
			if		(  fl  !=  NULL  ) {
				table = fl->name;
				if		(  ( fl = fl->next )  !=  NULL  ) {
					path = fl->name;
				} else {
					path = NULL;
				}
				Insert(DBG_Name,table,path);
			}
		}
	}

	return	(0);
}
