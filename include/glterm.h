/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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
*	GL term protocol value
*/

#ifndef	_INC_GLTERM_H
#define	_INC_GLTERM_H

#define	SCREEN_NULL					0
#define	SCREEN_CURRENT_WINDOW		1
#define	SCREEN_NEW_WINDOW			2
#define	SCREEN_CLOSE_WINDOW			3
#define	SCREEN_CHANGE_WINDOW		4
#define	SCREEN_JOIN_WINDOW			5
#define	SCREEN_FORK_WINDOW			6
#define	SCREEN_END_SESSION			7

#define	GL_Null			(PacketClass)0x00
#define	GL_Connect		(PacketClass)0x01
#define	GL_QueryScreen	(PacketClass)0x02
#define	GL_GetScreen	(PacketClass)0x03
#define	GL_GetData		(PacketClass)0x04
#define	GL_Event		(PacketClass)0x05
#define	GL_ScreenData	(PacketClass)0x06
#define	GL_ScreenDefine	(PacketClass)0x07
#define	GL_WindowName	(PacketClass)0x08
#define	GL_FocusName	(PacketClass)0x09
#define	GL_Auth			(PacketClass)0x0A
#define	GL_Name			(PacketClass)0x0B

#define	GL_OK			(PacketClass)0x80
#define	GL_END			(PacketClass)0x81
#define	GL_NOT			(PacketClass)0x83

#define	GL_E_VERSION	(PacketClass)0xF1
#define	GL_E_AUTH		(PacketClass)0xF2
#define	GL_E_APPL		(PacketClass)0xF3


#endif
