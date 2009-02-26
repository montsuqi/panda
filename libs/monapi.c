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
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<glib.h>
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"comms.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"front.h"
#include	"socket.h"
#define		_MONAPI
#include	"monapi.h"
#include	"debug.h"

//FIXME ; move another header
#define		TermPort "/tmp/wfc.term"

extern	MonAPIData	*
NewMonAPIData(void)
{
	MonAPIData	*data;

ENTER_FUNC;
	data = New(MonAPIData);
	memclear(data->ld,sizeof(data->ld));
	memclear(data->user,sizeof(data->user));
	memclear(data->term,sizeof(data->term));
	data->arguments = NewLBS();
	data->headers = NewLBS();
	data->body = NewLBS();
LEAVE_FUNC;
	return	(data); 
}

extern void
FreeMonAPIData(
	MonAPIData *data)
{
ENTER_FUNC;
	FreeLBS(data->arguments);
	FreeLBS(data->headers);
	FreeLBS(data->body);
	xfree(data);
LEAVE_FUNC;
}

extern gboolean
CallMonAPI(
	MonAPIData *data)
{
	int fd;
	Port *port;
	NETFILE *fp;
	PacketClass klass;

ENTER_FUNC;
	port = ParPort(TermPort, PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if (fd > 0) {
		fp = SocketToNet(fd);
		SendPacketClass(fp,WFC_API);		ON_IO_ERROR(fp,badio);
		SendString(fp, data->ld);			ON_IO_ERROR(fp,badio);
		SendString(fp, data->user);			ON_IO_ERROR(fp,badio);
		SendString(fp, data->term);			ON_IO_ERROR(fp,badio);
		SendPacketClass(fp, data->method);	ON_IO_ERROR(fp,badio);
		SendLBS(fp, data->arguments);		ON_IO_ERROR(fp,badio);
		SendLBS(fp, data->headers);			ON_IO_ERROR(fp,badio);
		SendLBS(fp, data->body);			ON_IO_ERROR(fp,badio);
		klass = RecvPacketClass(fp);		ON_IO_ERROR(fp,badio);
		if (klass == WFC_TRUE) {
			RecvLBS(fp, data->headers);    ON_IO_ERROR(fp,badio);
			RecvLBS(fp, data->body);			ON_IO_ERROR(fp,badio);
		}
		CloseNet(fp);
	} else {
		badio:
		Message("can not connect wfc server");
		return FALSE;
	}
	return TRUE;
LEAVE_FUNC;
}
