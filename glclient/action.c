/*
PANDA -- a simple transaction monitor
Copyright (C) 1998-1999 Ogochan.
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include    <sys/types.h>
#include	<sys/stat.h>
#include    <unistd.h>
#include	<sys/time.h>
#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif

#include	"types.h"
#include	"glclient.h"
#include	"glterm.h"
#include	"action.h"
#include	"message.h"
#include	"debug.h"

static	void
ClearWindowData(
	char		*wname,
	XML_Node	*node,
	gpointer	user_data)
{
ENTER_FUNC;
	if		(  node->UpdateWidget  !=  NULL  ) { 
		g_hash_table_destroy(node->UpdateWidget);
	}
	node->UpdateWidget = NewNameHash();
LEAVE_FUNC;
}

extern	void
ClearWindowTable(void)
{
ENTER_FUNC;
	if		(  WindowTable  !=  NULL  ) {
		g_hash_table_foreach(WindowTable,(GHFunc)ClearWindowData,NULL);
	}
LEAVE_FUNC;
}

static	gint
_GrabFocus(gpointer data)
{
	gtk_widget_grab_focus(data);
	return FALSE;
}

extern	void
GrabFocus(GtkWidget *widget)
{
ENTER_FUNC;
	gtk_idle_add(_GrabFocus, widget);
LEAVE_FUNC;
}

static	void
_ResetTimer(
	    GtkWidget	*widget,
	    gpointer	data)
{
	if (GTK_IS_CONTAINER (widget))
		gtk_container_forall (GTK_CONTAINER (widget), _ResetTimer, NULL);
	else if (GTK_IS_PANDA_TIMER (widget))
		gtk_panda_timer_reset (GTK_PANDA_TIMER (widget));
}

extern	void
ResetTimer(
	GtkWindow	*window)
{
	gtk_container_forall (GTK_CONTAINER (window), _ResetTimer, NULL);
}

extern	void
_UpdateWidget(
	GtkWidget	*widget,
	void		*user_data)
{
	const	char	*name;
	char	*wname;
	XML_Node	*node;
	GtkWidget	*window;

ENTER_FUNC;
	window = gtk_widget_get_toplevel(widget);
	ResetTimer(GTK_WINDOW (window));
	name = glade_get_widget_long_name(widget);
	wname = gtk_widget_get_name(window);
	if		( ( node = g_hash_table_lookup(WindowTable,wname) )  !=  NULL  ) {
		if	(  g_hash_table_lookup(node->UpdateWidget,name)      ==  NULL  ) {
			g_hash_table_insert(node->UpdateWidget,(char *)name,widget);
		}
	}
LEAVE_FUNC;
}

extern	void
UpdateWidget(
	GtkWidget	*widget,
	void		*user_data)
{
	if		(  !fInRecv  ) {
		_UpdateWidget(widget,user_data);
	}
}

extern	XML_Node	*
ShowWindow(
	char	*wname,
	byte	type)
{
	char		*fname;
	XML_Node	*node;
	GdkEvent	*event; 
ENTER_FUNC;
	dbgprintf("ShowWindow [%s][%d]",wname,type);
	fname = CacheFileName(wname);

	if		(  ( node = g_hash_table_lookup(WindowTable,wname) )  ==  NULL  ) {
		/* Create new node */
		switch	(type) {
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CURRENT_WINDOW:
			node = New(XML_Node);
			node->xml = glade_xml_new(fname, NULL);
			node->window = GTK_WINDOW(glade_xml_get_widget(node->xml, wname));
			node->name = StrDup(wname);
			node->UpdateWidget = NewNameHash();
			glade_xml_signal_autoconnect(node->xml);
			g_hash_table_insert(WindowTable,node->name,node);
		}
	}

	if		(  node  !=  NULL  ) {
		switch	(type) {
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CURRENT_WINDOW:
			gtk_widget_show_all(GTK_WIDGET(node->window));
			break;
		  case	SCREEN_CLOSE_WINDOW:
			while( event = gdk_event_get()) {
				if ( event->type == GDK_KEY_PRESS ) {
					gdk_event_free(event);
				} else {
					gtk_main_do_event(event);
				}
			}
			gtk_widget_hide_all(GTK_WIDGET(node->window));
			/* fall through */
		  default:
			node = NULL;
			break;
		}
	}

LEAVE_FUNC;
	return	(node);
}

extern	void
DestroyWindow(
	char	*sname)
{
	XML_Node	*node;

	if		(  ( node = (XML_Node *)g_hash_table_lookup(WindowTable,sname) )
			   !=  NULL  ) {
		gtk_widget_destroy(GTK_WIDGET(node->window));
		gtk_object_destroy((GtkObject *)node->xml);
		if		(  node->UpdateWidget  !=  NULL  ) {
			g_hash_table_destroy(node->UpdateWidget);
		}
		xfree(node->name);
		xfree(node);
		g_hash_table_remove(WindowTable,sname);
	}
}

static void
destroy_window_one(char *wname, XML_Node *node, gpointer user_data)
{
    gtk_widget_destroy(GTK_WIDGET(node->window));
    node->window = NULL;
}

extern void
DestroyWindowAll()
{
    g_hash_table_foreach(WindowTable, (GHFunc)destroy_window_one, NULL);
}
