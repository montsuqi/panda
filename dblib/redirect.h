/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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

#ifndef	_INC_LOG_H
#define	_INC_LOG_H
#include	"dbgroup.h"

#ifndef	PacketClass
#define	PacketClass		unsigned char
#endif

#define	RED_PING		(PacketClass)0x01
#define	RED_DATA		(PacketClass)0x02
#define	RED_LOCK		(PacketClass)0x03
#define	RED_STATUS		(PacketClass)0x04
#define	RED_NOT			(PacketClass)0xF0
#define	RED_PONG		(PacketClass)0xF1
#define	RED_OK			(PacketClass)0xFE
#define	RED_END			(PacketClass)0xFF

extern	void	OpenDB_RedirectPort(DBG_Struct *dbg);
extern	void	PutDB_Redirect(DBG_Struct *dbg, char *data);
extern	void	PutCheckDataDB_Redirect(DBG_Struct	*dbg, char	*data);
extern	void	AbortDB_Redirect(DBG_Struct *dbg);
extern	void	BeginDB_Redirect(DBG_Struct *dbg);
extern	void	CommitDB_Redirect(DBG_Struct *dbg);
extern	Bool	CheckDB_Redirect(DBG_Struct *dbg);

#endif
