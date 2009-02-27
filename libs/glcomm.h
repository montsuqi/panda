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

#define	FEATURE_NULL		(byte)0x0000
#define	FEATURE_CORE		(byte)0x0001
#define	FEATURE_I18N		(byte)0x0002
#define	FEATURE_BLOB		(byte)0x0004
#define	FEATURE_EXPAND		(byte)0x0008
#define	FEATURE_NETWORK		(byte)0x0010
#define	FEATURE_NEGO		(byte)0x0020
#define	FEATURE_OLD			(byte)0x0040

#define	fFeatureCore		(((TermFeature & FEATURE_CORE) != 0) ? TRUE : FALSE)
#define	fFeatureBlob		(((TermFeature & FEATURE_BLOB) != 0) ? TRUE : FALSE)
#define	fFeatureExpand		(((TermFeature & FEATURE_EXPAND) != 0) ? TRUE : FALSE)
#define	fFeatureI18N		(((TermFeature & FEATURE_I18N) != 0) ? TRUE : FALSE)
#define	fFeatureNetwork		(((TermFeature & FEATURE_NETWORK) != 0) ? TRUE : FALSE)
#define	fFeatureNego		(((TermFeature & FEATURE_NEGO) != 0) ? TRUE : FALSE)
#define	fFeatureOld			(((TermFeature & FEATURE_OLD) != 0) ? TRUE : FALSE)

typedef enum _ExpandType {
	EXPAND_PS,
	EXPAND_PNG,
	EXPAND_PDF,
} ExpandType;

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

extern	void	GL_SendValue(NETFILE *fp, ValueStruct *value, char *coding,
							 Bool fBlob, Bool fExpand, Bool fNetwork);
extern	void	GL_RecvValue(NETFILE *fp, ValueStruct *value, char *coding,
							 Bool fBlob, Bool fExpand, Bool fNetwork);

extern	void	InitGL_Comm(void);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
GLOBAL	byte		TermFeature;
GLOBAL	ExpandType	TermExpandType;

#endif
