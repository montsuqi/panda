/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2004 Ogochan & JMA (Japan Medical Association).

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
#ifdef ENABLE_GNOME
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
dbgmsg(">ClearWindowData");
	if		(  node->UpdateWidget  !=  NULL  ) { 
		g_hash_table_destroy(node->UpdateWidget);
	}
	node->UpdateWidget = NewNameHash();
dbgmsg("<ClearWindowData");
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
	gtk_idle_add(_GrabFocus, widget);
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
UpdateWidget(
	GtkWidget	*widget,
	void		*user_data)
{
	const	char	*name;
	char	*wname;
	XML_Node	*node;
	GtkWidget	*window;

ENTER_FUNC;
	if		(  !fInRecv  ) {
		window = gtk_widget_get_toplevel(widget);
		ResetTimer(GTK_WINDOW (window));
		name = glade_get_widget_long_name(widget);
		wname = gtk_widget_get_name(window);
		if		( ( node = g_hash_table_lookup(WindowTable,wname) )  !=  NULL  ) {
			if	(  g_hash_table_lookup(node->UpdateWidget,name)      ==  NULL  ) {
				g_hash_table_insert(node->UpdateWidget,(char *)name,widget);
			}
		}
	}
LEAVE_FUNC;
}

extern	XML_Node	*
ShowWindow(
	char	*wname,
	byte	type)
{
	char		*fname;
	XML_Node	*node;

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

