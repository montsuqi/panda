/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2004 Ogochan & JMA (Japan Medical Association).

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
#include	"comms.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"blobreq.h"
#include	"front.h"
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

ENTER_FUNC;
	port = ParPort(url,PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if ( fd > 0 ){
		fp = SocketToNet(fd);
		if		(  fKeep  ) {
			SendPacketClass(fp,WFC_TRUE);
		} else {
			SendPacketClass(fp,WFC_FALSE);
		}		
		SendStringDelim(fp,term);
		SendStringDelim(fp,"\n");
		SendStringDelim(fp,user);
		SendStringDelim(fp,"\n");
		SendStringDelim(fp,arg);
		SendStringDelim(fp,"\n");
	} else {
		Warning("can not connect wfc server");
		fp = NULL;
	}
	buff = NewLBS();
LEAVE_FUNC;
	return	(fp); 
}

static	void
_ForwardBLOB(
	NETFILE		*fp,
	ValueStruct	*value)
{
	int		i;

ENTER_FUNC;
	if		(  value  ==  NULL  )	return;
	if		(  IS_VALUE_NIL(value)  )	return;
	switch	(ValueType(value)) {
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			_ForwardBLOB(fp,ValueArrayItem(value,i));
		}
		break;
	  case	GL_TYPE_VALUES:
		for	( i = 0 ; i < ValueValuesSize(value) ; i ++ ) {
			_ForwardBLOB(fp,ValueValuesItem(value,i));
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			_ForwardBLOB(fp,ValueRecordItem(value,i));
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
		if		(  IS_OBJECT_NULL(ValueObject(value))  ) {
			ValueObject(value) = RequestImportBLOB(fp,WFC_BLOB,BlobCacheFileName(value));
		} else {
			RequestSaveBLOB(fp,WFC_BLOB,ValueObject(value),BlobCacheFileName(value));
		}
		break;
	  case	GL_TYPE_ALIAS:
	  default:
		break;
	}
LEAVE_FUNC;
}

static	void
ForwardBLOB(
	NETFILE		*fp,
	ValueStruct	*value)
{
	_ForwardBLOB(fp,value);
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

ENTER_FUNC;
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
			ForwardBLOB(fp,value);
			SendPacketClass(fp,WFC_OK);
			rc = TRUE;
		} else {
			/*	window not found	*/
			rc = FALSE;
		}
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
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

ENTER_FUNC;
  top: 
	switch	(c = RecvPacketClass(fp)) {
	  case	WFC_HEADER:
		dbgmsg("recv HEADER");
		RecvnString(fp, SIZE_NAME+1, user);
		RecvnString(fp, SIZE_NAME+1, window);
		RecvnString(fp, SIZE_NAME+1, widget);
		*type = TO_INT(RecvChar(fp));
		ctl->n = RecvInt(fp);
		for	( i = 0 ; i < ctl->n ; i ++ ) {
			ctl->control[i].PutType = (byte)RecvInt(fp);
			RecvnString(fp, SIZE_NAME+1, ctl->control[i].window);
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
LEAVE_FUNC;
	return	(rc);
}

static	void
_FeedBLOB(
	NETFILE		*fp,
	ValueStruct	*value)
{
	int		i;

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
		ValueIsNonNil(value);
		if		(  IS_OBJECT_NULL(ValueObject(value))  ) {
			ValueObject(value) = RequestNewBLOB(fp,WFC_BLOB,BLOB_OPEN_WRITE);
			RequestCloseBLOB(fp,WFC_BLOB,ValueObject(value));
		} else {
			RequestExportBLOB(fp,WFC_BLOB,ValueObject(value),BlobCacheFileName(value));
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
ENTER_FUNC;
	g_hash_table_foreach(scr->Windows,(GHFunc)_RecvWindow,fp);
	SendPacketClass(fp,WFC_DONE);
LEAVE_FUNC;
}

