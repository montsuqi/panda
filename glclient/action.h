/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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

extern	void		RegisterChangedHandler(GtkObject *object, GtkSignalFunc func, gpointer data);
extern	void		BlockChangedHandlers(void);
extern	void		UnblockChangedHandlers(void);
extern	GtkWidget	*GetWindow(	GtkWidget	*widget);
extern	char		*GetWindowName(	GtkWidget	*widget);
extern	void		GrabFocus(GtkWidget *widget);
extern	XML_Node	*ShowWindow(char *wname, byte type);
extern	void		DestroyWindow(char *sname);
extern	void		DestroyWindowAll();
extern	void		ClearWindowTable(void);
extern	void		ClearKeyBuffer(void);
extern	void		_UpdateWidget(GtkWidget *widget, void *user_data);
extern	void		UpdateWidget(GtkWidget *widget, void *user_data);
extern	GdkWindow	*ShowBusyCursor(GtkWidget *widget);
extern	void		HideBusyCursor(GdkWindow *pane);
extern	void		StartTimer( char *event, int timeout,
							  GtkFunction function, GtkWidget *widget);
extern	char		*GetTimerEvent(GtkWindow *window);
extern	void		ResetTimer(GtkWindow *window);
extern	void		StopTimer(GtkWindow *window);
extern	gpointer	*GetObjectData(GtkWidget	*widget, char *object_key);
extern	void		SetObjectData(GtkWidget	*widget, char *object_key, gpointer	*data);

#endif
