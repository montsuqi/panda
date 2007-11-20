/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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

#ifndef	_INC_COMM_H
#define	_INC_COMM_H
#include	"value.h"
#include	"net.h"
extern	void			SendPacketClass(NETFILE *fp, PacketClass c);
extern	PacketClass		RecvPacketClass(NETFILE *fp);
extern	PacketClass		RecvDataType(NETFILE *fp);
extern	void			SendLength(NETFILE *fp, size_t size);
extern	size_t			RecvLength(NETFILE *fp);
extern	void			SendLBS(NETFILE *fp, LargeByteString *lbs);
extern	void			RecvLBS(NETFILE *fp, LargeByteString *lbs);
extern	void			SendString(NETFILE *fp, char *str);
extern	void			RecvString(NETFILE *fp, char *str);
extern	void			RecvnString(NETFILE *fp, size_t  size, char *str);
extern	int				RecvInt(NETFILE *fp);
extern	void			SendInt(NETFILE *fp,int data);
extern	unsigned int	RecvUInt(NETFILE *fp);
extern	void			SendUInt(NETFILE *fp, unsigned int data);
extern	int				RecvChar(NETFILE *fp);
extern	void			SendChar(NETFILE *fp,int data);
extern	Bool			RecvBool(NETFILE *fp);
extern	void			SendBool(NETFILE *fp, Bool data);
extern	void			SendObject(NETFILE *fp, MonObjectType obj);
extern	MonObjectType	RecvObject(NETFILE *fp);

extern	void			InitComm(void);

#define	RecvName(fp,name)	RecvString((fp),(name))

#undef	GLOBAL
#ifdef	_COMM
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
#undef	GLOBAL

#endif
