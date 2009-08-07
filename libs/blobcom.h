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

#ifndef	_INC_BLOBCOM_H
#define	_INC_BLOBCOM_H

#include	"libmondai.h"
#include	"net.h"
#include	"blob.h"

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#define	BLOB_CREATE			(PacketClass)0x01
#define	BLOB_DESTROY		(PacketClass)0x02
#define	BLOB_IMPORT			(PacketClass)0x03
#define	BLOB_EXPORT			(PacketClass)0x04
#define	BLOB_SAVE			(PacketClass)0x05
#define	BLOB_CHECK			(PacketClass)0x06
#define	BLOB_OPEN			(PacketClass)0x07
#define	BLOB_READ			(PacketClass)0x08
#define	BLOB_WRITE			(PacketClass)0x09
#define	BLOB_CLOSE			(PacketClass)0x0A
#define	BLOB_SEEK			(PacketClass)0x0B
#define	BLOB_START			(PacketClass)0x0C
#define	BLOB_COMMIT			(PacketClass)0x0D
#define	BLOB_ABORT			(PacketClass)0x0E
#define	BLOB_REGISTER		(PacketClass)0x0F
#define	BLOB_LOOKUP			(PacketClass)0x10

#define	BLOB_OK				(PacketClass)0xFE
#define	BLOB_NOT			(PacketClass)0xF0
#define	BLOB_END			(PacketClass)0xFF

#define SYSDATA_END			(PacketClass)0x01
#define SYSDATA_BLOB		(PacketClass)0x02
#define SYSDATA_KV			(PacketClass)0x03

#endif
