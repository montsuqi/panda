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
#include	"value.h"
#include	"misc.h"
#include	"const.h"
#include	"enum.h"
#include	"wfc.h"
#include	"directory.h"
#include	"driver.h"
#include	"dirs.h"
#include	"option.h"
#include	"debug.h"

static	Bool	fNoConv;
static	Bool	fFiller;
static	Bool	fSPA;
static	Bool	fLinkage;
static	Bool	fScreen;
static	Bool	fWindowPrefix;
static	Bool	fLDW;
static	Bool	fLDR;
static	Bool	fDB;
static	Bool	fDBREC;
static	Bool	fDBCOMM;
static	Bool	fDBPATH;
static	Bool	fMCP;
static	int		TextSize;
static	int		ArraySize;
static	char	*Prefix;
static	char	*RecName;
static	char	*LD_Name;
static	char	*BD_Name;
static	char	*Directory;

static	int		level;

static	void	COBOL(ValueStruct *val, size_t arraysize, size_t textsize);

static	int		Col;

static	void
PutLevel(
	int		level)
{
	int		n;

	n = 8;
	if		(  level  >  1  ) {
		n += 4 + ( level - 2 ) * 2;
	}
	for	( ; n > 0 ; n -- ) {
		fputc(' ',stdout);
	}
	printf("%02d  ",level);
	Col = 0;
}

static	void
PutTab(
	int		unit)
{
	int		n;

	n = unit - ( Col % unit );
	Col += n;
	for	( ; n > 0 ; n -- ) {
		fputc(' ',stdout);
	}
}

static	void
PutChar(
	int		c)
{
	fputc(c,stdout);
	Col ++;
}

static	void
PutString(
	char	*str)
{
	int		c;

	while	(  *str  !=  0  ) {
		if		(  fNoConv  ) {
			PutChar(*str);
		} else {
			c = toupper(*str);
			if		(  c  ==  '_'  ) {
				c = '-';
			}
			PutChar(c);
		}
		str ++;
	}
}

static	void
PutName(
	char	*name)
{
	char	buff[SIZE_NAME+1];

	if		(	(  name           ==  NULL  )
			||	(  !stricmp(name,"filler")  ) ) {
		strcpy(buff,"filler");
	} else {
		sprintf(buff,"%s%s",Prefix,name);
	}
	PutString(buff);
}

static	void
COBOL(
	ValueStruct	*val,
	size_t		arraysize,
	size_t		textsize)
{
	int		i
	,		n;
	ValueStruct	*tmp;
	char	buff[SIZE_BUFF+1];

	if		(  val  ==  NULL  )	return;

	switch	(val->type) {
	  case	GL_TYPE_INT:
		PutString("PIC S9(9)   BINARY");
		break;
	  case	GL_TYPE_BOOL:
		PutString("PIC X");
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		sprintf(buff,"PIC X(%d)",val->body.CharData.len);
		PutString(buff);
		break;
	  case	GL_TYPE_NUMBER:
		if		(  val->body.FixedData.slen  ==  0  ) {
			sprintf(buff,"PIC S9(%d)",val->body.FixedData.flen);
		} else {
			sprintf(buff,"PIC S9(%d)V9(%d)",
					(val->body.FixedData.flen - val->body.FixedData.slen),
					val->body.FixedData.slen);
		}
		PutString(buff);
		break;
	  case	GL_TYPE_TEXT:
		sprintf(buff,"PIC X(%d)",textsize);
		PutString(buff);
		break;
	  case	GL_TYPE_ARRAY:
		tmp = val->body.ArrayData.item[0];
		n = val->body.ArrayData.count;
		if		(  n  ==  0  ) {
			n = arraysize;
		}
		if		(  tmp->type  ==  GL_TYPE_RECORD  ) {
			sprintf(buff,"OCCURS  %d TIMES",n);
			PutTab(8);
			PutString(buff);
			COBOL(tmp,arraysize,textsize);
		} else {
			COBOL(tmp,arraysize,textsize);
			sprintf(buff,"OCCURS  %d TIMES",n);
			PutTab(8);
			PutString(buff);
		}
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		for	( i = 0 ; i < val->body.RecordData.count ; i ++ ) {
			printf(".\n");
			PutLevel(level);
			tmp = val->body.RecordData.item[i];
			PutName(val->body.RecordData.names[i]);
			if		(  tmp->type  !=  GL_TYPE_RECORD  ) {
				PutTab(4);
			}
			COBOL(tmp,arraysize,textsize);
		}
		level --;
		break;
	  default:
		break;
	}
}

static	void
SIZE(
	ValueStruct	*val,
	size_t		arraysize,
	size_t		textsize)
{
	char	buff[SIZE_BUFF+1];

	if		(  val  ==  NULL  )	return;
	level ++;
	PutLevel(level);
	PutName("filler");
	PutTab(8);
	sprintf(buff,"PIC X(%d)",SizeValue(val,arraysize,textsize));
	PutString(buff);
	level --;
}

static	void
MakeFromRecord(
	char	*name)
{
	RecordStruct	*rec;

	if		(  fScreen  ) {
		level = 3;
	} else {
		level = 1;
	}
	DD_ParserInit();
	if		(  ( rec = DD_ParserDataDefines(name) )  !=  NULL  ) {
		PutLevel(level);
		if		(  *RecName  ==  0  ) {
			PutString(rec->name);
		} else {
			PutString(RecName);
		}
		if		(  fFiller  ) {
			printf(".\n");
			SIZE(rec->rec,ArraySize,TextSize);
		} else {
			COBOL(rec->rec,ArraySize,TextSize);
		}
		printf(".\n");
	}
}

static	void
MakeLD(void)
{
	LD_Struct	*ld;
	int		i;
	char	buff[SIZE_BUFF+1];
	char	*_prefix;
	size_t	size
	,		num
	,		spasize
	,		mcpsize;
	int		base;

dbgmsg(">MakeLD");
	InitDirectory(TRUE);
	SetUpDirectory(Directory,LD_Name,"","");
	if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
		Error("LD not found.\n");
	}

	PutLevel(1);
	if		(  *RecName  ==  0  ) {
		sprintf(buff,"%s",ld->name);
	} else {
		sprintf(buff,"%s",RecName);
	}
	PutString(buff);
	printf(".\n");

	base = 2;
	if		(	(  fLDR     )
			||	(  fLDW     ) ) {
		size =	SizeValue(ThisEnv->mcprec,ld->arraysize,ld->textsize)
			+	ThisEnv->linksize
			+	SizeValue(ld->sparec,ld->arraysize,ld->textsize);
		for	( i = 0 ; i < ld->cWindow ; i ++ ) {
			size += SizeValue(ld->window[i]->value,ld->arraysize,ld->textsize);
		}
		num = ( size / SIZE_BLOCK ) + 1;

		PutLevel(base);
		PutName("dataarea");
		printf(".\n");

		PutLevel(base+1);
		PutName("data");
		PutTab(12);
		printf("OCCURS  %d.\n",num);

		PutLevel(base+2);
		PutName("filler");
		PutTab(12);
		printf("PIC X(%d).\n",SIZE_BLOCK);

		PutLevel(base);
		PutName("inputarea-red");
		PutTab(4);
		printf("REDEFINES   ");
		PutName("dataarea.\n");
		base ++;
	} else {
		num = 0;
	}
	if		(  fMCP  ) {
		if		(  ( mcpsize = SizeValue(ThisEnv->mcprec,ld->arraysize,ld->textsize) )
				   >  0  ) {
			PutLevel(base);
			PutName("mcpdata");
			level = base;
			if		(	(  fFiller  )
					||	(  fLDW     ) ) {
				printf(".\n");
				SIZE(ThisEnv->mcprec,ld->arraysize,ld->textsize);
			} else {
				_prefix = Prefix;
				Prefix = "ldr-mcp-";
				COBOL(ThisEnv->mcprec,ld->arraysize,ld->textsize);
				Prefix = _prefix;
			}
			printf(".\n");
		}
	}
	if		(  fSPA  ) {
		if		(  ( spasize = SizeValue(ld->sparec,ld->arraysize,ld->textsize) )
				   >  0  ) {
			PutLevel(base);
			PutName("spadata");
			if		(  ld->sparec  ==  NULL  ) {
				printf(".\n");
				PutLevel(base+1);
				PutName("filler");
				printf("      PIC X(%d).\n",spasize);
			} else {
				level = base;
				if		(	(  fFiller  )
						||	(  fLDW     ) ) {
					printf(".\n");
					SIZE(ld->sparec,ld->arraysize,ld->textsize);
				} else {
					_prefix = Prefix;
					Prefix = "spa-";
					COBOL(ld->sparec,ld->arraysize,ld->textsize);
					Prefix = _prefix;
				}
				printf(".\n");
			}
		}
	}
	if		(  fLinkage  ) {
		if		(  SizeValue(ThisEnv->linkrec,ld->arraysize,ld->textsize)  >  0  ) {
			PutLevel(base);
			PutName("linkdata");
			level = base;
			if		(	(  fFiller  )
					||	(  fLDR     )
					||	(  fLDW     ) ) {
				printf(".\n");
				PutLevel(level+1);
				PutName("filler");
				printf("      PIC X(%d)",ThisEnv->linksize);
			} else {
				_prefix = Prefix;
				Prefix = "lnk-";
				COBOL(ThisEnv->linkrec,ld->arraysize,ld->textsize);
				Prefix = _prefix;
			}
			printf(".\n");
		}
	}
	if		(  fScreen  ) {
		PutLevel(base);
		sprintf(buff,"screendata");
		PutName(buff);
		printf(".\n");
		_prefix = Prefix;
		for	( i = 0 ; i < ld->cWindow ; i ++ ) {
			if		(  SizeValue(ld->window[i]->value,ld->arraysize,ld->textsize)  >  0  ) {
				Prefix = _prefix;
				PutLevel(base+1);
				sprintf(buff,"%s",ld->window[i]->name);
				PutName(buff);
				if		(  fWindowPrefix  ) {
					sprintf(buff,"%s-",ld->window[i]->name);
					Prefix = StrDup(buff);
				}
				level = base+1;
				if		(	(  fFiller  )
						||	(  fLDR     )
						||	(  fLDW     ) ) {
					printf(".\n");
					SIZE(ld->window[i]->value,ld->arraysize,ld->textsize);
				} else {
					COBOL(ld->window[i]->value,ld->arraysize,ld->textsize);
				}
				printf(".\n");
				if		(  fWindowPrefix  ) {
					xfree(Prefix);
				}
			}
		}
	}
	if		(	(  fLDR     )
			||	(  fLDW     ) ) {
		PutLevel(1);
		PutName("blocks");
		PutTab(8);
		printf("PIC S9(9)   BINARY  VALUE   %d.\n",num);
	}
dbgmsg("<MakeLD");
}

static	void
MakeLinkage(void)
{
	LD_Struct	*ld;
	char	*_prefix;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,LD_Name,"","");
	if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
		Error("LD not found.\n");
	}

	_prefix = Prefix;
	Prefix = "";
	PutLevel(1);
	PutName("linkarea.\n");
	PutLevel(2);
	PutName("linkdata-redefine.\n");
	PutLevel(3);
	PutName("filler");
	printf("      PIC X(%d).\n",ThisEnv->linksize);

	PutLevel(2);
	PutName(ld->name);
	PutTab(4);
	printf("REDEFINES   ");
	PutName("linkdata-redefine");
	Prefix = _prefix;
	level = 3;
	COBOL(ThisEnv->linkrec,
		  ld->arraysize,
		  ld->textsize);
	printf(".\n");
}

static	void
MakeDB(void)
{
	size_t	msize
	,		size;
	int		i;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
	}

	PutLevel(1);
	PutName("dbarea");
	printf(".\n");
	msize = 0;
	for	( i = 1 ; i < cDB ; i ++ ) {
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		msize = ( msize > size ) ? msize : size;
	}

	PutLevel(2);
	PutName("dbdata");
	PutTab(12);
	printf("PIC X(%d).\n",msize);
}

static	void
MakeDBREC(
	char	*name)
{
	RecordStruct	*rec;
	size_t	msize
	,		size;
	int		i;
	char	*_prefix;
	char	*rname;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

dbgmsg(">MakeDBREC");
	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
	}
	msize = 64;
	for	( i = 1 ; i < cDB ; i ++ ) {
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		msize = ( msize > size ) ? msize : size;
	}
	if		(  ( rec = DD_ParserDataDefines(name) )  !=  NULL  ) {
		level = 1;
		PutLevel(level);
		if		(  *RecName  ==  0  ) {
			rname = rec->name;
		} else {
			rname = RecName;
		}
		_prefix = Prefix;
		Prefix = "";
		PutName(rname);
		Prefix = _prefix;
		COBOL(rec->rec,arraysize,textsize);
		printf(".\n");

		size = SizeValue(rec->rec,arraysize,textsize);
		if		(  msize  !=  size  ) {
			PutLevel(2);
			PutName("filler");
			PutTab(12);
			printf("PIC X(%d).\n",msize - size);
		}
	}
dbgmsg("<MakeDBREC");
}

static	void
MakeDBCOMM(void)
{
	size_t	msize
	,		size
	,		num;
	int		i;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
	}

	msize = 0;
	for	( i = 1 ; i < cDB ; i ++ ) {
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		msize = ( msize > size ) ? msize : size;
	}

	PutLevel(1);
	PutName("dbcomm");
	printf(".\n");

	num = ( ( msize + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;

	PutLevel(2);
	PutName("dbcomm-blocks");
	printf(".\n");

	PutLevel(3);
	PutName("dbcomm-block");
	PutTab(12);
	printf("OCCURS  %d.\n",num);

	PutLevel(4);
	PutName("filler");
	PutTab(12);
	printf("PIC X(%d).\n",SIZE_BLOCK);

	PutLevel(2);
	PutName("dbcomm-data");
	PutTab(4);
	printf("REDEFINES   ");
	PutName("dbcomm-blocks");
	printf(".\n");

	PutLevel(3);
	PutName("dbcomm-ctrl");
	printf(".\n");

	PutLevel(4);
	PutName("dbcomm-func");
	PutTab(12);
	printf("PIC X(16).\n");

	PutLevel(4);
	PutName("dbcomm-rc");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(4);
	PutName("dbcomm-path");
	printf(".\n");

	PutLevel(5);
	PutName("dbcomm-path-blocks");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(5);
	PutName("dbcomm-path-rname");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(5);
	PutName("dbcomm-path-pname");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(3);
	PutName("dbcomm-record");
	printf(".\n");

	PutLevel(4);
	PutName("filler");
	PutTab(12);
	printf("PIC X(%d).\n",msize);
}

static	void
MakeDBPATH(void)
{
	int		i
	,		j
	,		blocks;
	size_t	size;
	char	buff[SIZE_NAME*2+1];
	DB_Struct	*db;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

dbgmsg(">MakeDBPATH");
	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,NULL);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		} else {
			dbrec = ld->db;
			arraysize = ld->arraysize;
			textsize = ld->textsize;
			cDB = ld->cDB;
		}
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		} else {
			dbrec = bd->db;
			arraysize = bd->arraysize;
			textsize = bd->textsize;
			cDB = bd->cDB;
		}
	} else {
		Error("LD or BD not specified");
	}


	PutLevel(1);
	PutName("dbpath");
	printf(".\n");
	for	( i = 1 ; i < cDB ; i ++ ) {
		db = dbrec[i]->opt.db;
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		blocks = ( ( size + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;
		
		for	( j = 0 ; j < db->pcount ; j ++ ) {
			PutLevel(2);
			sprintf(buff,"path-%s-%s",dbrec[i]->name,db->path[j]->name);
			PutName(buff);
			printf(".\n");

			PutLevel(3);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",blocks);

			PutLevel(3);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",i);

			PutLevel(3);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",j);
		}
	}
dbgmsg("<MakeDBPATH");
}

static	void
MakeMCP(void)
{
	InitDirectory(TRUE);
	SetUpDirectory(Directory,"","","");

	Prefix = "";
	PutLevel(1);
	PutName("mcparea");
	Prefix = "mcp_";
	level = 1;
	COBOL(ThisEnv->mcprec,0,0);
	printf(".\n");
}

static	ARG_TABLE	option[] = {
	{	"ldw",	BOOLEAN,	TRUE,		(void*)&fLDW,
		"制御フィールド以外をFILLERにする"				},
	{	"ldr",	BOOLEAN,	TRUE,		(void*)&fLDR,
		"制御フィールドとSPA以外をFILLERにする"			},
	{	"spa",		BOOLEAN,	TRUE,	(void*)&fSPA,
		"SPA領域を出力する"								},
	{	"linkage",	BOOLEAN,	TRUE,	(void*)&fLinkage,
		"連絡領域を出力する"							},
	{	"screen",	BOOLEAN,	TRUE,	(void*)&fScreen,
		"画面レコード領域を出力する"					},
	{	"dbarea",	BOOLEAN,	TRUE,	(void*)&fDB,
		"MCPSUB用領域を出力する"						},
	{	"dbrec",	BOOLEAN,	TRUE,	(void*)&fDBREC,
		"MCBSUBの引数に使うレコード領域を出力する"	},
	{	"dbpath",	BOOLEAN,	TRUE,	(void*)&fDBPATH,
		"DBのパス名テーブルを出力する"					},
	{	"dbcomm",	BOOLEAN,	TRUE,	(void*)&fDBCOMM,
		"MCPSUBとDBサーバとの通信領域を出力する"		},
	{	"mcp",		BOOLEAN,	TRUE,	(void*)&fMCP,
		"MCPAREAを出力する"								},
	{	"textsize",	INTEGER,	TRUE,	(void*)&TextSize,
		"textの最大長"									},
	{	"arraysize",INTEGER,	TRUE,	(void*)&ArraySize,
		"可変要素配列の最大繰り返し数"					},
	{	"prefix",	STRING,		TRUE,	(void*)&Prefix,
		"項目名の前に付加する文字列"					},
	{	"wprefix",	BOOLEAN,	TRUE,	(void*)&fWindowPrefix,
		"画面レコードの項目の前にウィンドウ名を付加する"},
	{	"name",		STRING,		TRUE,	(void*)&RecName,
		"レコードの名前"								},
	{	"filler",	BOOLEAN,	TRUE,	(void*)&fFiller,
		"レコードの中身をFILLERにする"					},
	{	"noconv",	BOOLEAN,	TRUE,	(void*)&fNoConv,
		"項目名を大文字に加工しない"					},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"レコードのあるディレクトリ"					},
	{	"lddir",	STRING,		TRUE,	(void*)&LD_Dir,
		"LD定義格納ディレクトリ"	 					},
	{	"bddir",	STRING,		TRUE,	(void*)&BD_Dir,
		"BD定義格納ディレクトリ"	 					},
	{	"ld",		STRING,		TRUE,	(void*)&LD_Name,
		"LD名"						 					},
	{	"bd",		STRING,		TRUE,	(void*)&BD_Name,
		"BD名"						 					},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	fNoConv = FALSE;
	fFiller = FALSE;
	fSPA = FALSE;
	fLinkage = FALSE;
	fScreen = FALSE;
	fDB = FALSE;
	fDBREC = FALSE;
	fDBCOMM = FALSE;
	fDBPATH = FALSE;
	fMCP = FALSE;
	ArraySize = -1;
	TextSize = -1;
	Prefix = "";
	RecName = "";
	LD_Name = NULL;
	BD_Name = NULL;
	fLDW = FALSE;
	fLDR = FALSE;
	fWindowPrefix = FALSE;
	RecordDir = NULL;
	LD_Dir = NULL;
	BD_Dir = NULL;
	Directory = "./directory";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*name
	,			*ext;

	SetDefault();
	fl = GetOption(option,argc,argv);

	if		(  fl  !=  NULL  ) {
		name = fl->name;
		ext = GetExt(name);
		if		(  fDBREC  ) {
			MakeDBREC(name);
		} else
		if		(	(  !stricmp(ext,".rec")  )
				||	(  !stricmp(ext,".db")  ) ) {
			MakeFromRecord(name);
		}
	} else {
		if		(  fScreen  ) {
			if		(  LD_Name  ==  NULL  ) {
				PutLevel(1);
				PutString("scrarea");
				printf(".\n");
				PutLevel(2);
				PutString("screendata");
				printf(".\n");
			} else {
				MakeLD();
			}
		} else
		if		(  fLinkage  ) {
			MakeLinkage();
		} else
		if		(  fDB  ) {
			MakeDB();
		} else
		if		(  fDBCOMM  ) {
			MakeDBCOMM();
		} else
		if		(  fDBPATH  ) {
			MakeDBPATH();
		} else
		if		(  fMCP  ) {
			MakeMCP();
		} else
		if		(  fLDW  ) {
			fMCP = TRUE;
			fSPA = TRUE;
			fLinkage = TRUE;
			fScreen = TRUE;
			RecName = "ldw";
			Prefix = "ldw-";
			MakeLD();
		} else
		if		(  fLDR  ) {
			fMCP = TRUE;
			fSPA = TRUE;
			fLinkage = TRUE;
			fScreen = TRUE;
			RecName = "ldr";
			Prefix = "ldr-";
			MakeLD();
		}
	}
	return	(0);
}
