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

#include 	<glade/glade.h>
#include	"libmondai.h"
#include	"net.h"
#include	"queue.h"

#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#define UI_VERSION_1 1
#define UI_VERSION_2 2

typedef struct {
	NETFILE		*fpComm;
	Port		*port;
	char		*title;
#ifdef	USE_SSL
	SSL_CTX		*ctx;
#ifdef  USE_PKCS11
	ENGINE		*engine;
#endif  /* USE_PKCS11 */
#endif  /* USE_SSL */
}	Session;

typedef struct {
	char		*name;
	char		*title;
	void		*xml;
	Bool		fWindow;
	Bool		fAccelGroup;
	GHashTable	*ChangedWidgetTable;
	GHashTable	*TimerWidgetTable;
	Queue		*UpdateWidgetQueue;
}	WindowData;

extern	char	*CacheDirName(void);
extern	char	*CacheFileName(char *name);
extern	void	ExitSystem(void);
extern  void	MakeCacheDir(void);
extern  void	SetSessionTitle(char *dname);

GLOBAL	char		*FocusedWindowName;
GLOBAL	char		*ThisWindowName;
GLOBAL	GHashTable	*WindowTable;

GLOBAL	char	*CurrentApplication;

GLOBAL	Bool	fInRecv;

GLOBAL	Session	*glSession;
GLOBAL	char	*PortNumber;
GLOBAL	Bool	Protocol1;
GLOBAL	Bool	Protocol2;
GLOBAL	char	*User;
GLOBAL	char	*Pass;
GLOBAL	Bool	SavePass;
GLOBAL	char	*Cache; 
GLOBAL	char	*Style; 
GLOBAL	char	*Gtkrc; 
GLOBAL	Bool	fMlog;
GLOBAL	Bool	fKeyBuff;
GLOBAL	Bool	fTimer;
GLOBAL	char	*TimerPeriod;
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

#define GLCLIENT_DATADIR			"/usr/share/panda-client"

#define DEFAULT_HOST				"localhost"
#define DEFAULT_PORT				"8000"
#define DEFAULT_APPLICATION			"panda:"
#define DEFAULT_CACHE_PATH			"/.glclient/cache"
#define DEFAULT_PROTOCOL_V1			TRUE
#define DEFAULT_PROTOCOL_V1_STR		"true" 
#define DEFAULT_PROTOCOL_V2			FALSE
#define DEFAULT_PROTOCOL_V2_STR		"false"
#define DEFAULT_MLOG				TRUE 
#define DEFAULT_MLOG_STR			"true"
#define DEFAULT_KEYBUFF				FALSE
#define DEFAULT_KEYBUFF_STR			"false" 
#define DEFAULT_TIMER				TRUE
#define DEFAULT_TIMER_STR			"true"
#define DEFAULT_TIMERPERIOD			"1000"
#define DEFAULT_STYLE				""
#define DEFAULT_GTKRC				""
#define DEFAULT_USER				(getenv("USER"))
#define DEFAULT_PASSWORD			""
#define DEFAULT_SAVEPASSWORD		TRUE
#define DEFAULT_SAVEPASSWORD_STR	"true"
#ifdef USE_SSL
#define DEFAULT_SSL					FALSE
#define DEFAULT_SSL_STR				"false"
#define DEFAULT_CAPATH				""
#define DEFAULT_CAFILE				""
#define DEFAULT_KEY					""
#define DEFAULT_CERT				""
#define DEFAULT_CIPHERS				"ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH"
#ifdef USE_PKCS11
#define DEFAULT_PKCS11				FALSE
#define DEFAULT_PKCS11_STR			"false"
#define DEFAULT_PKCS11_LIB			""
#define DEFAULT_SLOT				""
#endif
#endif

#define	PING_TIMER_PERIOD	(10000)

#define	FPCOMM(session)		((session)->fpComm)
#define	TITLE(session)		((session)->title)
#ifdef	USE_SSL
#define	CTX(session)		((session)->ctx)
#ifdef	USE_PKCS11
#define	ENGINE(session)		((session)->engine)
#endif	/* USE_PKCS11 */
#endif	/* USE_SSL */

#endif
