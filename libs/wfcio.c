/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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
		if		(  IS_OBJECT_NULL(ValueObjectId(value))  ) {
			ValueObjectId(value) = RequestImportBLOB(fp,WFC_BLOB,BlobCacheFileName(value));
		} else {
			RequestSaveBLOB(fp,WFC_BLOB,ValueObjectId(value),BlobCacheFileName(value));
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

static	Bool
_SendTermServer(
	NETFILE	*fp,
	char	*window,
	char	*widget,
	char	*event,
	ValueStruct	*value)
{
	size_t	size;
	Bool	rc;
	LargeByteString	*buff;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_DATA);	ON_IO_ERROR(fp,badio);
	dbgmsg("send DATA");
	dbgprintf("window = [%s]",window);
	SendString(fp,window);			ON_IO_ERROR(fp,badio);
	dbgprintf("widget = [%s]",widget);
	SendString(fp,widget);			ON_IO_ERROR(fp,badio);
	dbgprintf("event  = [%s]",event);
	SendString(fp,event);			ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_OK  ) {
		ON_IO_ERROR(fp,badio);
		dbgmsg("recv OK");
		if		(  value  !=  NULL  ) {
			SendPacketClass(fp,WFC_DATA);		ON_IO_ERROR(fp,badio);
			size = NativeSizeValue(NULL,value);
			buff = NewLBS();
			LBS_ReserveSize(buff,size,FALSE);
			NativePackValue(NULL,LBS_Body(buff),value);
			SendLBS(fp,buff);				ON_IO_ERROR(fp,badio);
			FreeLBS(buff);
			ForwardBLOB(fp,value);			ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,WFC_NODATA);	ON_IO_ERROR(fp,badio);
		}
		SendPacketClass(fp,WFC_OK);		ON_IO_ERROR(fp,badio);
		rc = TRUE;
	}
  badio:
LEAVE_FUNC;
	return	(rc); 
}

extern	NETFILE	*
ConnectTermServer(
	char	*url,
	char	*term,
	char	*user,
	char	*window,
	Bool	fKeep,
	char	*arg)
{
	int		fd;
	Port	*port;
	NETFILE	*fp;
	PacketClass	klass;
	RecordStruct	*rec;

ENTER_FUNC;
	port = ParPort(url,PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if ( fd > 0 ){
		fp = SocketToNet(fd);
		SendString(fp,term);		ON_IO_ERROR(fp,badio);
		if		(  fKeep  ) {
			SendPacketClass(fp,WFC_TRUE);
		} else {
			SendPacketClass(fp,WFC_FALSE);
		}		
		SendString(fp,user);		ON_IO_ERROR(fp,badio);
		SendString(fp,arg);			ON_IO_ERROR(fp,badio);
		klass = RecvPacketClass(fp);		ON_IO_ERROR(fp,badio);
		if		(  klass  !=  WFC_OK  ) {
			CloseNet(fp);
			fp = NULL;
		} else {
			dbgprintf("[%s]",window);
			dbgprintf("[%p %p]",ThisScreen, ThisScreen->Records);
			if		(  ( rec = GetWindowRecord(window) )  !=  NULL  ) {
				_SendTermServer(fp,window,"","",rec->value);
			} else {
				dbgmsg("*");
				SendPacketClass(fp,WFC_OK);
			}
			dbgmsg("*");
		}
	} else {
	  badio:
		Warning("can not connect wfc server");
		fp = NULL;
	}
LEAVE_FUNC;
	return	(fp); 
}

extern	Bool
SendTermServer(
	NETFILE	*fp,
	char	*term,
	char	*window,
	char	*widget,
	char	*event,
	ValueStruct	*value)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	dbgprintf("term = [%s]",term);
	SendString(fp,term);				ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		dbgmsg("recv PONG");
		rc = _SendTermServer(fp,window,widget,event,value);
	}
  badio:
LEAVE_FUNC;
	return	(rc); 
}

extern	Bool
SendTermServerEnd(
	NETFILE	*fp,
	char	*term)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendString(fp,term);				ON_IO_ERROR(fp,badio);
	SendPacketClass(fp,WFC_PING);		ON_IO_ERROR(fp,badio);
	dbgmsg("send PING");
	if		(  RecvPacketClass(fp)  ==  WFC_PONG  ) {
		dbgmsg("recv PONG");
		SendPacketClass(fp,WFC_END);	ON_IO_ERROR(fp,badio);
		rc = TRUE;
	}
  badio:
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
	rc = FALSE;
	switch	(c = RecvPacketClass(fp)) {
	  case	WFC_HEADER:
		dbgmsg(">recv HEADER");
		RecvnString(fp, SIZE_NAME+1, user);		ON_IO_ERROR(fp,badio);
		RecvnString(fp, SIZE_NAME+1, window);	ON_IO_ERROR(fp,badio);
		RecvnString(fp, SIZE_NAME+1, widget);	ON_IO_ERROR(fp,badio);
		dbgprintf("window = [%s]",window);
		*type = RecvChar(fp);					ON_IO_ERROR(fp,badio);
		ctl->n = RecvInt(fp);					ON_IO_ERROR(fp,badio);
		dbgprintf("ctl->n = %d\n",ctl->n);
		for	( i = 0 ; i < ctl->n ; i ++ ) {
			ctl->control[i].PutType = (byte)RecvInt(fp);
			ON_IO_ERROR(fp,badio);
			RecvnString(fp, SIZE_NAME+1, ctl->control[i].window);
			ON_IO_ERROR(fp,badio);
			dbgprintf("wname = [%s]\n",ctl->control[i].window);
		}
		rc = TRUE;
		dbgmsg("<recv HEADER");
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
		break;
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}

static	void
_FeedBLOB(
	NETFILE		*fp,
	ValueStruct	*value)
{
	int		i;

	//ENTER_FUNC;
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
		if		(  IS_OBJECT_NULL(ValueObjectId(value))  ) {
			ValueObjectId(value) = RequestNewBLOB(fp,WFC_BLOB,BLOB_OPEN_WRITE);
			RequestCloseBLOB(fp,WFC_BLOB,ValueObjectId(value));
		} else {
			RequestExportBLOB(fp,WFC_BLOB,ValueObjectId(value),BlobCacheFileName(value));
		}
		break;
	  case	GL_TYPE_ALIAS:
	  default:
		break;
	}
	//LEAVE_FUNC;
}

static	void
FeedBLOB(
	NETFILE		*fp,
	ValueStruct	*value)
{
ENTER_FUNC;
	_FeedBLOB(fp,value);
LEAVE_FUNC;
}

static	void
_RecvWindow(
	char		*wname,
	WindowData	*win,
	NETFILE		*fp)
{
	PacketClass	klass;
	LargeByteString	*buff;

ENTER_FUNC;
	dbgprintf("name = [%s]",wname);
	dbgprintf("puttype = %02X",win->PutType);
	switch	(win->PutType) {
	  case	SCREEN_NULL:
		break;
	  default:
		dbgmsg("recv");
		SendPacketClass(fp,WFC_DATA);		ON_IO_ERROR(fp,badio);
		SendString(fp,wname);				ON_IO_ERROR(fp,badio);
		klass = RecvPacketClass(fp);		ON_IO_ERROR(fp,badio);
		switch	(klass)	{
		  case	WFC_OK:
			buff = NewLBS();
			RecvLBS(fp,buff);				ON_IO_ERROR(fp,badio);
			if		(  win->rec  !=  NULL  ) {
				NativeUnPackValue(NULL,LBS_Body(buff),win->rec->value);
				FeedBLOB(fp,win->rec->value);	ON_IO_ERROR(fp,badio);
			}
			FreeLBS(buff);
			break;
		  case	WFC_NODATA:
			break;
		  case	WFC_NOT:
			break;
		}
	}
  badio:
LEAVE_FUNC;
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

