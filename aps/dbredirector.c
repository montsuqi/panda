/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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
#define	DEBUG
#define	TRACE
/*
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<time.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"
#include	"value.h"
#include	"comm.h"
#include	"dirs.h"
#include	"redirect.h"
#include	"dbgroup.h"
#ifdef	HAVE_POSTGRES
#include	"Postgres.h"
#endif
#ifdef	USE_SHELL
#include	"Shell.h"
#endif
#include	"directory.h"
#include	"queue.h"
#include	"tcp.h"
#include	"option.h"
#include	"debug.h"

GLOBAL	LD_Struct		*ThisLD;
static	char	*PortNumber;
static	int		Back;
static	char	*Directory;

static	sigset_t	hupset;
static	DBG_Struct	*ThisDBG;
static	pthread_t	_FileThread;
static	Queue		*FileQueue;

typedef	struct {
	size_t	asize
	,		usize;
	char	*body;
}	LOG_DATA;

static	int
InitServerPort(
	char	*port)
{	int		fh;

dbgmsg(">InitServerPort");
	fh = BindSocket(port,SOCK_STREAM);

	if		(  listen(fh,Back)  <  0  )	{
		shutdown(fh, 2);
		Error("INET Domain listen");
	}
dbgmsg("<InitServerPort");
	return	(fh);
}

static	void
LogThread(
	void	*para)
{
	int		fhLog = (int)para;
	LOG_DATA	*data;
	char	buff[SIZE_SQL];
	FILE	*fpLog;
	PacketClass	c;
	Bool	fSuc;
	size_t	left;
	char	*p;

dbgmsg(">LogThread");
	fpLog = fdopen(fhLog,"w+");
	data = NULL;
	do {
	  top:
		switch	( c = RecvPacketClass(fpLog) ) {
		  case	RED_DATA:
			RecvString(fpLog,buff);
			SendPacketClass(fpLog,RED_OK);
			fflush(fpLog);
			fSuc = TRUE;
			break;
		  case	RED_PING:
			SendPacketClass(fpLog,RED_PONG);
			fflush(fpLog);
			goto	top;
			break;
		  default:
			SendPacketClass(fpLog,RED_NOT);
			fflush(fpLog);
			fSuc = FALSE;
			break;
		}
		if		(  !stricmp(buff,"begin")  ) {
			data = New(LOG_DATA);
			data->asize = SIZE_SQL;
			data->usize = 0;
			data->body = (char *)xmalloc(data->asize);
			*data->body = 0;
		} else
		if		(	(  !stricmp(buff,"commit work")  )
				||	(  !stricmp(buff,"commit")       ) ) {
			if		(  data  !=  NULL  ) {
				EnQueue(FileQueue,data);
				data = NULL;
			}
		} else {
			if		(  data  !=  NULL  ) {
				left = data->asize - data->usize;
				strcat(buff,";");
				if		(  left  <  strlen(buff)  ) {
					data->asize += SIZE_SQL;
					p = (char *)xmalloc(data->asize);
					strcpy(p,data->body);
					xfree(data->body);
					data->body = p;
				}
				strcpy(&data->body[data->usize],buff);
				data->usize += strlen(buff);
			} else {
				Warning("transaction not begin");
			}
		}
	}	while	(  fSuc  );
	shutdown(fhLog, 2);
	fclose(fpLog);
	//	pthread_exit(NULL);
dbgmsg("<LogThread");
}

extern	pthread_t
ConnectLog(
	int		_fhLog)
{
	int		fhLog;
	pthread_t	thr;

dbgmsg(">ConnectLog");
	if		(  ( fhLog = accept(_fhLog,0,0) )  <  0  )	{
		printf("_fhLog = %d\n",_fhLog);
		Error("INET Domain Accept");
	}
	pthread_create(&thr,NULL,(void *(*)(void *))LogThread,(void *)fhLog);
dbgmsg("<ConnectLog");
	return	(thr); 
}

static	void
FileThread(
	void	*dummy)
{
	LOG_DATA	*data;
	FILE	*fp;
	time_t	nowtime;
	struct	tm	*Now;
	int		count;

dbgmsg(">FileThread");
	if		(  ThisDBG->dbname  !=  NULL  ) {
		ExecDBG_Operation(ThisDBG,"DBOPEN");
	}
	if		(  ( fp = fopen(ThisDBG->file,"a+") )  ==  NULL  ) {
		Error("log file can not open");
	}
	count = 0;
	while	(TRUE)	{
		data = (LOG_DATA *)DeQueue(FileQueue);
		dbgmsg("de queue");
		if		(  ThisDBG->dbname  !=  NULL  )	{
			ExecDBG_Operation(ThisDBG,"DBSTART");
			ExecDBOP(ThisDBG,data->body);
			ExecDBG_Operation(ThisDBG,"DBCOMMIT");
		}
		time(&nowtime);
		Now = localtime(&nowtime);
		fprintf(fp,"%s%04d/%02d/%02d/%02d:%02d:%02d/%08d%s"
				,ThisDBG->func->commentStart
				, Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
				, Now->tm_hour,Now->tm_min,Now->tm_sec,count
				,ThisDBG->func->commentEnd);
		fprintf(fp,"%s\n",data->body);
		fflush(fp);
		count ++;
		xfree(data->body);
		xfree(data);
	}
	//	pthread_exit(NULL);
dbgmsg("<FileThread");
}


extern	void
ExecuteServer(void)
{
	int		_fhLog;
	fd_set	ready;
	int		maxfd;

dbgmsg(">ExecuteServer");
	pthread_create(&_FileThread,NULL,(void *(*)(void *))FileThread,NULL); 
	_fhLog = InitServerPort(PortNumber);
	maxfd = _fhLog;

	while	(TRUE)	{
dbgmsg("loop");
		FD_ZERO(&ready);
		FD_SET(_fhLog,&ready);
		select(maxfd+1,&ready,NULL,NULL,NULL);
		if		(  FD_ISSET(_fhLog,&ready)  ) {
			ConnectLog(_fhLog);
		}
	}
dbgmsg("<ExecuteServer");
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
	if		(  dbg->port  !=  NULL  ) {
		printf("\thost     = [%s]\n",dbg->port->host);
		printf("\tport     = [%s]\n",dbg->port->port);
	}
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

static	void
InitDB_Process(void)
{
dbgmsg(">InitDB_Process");
	InitDBG();
#ifdef	HAVE_POSTGRES
	InitPostgres();
#endif
#ifdef	USE_SHELL
	InitShellOperation();
#endif
	SetUpDBG();
dbgmsg("<InitDB_Process");
}

extern	void
InitSystem(
	char	*name)
{
	DBG_Struct	*Orig;

dbgmsg(">InitSystem");
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);
	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL);
	InitDB_Process();
#ifdef	DEBUG
	g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)DumpDBG,NULL);
#endif
	if		(  ( ThisDBG = (DBG_Struct *)g_hash_table_lookup(ThisEnv->DBG_Table,name) )
			   ==  NULL  ) {
		Error("DB group not found");
	}
	Orig = ThisDBG;
	if		(  ThisDBG->redirect  !=  NULL  ) {
		ThisDBG = ThisDBG->redirect;
	}
	ThisDBG->dbt = Orig->dbt;
	if		(  PortNumber  ==  NULL  ) {
		if		(  Orig->redirectPort  !=  NULL  ) {
			PortNumber = Orig->redirectPort->port;
		} else {
			PortNumber = IntStrDup(PORT_REDIRECT);
		}
	}
	FileQueue = NewQueue();
dbgmsg("<InitSystem");
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
	{	"bddir",	STRING,		TRUE,	(void*)&BD_Dir,
		"バッチ定義格納ディレクトリ"	 				},
	{	"lddir",	STRING,		TRUE,	(void*)&LD_Dir,
		"データ定義格納ディレクトリ"	 				},

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
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = NULL;
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	LD_Dir = NULL;
	BD_Dir = NULL;
	Directory = "./directory";

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
	char		*name;

	SetDefault();
	fl = GetOption(option,argc,argv);

	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		name = fl->name;
	} else {
		name = "";
	}
	InitSystem(name);
	ExecuteServer();
	return	(0);
}
