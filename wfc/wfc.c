/*
PANDA -- a simple transaction monitor
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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

/*
  TODO
	wfc間通信
*/

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <sys/select.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>		/*	for	mknod	*/
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"libmondai.h"
#include	"RecParser.h"
#include	"net.h"
#include	"comm.h"
#include	"dirs.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"mqthread.h"
#include	"corethread.h"
#include	"termthread.h"
#include	"controlthread.h"
#include	"blobthread.h"
#include	"blob.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

#include	"socket.h"
#include	"queue.h"

#include	"directory.h"

static	char	*ApsPortNumber;
static	char	*PortNumber;
static	char	*ControlPortNumber;
static	int		Back;
static	char	*Directory;

static	Port	*ApsPort;
static	Port	*WfcPort;
static	Port	*ControlPort;
static	BLOB_Space	*Blob;

static	sigset_t SigMask;

#ifndef __USE_XOPEN2K
extern int pselect (int __nfds, fd_set *__restrict __readfds,
		    fd_set *__restrict __writefds,
		    fd_set *__restrict __exceptfds,
		    const struct timespec *__restrict __timeout,
		    const __sigset_t *__restrict __sigmask);
#endif

#ifdef	DEBUG
extern	void
DumpNode(
	SessionData	*data)
{
dbgmsg(">DumpNode");
	printf("window   = [%s]\n",data->hdr->window);
	printf("widget   = [%s]\n",data->hdr->widget);
	printf("event    = [%s]\n",data->hdr->event);
	printf("term     = [%s]\n",data->hdr->term);
	printf("user     = [%s]\n",data->hdr->user);
dbgmsg("<DumpNode");
}
#endif

extern	void
ExecuteServer(void)
{
	int		_fhTerm
		,	_fhAps
		,	_fhControl
		,	_fhBlob;
	fd_set	ready;
	int		maxfd;
    int		ret;
	struct	timespec	timeout;

ENTER_FUNC;
	_fhTerm = InitServerPort(WfcPort,Back);
	maxfd = _fhTerm;
	_fhAps = InitServerPort(ApsPort,Back);
	maxfd = maxfd < _fhAps ? _fhAps : maxfd;
	if		(  ControlPort  !=  NULL  ) {
		_fhControl = InitServerPort(ControlPort,Back);
		maxfd = maxfd < _fhControl ? _fhControl : maxfd;
	} else {
		_fhControl = -1;
	}
	if		(	(  ThisEnv->blob        !=  NULL  )
			&&	(  ThisEnv->blob->port  !=  NULL  )
			&&	(  ThisEnv->blob->auth  !=  NULL  ) ) {
		_fhBlob = InitServerPort(ThisEnv->blob->port,Back);
		maxfd = maxfd < _fhBlob ? _fhBlob : maxfd;
	} else {
		_fhBlob = -1;
	}
	fShutdown = FALSE;
	do {
		timeout.tv_sec = 1;
		timeout.tv_nsec = 0;
		FD_ZERO(&ready);
		FD_SET(_fhTerm,&ready);
		FD_SET(_fhAps,&ready);
		if		(  _fhControl  >=  0  ) {
			FD_SET(_fhControl,&ready);
		}
		if		(  _fhBlob  >=  0  ) {
			FD_SET(_fhBlob,&ready);
		}
		ret = pselect(maxfd+1,&ready,NULL,NULL,&timeout,&SigMask);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            Error("select: ", strerror(errno));
        }
		if		(  FD_ISSET(_fhTerm,&ready)  ) {		/*	term connect	*/
			ConnectTerm(_fhTerm);
		}
		if		(  FD_ISSET(_fhAps,&ready)  ) {			/*	APS connect		*/
			ConnectAPS(_fhAps);
		}
		if		(	(  _fhControl  >=  0  )
				&&	(  FD_ISSET(_fhControl,&ready)  ) ) {	/*	control connect		*/
			ConnectControl(_fhControl);
		}
		if		(	(  _fhBlob  >=  0  )
				&&	(  FD_ISSET(_fhBlob,&ready)  ) ) {		/*	blob connect		*/
			ConnectBlob(_fhBlob);
		}
	}	while	(!fShutdown);
LEAVE_FUNC;
}


static	void
InitSystem(void)
{
ENTER_FUNC;
	InitDirectory();
	SetUpDirectory(Directory,NULL,"","",TRUE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
	if		(  ApsPortNumber  ==  NULL  ) {
		ApsPort = ThisEnv->WfcApsPort;
	} else {
		ApsPort = ParPortName(ApsPortNumber);
	}
	if		(  PortNumber  ==  NULL  ) {
		WfcPort = ThisEnv->TermPort;
	} else {
		WfcPort = ParPortName(PortNumber);
	}
	if		(  ControlPort  ==  NULL  ) {
		ControlPort = ThisEnv->ControlPort;
	} else {
		ControlPort = ParPortName(ControlPortNumber);
	}
	InitNET();
	if		(	(  ThisEnv->blob       !=  NULL  )
			&&	(  ThisEnv->blob->dir  !=  NULL  ) ) {
		Blob = InitBLOB(ThisEnv->blob->dir);
		BlobState = ConnectBLOB(Blob);
	} else {
		Blob = NULL;
		BlobState = NULL;
	}
	InitMessageQueue();
	ReadyAPS();
	SetupMessageQueue();
	InitTerm();
	StartCoreThread();
	InitControl();
LEAVE_FUNC;
}

static	void
CleanUp(void)
{
	if (BlobState != NULL) {
		DisConnectBLOB(BlobState);
		FinishBLOB(Blob);
	}
}

static	void
StopSystem(void)
{
	fShutdown = TRUE;
	Message("receive stop signal");
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"apsport",	STRING,		TRUE,	(void*)&ApsPortNumber,
		"APS接続待ちポート"								},
	{	"control",	STRING,		TRUE,	(void*)&ControlPortNumber,
		"制御待ちポート"								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"lddir",	STRING,		TRUE,	(void*)&D_Dir,
		"LD定義格納ディレクトリ"		 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},

	{	"retry",	INTEGER,	TRUE,	(void*)&MaxRetry,
		"トランザクションを再試行する時の上限数"		},

	{	"loopback",	BOOLEAN,	TRUE,	(void*)&fLoopBack,
		"loopback test"									},

	{	NULL,		0,			TRUE,	NULL		 	}
};

static	void
SetDefault(void)
{
ENTER_FUNC;
	PortNumber = NULL;
	ApsPortNumber = NULL;
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	MaxRetry = 0;
	ControlPort = NULL;
	fLoopBack = FALSE;
LEAVE_FUNC;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	int			rc;
    sigset_t sigmask;

ENTER_FUNC;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigmask, &SigMask);

	(void)signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1,(void *)StopSystem);
	InitMessage("wfc",NULL);

	SetDefault();
	GetOption(option,argc,argv);

	InitSystem();
	Message("wfc server start");
	ExecuteServer();
	Message("wfc server end");
	CleanUp();
	rc = 0;
LEAVE_FUNC;
	return	(rc);
}
