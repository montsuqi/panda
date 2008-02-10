/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_GLCLIENT_H
#define	_INC_GLCLIENT_H

#include <glade/glade.h>
#include	"libmondai.h"
#include	"net.h"

#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

typedef	struct {
	char		*name;
	GladeXML	*xml;
	GtkWindow	*window;
	GHashTable	*UpdateWidget;
}	XML_Node;

typedef struct {
	NETFILE		*fpComm;
	Port		*port;
#ifdef	USE_SSL
	SSL_CTX		*ctx;
#ifdef  USE_PKCS11
	ENGINE		*engine;
#endif  /* USE_PKCS11 */
#endif  /* USE_SSL */
}	Session;

extern	char	*CacheFileName(char *name);
extern	void	ExitSystem(void);
extern  void	MakeCacheDir(char *dname);

GLOBAL	GHashTable	*WindowTable;
GLOBAL	XML_Node	*FocusedScreen;

GLOBAL	char		*CurrentApplication;

GLOBAL	Bool	fInRecv;

GLOBAL	Session	*glSession;
GLOBAL	char	*User;
GLOBAL	char	*Pass;
GLOBAL	Bool	fMlog;
GLOBAL	Bool	fKeyBuff;
#ifdef	USE_SSL
GLOBAL	Bool	fSsl;
GLOBAL	char	*KeyFile;
GLOBAL	char	*CertFile;
GLOBAL	char	*CA_Path;
GLOBAL	char	*CA_File;
GLOBAL	char	*Ciphers;
#ifdef  USE_PKCS11
GLOBAL	Bool	fPKCS11;
GLOBAL	char	*PKCS11_Lib;
GLOBAL	char	*Slot;
#endif	/* USE_PKCS11 */
#endif	/* USE_SSL */

#define	FPCOMM(session)	((session)->fpComm)
#ifdef	USE_SSL
#define	CTX(session)	((session)->ctx)
#ifdef	USE_PKCS11
#define	ENGINE(session)	((session)->engine)
#endif	/* USE_PKCS11 */
#endif	/* USE_SSL */

#endif
