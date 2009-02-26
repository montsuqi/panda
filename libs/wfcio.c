/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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
*/
#define	DEBUG
#define	TRACE

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
#include	"message.h"
#include	"debug.h"
#include	"socket.h"

extern	Bool
TermCreateSession(
	NETFILE	*fp,
	char	*term,
	char	*user,
	char	*lang,
	Bool	fKeep)
{
	Bool	rc;
	char	buff[SIZE_NAME+1];

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_INIT);	ON_IO_ERROR(fp,badio);
	if		(  term  ==  NULL  ) {
		*buff = 0;
	} else {
		strcpy(buff,term);
	}
	SendString(fp,buff);			ON_IO_ERROR(fp,badio);
	if		(  fKeep  ) {
		SendPacketClass(fp,WFC_TRUE);	ON_IO_ERROR(fp,badio);
	} else {
		SendPacketClass(fp,WFC_FALSE);	ON_IO_ERROR(fp,badio);
	}
	SendString(fp,user);			ON_IO_ERROR(fp,badio);
	SendString(fp,lang);			ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		RecvnString(fp,SIZE_NAME,buff);	ON_IO_ERROR(fp,badio);
		if		(  term  !=  NULL  ) {
			strcpy(term,buff);
		}
	}
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermBreakSession(
	NETFILE	*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_BREAK);	ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_DONE  ) {
		rc = TRUE;
	}
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendID(
	NETFILE		*fp,
	char		*term)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_TERMID);	ON_IO_ERROR(fp,badio);
	SendString(fp,term);			ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		rc = TRUE;
	}
	dbgmsg("*");
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSwitchLD(
	NETFILE		*fp,
	char		*ldname)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_SWITCH_LD);	ON_IO_ERROR(fp,badio);
	SendString(fp,ldname);				ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		rc = TRUE;
	}
  badio:;
LEAVE_FUNC;
	return	(rc);
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

extern	Bool
TermSendDataHeader(
	NETFILE		*fp,
	char		*lang,
	char		*window,
	char		*widget,
	char		*event)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_HEADER);	ON_IO_ERROR(fp,badio);
	SendString(fp,lang);			ON_IO_ERROR(fp,badio);
	SendString(fp,window);			ON_IO_ERROR(fp,badio);
	SendString(fp,widget);			ON_IO_ERROR(fp,badio);
	SendString(fp,event);			ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendDataBody(
	NETFILE		*fp,
	char		*window,
	ValueStruct	*value)
{
	Bool	rc;
	size_t	size;
	LargeByteString	*buff;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_BODY);	ON_IO_ERROR(fp,badio);
	SendString(fp,window);			ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		if		(  value  !=  NULL  ) {
			SendPacketClass(fp,WFC_DATA);	ON_IO_ERROR(fp,badio);
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
		rc = TRUE;
	}
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendExec(
	NETFILE	*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_EXEC);	ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendEnd(
	NETFILE	*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_END);	ON_IO_ERROR(fp,badio);
	rc = TRUE;
LEAVE_FUNC;
  badio:;
	return	(rc);
}

extern	Bool
TermRequestHeader(
	NETFILE	*fp,
	char	*user,
	char	*lang,
	char	*window,
	char	*widget,
	int		*type,
	WindowControl	*ctl)
{
	Bool	rc;
	int		i;
	PacketClass	c;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_REQ_HEAD);	ON_IO_ERROR(fp,badio);
	switch	(c = RecvPacketClass(fp)) {
	  case	WFC_HEADER:
		RecvnString(fp, SIZE_NAME+1, user);		ON_IO_ERROR(fp,badio);
		RecvnString(fp, SIZE_NAME+1, lang);		ON_IO_ERROR(fp,badio);
		RecvnString(fp, SIZE_NAME+1, window);	ON_IO_ERROR(fp,badio);
		RecvnString(fp, SIZE_NAME+1, widget);	ON_IO_ERROR(fp,badio);
		dbgprintf("window = [%s]",window);
		*type = RecvChar(fp);					ON_IO_ERROR(fp,badio);
		ctl->n = RecvInt(fp);					ON_IO_ERROR(fp,badio);
		dbgprintf("type   = %02X\n",*type);
		dbgprintf("ctl->n = %d\n",ctl->n);
		for	( i = 0 ; i < ctl->n ; i ++ ) {
			ctl->control[i].PutType = (byte)RecvInt(fp);			ON_IO_ERROR(fp,badio);
			RecvnString(fp, SIZE_NAME+1, ctl->control[i].window);	ON_IO_ERROR(fp,badio);
			dbgprintf("wname   = [%s]\n",ctl->control[i].window);
			dbgprintf("PutType = [%02X]\n",ctl->control[i].PutType);
		}
		rc = TRUE;
		break;
	  case	WFC_NODATA:
		rc = TRUE;
		break;
	  default:
		Warning("Recv packet failure [%x]", c);
		dbgmsg("recv default");
		break;
	}
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermRequestBody(
	NETFILE		*fp,
	char		*window,
	ValueStruct	*value)
{
	Bool	rc;
	LargeByteString	*buff;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_REQ_DATA);	ON_IO_ERROR(fp,badio);
	SendString(fp,window);			ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		buff = NewLBS();
		RecvLBS(fp,buff);				ON_IO_ERROR(fp,badio);
		if		(  value  !=  NULL  ) {
			NativeUnPackValue(NULL,LBS_Body(buff),value);
			FeedBLOB(fp,value);				ON_IO_ERROR(fp,badio);
		}
		FreeLBS(buff);
		rc = TRUE;
	}
LEAVE_FUNC;
  badio:;
	return	(rc);
}

extern	Bool
TermSendStart(
	NETFILE	*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_START);		ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendPrepare(
	NETFILE	*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_PREPARE);	ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendCommit(
	NETFILE	*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_COMMIT);		ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendAbort(
	NETFILE	*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_ABORT);		ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
TermSendPing(
	NETFILE		*fp)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_PING);	ON_IO_ERROR(fp,badio);
	if		(  RecvPacketClass(fp)  ==  WFC_PONG  ) {
		rc = TRUE;
	}
  badio:;
LEAVE_FUNC;
	return	(rc);
}


extern	NETFILE	*
TermConnectServer(
	char	*url)
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
	} else {
		Warning("can not connect wfc server");
		fp = NULL;
	}
LEAVE_FUNC;
	return	(fp); 
}

static	void
_RecvWindow(
	char		*wname,
	WindowData	*win,
	NETFILE		*fp)
{
	ValueStruct	*value;

ENTER_FUNC;
	dbgprintf("name = [%s]",wname);
	dbgprintf("puttype = %02X",win->PutType);
	switch	(win->PutType) {
	  case	SCREEN_NULL:
		break;
	  default:
		dbgmsg("recv");
		if		(  win->rec  !=  NULL  ) {
			value = win->rec->value;
		} else {
			value = NULL;
		}
		TermRequestBody(fp,wname,value);
		break;
	}
LEAVE_FUNC;
}

extern	void
TermRecvServerData(
	NETFILE		*fp,
	ScreenData	*scr)
{
ENTER_FUNC;
	g_hash_table_foreach(scr->Windows,(GHFunc)_RecvWindow,fp);
LEAVE_FUNC;
}

