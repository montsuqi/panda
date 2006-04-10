/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2006 Ogochan.
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
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	<errno.h>
#include	<iconv.h>
#include	<sys/mman.h>
#include	<sys/stat.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"HTCparser.h"
#include	"cgi.h"
#include	"mon.h"
#include	"tags.h"
#include	"exec.h"
#include	"RecParser.h"
#include	"option.h"
#include	"message.h"
#include	"multipart.h"
#include	"debug.h"

#define	SRC_CODESET		"euc-jp"

static	Bool		fComm;
static	ValueStruct	*Value;
static	char		*Type;
static	size_t		ArraySize;
static	size_t		TextSize;
static	char		*RecName;
static	char		*ValueName;
static	char		*Code;


static	ARG_TABLE	option[] = {
	{	"screen",	STRING,		TRUE,	(void*)&ScreenDir,
		"画面格納ディレクトリ"	 						},
	{	"get",		BOOLEAN,	TRUE,	(void*)&fGet,
		"actionをgetで処理する"	 						},
	{	"dump",		BOOLEAN,	TRUE,	(void*)&fDump,
		"変数のダンプを行う"	 						},
	{	"cookie",	BOOLEAN,	TRUE,	(void*)&fCookie,
		"セション変数をcookieで行う"					},
	{	"jslink",	BOOLEAN,	TRUE,	(void*)&fJavaScriptLink,
		"<htc:hyperlink>によるリンクをJavaScriptで行う"	},

	{	"type",		STRING,		TRUE,	(void*)&Type	,
		"データ形式名"			 						},
	{	"textsize",	INTEGER,	TRUE,	(void*)&TextSize,
		"textの最大長"									},
	{	"arraysize",INTEGER,	TRUE,	(void*)&ArraySize,
		"可変要素配列の最大繰り返し数"					},
	{	"rec",		STRING,		TRUE,	(void*)&RecName,
		"recの名前"										},
	{	"code",		STRING,		TRUE,	(void*)&Code,
		"文字コードセット名"							},
	{	"js",		BOOLEAN,	TRUE,	(void*)&fJavaScript,
		"JavaScriptを使ったHTML生成を行う"	},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	ScreenDir = getcwd(NULL,0);
	SesDir = NULL;
	fDump = FALSE;
	fGet = FALSE;
	fComm = FALSE;
	fCookie = FALSE;
	CommandLine = NULL;
	Code = NULL;
	fJavaScript = TRUE;

	RecordDir = NULL;
	ArraySize = -1;
	TextSize = -1;
	RecName = NULL;
	Type = "CSV";
}

static	void
Session(
	char	*name)
{
	HTCInfo	*htc;
	LargeByteString	*html;

ENTER_FUNC;
	htc = ParseScreen(name);
	if		(  htc  ==  NULL  ) {
        fprintf(stderr, "HTC file not found: %s\n", name);
        dbgprintf("HTC file not found: %s\n", name);
		exit(1);
	}
	html = NewLBS();
	LBS_EmitStart(html);
	ExecCode(html,htc);
	WriteLargeString(stdout,html,Codeset);
LEAVE_FUNC;
}

static	ValueStruct	*
TestGetValue(char *name, Bool fClear)
{
    ValueStruct *value;

ENTER_FUNC;
	if		(  Value  ==  NULL  ) {
		value = NULL;
	} else {
		if		(  strlcmp(name,ValueName)  ==  0  ) {
			name += strlen(ValueName) + 1;
			value = GetItemLongName(Value,name);
		} else {
			value = NULL;
		}
	}
LEAVE_FUNC;
	return value;
}

static	ValueStruct	*
ReadValueDefine(
	char	*name,
	char	**ValueName)
{
	ValueStruct	*value;
	char		buf[SIZE_LONGNAME+1]
	,			dir[SIZE_LONGNAME+1];
	char		*p
	,			*q;
	Bool		fExit;

ENTER_FUNC;
	strcpy(dir,RecordDir);
	p = dir;
	value = NULL;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
			fExit = FALSE;
		} else {
			fExit = TRUE;
		}
		sprintf(buf,"%s/%s",p,name);
		if		(  ( value = RecParseValue(buf,ValueName) )  !=  NULL  ) {
			break;
		}
		p = q + 1;
	}	while	(  !fExit  );
LEAVE_FUNC;
	return	(value);
}

extern	int
main(
	int		argc,
	char	**argv)
{
	struct	stat	sb;
	char		*p
		,		*q;
	FILE_LIST	*fl;
	char		*htcname
		,		*dataname;
	CONVOPT	*Conv;
	ConvFuncs	*func;
	FILE	*file;
	int		i;

	SetDefault();
	InitMessage("htcproc",NULL);
	fl = GetOption(option,argc,argv);
#if	0
				   "\tLang can accept\n"
				   "\tOpenCOBOL\n"
				   "\tdotCOBOL\n"
				   "\tCSV1(number and string delimited by \"\")\n"
				   "\tCSV2(number and string *not* delimitd by \"\")\n"
				   "\tCSV3(string delimitd by \"\")\n"
				   "\tCSVE(Excel type CSV)\n"
				   "\tCSV(same as CSV3)\n"
				   "\tSQL(SQL type)\n"
				   "\tXML1(tag is data class)\n"
				   "\tXML2(tag is data name)\n"
				   "\tCGI('name=value&...)\n"
				   "\tRFC822('name: value\\n')\n"
		);
#endif
	InitHTC(NULL,TestGetValue);
	CGI_InitValues();
	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		htcname = fl->name;
		fl = fl->next;
	} else {
		htcname = NULL;
	}
	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		dataname = fl->name;
		fl = fl->next;
	} else {
		dataname = NULL;
	}
	if		(  RecName  !=  NULL  ) {
		if		(  ( func = GetConvFunc(Type) )  ==  NULL  ) {
			fprintf(stderr,"invalid data type [%s]\n",Type);
			exit(1);
		}
		ConvSetLanguage(Type);
		Conv = NewConvOpt();
		ConvSetSize(Conv,TextSize,ArraySize);
		ConvSetCodeset(Conv,Code);
		ConvSetEncoding(Conv,STRING_ENCODING_NULL);
		ConvSetUseName(Conv,TRUE);
		RecParserInit();
		if		(  RecordDir  !=  NULL  ) {
			Value = ReadValueDefine(RecName,&ValueName);
		} else {
			Value = RecParseValue(RecName,&ValueName);
		}
		ConvSetRecName(Conv,StrDup(ValueName));
		if		(  dataname  ==  NULL  ) {
			file = stdin;
		} else {
			file = fopen(dataname,"r");
		}
		if		(	(  file  !=  NULL  )
				&&	(  fstat(fileno(file),&sb)  ==  0  ) ) {
			if		(  ( p = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fileno(file),0) )
					   !=  NULL  ) {
				InitializeValue(Value);
				q = (char *)xmalloc(sb.st_size);
				memcpy(q,p,sb.st_size);
				munmap(p,sb.st_size);
				if		(  !func->fBinary  ) {
					i = sb.st_size - 1;
					while	(  isspace(q[i])  )	{
						q[i] = 0;
						i --;
					}
				}
				UnPackValue(Conv,(byte *)q,Value);
			} else {
				Value = NULL;
			}
		} else {
			Value = NULL;
		}
	} else {
		Value = NULL;
	}
	Session(htcname);
	Dump();
	return	(0);
}
