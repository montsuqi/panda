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

#define	WFC_INIT		(PacketClass)0x01
#define	WFC_BREAK		(PacketClass)0x02
#define	WFC_TERMID		(PacketClass)0x03
#define	WFC_SWITCH_LD	(PacketClass)0x04
#define	WFC_EXEC		(PacketClass)0x05
#define	WFC_REQ_HEAD	(PacketClass)0x06
#define	WFC_REQ_DATA	(PacketClass)0x07
#define	WFC_BLOB		(PacketClass)0x08
#define	WFC_START		(PacketClass)0x09
#define	WFC_PREPARE		(PacketClass)0x0A
#define	WFC_COMMIT		(PacketClass)0x0B
#define	WFC_ABORT		(PacketClass)0x0C
#define	WFC_TERM		(PacketClass)0x0D
#define	WFC_API			(PacketClass)0x0E

#define	WFC_HEADER		(PacketClass)0x11
#define	WFC_BODY		(PacketClass)0x12

#define	WFC_NODATA		(PacketClass)0x13
#define	WFC_DATA		(PacketClass)0x14
#define	WFC_DONE		(PacketClass)0x15

#define	WFC_FALSE		(PacketClass)0xF0
#define	WFC_TRUE		(PacketClass)0xF1

#define	WFC_PING		(PacketClass)0xF3
#define	WFC_PONG		(PacketClass)0xF4

#define	WFC_END			(PacketClass)0xFF

extern	Bool	TermCreateSession(NETFILE *fp, char *term, char *user, char *lang, Bool fKeep);
extern	Bool	TermBreakSession(NETFILE *fp);
extern	Bool	TermSendID(NETFILE *fp, char *term);
extern	Bool	TermSwitchLD(NETFILE *fp, char *ldname);
extern	Bool	TermSendDataHeader(NETFILE *fp, char *lang, char *window, char *widget, char *event);
extern	Bool	TermSendDataBody(NETFILE *fp, char *window, ValueStruct *value);
extern	Bool	TermSendExec(NETFILE *fp);
extern	Bool	TermSendEnd(NETFILE *fp);
extern	Bool	TermRequestHeader(NETFILE *fp, char *user, char *lang, char *window, char *widget,
								  int *type, WindowControl *ctl);
extern	Bool	TermRequestBody(NETFILE *fp, char *window, ValueStruct *value);
extern	Bool	TermSendStart(NETFILE *fp);
extern	Bool	TermSendPrepare(NETFILE *fp);
extern	Bool	TermSendCommit(NETFILE *fp);
extern	Bool	TermSendAbort(NETFILE *fp);
extern	Bool	TermSendPing(NETFILE *fp);
extern	NETFILE	*TermConnectServer(char *url);
extern	void	TermRecvServerData(NETFILE *fp, ScreenData *scr);

#endif
