/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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
#include	"blobcom.h"
#include	"blobserv.h"
#include	"keyvaluecom.h"
#include	"kvserv.h"
#include	"message.h"
#include	"debug.h"
#define 	MAIN
#include	"sysdatathread.h"

static	void
SysDataThread(
	void	*para)
{
	int		fhSysData = (int)(long)para;
	NETFILE	*fp;
	PacketClass c;
	Bool fLoop;

ENTER_FUNC;
	fp = SocketToNet(fhSysData);
	fLoop = TRUE;
	while	(  fLoop  ) {
		c = RecvChar(fp);	ON_IO_ERROR(fp,badio);
		switch(c) {
		case SYSDATA_BLOB:
			dbgmsg("Call PassiveBLOB");
			PassiveBLOB(fp,BlobState);	ON_IO_ERROR(fp,badio);
			break;
		case SYSDATA_KV:
			dbgmsg("Call PassiveKV");
			PassiveKV(fp,KVState);		ON_IO_ERROR(fp,badio);
			break;
		case SYSDATA_END:
			dbgmsg("SYSDATA END");
			fLoop = FALSE;
			break;
		default:
			dbgprintf("invalid packet[%d]",(int)c);
			fLoop = FALSE;
			break;
		}
	}
	CloseNet(fp);
  badio:
LEAVE_FUNC;
}

extern	pthread_t
ConnectSysData(
	int		_fhSysData)
{
	int		fhSysData;
	pthread_t	thr;

ENTER_FUNC;
	if		(  ( fhSysData = accept(_fhSysData,0,0) )  <  0  )	{
		Message("_fhSysData = %d INET Domain Accept",_fhSysData);
		dbgprintf("_fhSysData = %d INET Domain Accept",_fhSysData);
		exit(1);
	}
	pthread_create(&thr,NULL,(void *(*)(void *))SysDataThread,(void *)(long)fhSysData);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(thr); 
}

extern	void
InitSysData(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}
