/*
PANDA -- a simple transaction monitor
Copyright (C) 1998-1999 Ogochan.
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef	_SERVER_H
#define	_SERVER_H
#include	"port.h"

#define	FETURE_NULL		(byte)0x0000
#define	FETURE_CORE		(byte)0x0001
#define	FETURE_I18N		(byte)0x0002
#define	FETURE_BLOB		(byte)0x0004
#define	FETURE_EXPAND	(byte)0x0008
#define	FETURE_NETWORK	(byte)0x0010
#define	FETURE_PS		(byte)0x0020
#define	FETURE_OLD		(byte)0x0040
#define	FETURE_PDF		(byte)0x0080

#define	fFetureCore		(((TermFeture & FETURE_CORE) != 0) ? TRUE : FALSE)
#define	fFetureBlob		(((TermFeture & FETURE_BLOB) != 0) ? TRUE : FALSE)
#define	fFetureExpand	(((TermFeture & FETURE_EXPAND) != 0) ? TRUE : FALSE)
#define	fFetureI18N		(((TermFeture & FETURE_I18N) != 0) ? TRUE : FALSE)
#define	fFetureNetwork	(((TermFeture & FETURE_NETWORK) != 0) ? TRUE : FALSE)
#define	fFeturePS		(((TermFeture & FETURE_PS) != 0) ? TRUE : FALSE)
#define	fFetureOld		(((TermFeture & FETURE_OLD) != 0) ? TRUE : FALSE)
#define	fFeturePDF		(((TermFeture & FETURE_PDF) != 0) ? TRUE : FALSE)

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
GLOBAL	Bool	fVerify;
GLOBAL	char	*KeyFile;
GLOBAL	char	*CertFile;
GLOBAL	char	*CA_Path;
GLOBAL	char	*CA_File;
#endif
GLOBAL	URL		Auth;
GLOBAL	byte	TermFeture;

extern	void	InitSystem(int argc, char **argv);
extern	void	ExecuteServer(void);

#endif
