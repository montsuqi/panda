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
