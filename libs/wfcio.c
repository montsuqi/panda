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
#include	"wfc.h"
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
	fh = ConnectSocket(port->port,SOCK_STREAM,port->host);
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
	NativePackValue(NULL,buff->body,value);
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
	CloseWindows	*cls)
{
	Bool	rc;
	PacketClass	c;
	int		i;

dbgmsg(">RecvTermServerHeader");
  top: 
	switch	(c = RecvPacketClass(fp)) {
	  case	WFC_DATA:
		dbgmsg("recv DATA");
		RecvString(fp,user);
		RecvString(fp,window);
		RecvString(fp,widget);
		*type = TO_INT(RecvChar(fp));
		cls->n = RecvInt(fp);
		for	( i = 0 ; i < cls->n ; i ++ ) {
			RecvString(fp,cls->close[i].window);
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

extern	void
RecvTermServerData(
	NETFILE	*fp,
	WindowData	*win)
{
dbgmsg(">RecvTermServerData");
	RecvLBS(fp,buff);
	NativeUnPackValue(NULL,LBS_Body(buff),win->rec->value);
dbgmsg("<RecvTermServerData");
}
