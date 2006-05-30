/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
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
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<libgen.h>
#include    <sys/socket.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"enum.h"
#include	"dirs.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"directory.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"handler.h"
#include	"wfcdata.h"
#include	"apsio.h"
#include	"aps_main.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*WfcPortNumber;

static	sigset_t	hupset;
static	int		MaxTran;
static	int		Sleep;

static	void
InitSystem(
	char	*name)
{
	int		i;

ENTER_FUNC;
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);
	pthread_sigmask(SIG_BLOCK,&hupset,NULL);
	InitDirectory();
	SetUpDirectory(Directory,name,"","",TRUE);
	if		(  ( ThisLD = GetLD(name) )  ==  NULL  ) {
		Error("LD \"%s\" not found.",name);
	}
	if		(  ThisLD->home  !=  NULL  ) {
		chdir(ThisLD->home);
	}
	ThisBD = NULL;
	ThisDBD = NULL;

	InitiateHandler();
	ThisDB = ThisLD->db;
	DB_Table = ThisLD->DB_Table;
	TextSize = ThisLD->textsize;
	InitializeValue(ThisEnv->mcprec->value);
	InitializeValue(ThisEnv->linkrec->value);
	InitializeValue(ThisLD->sparec->value);

	for	( i = 0 ; i < ThisLD->cBind ; i ++ ) {
		if		(	(  ThisLD->binds[i]  !=  NULL  )
				&&	(  ThisLD->binds[i]->rec  !=  NULL  ) ) {
			dbgprintf("[%s]",ThisLD->binds[i]->rec->name);
			InitializeValue(ThisLD->binds[i]->rec->value);
		}
	}
	ReadyDC();
LEAVE_FUNC;
}

static	ProcessNode	*
MakeProcessNode(void)
{
	ProcessNode	*node;
	int			i;

ENTER_FUNC;
	node = New(ProcessNode);
	node->mcprec = ThisEnv->mcprec;
	node->linkrec = ThisEnv->linkrec;
	node->sparec = ThisLD->sparec;
	node->cWindow = ThisLD->cWindow;
	node->cBind = ThisLD->cBind;
	node->bhash = ThisLD->bhash;
	node->textsize = ThisLD->textsize;
	node->scrrec = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * node->cWindow);
	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		node->scrrec[i] = ThisLD->windows[i];
	}

	/*	get initialize memory area	*/

	SetValueInteger(GetItemLongName(node->mcprec->value,"private.pstatus"),APL_SESSION_LINK);
	SetValueInteger(GetItemLongName(node->mcprec->value,"private.pputtype"),0);
	SetValueInteger(GetItemLongName(node->mcprec->value,"private.prc"),0);
LEAVE_FUNC;
	return	(node);
}

static	void
FinishSession(
	ProcessNode	*node)
{
ENTER_FUNC;
	xfree(node->scrrec); 
	xfree(node);
LEAVE_FUNC;
}

static	int
ExecuteServer(void)
{
	int		fhWFC
		,	rc;
	Port	*port;
	NETFILE		*fpWFC;
	ProcessNode	*node;
	WindowBind	*bind;
	int		tran;
	char	wname[SIZE_LONGNAME+1];

ENTER_FUNC;
	if		(  WfcPortNumber  ==  NULL  ) {
		port = ThisEnv->WfcApsPort;
	} else {
		port = ParPortName(WfcPortNumber);
	}
	while	(  ( fhWFC = ConnectSocket(port,SOCK_STREAM) )  <  0  ) {
		Warning("WFC connection retry");
		sleep(1);
	}
	fpWFC = SocketToNet(fhWFC);
	SendStringDelim(fpWFC,ThisLD->name);
	SendStringDelim(fpWFC,"\n");
	if		(  RecvPacketClass(fpWFC)  !=  APS_OK  ) {
		if		(  !CheckNetFile(fpWFC) ) {
			Error("WFC connection lost");
		}
		Error("invalid LD name");
	}
	InitAPSIO(fpWFC);
	if		(  ThisLD->cDB  >  0  ) {
		if ( ReadyOnlineDB(fpWFC) < 0 ){
			Error("Online DB is not ready");
		}
	}
	node = MakeProcessNode();
	for	( tran = MaxTran;(	(  MaxTran  ==  0  )
						||	(  tran     >   0  ) ); tran -- ) {
		if		(  !GetWFC(fpWFC,node)	) {
			Message("GetWFC failure");
			rc = 0;
			break;
		}
		dbgprintf("ld     = [%s]",ThisLD->name);
		dbgprintf("window = [%s]",ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")));
		PureComponentName(ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")),
					   wname);
		if		(  ( bind = (WindowBind *)g_hash_table_lookup(ThisLD->bhash,wname) )
				   !=  NULL  ) {
			if		(  bind->module  ==  NULL  ){
				Message("bind->module not found");
				rc = 2;
				break;
			}
			SetValueString(GetItemLongName(node->mcprec->value,"dc.module"),bind->module,NULL);
			if ( node->dbstatus == REDFAILURE ) {
				RedirectError();
			} else {
				node->dbstatus = GetDBStatus();
			}
			TransactionStart(NULL);
			ExecuteProcess(node);
			if		(  Sleep  >  0  ) {
				sleep(Sleep);
			}
			TransactionEnd(NULL);
			PutWFC(fpWFC,node);
		} else {
			Message("window [%s] not found.",wname);
			rc = 2;
			break;
		}
	}
	MessageLog("exiting DC_Thread");
	FinishSession(node);
LEAVE_FUNC;
	return	(rc);
}

static	void
StopProcess(void)
{
ENTER_FUNC;
	StopOnlineDB(); 
	CleanUpOnlineDB(); 
	StopDC();
	CleanUpOnlineDC();
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"wfcport",	STRING,		TRUE,	(void*)&WfcPortNumber,
		"WFC接続待ちポート番号"	 						},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"lddir",	STRING,		TRUE,	(void*)&D_Dir,
		"LD定義格納ディレクトリ"		 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"path",		STRING,		TRUE,	(void*)&LibPath,
		"モジュールのロードパス"						},

	{	"dbhost",	STRING,		TRUE,	(void*)&DB_Host,
		"データベース稼働ホスト名"						},
	{	"dbport",	STRING,		TRUE,	(void*)&DB_Port,
		"データベース待機ポート番号"					},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"データベース名"								},
	{	"dbuser",	STRING,		TRUE,	(void*)&DB_User,
		"データベースのユーザ名"						},
	{	"dbpass",	STRING,		TRUE,	(void*)&DB_Pass,
		"データベースのパスワード"						},

	{	"maxtran",	INTEGER,	TRUE,	(void*)&MaxTran,
		"apsの処理するトランザクション数を指定する"		},
	{	"cache",	INTEGER,	TRUE,	(void*)&nCache,
		"端末情報をキャッシュする端末数"				},

	{	"sleep",	INTEGER,	TRUE,	(void*)&Sleep,
		"実行時間に足す処理時間(for debug)"				},
	{	"timer",	BOOLEAN,	TRUE,	(void*)&fTimer,
		"時間計測を行う"								},

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"dbredirectorの起動をチェックしない"			},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"dbredirectorを使わない"						},
	{	"maxretry",	INTEGER,	TRUE,	(void*)&MaxRetry,
		"dbredirector送信の再試行数を指定する"			},
	{	"retryint",	INTEGER,	TRUE,	(void*)&RetryInterval,
		"dbredirector送信の再試行の間隔を指定する(秒)"	},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
ENTER_FUNC;
	WfcPortNumber = NULL;
	fNoCheck = FALSE;
	fNoRedirect = FALSE;

	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	LibPath = NULL;
	MaxTran = 0;
	nCache = 0;
	Sleep = 0;

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;

	MaxRetry = 3;
	RetryInterval = 5;

	fTimer = FALSE;
LEAVE_FUNC;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	int			rc;
	char		id[128];

	SetDefault();
	fl = GetOption(option,argc,argv);
	(void)signal(SIGHUP,(void *)StopProcess);
	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		snprintf(id, sizeof(id), "aps-%s",fl->name);
		InitMessage(id,NULL);
		InitNET();
		InitSystem(fl->name);
		rc = ExecuteServer();
		StopProcess();
	} else {
		rc = -1;
		fprintf(stderr, "LD name is not specified.\n");
	}
	exit(rc);
}
