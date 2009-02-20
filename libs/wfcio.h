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

#ifndef	_WFCIO_H
#define	_WFCIO_H
#include	"const.h"
#include	"enum.h"
#include	"driver.h"
#include	"wfcdata.h"
#include	"net.h"

#ifndef	PacketClass
#define	PacketClass		unsigned char
#endif

#define	WFC_Null		(PacketClass)0x00
#define	WFC_DATA		(PacketClass)0x01
#define	WFC_PING		(PacketClass)0x02
#define	WFC_BLOB		(PacketClass)0x03
#define	WFC_HEADER		(PacketClass)0x04
#define	WFC_TERM		(PacketClass)0x05
#define	WFC_API			(PacketClass)0x06

#define	WFC_FALSE		(PacketClass)0xE0
#define	WFC_TRUE		(PacketClass)0xE1
#define	WFC_NOT			(PacketClass)0xF0
#define	WFC_PONG		(PacketClass)0xF2
#define	WFC_NODATA		(PacketClass)0xFC
#define	WFC_DONE		(PacketClass)0xFD
#define	WFC_OK			(PacketClass)0xFE
#define	WFC_END			(PacketClass)0xFF

extern	NETFILE	*ConnectTermServer(char *url, char *term, char *user, char *window, Bool fKeep, char *arg);
extern	Bool	SendTermServer(NETFILE *fp, char *term, char *window, char *widget,
							   char *event,
							   ValueStruct *value);
extern	Bool	RecvTermServerHeader(NETFILE *fp, char *user, char *window, char *widget,
									 int *type, WindowControl *ctl);
extern	void	RecvTermServerData(NETFILE *fp, ScreenData *scr);
extern	Bool	SendTermServerEnd(NETFILE *fp, char *term);

#endif
