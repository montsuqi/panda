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
#include	"option.h"
#include	"message.h"
#include	"multipart.h"
#include	"debug.h"

#define	SRC_CODESET		"euc-jp"

#define	SIZE_CHARS		16

static	Bool		fComm;

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
	PutHTML(html);
LEAVE_FUNC;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("htctest",NULL);

	InitHTC(NULL,NULL);
	CGI_InitValues();
	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		Session(fl->name);
	}
	Dump();
	return	(0);
}
