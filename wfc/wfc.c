/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>		/*	for	mknod	*/
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"value.h"
#include	"net.h"
#include	"comm.h"
#include	"dirs.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"mqthread.h"
#include	"corethread.h"
#include	"termthread.h"
#include	"controlthread.h"
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
static	char	*BLOB_Dir;

static	Port	*ApsPort;
static	Port	*WfcPort;
static	Port	*ControlPort;

static	sigset_t SigMask;

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
	struct	timeval	timeout;

dbgmsg(">ExecuteServer");
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
		timeout.tv_usec = 0;
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
            perror("select");
            exit(1);
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
dbgmsg("<ExecuteServer");
}


static	void
InitSystem(void)
{
ENTER_FUNC;
	InitDirectory();
	SetUpDirectory(Directory,NULL,"","");
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
	} else {
		Blob = NULL;
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
	FinishBLOB(Blob);
}

static	void
StopSystem(void)
{
	fShutdown = TRUE;
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

	{	NULL,		0,			TRUE,	NULL		 	}
};

static	void
SetDefault(void)
{
	PortNumber = NULL;
	ApsPortNumber = NULL;
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	MaxRetry = 0;
	ControlPort = NULL;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	int			rc;
    sigset_t sigmask;

    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigmask, &SigMask);

	(void)signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1,(void *)StopSystem);

	SetDefault();
	GetOption(option,argc,argv);
	InitMessage("wfc",NULL);

	InitSystem();
	ExecuteServer();
	CleanUp();
	rc = 0;
	return	(rc);
}
