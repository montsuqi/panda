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

#define GL_SendDataType(fp,c)    GL_SendPacketClass(fp,c)
#define GL_RecvDataType(fp)      GL_RecvPacketClass(fp)
#define GL_SendName(fp,str)      GL_SendString(fp,str)
#define GL_RecvName(fp,size,str) GL_RecvString(fp,size,str)

extern	void		GL_SendPacketClass(NETFILE *fp, PacketClass c);
extern	PacketClass	GL_RecvPacketClass(NETFILE *fp);


extern	size_t	GL_RecvLength(NETFILE *fp);
extern	void	GL_SendLength(NETFILE *fp,size_t data);

extern	void	GL_SendInt(NETFILE *fp,int data);
extern	int		GL_RecvInt(NETFILE *fp);

extern	void	GL_SendString(NETFILE *fp,char *str);
extern	void	GL_RecvString(NETFILE *fp,size_t size, char *str);


extern	Fixed	*GL_RecvFixed(NETFILE *fp);
extern	void	GL_SendFixed(NETFILE *fp,Fixed *xval);

extern	double	GL_RecvFloat(NETFILE *fp);
extern	void	GL_SendFloat(NETFILE *fp,double data);

extern	Bool	GL_RecvBool(NETFILE *fp);
extern	void	GL_SendBool(NETFILE *fp,Bool data);

extern	void	GL_RecvLBS(NETFILE *fp,LargeByteString *lbs);
extern	void	GL_SendLBS(NETFILE *fp,LargeByteString *lbs);

extern	void	GL_RecvName(NETFILE *fp, size_t size, char *name);
extern	void	GL_SendName(NETFILE *fp, char *name);

extern	void	InitGL_Comm(void);
extern	void	SetfNetwork(Bool f);

#undef	GLOBAL
#ifdef	GLCOMM_MAIN
#	define	GLOBAL		/*	*/
#else
#	define	GLOBAL		extern
#endif

#endif
