/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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

#ifndef	_SERVER_H
#define	_SERVER_H
#include	"port.h"

#define	FEATURE_NULL		(byte)0x0000
#define	FEATURE_CORE		(byte)0x0001
#define	FEATURE_I18N		(byte)0x0002
#define	FEATURE_BLOB		(byte)0x0004
#define	FEATURE_EXPAND		(byte)0x0008
#define	FEATURE_NETWORK		(byte)0x0010
#define	FEATURE_PS			(byte)0x0020
#define	FEATURE_OLD			(byte)0x0040
#define	FEATURE_PDF			(byte)0x0080

#define	fFeatureCore		(((TermFeture & FEATURE_CORE) != 0) ? TRUE : FALSE)
#define	fFeatureBlob		(((TermFeture & FEATURE_BLOB) != 0) ? TRUE : FALSE)
#define	fFeatureExpand		(((TermFeture & FEATURE_EXPAND) != 0) ? TRUE : FALSE)
#define	fFeatureI18N		(((TermFeture & FEATURE_I18N) != 0) ? TRUE : FALSE)
#define	fFeatureNetwork		(((TermFeture & FEATURE_NETWORK) != 0) ? TRUE : FALSE)
#define	fFeaturePS			(((TermFeture & FEATURE_PS) != 0) ? TRUE : FALSE)
#define	fFeatureOld			(((TermFeture & FEATURE_OLD) != 0) ? TRUE : FALSE)
#define	fFeaturePDF			(((TermFeture & FEATURE_PDF) != 0) ? TRUE : FALSE)

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	char	*PortNumber;
GLOBAL	int		Back;
#ifdef	USE_SSL
GLOBAL	Bool	fSsl;
GLOBAL	char	*KeyFile;
GLOBAL	char	*CertFile;
GLOBAL	char	*CA_Path;
GLOBAL	char	*CA_File;
GLOBAL	char	*Ciphers;
#endif
GLOBAL	URL		Auth;
GLOBAL	byte	TermFeture;

extern	void	InitSystem(int argc, char **argv);
extern	void	ExecuteServer(void);

#endif
