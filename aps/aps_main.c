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
#include	"enum.h"
#include	"dirs.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
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

dbgmsg(">InitSystem");
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);
	pthread_sigmask(SIG_BLOCK,&hupset,NULL);
	InitDirectory();
	SetUpDirectory(Directory,name,"","");
	if		(  ( ThisLD = GetLD(name) )  ==  NULL  ) {
		dbgprintf("LD \"%s\" not found.",name);
		exit(1);
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
	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		dbgprintf("[%s]",ThisLD->window[i]->rec->name);
		InitializeValue(ThisLD->window[i]->rec->value);
	}
	ReadyDC();
	if		(  ThisLD->cDB  >  0  ) {
		ReadyOnlineDB();
	}
dbgmsg("<InitSystem");
}

static	ProcessNode	*
MakeProcessNode(void)
{
	ProcessNode	*node;
	int			i;

dbgmsg(">MakeProcessNode");
	node = New(ProcessNode);
	node->mcprec = ThisEnv->mcprec;
	node->linkrec = ThisEnv->linkrec;
	node->sparec = ThisLD->sparec;
	node->cWindow = ThisLD->cWindow;
	node->whash = ThisLD->whash;
	node->textsize = ThisLD->textsize;
	node->scrrec = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * node->cWindow);
	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		node->scrrec[i] = ThisLD->window[i]->rec;
	}

	/*	get initialize memory area	*/

	SetValueInteger(GetItemLongName(node->mcprec->value,"private.pstatus"),APL_SESSION_LINK);
	SetValueInteger(GetItemLongName(node->mcprec->value,"private.pputtype"),0);
	SetValueInteger(GetItemLongName(node->mcprec->value,"private.prc"),0);
dbgmsg("<MakeProcessNode");
	return	(node);
}

static	void
FinishSession(
	ProcessNode	*node)
{
dbgmsg(">FinishSession");
	xfree(node->scrrec); 
	xfree(node);
dbgmsg("<FinishSession");
}

static	void
ExecuteDC(
	void	*para)
{
	int		fhWFC
	,		_fh;
	NETFILE	*fpWFC;
	Port	*port;
	ProcessNode	*node;
	WindowBind	*bind;
	int		tran;

ENTER_FUNC;
	if		(  WfcPortNumber  ==  NULL  ) {
		port = ThisEnv->WfcApsPort;
	} else {
		port = ParPortName(WfcPortNumber);
	}
	if		(  ( fhWFC = ConnectSocket(port,SOCK_STREAM) )  <  0  ) {
		Error("WFC not ready");
	}
	fpWFC = SocketToNet(fhWFC);
	SendString(fpWFC,ThisLD->name);
	if		(  RecvPacketClass(fpWFC)  !=  APS_OK  ) {
		Error("invalid LD name");
	}
	InitAPSIO(fpWFC);
	node = MakeProcessNode();
	for	( tran = MaxTran;(	(  MaxTran  ==  0  )
						||	(  tran     >   0  ) ); tran -- ) {
		if		(  !GetWFC(fpWFC,node)  )	break;
		dbgprintf("[%s]",ThisLD->name);
		if		(  ( bind = (WindowBind *)g_hash_table_lookup(ThisLD->whash,
															  ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window"))))  !=  NULL  ) {
			if		(  bind->module  ==  NULL  )	break;
			SetValueString(GetItemLongName(node->mcprec->value,"dc.module"),bind->module,NULL);
			TransactionStart(NULL);
			ExecuteProcess(node);
			if		(  Sleep  >  0  ) {
				sleep(Sleep);
			}
			TransactionEnd(NULL);
			PutWFC(fpWFC,node);
		} else {
			MessagePrintf("window [%s] not found.\n",
						  ValueToString(GetItemLongName(node->mcprec->value,"dc.window"),NULL));
			break;
		}
	}
	MessageLog("exiting DC_Thread\n");
	CloseNet(fpWFC);
	FinishSession(node);
LEAVE_FUNC;
}

static	void
ExecuteServer(void)
{
dbgmsg(">ExecuteServer");
	ExecuteDC(NULL);
dbgmsg("<ExecuteServer");
}

static	void
StopProcess(
	int		ec)
{
dbgmsg(">StopProcess");
	StopOnlineDB(); 
	CleanUpOnlineDB(); 
	StopDC();
	CleanUpOnlineDC();
dbgmsg("<StopProcess");
	exit(ec);
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

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"dbredirectorの起動をチェックしない"			},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"dbredirectorを使わない"						},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
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
		sprintf(id,"aps-%s",fl->name);
		InitMessage(id,NULL);
		InitNET();
		InitSystem(fl->name);
		ExecuteServer();
		StopProcess(0);
		rc = 0;
	} else {
		rc = -1;
		fprintf(stderr,"LD名が指定されていません\n");
	}
	exit(rc);
}
