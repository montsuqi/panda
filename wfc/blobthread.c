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
#include	"comms.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"auth.h"
#include	"authstub.h"
#include	"blobthread.h"
#include	"blobcom.h"
#include	"message.h"
#include	"debug.h"

static	Bool
InitSession(
	NETFILE	*fp)
{
	char	user[SIZE_NAME+1]
		,	pass[SIZE_NAME+1];
	char	msg[SIZE_BUFF];
	Bool	rc;

ENTER_FUNC;
	RecvStringDelim(fp,SIZE_NAME,user);		ON_IO_ERROR(fp,badio);
	RecvStringDelim(fp,SIZE_NAME,pass);		ON_IO_ERROR(fp,badio);
	dbgprintf("user = [%s]\n",user);
	dbgprintf("pass = [%s]\n",pass);
	rc = FALSE;
	if		(  AuthUser(ThisEnv->blob->auth,user,pass,NULL)  ) {
		SendPacketClass(fp,APS_OK);
		sprintf(msg,"[%s] Native BLOB connect",user);
		MessageLog(msg);
		rc = TRUE;
	} else {
		sprintf(msg,"[%s] Native BLOB auth error.",user);
		MessageLog(msg);
	  badio:
		SendPacketClass(fp,APS_NOT);
	}
LEAVE_FUNC;
	return	(rc);
}

static	void
BlobThread(
	void	*para)
{
	int		fhBlob = (int)para;
	NETFILE	*fp;

ENTER_FUNC;

	fp = SocketToNet(fhBlob);
	
	if		(  InitSession(fp)  ) {
		while	(  RecvChar(fp)  ==  APS_BLOB  ) {
			PassiveBLOB(fp,Blob);		ON_IO_ERROR(fp,badio);
		}
	}
  badio:
	CloseNet(fp);
LEAVE_FUNC;
	pthread_exit(NULL);
}

extern	pthread_t
ConnectBlob(
	int		_fhBlob)
{
	int		fhBlob;
	pthread_t	thr;

ENTER_FUNC;
	if		(  ( fhBlob = accept(_fhBlob,0,0) )  <  0  )	{
		MessagePrintf("_fhBlob = %d INET Domain Accept",_fhBlob);
		exit(1);
	}
	pthread_create(&thr,NULL,(void *(*)(void *))BlobThread,(void *)fhBlob);
LEAVE_FUNC;
	return	(thr); 
}

extern	void
InitBlob(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}
