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

#ifndef	_WFCIO_H
#define	_WFCIO_H
#include	"const.h"
#include	"enum.h"
#include	"driver.h"
#include	"wfc.h"
#include	"net.h"

#ifndef	PacketClass
#define	PacketClass		unsigned char
#endif

#define	WFC_Null		(PacketClass)0x00
#define	WFC_DATA		(PacketClass)0x01
#define	WFC_PING		(PacketClass)0x02
#define	WFC_FALSE		(PacketClass)0xE0
#define	WFC_TRUE		(PacketClass)0xE1
#define	WFC_NOT			(PacketClass)0xF0
#define	WFC_PONG		(PacketClass)0xF2
#define	WFC_OK			(PacketClass)0xFE
#define	WFC_END			(PacketClass)0xFF

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

extern	NETFILE	*ConnectTermServer(char *url, char *term, char *user, Bool fKeep, char *arg);
extern	Bool	SendTermServer(NETFILE *fp, char *window, char *widget, char *event,
							   ValueStruct *value);
extern	Bool	RecvTermServerHeader(NETFILE *fp, char *window, char *widget,
									 int *type, CloseWindows *cls);
extern	void	RecvTermServerData(NETFILE *fp, WindowData *win);

#endif
