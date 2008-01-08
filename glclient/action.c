/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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

static struct changed_hander {
	GtkObject       *object;
	GtkSignalFunc	func;
	gpointer	data;
	gint		block_flag;
	struct changed_hander *next;
} *changed_hander_list = NULL;

static Bool TimerFlag = FALSE;

extern	void
RegisterChangedHandler(
	GtkObject *object,
	GtkSignalFunc func,
	gpointer data)
{
  struct changed_hander *p = xmalloc (sizeof (struct changed_hander));
ENTER_FUNC;
	p->object = object;
	p->func = func;
	p->data = data;
	p->next = changed_hander_list;
	p->block_flag = FALSE;
	changed_hander_list = p;
LEAVE_FUNC;
}

extern void
BlockChangedHandlers(void)
{
  struct changed_hander *p;

ENTER_FUNC;
	for (p = changed_hander_list; p != NULL; p = p->next) {
		p->block_flag = TRUE;		 
		gtk_signal_handler_block_by_func (p->object, p->func, p->data);
	}
LEAVE_FUNC;
}

extern void
UnblockChangedHandlers(void)
{
  struct changed_hander *p;
ENTER_FUNC;
	for (p = changed_hander_list; p != NULL; p = p->next) {
		if (p->block_flag) {
			gtk_signal_handler_unblock_by_func (p->object, p->func, p->data);
		}
	}
LEAVE_FUNC;
}

extern	GtkWidget	*
GetWindow(
	GtkWidget	*widget)
{
	GtkWidget	*window;
ENTER_FUNC;
	window = gtk_widget_get_toplevel(widget);
LEAVE_FUNC;
	return (window);
}

extern	char	*
GetWindowName(
	GtkWidget	*widget)
{
	GtkWidget	*window;
	static char	wname[SIZE_LONGNAME];

ENTER_FUNC;
	window = GetWindow(widget);
#if	1	/*	This logic is escape code for GTK bug.	*/
	strcpy(wname,glade_get_widget_long_name(widget));
	*(strchr(wname,'.')) = 0;
#else
	strcpy(wname,gtk_widget_get_name(window));
#endif
LEAVE_FUNC;
	return (wname);
}

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
	static guint tag = 0;

	gtk_widget_grab_focus(widget);
	if (tag != 0) {
		gtk_idle_remove (tag);	
	}
	/* Do not erase.  The interval is necessary in GtkCombo. */
	tag = gtk_idle_add(_GrabFocus, widget);
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
	if		(  TimerFlag  )	{
		ResetTimer(GTK_WINDOW (window));
	}
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

extern	void
ClearKeyBuffer(void)
{
	GdkEvent	*event; 
ENTER_FUNC;
	while( (event = gdk_event_get()) != NULL) {
		if ( (event->type == GDK_KEY_PRESS ||
			  event->type == GDK_KEY_RELEASE) ) {
 			gdk_event_free(event); 
		} else {
			gtk_main_do_event(event);
		}
	}
LEAVE_FUNC;
}

extern	void
StartTimer(
	char		*event,
	int			timeout,
	GtkFunction function,
	GtkWidget	*widget)
{
	GtkWidget	*window;
	static gint timeout_handler_id;
ENTER_FUNC;
	window = gtk_widget_get_toplevel(widget);
	gtk_object_set_data(GTK_OBJECT(window), "timeout_event", event);
	timeout_handler_id = gtk_timeout_add (timeout, function, widget);
	gtk_object_set_data(GTK_OBJECT(window), "timeout_handler_id", &timeout_handler_id);
	TimerFlag = TRUE;
LEAVE_FUNC;
}

extern	char	*
GetTimerEvent(
	GtkWindow	*window)
{
	static char *timeout_event;
ENTER_FUNC;
	timeout_event = (char *)gtk_object_get_data(GTK_OBJECT(window), "timeout_event");
LEAVE_FUNC;	
	return (timeout_event);
}

extern	void
ResetTimer(
	GtkWindow	*window)
{
	gtk_container_forall (GTK_CONTAINER (window), _ResetTimer, NULL);
}

extern void
StopTimer(
		GtkWindow	*window)
{
	gint *timeout_handler_id;
ENTER_FUNC;	
	if (TimerFlag) {
		timeout_handler_id = gtk_object_get_data(GTK_OBJECT(window), 
												 "timeout_handler_id");
		if ((timeout_handler_id) && (*timeout_handler_id != 0)) {
			gtk_timeout_remove(*timeout_handler_id);
			*timeout_handler_id = 0;
		}
		gtk_object_set_data(GTK_OBJECT(window), "timeout_handler_id", 
							timeout_handler_id);
		TimerFlag = FALSE;
	}
LEAVE_FUNC;
}

static	XML_Node	*
CreateNewNode(
	char	*wname)
{
	char	*fname;
	GladeXML	*xml;
	XML_Node	*node;
ENTER_FUNC;
	fname = CacheFileName(wname);
	xml = glade_xml_new(fname, NULL);
	if ( xml == NULL ) {
		node = NULL;
	} else {
		DestroyWindow(wname);
		node = New(XML_Node);
		node->xml = xml;
		node->window = GTK_WINDOW(glade_xml_get_widget(node->xml, wname));
		node->name = StrDup(wname);
		node->UpdateWidget = NewNameHash();
		glade_xml_signal_autoconnect(node->xml);
		g_hash_table_insert(WindowTable,node->name,node);
	}
LEAVE_FUNC;
	return (node);
}

extern	XML_Node	*
ShowWindow(
	char	*wname,
	byte	type)
{
	char		*fname;
	XML_Node	*node;
	GtkWidget	*widget;
ENTER_FUNC;
	widget = NULL;
	dbgprintf("ShowWindow [%s][%d]",wname,type);
	fname = CacheFileName(wname);

	if		(  ( node = g_hash_table_lookup(WindowTable,wname) )  ==  NULL  ) {
		if		(	(  type  ==  SCREEN_NEW_WINDOW      )
				||	(  type  ==  SCREEN_CURRENT_WINDOW  ) ){
			node = CreateNewNode(wname);
		}
	}

	if		(  node  !=  NULL  ) {
		switch	(type) {
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CURRENT_WINDOW:
			gtk_widget_set_sensitive (GTK_WIDGET(node->window), TRUE);
			gtk_widget_show_all(GTK_WIDGET(node->window));
			break;
		  case	SCREEN_CLOSE_WINDOW:
			StopTimer(node->window);
			if (node->window->focus_widget != NULL ){
				widget = GTK_WIDGET(node->window->focus_widget);
			}
			gtk_widget_set_sensitive (GTK_WIDGET(node->window), FALSE);
			if ((widget != NULL) && GTK_IS_BUTTON (widget)){
				gtk_button_released (GTK_BUTTON(widget));
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
	char		*key;
	gpointer	n
	,			k;

	if		(  g_hash_table_lookup_extended(WindowTable,sname,&n,&k)  )	{
		node = (XML_Node *)n;
		key = (char *)k;
		gtk_widget_destroy(GTK_WIDGET(node->window));
		gtk_object_destroy((GtkObject *)node->xml);
		if		(  node->UpdateWidget  !=  NULL  ) {
			g_hash_table_destroy(node->UpdateWidget);
		}
		xfree(node->name);
		xfree(node);
		xfree(key);
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

static	GdkCursor *Busycursor;

extern	GdkWindow	*
ShowBusyCursor(
	GtkWidget	*widget)
{
	static GdkWindow	*pane;
	GtkWidget	*window;
	GdkWindowAttr	attr;
ENTER_FUNC;
	memset (&attr, 0, sizeof (GdkWindowAttr));
	attr.wclass = GDK_INPUT_ONLY;
	attr.window_type = GDK_WINDOW_CHILD;
	Busycursor = gdk_cursor_new (GDK_WATCH);
	attr.cursor = Busycursor;
	attr.x = attr.y = 0;
	attr.width = attr.height = 32767;

	if		(  widget  !=  NULL  ) {
		window = gtk_widget_get_toplevel(widget);
	} else {
		window = NULL;
	}
	pane = gdk_window_new(window->window, &attr, GDK_WA_CURSOR);
	gdk_window_show (pane);
	gdk_flush ();
LEAVE_FUNC;
	return	(pane); 
}

extern	void
HideBusyCursor(GdkWindow *pane)
{
ENTER_FUNC;
	gdk_cursor_destroy (Busycursor);
	gdk_window_destroy (pane);
LEAVE_FUNC;
}

