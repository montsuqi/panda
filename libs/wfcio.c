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
	int		fd;
	Port	*port;
	NETFILE	*fp;

dbgmsg(">ConnectTermServer");
	port = ParPort(url,PORT_WFC);
#ifdef	DEBUG
	printf("host = [%s]\n",port->host);
	printf("port = [%s]\n",port->port);
	fflush(stdout);
#endif
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	fp = SocketToNet(fd);
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
		*type = TO_INT(RecvChar(fp));
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

static	char	*
CacheFileName(
	ValueStruct	*value)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"%s/%s",CacheDir,ValueToString(value,NULL));
	return	(buf);
}


static	void
_FeedBLOB(
	NETFILE		*fp,
	ValueStruct	*value)
{
	int		i;
	FILE	*fpBLOB;

ENTER_FUNC;
	if		(  value  ==  NULL  )	return;
	if		(  IS_VALUE_NIL(value)  )	return;
	switch	(ValueType(value)) {
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			_FeedBLOB(fp,ValueArrayItem(value,i));
		}
		break;
	  case	GL_TYPE_VALUES:
		for	( i = 0 ; i < ValueValuesSize(value) ; i ++ ) {
			_FeedBLOB(fp,ValueValuesItem(value,i));
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			_FeedBLOB(fp,ValueRecordItem(value,i));
		}
		break;
	  case	GL_TYPE_INT:
	  case	GL_TYPE_FLOAT:
	  case	GL_TYPE_BOOL:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_NUMBER:
		break;
	  case	GL_TYPE_OBJECT:
		SendPacketClass(fp,WFC_BLOB);
		SendObject(fp,ValueObject(value));
		if		(  RecvPacketClass(fp)  ==  WFC_OK  ) {
			RecvLBS(fp,buff);
			if		(  ( fpBLOB = Fopen(CacheFileName(value),"w") )  !=  NULL  ) {
				fwrite(LBS_Body(buff),LBS_Size(buff),1,fpBLOB);
				fclose(fpBLOB);
			}
		}
		break;
	  case	GL_TYPE_ALIAS:
	  default:
		break;
	}
LEAVE_FUNC;
}

static	void
FeedBLOB(
	NETFILE		*fp,
	ValueStruct	*value)
{
	_FeedBLOB(fp,value);
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
			FeedBLOB(fp,win->rec->value);
		}
	}
}

extern	void
RecvTermServerData(
	NETFILE		*fp,
	ScreenData	*scr)
{
dbgmsg(">RecvTermServerData");
	g_hash_table_foreach(scr->Windows,(GHFunc)_RecvWindow,fp);
	SendPacketClass(fp,WFC_DONE);
dbgmsg("<RecvTermServerData");
}

