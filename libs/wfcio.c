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
#include	<unistd.h>

#include	"types.h"
#include	"const.h"

#include	"libmondai.h"
#include	"comm.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"debug.h"
#include	"socket.h"

static	LargeByteString	*buff;

extern	NETFILE	*
ConnectTermServer(
	char	*url,
	char	*term,
	char	*user,
	Bool	fKeep,
	char	*arg)
{
	int		fh;
	Port	*port;
	NETFILE	*fp;

dbgmsg(">ConnectTermServer");
	port = ParPort(url,PORT_WFC);
#ifdef	DEBUG
	printf("host = [%s]\n",port->host);
	printf("port = [%s]\n",port->port);
	fflush(stdout);
#endif
	fh = ConnectIP_Socket(port->port,SOCK_STREAM,port->host);
	DestroyPort(port);
	fp = SocketToNet(fh);
	if		(  fKeep  ) {
		SendPacketClass(fp,WFC_TRUE);
	} else {
		SendPacketClass(fp,WFC_FALSE);
	}		
	SendString(fp,term);
	SendString(fp,user);
	SendString(fp,arg);
	buff = NewLBS();
dbgmsg("<ConnectTermServer");
	return	(fp); 
}
#if	0
static	Bool
RequestBLOB(
	NETFILE	*fp,
	PacketClass	op)
{
	Bool	rc;

	rc = FALSE;
	SendPacketClass(fp,WFC_BLOB);		ON_IO_ERROR(fp,badio);
	SendPacketClass(fp,op);				ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:
	return	(rc);
}

extern	Bool
NewBLOB(
	NETFILE	*fp,
	int		mode,
	MonObjectType	*obj)
{
	Bool	rc;

	rc = FALSE;
	RequestBLOB(BLOB_CREATE);			ON_IO_ERROR(fp,badio);
	SendInt(fp,mode);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_OK  ) {
		RecvObject(fp,obj);				ON_IO_ERROR(fp,badio);
		rc = TRUE;
	}
  badio:
	return	(rc);
}

extern	Bool
OpenBLOB(
	NETFILE	*fp,
	int		mode,
	MonObjectType	*obj)
{
	Bool	rc;
	
	rc = FALSE;
	RequestBLOB(BLOB_OPEN);				ON_IO_ERROR(fp,badio);
	SendInt(fp,mode);					ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_OK  ) {
		rc = TRUE;
	}
  badio:
	return	(rc);
}

extern	size_t
WriteBLOB(
	NETFILE	*fp,
	MonObjectType	*obj,
	byte	*buff,
	size_t	size)
{
	size_t	wrote;
	
	wrote = 0;
	RequestBLOB(BLOB_WRITE);			ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	SendLength(fp,size);				ON_IO_ERROR(fp,badio);
	if		(  size  >  0  ) {
		Send(fp,buff,size);					ON_IO_ERROR(fp,badio);
		wrote = RecvLength(fp);				ON_IO_ERROR(fp,badio);
	}
  badio:
	return	(wrote);
}

extern	size_t
ReadBLOB(
	NETFILE	*fp,
	MonObjectType	*obj,
	byte	*buff,
	size_t	size)
{
	size_t	red;
	
	red = 0;
	RequestBLOB(BLOB_READ);				ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	SendLength(fp,size);				ON_IO_ERROR(fp,badio);
	if		(  size  >  0  ) {
		Recv(fp,buff,size);					ON_IO_ERROR(fp,badio);
		red = RecvLength(fp);				ON_IO_ERROR(fp,badio);
	}
  badio:
	return	(red);
}

extern	Bool
CloseBLOB(
	NETFILE	*fp,
	MonObjectType	*obj)
{
	Bool	rc;
	
	rc = FALSE;
	RequestBLOB(BLOB_CLOSE);			ON_IO_ERROR(fp,badio);
	SendObject(fp,obj);					ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_OK  ) {
		rc = TRUE;
	}
  badio:
	return	(rc);
}
#endif

extern	Bool
SendTermServer(
	NETFILE	*fp,
	char	*window,
	char	*widget,
	char	*event,
	ValueStruct	*value)
{
	size_t	size;
	Bool	rc;

dbgmsg(">SendTermServer");
	size = NativeSizeValue(NULL,value);
	LBS_ReserveSize(buff,size,FALSE);
	NativePackValue(NULL,LBS_Body(buff),value);
	SendPacketClass(fp,WFC_PING);
	dbgmsg("send PING");
	if		(  RecvPacketClass(fp)  ==  WFC_PONG  ) {
		dbgmsg("recv PONG");
		SendPacketClass(fp,WFC_DATA);
		dbgmsg("send DATA");
		SendString(fp,window);
		SendString(fp,widget);
		SendString(fp,event);
		if		(  RecvPacketClass(fp)  ==  WFC_OK  ) {
			dbgmsg("recv OK");
			SendLBS(fp,buff);
			SendPacketClass(fp,WFC_OK);
			rc = TRUE;
		} else {
			/*	window not found	*/
			rc = FALSE;
		}
	} else {
		rc = FALSE;
	}
dbgmsg("<SendTermServer");
	return	(rc); 
}

extern	Bool
RecvTermServerHeader(
	NETFILE	*fp,
	char	*user,
	char	*window,
	char	*widget,
	int		*type,
	WindowControl	*ctl)
{
	Bool	rc;
	PacketClass	c;
	int		i;

dbgmsg(">RecvTermServerHeader");
  top: 
	switch	(c = RecvPacketClass(fp)) {
	  case	WFC_HEADER:
		dbgmsg("recv HEADER");
		RecvString(fp,user);
		RecvString(fp,window);
		RecvString(fp,widget);
#if	1
		*type = TO_INT(RecvChar(fp));
#endif
		ctl->n = RecvInt(fp);
		for	( i = 0 ; i < ctl->n ; i ++ ) {
			ctl->control[i].PutType = (byte)RecvInt(fp);
			RecvString(fp,ctl->control[i].window);
		}
		rc = TRUE;
		break;
	  case	WFC_PING:
		dbgmsg("recv PING");
		SendPacketClass(fp,WFC_PONG);
		dbgmsg("send PONG");
		goto	top;
		break;
	  default:
		dbgmsg("recv default");
		SendPacketClass(fp,WFC_NOT);
		dbgmsg("send NOT");
		rc = FALSE;
		break;
	}
dbgmsg("<RecvTermServerHeader");
	return	(rc);
}

static	void
_RecvWindow(
	char		*wname,
	WindowData	*win,
	NETFILE		*fp)
{
	switch	(win->PutType) {
	  case	SCREEN_NULL:
	  case	SCREEN_CLOSE_WINDOW:
		break;
	  default:
		SendPacketClass(fp,WFC_DATA);
		SendString(fp,wname);
		if		(  RecvPacketClass(fp)  ==  WFC_OK  ) {
			RecvLBS(fp,buff);
			NativeUnPackValue(NULL,LBS_Body(buff),win->rec->value);
		}
	}
}

extern	void
RecvTermServerData(
	NETFILE		*fp,
	ScreenData	*scr)
{
	WindowData	*win;
dbgmsg(">RecvTermServerData");
	g_hash_table_foreach(scr->Windows,(GHFunc)_RecvWindow,fp);
	SendPacketClass(fp,WFC_OK);
dbgmsg("<RecvTermServerData");
}

