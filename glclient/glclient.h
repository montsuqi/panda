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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<glib.h>
#include	<json.h>
#include	<curl/curl.h>
#ifdef USE_SSL
#include	<openssl/engine.h>
#endif

#include	"libmondai.h"

#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#define DEFAULT_PING_TIMER_PERIOD   (7000) //7sec

typedef struct {
	char			*AuthURI;
	char			*RPCURI;
	char			*RESTURI;
	char			*ProtocolVersion;
	char			*AppVersion;
	char			*title;
	char			*bgcolor;
	Bool			IsRecv;
	char			*FocusedWindow;
	char			*FocusedWidget;
	char			*ThisWindow;
	GHashTable		*WindowTable;
	unsigned int	RPCID;
	char			*SessionID;
	json_object		*ScreenData;
}	GLSession;

#define	AUTHURI(session)		((session)->AuthURI)
#define RPCURI(session)			((session)->RPCURI)
#define RESTURI(session)		((session)->RESTURI)
#define PROTOVER(session)		((session)->ProtocolVersion)
#define APPVER(session)			((session)->AppVersion)
#define	TITLE(session)			((session)->title)
#define	BGCOLOR(session)		((session)->bgcolor)
#define	ISRECV(session)			((session)->IsRecv)
#define	FOCUSEDWINDOW(session)	((session)->FocusedWindow)
#define	FOCUSEDWIDGET(session)	((session)->FocusedWidget)
#define	THISWINDOW(session)		((session)->ThisWindow)
#define	WINDOWTABLE(session)	((session)->WindowTable)
#define	RPCID(session)			((session)->RPCID)
#define	SESSIONID(session)		((session)->SessionID)
#define	SCREENDATA(session)		((session)->ScreenData)

typedef struct {
	char		*name;
	char		*title;
	void		*xml;
	Bool		fWindow;
	Bool		fAccelGroup;
	GHashTable	*ChangedWidgetTable;
	GList		*Timers;
	json_object	*tmpl;
}	WindowData;

extern	void		ExitSystem(void);
extern  void		SetSessionTitle(const char *title);
extern  void		SetSessionBGColor(const char *color);

GLOBAL	char		*ConfigName;
GLOBAL	Bool		CancelScaleWindow;
GLOBAL	char		*CurrentApplication;
GLOBAL	Bool		fV47;
GLOBAL	char		*TempDir;
GLOBAL	char		*ConfDir;
GLOBAL	GLSession	*Session;

GLOBAL	char		*User;
GLOBAL	char		*Pass;
GLOBAL	Bool		SavePass;
GLOBAL	char		*Style; 
GLOBAL	char		*Gtkrc; 
GLOBAL	Bool		fMlog;
GLOBAL	Bool		fKeyBuff;
GLOBAL	Bool		fIMKanaOff;
GLOBAL	Bool		fTimer;
GLOBAL	int			TimerPeriod;
GLOBAL	int			PingTimerPeriod;
GLOBAL	char		*FontName;

GLOBAL	Bool		fDialog;

GLOBAL	Bool		fSSL;
GLOBAL	char		*CertFile;
GLOBAL	char		*CertKeyFile;
GLOBAL	char		*CertPass;
GLOBAL	Bool		SaveCertPass;
GLOBAL	char		*CAFile;
GLOBAL	char		*Ciphers;

GLOBAL	Bool		fPKCS11;
GLOBAL	char		*PKCS11Lib;
GLOBAL	char		*PIN;
GLOBAL	Bool		fSavePIN;
#	ifdef USE_SSL
GLOBAL	ENGINE		*Engine;
#	endif
GLOBAL	CURL		*Curl;

GLOBAL	Bool		Ginbee;

#endif
