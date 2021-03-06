/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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
*	GL term protocol value
*/

#ifndef _INC_GLTERM_H
#define _INC_GLTERM_H

#define SCREEN_NULL (unsigned char)0x00
#define SCREEN_INIT (unsigned char)0x01
#define SCREEN_CURRENT_WINDOW (unsigned char)0x02
#define SCREEN_NEW_WINDOW (unsigned char)0x03
#define SCREEN_CLOSE_WINDOW (unsigned char)0x04
#define SCREEN_CHANGE_WINDOW (unsigned char)0x05
#define SCREEN_JOIN_WINDOW (unsigned char)0x06

#define GL_Null (PacketClass)0x00
#define GL_Connect (PacketClass)0x01
#define GL_QueryScreen (PacketClass)0x02
#define GL_GetScreen (PacketClass)0x03
#define GL_GetData (PacketClass)0x04
#define GL_Event (PacketClass)0x05
#define GL_ScreenData (PacketClass)0x06
#define GL_ScreenDefine (PacketClass)0x07
#define GL_WindowName (PacketClass)0x08
#define GL_FocusName (PacketClass)0x09
#define GL_Auth (PacketClass)0x0A
#define GL_Name (PacketClass)0x0B
#define GL_Session (PacketClass)0x0C
#define GL_RedirectName (PacketClass)0x0D
#define GL_ServerVersion (PacketClass)0x0E
#define GL_Ping (PacketClass)0x0F
#define GL_Pong (PacketClass)0x10
#define GL_Pong_Dialog (PacketClass)0x11
#define GL_Pong_Popup (PacketClass)0x12
#define GL_Pong_Abort (PacketClass)0x13
#define GL_ListDownloads (PacketClass)0x14
#define GL_GetBLOB (PacketClass)0x15

#define HTTP_GET (PacketClass)0x47
#define HTTP_POST (PacketClass)0x50

#define GL_OK (PacketClass)0x80
#define GL_END (PacketClass)0x81
#define GL_NOT (PacketClass)0x83
#define GL_CONTINUE (PacketClass)0x84
#define GL_STOP (PacketClass)0x85

#define GL_E_VERSION (PacketClass)0xF1
#define GL_E_AUTH (PacketClass)0xF2
#define GL_E_APPL (PacketClass)0xF3
#define GL_E_Session (PacketClass)0xF4

#endif
