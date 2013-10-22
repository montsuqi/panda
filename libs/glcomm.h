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

#define	FEATURE_NULL		(unsigned char)0x0000
#define	FEATURE_UNUSED		(unsigned char)0x0001
#define	FEATURE_NETWORK		(unsigned char)0x0002
#define	FEATURE_NEGO		(unsigned char)0x0004
#define	FEATURE_V48			(unsigned char)0x0008

#define	fFeatureNetwork		(((TermFeature & FEATURE_NETWORK) != 0) ? TRUE : FALSE)
#define	fFeatureNego		(((TermFeature & FEATURE_NEGO) != 0) ? TRUE : FALSE)
#define	fFeatureV48			(((TermFeature & FEATURE_V48) != 0) ? TRUE : FALSE)

extern	void	GL_SendPacketClass(NETFILE *fp, PacketClass c, Bool fNetwork);
extern	PacketClass	GL_RecvPacketClass(NETFILE *fp, Bool fNetwork);

extern	void	GL_SendInt(NETFILE *fp, int data, Bool fNetwork);
extern	void	GL_SendLong(NETFILE *fp, long data, Bool fNetwork);
extern	void	GL_SendString(NETFILE *fp, char *str, Bool fNetwork);
extern	int		GL_RecvInt(NETFILE *fp ,Bool fNetwork);
extern	void	GL_RecvString(NETFILE *fp, size_t size, char *str, Bool fNetwork);
extern	Fixed	*GL_RecvFixed(NETFILE *fp, Bool fNetwork);
extern	double	GL_RecvFloat(NETFILE *fp, Bool fNetwork);
extern	Bool	GL_RecvBool(NETFILE *fp, Bool fNetwork);

extern	void	GL_SendValue(NETFILE *fp, ValueStruct *value, char *coding, Bool fNetwork);
extern	void	GL_RecvValue(NETFILE *fp, ValueStruct *value, char *coding, Bool fNetwork);

extern	void	InitGL_Comm(void);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
GLOBAL	unsigned char		TermFeature;

#endif
