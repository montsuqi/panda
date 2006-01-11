/*
PANDA -- a simple transaction monitor
Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include	<string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <sys/time.h>
#include    <sys/socket.h>
#include    <sys/select.h>
#include	<time.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"comm.h"
#include	"dirs.h"
#include	"redirect.h"
#include	"dblib.h"
#include	"directory.h"
#include	"queue.h"
#include	"socket.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*PortNumber;
static	int		Back;
static	char	*Directory;

static	sigset_t	hupset;
static	DBG_Struct	*ThisDBG;
static	pthread_t	_FileThread;
static	Queue		*FileQueue;
static	Port		*RedirectPort;

static	void
LogThread(
	void	*para)
{
	int		fhLog = (int)para;
	LargeByteString	*data;
	NETFILE	*fpLog;
	PacketClass	c;
	Bool	fSuc;

ENTER_FUNC;
	fpLog = SocketToNet(fhLog);
	do {
		switch	( c = RecvPacketClass(fpLog) ) {
		  case	RED_DATA:
			data = NewLBS();
			LBS_EmitStart(data);
			RecvLBS(fpLog,data);
			SendPacketClass(fpLog,RED_OK);
			EnQueue(FileQueue,data);
			fSuc = TRUE;
			break;
		  case	RED_PING:
			SendPacketClass(fpLog,RED_PONG);
			fSuc = TRUE;
			break;
		  case	RED_END:
			fSuc = FALSE;
			break;
		  default:
			SendPacketClass(fpLog,RED_NOT);
			fSuc = FALSE;
			break;
		}
	}	while	(  fSuc  );
	CloseNet(fpLog);
LEAVE_FUNC;
}

extern	pthread_t
ConnectLog(
	int		_fhLog)
{
	int		fhLog;
	pthread_t	thr;

ENTER_FUNC;
	if		(  ( fhLog = accept(_fhLog,0,0) )  <  0  )	{
		printf("_fhLog = %d\n",_fhLog);
		Error("INET Domain Accept");
	}
	pthread_create(&thr,NULL,(void *(*)(void *))LogThread,(void *)fhLog);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(thr); 
}

static	void
FileThread(
	void	*dummy)
{
	LargeByteString	*data;
	char	*p;
	FILE	*fp;
	time_t	nowtime;
	struct	tm	*Now;
	int		count;

ENTER_FUNC;
	if		(  ThisDBG->dbname  !=  NULL  ) {
		OpenRedirectDB(ThisDBG);
	} else {
		OpenDB_RedirectPort(ThisDBG);
	}
	if		(  ThisDBG->file  !=  NULL  ) {
		umask((mode_t) 0077);
		if		(  ( fp = fopen(ThisDBG->file,"a+") )  ==  NULL  ) {
			Error("log file can not open :%s", ThisDBG->file);
		}
	} else {
		fp = NULL;
	}
	count = 0;
	while	(TRUE)	{
		data = (LargeByteString *)DeQueue(FileQueue);
		dbgmsg("de queue");
		LBS_EmitEnd(data);
		p = LBS_Body(data);
		if		(  *p  !=  0  ) {
			if		(  ThisDBG->fConnect  )	{
				TransactionRedirectStart(ThisDBG);
				ExecRedirectDBOP(ThisDBG,p);
				TransactionRedirectEnd(ThisDBG);
			}
			BeginDB_Redirect(ThisDBG);
			PutDB_Redirect(ThisDBG,p);
			CommitDB_Redirect(ThisDBG);
			if		(  fp  !=  NULL  ) {
				time(&nowtime);
				Now = localtime(&nowtime);
				fprintf(fp,"%s%04d/%02d/%02d/%02d:%02d:%02d/%08d%s"
						,ThisDBG->func->commentStart
						, Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
						, Now->tm_hour,Now->tm_min,Now->tm_sec,count
						,ThisDBG->func->commentEnd);
				fprintf(fp,"%s\n",p);
				fflush(fp);
			}
			count ++;
		}
		FreeLBS(data);
	}
	if		(  ThisDBG->dbname  ==  NULL  ) {
		CloseDB_RedirectPort(ThisDBG);
	}
	//	pthread_exit(NULL);
LEAVE_FUNC;
}


extern	void
ExecuteServer(void)
{
	int		_fhLog;
	fd_set	ready;
	int		maxfd;

ENTER_FUNC;
	pthread_create(&_FileThread,NULL,(void *(*)(void *))FileThread,NULL); 
	_fhLog = InitServerPort(RedirectPort,Back);
	maxfd = _fhLog;

	while	(TRUE)	{
		FD_ZERO(&ready);
		FD_SET(_fhLog,&ready);
		select(maxfd+1,&ready,NULL,NULL,NULL);
		if		(  FD_ISSET(_fhLog,&ready)  ) {
			ConnectLog(_fhLog);
		}
	}
LEAVE_FUNC;
}

#ifdef	DEBUG
static	void
DumpDBG(
	char		*name,
	DBG_Struct	*dbg,
	void		*dummy)
{
	printf("name     = [%s]\n",dbg->name);
	printf("\ttype     = [%s]\n",dbg->type);
	printf("\tDB name  = [%s]\n",dbg->dbname);
	printf("\tDB user  = [%s]\n",dbg->user);
	printf("\tDB pass  = [%s]\n",dbg->pass);
	if		(  dbg->file  !=  NULL  ) {
		printf("\tlog file = [%s]\n",dbg->file);
	}
	if		(  dbg->redirect  !=  NULL  ) {
		dbg = dbg->redirect;
		printf("\tredirect = [%s]\n",dbg->name);
	}
}
#endif

extern	void
InitSystem(
	char	*name)
{

ENTER_FUNC;
	InitNET();
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,TRUE);
    if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
	InitDB_Process(NULL);
#ifdef	DEBUG
	g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)DumpDBG,NULL);
#endif
	if		(  ( ThisDBG = (DBG_Struct *)g_hash_table_lookup(ThisEnv->DBG_Table,name) )
			   ==  NULL  ) {
		Error("DB group not found");
	}
	if		(  PortNumber  ==  NULL  ) {
		if		(  ThisDBG->redirectPort  !=  NULL  ) {
			RedirectPort = ThisDBG->redirectPort;
		} else {
			RedirectPort = ParPortName(PORT_REDIRECT);
		}
	} else {
		RedirectPort = ParPortName(PortNumber);
	}
	FileQueue = NewQueue();
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},

	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"ddir",	STRING,			TRUE,	(void*)&D_Dir,
		"定義格納ディレクトリ"			 				},

	{	"host",		STRING,		TRUE,	(void*)&DB_Host,
		"PostgreSQL稼働ホスト名"						},
	{	"port",		STRING,		TRUE,	(void*)&DB_Port,
		"PostgreSQLポート番号"							},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"データベース名"								},
	{	"user",		STRING,		TRUE,	(void*)&DB_User,
		"ユーザ名"										},
	{	"pass",		STRING,		TRUE,	(void*)&DB_Pass,
		"パスワード"									},

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
	PortNumber = NULL;
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;

	fNoCheck = FALSE;
	fNoRedirect = FALSE;
	MaxRetry = 3;
	RetryInterval = 5;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*name;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("dbredirector",NULL);

	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		name = fl->name;
	} else {
		name = "";
	}
	InitSystem(name);
	Message("dbredirector start");
	ExecuteServer();
	Message("dbredirector end");
	return	(0);
}
