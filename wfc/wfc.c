/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
#define __FD_SETSIZE	8192
#define SOMAXCONN		1024

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
#include	"dbgroup.h"
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
		    const sigset_t *__restrict __sigmask);
#endif

#ifdef	DEBUG
extern	void
DumpNode(
	SessionData	*data)
{
ENTER_FUNC;
	printf("window   = [%s]\n",data->hdr->window);
	printf("widget   = [%s]\n",data->hdr->widget);
	printf("event    = [%s]\n",data->hdr->event);
	printf("term     = [%s]\n",data->hdr->term);
	printf("user     = [%s]\n",data->hdr->user);
LEAVE_FUNC;
}
#endif

extern	LargeByteString	*
GetScreenData(
	SessionData	*data,
	char		*name)
{
	LargeByteString	*scrdata;
	RecordStruct	*rec;

ENTER_FUNC;
	if		(  ( scrdata = (LargeByteString *)g_hash_table_lookup(data->scrpool,name) )
			   ==  NULL  ) {
		if		(  ( rec = GetWindow(name) )  !=  NULL  ) {
			scrdata = NewLBS();
			LBS_ReserveSize(scrdata,
							NativeSizeValue(NULL,rec->value),FALSE);
			NativePackValue(NULL,LBS_Body(scrdata),rec->value);
			g_hash_table_insert(data->scrpool,StrDup(name),scrdata);
		} else {
			scrdata = NULL;
		}
	}
LEAVE_FUNC;
	return	(scrdata);
}


extern	void
ChangeLD(
	SessionData	*data,
	LD_Node		*newld)
{
	int		i;
	LargeByteString	**scrdata;

ENTER_FUNC;
	if		(  newld  !=  NULL  ) {
		if		(  ( data->spa = g_hash_table_lookup(data->spadata,newld->info->name) )
				   ==  NULL  ) {
			if		(  newld->info->sparec  !=  NULL  ) {
				data->spa = NewLBS();
				g_hash_table_insert(data->spadata,StrDup(newld->info->name),data->spa);
				LBS_ReserveSize(data->spa,
								NativeSizeValue(NULL,newld->info->sparec->value),FALSE);
				NativePackValue(NULL,LBS_Body(data->spa),newld->info->sparec->value);
			}
		}
		if		(  data->scrdata  !=  NULL  ) {
			xfree(data->scrdata);
		}
		scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
											  * newld->info->cWindow);
		for	( i = 0 ; i < newld->info->cWindow ; i ++ ) {
			scrdata[i] = GetScreenData(data,newld->info->windows[i]->name);
		}
		data->cWindow = newld->info->cWindow;
		data->scrdata = scrdata;
	}
	data->ld = newld;
	data->apsid = -1;
LEAVE_FUNC;
}

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
	if		(  ControlPortNumber  ==  NULL  ) {
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
		"waiting port name"	 							},
	{	"apsport",	STRING,		TRUE,	(void*)&ApsPortNumber,
		"APS port name"									},
	{	"control",	STRING,		TRUE,	(void*)&ControlPortNumber,
		"control port name"								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"connection waiting queue number"				},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"base directory"			 					},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"record directory"			 					},
	{	"ddir",		STRING,		TRUE,	(void*)&D_Dir,
		"LD directory"				 					},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"environment file name"							},
	{	"sesdir",	STRING,		TRUE,	(void*)&SesDir,
		"session variable keep directory" 				},

	{	"retry",	INTEGER,	TRUE,	(void*)&MaxTransactionRetry,
		"maximun retry count"							},
	{	"cache",	INTEGER,	TRUE,	(void*)&nCache,
		"terminal cache number"							},

	{	"loopback",	BOOLEAN,	TRUE,	(void*)&fLoopBack,
		"loopback test"									},
	{	"timer",	BOOLEAN,	TRUE,	(void*)&fTimer,
		"timer"											},

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
	MaxTransactionRetry = 0;
	ControlPort = NULL;
	fLoopBack = FALSE;
	nCache = 100;
	SesDir = NULL;
LEAVE_FUNC;
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
	(void)signal(SIGUSR1,(void *)StopSystem);
	(void)signal(SIGUSR2, SIG_IGN);
	InitMessage("wfc",NULL);
ENTER_FUNC;

	SetDefault();
	GetOption(option,argc,argv,NULL);

	InitSystem();
	Message("wfc server start");
	ExecuteServer();
	Message("wfc server end");
	CleanUp();
	rc = 0;
LEAVE_FUNC;
	return	(rc);
}
