/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2004 Ogochan & JMA (Japan Medical Association).

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
	htc = HTCParserFile(name);
	if (htc == NULL)
		exit(1);
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
		Value = RecParseValue(RecName,&ValueName);
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
				UnPackValue(Conv,q,Value);
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
