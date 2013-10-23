/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#include	"const.h"

#include	"libmondai.h"
#include	"comm.h"
#include	"comms.h"
#include	"wfcdata.h"
#include	"glterm.h"
#include	"wfcio.h"
#include	"debug.h"
#include	"socket.h"

static	Bool
_SendTermServer(
	NETFILE		*fp,
	ScreenData	*scr,
	ValueStruct	*value)
{
	size_t			size;
	LargeByteString	*buff;
	PacketClass		c;
	int				i;

ENTER_FUNC;
	dbgmsg("send DATA");
	dbgprintf("window = [%s]",scr->window);
	dbgprintf("widget = [%s]",scr->widget);
	dbgprintf("event  = [%s]",scr->event);

	SendPacketClass(fp,WFC_DATA);	ON_IO_ERROR(fp,badio);
	SendString(fp,scr->window);		ON_IO_ERROR(fp,badio);
	SendString(fp,scr->widget);		ON_IO_ERROR(fp,badio);
	SendString(fp,scr->event);		ON_IO_ERROR(fp,badio);

	SendInt(fp,scr->w.sp);			ON_IO_ERROR(fp,badio);
	for (i=0;i<scr->w.sp;i++) {
		SendChar(fp,scr->w.s[i].puttype);ON_IO_ERROR(fp,badio);
		SendString(fp,scr->w.s[i].window);ON_IO_ERROR(fp,badio);
	}

	c = RecvPacketClass(fp);		ON_IO_ERROR(fp,badio);
	if (c != WFC_OK) {
		return FALSE;
	}
	dbgmsg("recv OK");
	if (value != NULL) {
		SendPacketClass(fp,WFC_DATA);		ON_IO_ERROR(fp,badio);
		size = NativeSizeValue(NULL,value);
		buff = NewLBS();
		LBS_ReserveSize(buff,size,FALSE);
		NativePackValue(NULL,LBS_Body(buff),value);
		SendLBS(fp,buff);				ON_IO_ERROR(fp,badio);
		FreeLBS(buff);
	} else {
		SendPacketClass(fp,WFC_NODATA);	ON_IO_ERROR(fp,badio);
	}
	SendPacketClass(fp,WFC_OK);		ON_IO_ERROR(fp,badio);
LEAVE_FUNC;
	return TRUE; 
badio:
LEAVE_FUNC;
	return FALSE; 
}

extern	NETFILE	*
ConnectTermServer(
	char		*url,
	ScreenData	*scr)
{
	int		fd;
	Port	*port;
	NETFILE	*fp;
	PacketClass	klass;
ENTER_FUNC;
	port = ParPort(url,PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if (fd <= 0){
		Warning("can not connect wfc server");
		return NULL;
	}
	fp = SocketToNet(fd);
	SendPacketClass(fp,WFC_TERM_INIT);
	SendString(fp,scr->user);		ON_IO_ERROR(fp,badio);
	SendString(fp,scr->host);		ON_IO_ERROR(fp,badio);
	SendString(fp,scr->agent);		ON_IO_ERROR(fp,badio);
	klass = RecvPacketClass(fp);	ON_IO_ERROR(fp,badio);
	if (klass == WFC_OK) {
		RecvnString(fp,SIZE_TERM,scr->term);ON_IO_ERROR(fp,badio);
	} else {
		CloseNet(fp);
		fp = NULL;
	}
LEAVE_FUNC;
	return fp; 
badio:
	Warning("badio");
LEAVE_FUNC;
	return NULL;
}

extern	Bool
SendTermServer(
	NETFILE		*fp,
	ScreenData	*scr,
	ValueStruct	*value)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	dbgprintf("term = [%s]",scr->term);
	SendPacketClass(fp,WFC_TERM);
	SendString(fp,scr->term);				ON_IO_ERROR(fp,badio);
	if (RecvPacketClass(fp) == WFC_TRUE) {
		rc = _SendTermServer(fp,scr,value);
	}
  badio:
LEAVE_FUNC;
	return	(rc); 
}

extern	Bool
SendTermServerEnd(
	NETFILE		*fp,
	ScreenData	*scr)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;

	SendPacketClass(fp,WFC_TERM);
	SendString(fp,scr->term);				ON_IO_ERROR(fp,badio);
	if (RecvPacketClass(fp) == WFC_TRUE) {
		SendPacketClass(fp,WFC_END);	ON_IO_ERROR(fp,badio);
		RecvPacketClass(fp); // WFC_DONE
		rc = TRUE;
	}
  badio:
LEAVE_FUNC;
	return	(rc); 
}

extern	Bool
RecvTermServerHeader(
	NETFILE			*fp,
	ScreenData		*scr)
{
	PacketClass	c;
	int i;
ENTER_FUNC;
	while(1) {
		c = RecvPacketClass(fp);						ON_IO_ERROR(fp,badio);
		switch (c) {
		case WFC_HEADER:
			dbgmsg(">recv HEADER");
			RecvnString(fp, SIZE_USER, scr->user);		ON_IO_ERROR(fp,badio);
			RecvnString(fp, SIZE_NAME, scr->window);	ON_IO_ERROR(fp,badio);
			RecvnString(fp, SIZE_NAME, scr->widget);	ON_IO_ERROR(fp,badio);
			dbgprintf("window = [%s]",scr->window);
			scr->puttype = RecvChar(fp);				ON_IO_ERROR(fp,badio);
			scr->w.sp = RecvInt(fp);					ON_IO_ERROR(fp,badio);
			for (i=0;i<scr->w.sp ;i++) {
				scr->w.s[i].puttype = RecvChar(fp);		ON_IO_ERROR(fp,badio);
				RecvnString(fp,SIZE_NAME,scr->w.s[i].window);
					ON_IO_ERROR(fp,badio);
				dbgprintf("wname = [%s]\n",scr->w.s[i].window);
			}
			dbgmsg("<recv HEADER");
			return TRUE;
		default:
			Warning("Recv packet failure [%x]", c);
			dbgmsg("recv default");
			SendPacketClass(fp,WFC_NOT);
			dbgmsg("send NOT");
			return FALSE;
		}
	}
LEAVE_FUNC;
	Warning("does not reach");
	return FALSE;
badio:
LEAVE_FUNC;
	return FALSE;
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
	switch (win->puttype) {
	case SCREEN_NULL:
		break;
	default:
		dbgmsg("recv");
		SendPacketClass(fp,WFC_DATA);		ON_IO_ERROR(fp,badio);
		SendString(fp,wname);				ON_IO_ERROR(fp,badio);
		klass = RecvPacketClass(fp);		ON_IO_ERROR(fp,badio);
		switch (klass) {
		case WFC_OK:
			buff = NewLBS();
			RecvLBS(fp,buff);				ON_IO_ERROR(fp,badio);
			if (win->rec != NULL) {
				NativeUnPackValue(NULL,LBS_Body(buff),win->rec->value);
			}
			FreeLBS(buff);
			break;
		case WFC_NODATA:
			break;
		case WFC_NOT:
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
	PacketClass	klass;

ENTER_FUNC;
	g_hash_table_foreach(scr->Windows,(GHFunc)_RecvWindow,fp);
	SendPacketClass(fp,WFC_DONE);
	klass = RecvPacketClass(fp);
	if (klass != WFC_DONE) {
		Warning("Error wfc fail");
	}
LEAVE_FUNC;
}

