/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_KEYVALUECOM_H
#define	_INC_KEYVALUECOM_H

#include	"libmondai.h"
#include	"net.h"
#include	"blobcom.h"

#define	KV_GETVALUE		(PacketClass)0x01
#define	KV_SETVALUE		(PacketClass)0x02
#define	KV_SETVALUEALL	(PacketClass)0x03
#define	KV_LISTKEY		(PacketClass)0x04
#define	KV_LISTENTRY	(PacketClass)0x05
#define	KV_NEWENTRY		(PacketClass)0x06
#define	KV_DELETEENTRY	(PacketClass)0x07

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif


#endif
