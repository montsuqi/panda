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
#include	<string.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/time.h>
#include    <sys/socket.h>
#include    <sys/select.h>
#include	<time.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"comms.h"
#include	"queue.h"
#include	"socket.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*PortNumber;
static	int		Back;
static	Bool	fDate;
static	Bool	fStdout;

static	sigset_t	hupset;
static	pthread_t	_FileThread;
static	Queue		*FileQueue;

static	void
LogThread(
	void	*para)
{
	int		fhLog = (int)para;
	NETFILE	*fpLog;
	char	buff[SIZE_BUFF+1];
	Bool	fOK;
	char	*str;

dbgmsg(">LogThread");
	fpLog = SocketToNet(fhLog);
	do {
		if		(  ( fOK = RecvStringDelim(fpLog,SIZE_BUFF,buff) )  ) {
			str = StrDup(buff);
			EnQueue(FileQueue,str);
		}
		if		(  !CheckNetFile(fpLog)  )	break;
	}	while	(  fOK  );
	CloseNet(fpLog);
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
	char	*fn)
{
	char	*p;
	FILE	*fp;
	time_t	nowtime;
	struct	tm	*Now;

dbgmsg(">FileThread");
	if		(  fn  !=  NULL  ) { 
		if		(  ( fp = fopen(fn,"w") )  ==  NULL  ) {
			Error("log file can not open");
		}
	} else {
		fp = NULL;
	}
	while	(TRUE)	{
		p = (char *)DeQueue(FileQueue);
		dbgmsg("de queue");
		if		(  fDate  ) {
			time(&nowtime);
			Now = localtime(&nowtime);
			if		(  fp  !=  NULL  ) {
				fprintf(fp,"%04d/%02d/%02d/%02d:%02d:%02d "
						, Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
						, Now->tm_hour,Now->tm_min,Now->tm_sec);
			}
			if		(  fStdout  ) {
				printf("%04d/%02d/%02d/%02d:%02d:%02d "
					   , Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
					   , Now->tm_hour,Now->tm_min,Now->tm_sec);
			}
		}
		if		(  fp  !=  NULL  ) {
			fprintf(fp,"%s\n",p);
			fflush(fp);
		}
		if		(  fStdout  ) {
			printf("%s\n",p);
			fflush(stdout);
		}
		xfree(p);
	}
dbgmsg("<FileThread");
}


extern	void
ExecuteServer(
	char	*fn)
{
	int		_fhLog;
	fd_set	ready;
	int		maxfd;
	Port	*port;

dbgmsg(">ExecuteServer");
	pthread_create(&_FileThread,NULL,(void *(*)(void *))FileThread,(void *)fn); 
	port = ParPortName(PortNumber);
	_fhLog = InitServerPort(port,Back);
	maxfd = _fhLog;

	while	(TRUE)	{
		FD_ZERO(&ready);
		FD_SET(_fhLog,&ready);
		select(maxfd+1,&ready,NULL,NULL,NULL);
		if		(  FD_ISSET(_fhLog,&ready)  ) {
			ConnectLog(_fhLog);
		}
	}
	DestroyPort(port);
dbgmsg("<ExecuteServer");
}

extern	void
InitSystem(void)
{
dbgmsg(">InitSystem");
	InitNET();
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);

	FileQueue = NewQueue();
dbgmsg("<InitSystem");
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},
	{	"date",		BOOLEAN,	TRUE,	(void*)&fDate,
		"時刻表示を行う"								},
	{	"stdout",	BOOLEAN,	TRUE,	(void*)&fStdout,
		"ログをstdoutに出力する"						},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = PORT_MSGD;
	Back = 5;
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
	InitMessage("msgd",NULL);

	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		name = fl->name;
	} else {
		name = NULL;
	}
	InitSystem();
	ExecuteServer(name);
	return	(0);
}
