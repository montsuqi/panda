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
#include	<ctype.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"

#include	"dirs.h"
#include	"const.h"
#include	"enum.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"directory.h"
#include	"handler.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

#include	"directory.h"

static	char	*Directory;
static	char	*CommandParameter;
static	char	*BD_Name;
static	BatchBind	*Bind;

static	void
InitData(
	char	*name)
{
dbgmsg(">InitData");
	InitDirectory();
	SetUpDirectory(Directory,"",name,"");
dbgmsg("<InitData");
}

extern	void
InitSystem(
	char	*name)
{
dbgmsg(">InitSystem");
	InitData(BD_Name);
	if		(  ( ThisBD = GetBD(BD_Name) )  ==  NULL  ) {
		fprintf(stderr,"BD file not found.\n");
		exit(1);
	}
	ThisLD = NULL;
	ThisDBD = NULL;
	InitiateBatchHandler();
	ThisDB = ThisBD->db;
	DB_Table = ThisBD->DB_Table;
	TextSize = ThisBD->textsize;
	if		(  ( Bind = g_hash_table_lookup(ThisBD->BatchTable,name) )  ==  NULL  ) {
		fprintf(stderr,"%s application is not in BD.\n",name);
		exit(1);
	}
	if		(  ThisBD->cDB  >  0  ) {
		InitDB_Process();
		OpenDB(NULL);
		ReadyHandlerDB(Bind->handler);
	}
dbgmsg("<InitSystem");
}

static	int
ExecuteSubProcess(
	char	*name)
{
	int		rc;
dbgmsg(">ExecuteSubProcess");
	printf("[%s][%s]\n",name,CommandParameter);
	rc = StartBatch(name,CommandParameter);
dbgmsg("<ExecuteSubProcess");
	return	(rc); 
}

static	void
StopProcess(
	int		ec)
{
dbgmsg(">StopProcess");
	if		(  ThisBD->cDB  >  0  ) {
		CloseDB(NULL);
		StopHandlerDB(Bind->handler);
		CleanUpHandlerDB(Bind->handler);
	}
dbgmsg("<StopProcess");
	exit(ec);
}
static	ARG_TABLE	option[] = {
	{	"host",		STRING,		TRUE,	(void*)&DB_Host,
		"PostgreSQL稼働ホスト名"						},
	{	"port",		STRING,		TRUE,	(void*)&DB_Port,
		"PostgreSQLポート番号"							},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"bddir",	STRING,		TRUE,	(void*)&D_Dir,
		"バッチ定義格納ディレクトリ"	 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"path",		STRING,		TRUE,	(void*)&LibPath,
		"モジュールのロードパス"						},
	{	"parameter",STRING,		TRUE,	(void*)&CommandParameter,
		"コマンドライン"								},

	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"データベース名"								},
	{	"user",		STRING,		TRUE,	(void*)&DB_User,
		"ユーザ名"										},
	{	"pass",		STRING,		TRUE,	(void*)&DB_Pass,
		"パスワード"									},
	{	"bd",		STRING,		TRUE,	(void*)&BD_Name,
		"BD定義名"										},

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"dbredirectorの起動をチェックしない"			},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"dbredirectorを使わない"						},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	LibPath = NULL;

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;
	BD_Name = NULL;
	CommandParameter = "";

	fNoCheck = FALSE;
	fNoRedirect = FALSE;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	int		rc;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("dbstub",NULL);
	InitNET();

	(void)signal(SIGHUP,(void *)StopProcess);
	if		(  BD_Name  ==  NULL  ) {
		fprintf(stderr,"BD名が指定されていません\n");
		exit(1);
	}
	InitSystem(fl->name);
	rc = ExecuteSubProcess(fl->name);
	StopProcess(0);
	return	(rc);
}
