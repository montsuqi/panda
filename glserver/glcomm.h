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

#ifndef	_GLCOMM_H
#define	_GLCOMM_H
#include	"net.h"

extern	void	GL_SendPacketClass(NETFILE *fp, PacketClass c);
extern	PacketClass	GL_RecvPacketClass(NETFILE *fp);

extern	void	GL_SendInt(NETFILE *fp, int data);
extern	void	GL_SendLong(NETFILE *fp, long data);
extern	void	GL_SendString(NETFILE *fp, char *str);
extern	int		GL_RecvInt(NETFILE *fp);
extern	void	GL_RecvString(NETFILE *fp, size_t size, char *str);
extern	Fixed	*GL_RecvFixed(NETFILE *fp);
extern	double	GL_RecvFloat(NETFILE *fp);
extern	Bool	GL_RecvBool(NETFILE *fp);
extern	void	GL_SendLBS(NETFILE *fp,LargeByteString *lbs);

extern	void	GL_SendValue(NETFILE *fp, ValueStruct *value, char *coding);
extern	void	GL_RecvValue(NETFILE *fp, ValueStruct *value, char *coding);

extern	void	InitGL_Comm(void);
extern	void	SetfNetwork(Bool f);

#undef	GLOBAL
#ifdef	GLCOMM_MAIN
#	define	GLOBAL		/*	*/
#else
#	define	GLOBAL		extern
#endif

#endif
