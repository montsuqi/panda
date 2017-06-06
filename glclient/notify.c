/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2011 JMA (Japan Medical Association).
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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<gtk/gtk.h>

#include	"notify.h"

static int yy = 0;
static int n = 0;

static gboolean
DestroyDialog(gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
	if (n > 0) {
		n -= 1;
	}
	return FALSE;
}

static gboolean
cb_enter(GtkWidget *widget,
	GdkEvent *e,
	gpointer data)
{
	gtk_widget_hide(widget);
	return FALSE;
}

void
Notify(gchar *summary,
	gchar *body,
	gchar *icon,
	gint timeout)
{
	GtkWidget *widget;
	int x,y,w,h;

	widget = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_OTHER,GTK_BUTTONS_NONE,"%s\n\n%s",summary,body);
	gtk_window_set_decorated(GTK_WINDOW(widget),FALSE);
	gtk_window_set_modal(GTK_WINDOW(widget),FALSE);
	gtk_window_set_focus_on_map(GTK_WINDOW(widget),FALSE);
	gtk_window_set_accept_focus(GTK_WINDOW(widget),FALSE);
	gtk_window_set_gravity(GTK_WINDOW(widget), GDK_GRAVITY_NORTH_EAST);

	if (n == 0) {
		yy = 0;
	}

	g_signal_connect(G_OBJECT(widget),"enter-notify-event",G_CALLBACK(cb_enter),NULL);

	gtk_widget_show(widget);
	gtk_window_get_position(GTK_WINDOW(widget),&x,&y);
	gtk_window_get_size(GTK_WINDOW(widget),&w,&h);
	if (timeout < 1) {
		timeout = 5;
	}
	gtk_window_move(GTK_WINDOW(widget),w,y);
	gtk_timeout_add(timeout*1000,DestroyDialog,widget);
	n += 1;
	yy = yy + h;
}
