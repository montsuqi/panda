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

extern	void		RegisterChangedHandler(GtkObject *object, 
						GtkSignalFunc func, gpointer data);
extern	void		BlockChangedHandlers(void);
extern	void		UnblockChangedHandlers(void);
extern	GtkWidget	*GetWindow(	GtkWidget	*widget);
extern	char		*GetWindowName(	GtkWidget	*widget);
extern	void		GrabFocus(GtkWidget *widget);
extern	void		CreateWindow(char *wname,int size, char *buff);
extern	void		ShowWindow(char *wname);
extern	void		CloseWindow(char *wname);
extern	void		ClearKeyBuffer(void);
extern	void		_AddChangedWidget(GtkWidget *widget);
extern	void		AddChangedWidget(GtkWidget *widget);
extern	GdkWindow	*ShowBusyCursor(GtkWidget *widget);
extern	void		HideBusyCursor(GdkWindow *pane);
extern  void 		ResetScrolledWindow(GtkWidget *widget, gpointer user_data);
extern	void		StopTimerWidgetAll(void);
extern	void		SetTitle(GtkWidget *window,char *window_title);
extern	GtkWidget	*GetWidgetByLongName(char *longName);
extern	GtkWidget	*GetWidgetByName(char *dataName);
extern	GtkWidget	*GetWidgetByWindowNameAndLongName(char *wname, char *name);
extern	GtkWidget	*GetWidgetByWindowNameAndName(char *wname, char *name);
extern	void		SetPingTimer(void);
extern	void		ScaleWindow(GtkWidget *widget,GtkAllocation *alloc);

#endif
