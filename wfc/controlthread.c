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
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"
#include	"enum.h"

#include	"libmondai.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"queue.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"wfc.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"controlthread.h"
#include	"message.h"
#include	"debug.h"

static	void
InitSession(
	NETFILE	*fp)
{
ENTER_FUNC;
	MessageLog("control session start");
LEAVE_FUNC;
}

static	void
FinishSession(void)
{
ENTER_FUNC;
	MessageLog("control session end");
LEAVE_FUNC;
}

static	void
ControlThread(
	void	*para)
{
	NETFILE	*fp;
	Bool	fExit;

ENTER_FUNC;
	fp = SocketToNet((int)para);
	
	InitSession(fp);
	fExit = FALSE;
	while	(!fExit) {
		switch	(RecvPacketClass(fp)) {
		  case	WFCCONTROL_STOP:
			fShutdown = TRUE;
			fExit = TRUE;
			break;
		  case	WFCCONTROL_END:
			fExit = TRUE;
			break;
		  default:
			break;
		}
		ON_IO_ERROR(fp,badio);
	}
  badio:
	FinishSession();
	CloseNet(fp);
LEAVE_FUNC;
	pthread_exit(NULL);
}

extern	pthread_t
ConnectControl(
	int		_fhControl)
{
	int		fhControl;
	pthread_t	thr;

ENTER_FUNC;
	if		(  ( fhControl = accept(_fhControl,0,0) )  <  0  )	{
		MessagePrintf("_fhControl = %d INET Domain Accept",_fhControl);
		exit(1);
	}
	pthread_create(&thr,NULL,(void *(*)(void *))ControlThread,(void *)fhControl);
LEAVE_FUNC;
	return	(thr); 
}

extern	void
InitControl(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}
