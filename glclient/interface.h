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

#include 	<gtk/gtk.h>
#include	"marshaller.h"

#ifndef	_INC_INTERFACE_H
#define	_INC_INTERFACE_H

#define	DEFAULT_WINDOW_WIDTH	1024
#define	DEFAULT_WINDOW_HEIGHT	768
#define	DEFAULT_WINDOW_FOOTER	24

#undef	GLOBAL
#ifdef	INTERFACE_MAIN
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

typedef	gint		(*_PingTimerFunc)	(gpointer data);

extern	void		UI_list_config();
extern	void		UI_load_config(char *configname);

extern	void		UI_CreateWindow(char *sname,int size, char *buff);
extern	void		UI_CloseWindow(char *sname);
extern	void		UI_ShowWindow(char *sname);
extern	void		UI_UpdateScreen(char *window);
extern	void		UI_ResetScrolledWindow(char *window);
extern	void		UI_GrabFocus(char *window, char *widgetName);
extern  void 		UI_GetWidgetData(WidgetData *data);
extern	WidgetType	UI_GetWidgetType(char *windowname, char *widgetName);
extern	gboolean	UI_IsWidgetName(char *name);
extern	gboolean	UI_IsWidgetName2(char *dataname);
extern	void		UI_MessageDialog(const char *msg);
extern	void		UI_ErrorDialog(const char *msg);
extern	int			UI_AskPass(char *buff, size_t buflen, const char *prmpt);
extern	void		UI_BootDialogRun(void);
extern	void		UI_SetPingTimerFunc(_PingTimerFunc func, gpointer data);
extern	void		UI_Notify(char *summary,char *body, char *icon,int timeout);

extern	void		UI_Init(int argc, char **argv);
extern	void		UI_InitStyle(void);
extern	void		UI_InitTopWindow(void);
extern	void		UI_Main(void);

#endif