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

#ifndef	_INC_ACTION_H
#define	_INC_ACTION_H
#include	"glclient.h"

#define	DEFAULT_WINDOW_WIDTH	1024
#define	DEFAULT_WINDOW_HEIGHT	768
#define	DEFAULT_WINDOW_FOOTER	24

#undef	GLOBAL
#ifdef	ACTION_MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

typedef struct {
	float v;
	float h;
} WindowScale;

GLOBAL	GtkWidget	*TopWindow;
GLOBAL	GtkWidget	*TopNoteBook;
GLOBAL	GList		*DialogStack;
GLOBAL	WindowScale	TopWindowScale;

extern	void		RegisterChangedHandler(GObject *object, 
						GCallback func, gpointer data);
extern	void		BlockChangedHandlers(void);
extern	void		UnblockChangedHandlers(void);
extern	GtkWidget	*GetWindow(	GtkWidget	*widget);
extern	char		*GetWindowName(	GtkWidget	*widget);
extern	void		ClearKeyBuffer(void);
extern	void		ResetTimer(char *wname);
extern	void		_AddChangedWidget(GtkWidget *widget);
extern	void		AddChangedWidget(GtkWidget *widget);
extern	void		ShowBusyCursor(GtkWidget *widget);
extern	void		StopTimerWidgetAll(void);
extern	void		HideBusyCursor(GtkWidget *widget);
extern	void		ResetScrolledWindow(char *windowName);
extern  void		SetTitle(GtkWidget *window);
extern  void		SetBGColor(GtkWidget *window);
extern	GtkWidget	*GetWidgetByLongName(const char *longName);
extern	GtkWidget	*GetWidgetByName(const char *dataName);
extern	void		ConfigureWindow(GtkWidget *widget,GdkEventConfigure *ev, 
						gpointer data);
extern	void		InitTopWindow(void);
extern	gboolean	IsDialog(GtkWidget *widget);
extern  gboolean    IsWidgetName(char *name);
extern  gboolean    IsWidgetName2(char *name);

extern	void		ListConfig();
extern	void		LoadConfig(const char *configname);
extern  void        UI_Init(int argc, char **argv);
extern	void		UI_Main(void);
extern	void		InitStyle(void);
extern	int			AskPass(char *buf, size_t buflen,const char	*prompt);
extern	void		SetPingTimerFunc();
extern	WindowData	*GetWindowData(const char *wname);
extern	void		SendEvent(const char*window,const char*widget,
						const char*event);
extern	void		UpdateScreen();

#endif

