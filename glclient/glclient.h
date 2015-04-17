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

#include	<glade/glade.h>
#include	<glib.h>
#include	<gconf/gconf-client.h>

#include	"libmondai.h"
#include	"net.h"
#include	"queue.h"

#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

typedef struct {
	NETFILE		*fpComm;
	Port		*port;
	char		*title;
	char		*bgcolor;
	Bool		IsRecv;
	char		*FocusedWindow;
	char		*FocusedWidget;
	char		*ThisWindow;
	GHashTable	*WindowTable;
	GHashTable	*WidgetTable;
	GList		*PrintList;
	GList		*DLList;
#ifdef	USE_SSL
	SSL_CTX		*ctx;
#ifdef  USE_PKCS11
	ENGINE		*engine;
#endif  /* USE_PKCS11 */
#endif  /* USE_SSL */
}	GLSession;

#define	FPCOMM(session)			((session)->fpComm)
#define PORT(session)			((session)->port)
#define	TITLE(session)			((session)->title)
#define	BGCOLOR(session)		((session)->bgcolor)
#define	ISRECV(session)			((session)->IsRecv)
#define	FOCUSEDWINDOW(session)	((session)->FocusedWindow)
#define	FOCUSEDWIDGET(session)	((session)->FocusedWidget)
#define	THISWINDOW(session)		((session)->ThisWindow)
#define	WINDOWTABLE(session)	((session)->WindowTable)
#define	WIDGETTABLE(session)	((session)->WidgetTable)
#define	PRINTLIST(session)		((session)->PrintList)
#define	DLLIST(session)			((session)->DLList)
#ifdef	USE_SSL
#define	CTX(session)			((session)->ctx)
#ifdef	USE_PKCS11
#define	ENGINE(session)			((session)->engine)
#endif	/* USE_PKCS11 */
#endif	/* USE_SSL */

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

extern	void		ExitSystem(void);
extern  void		SetSessionTitle(const char *title);
extern  void		SetSessionBGColor(const char *color);

GLOBAL	Bool		DelayDrawWindow;
GLOBAL	Bool		CancelScaleWindow;
GLOBAL	char		*CurrentApplication;
GLOBAL	Bool		fV47;
GLOBAL	char		*TempDir;
GLOBAL	char		*ConfDir;
GLOBAL	GLSession	*Session;

GLOBAL	char		*Host;
GLOBAL	char		*PortNum;
GLOBAL	char		*User;
GLOBAL	char		*Pass;
GLOBAL	Bool		SavePass;
GLOBAL	char		*Style; 
GLOBAL	char		*Gtkrc; 
GLOBAL	Bool		fMlog;
GLOBAL	Bool		fKeyBuff;
GLOBAL	Bool		fIMKanaOff;
GLOBAL	Bool		fTimer;
GLOBAL	char		*TimerPeriod;
GLOBAL	int			PingTimerPeriod;
GLOBAL	char		*FontName;
#ifdef	USE_SSL
GLOBAL	Bool		fSsl;
GLOBAL	char		*CertFile;
GLOBAL	char		*CA_File;
GLOBAL	char		*Ciphers;
GLOBAL	char		*Passphrase;
#ifdef  USE_PKCS11
GLOBAL	Bool		fPKCS11;
GLOBAL	char		*PKCS11_Lib;
GLOBAL	char		*Slot;
#endif	/* USE_PKCS11 */
#endif	/* USE_SSL */

/* gconf */
GLOBAL	GConfClient	*GConfCTX;
GLOBAL	gchar		*ConfigName;

#define GL_GCONF_BASE				"/apps/glclient"
#define GL_GCONF_SERVERS			(GL_GCONF_BASE "/servers")
#define GL_GCONF_DEFAULT_SERVER		(GL_GCONF_BASE "/servers/1")
#define GL_GCONF_SERVER				(GL_GCONF_BASE "/server")
#define GL_GCONF_NEXT_ID			(GL_GCONF_BASE "/next_id")
#define GL_GCONF_CONF_CONVERTED		(GL_GCONF_BASE "/confconverted")
#define GL_GCONF_WCACHE				(GL_GCONF_BASE "/widgetcache")
#define GL_GCONF_WCACHE_CONVERTED	(GL_GCONF_BASE "/wcacheconverted")

#endif
