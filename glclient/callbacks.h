/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_CALLBACKS_H
#define	_CALLBACKS_H

#ifdef ENABLE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif

extern	gboolean	select_all(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);
extern	gboolean	unselect_all(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);
extern	gboolean	press_filter(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
extern	void	send_event(GtkWidget *widget, char *event);
extern	void	send_event_if_changed(GtkWidget *widget, GdkEventFocus *focus, char *event);
extern	void	send_event_when_idle(GtkWidget *widget, char *event);
extern	void	entry_next_focus(GtkEntry *entry, char *next);
extern	void	entry_changed(GtkWidget *entry, gpointer user_data);
extern	void	togglebutton_toggled(GtkWidget *entry, gpointer user_data);
extern	void	entry_set_editable(GtkEntry *entry, gpointer user_data);
extern	void	map_event(GtkWidget *widget, gpointer user_data);
extern	void	window_close(GtkWidget *widget, gpointer user_data);
#ifdef	GTK_PANDA_TYPE_HTML
extern	void	open_browser(GtkPandaHTML *html, XmHTMLAnchorCallbackStruct *cbs, void *data);
#endif
extern	void	ClearWindowTable(void);
#endif
