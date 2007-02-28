/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2006 Ogochan.
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
#include	"blobserv.h"
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
	rc = FALSE;
	RecvStringDelim(fp,SIZE_NAME,user);		ON_IO_ERROR(fp,badio);
	RecvStringDelim(fp,SIZE_NAME,pass);		ON_IO_ERROR(fp,badio);
	dbgprintf("user = [%s]\n",user);
	dbgprintf("pass = [%s]\n",pass);
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
	int		fhBlob = (int)(long)para;
	NETFILE	*fp;

ENTER_FUNC;

	fp = SocketToNet(fhBlob);
	
	if		(  InitSession(fp)  ) {
		while	(  RecvChar(fp)  ==  APS_BLOB  ) {
			PassiveBLOB(fp,BlobState);		ON_IO_ERROR(fp,badio);
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
		Message("_fhBlob = %d INET Domain Accept",_fhBlob);
		exit(1);
	}
	pthread_create(&thr,NULL,(void *(*)(void *))BlobThread,(void *)(long)fhBlob);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(thr); 
}

extern	void
InitBlob(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}
