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
#include 	<gtk/gtk.h>
#include	<gtkpanda/gtkpanda.h>

#include	"types.h"
#include	"glclient.h"
#include	"glterm.h"
#include	"message.h"
#include	"debug.h"
#include	"marshaller.h"
#define		_ACTION_C
#include	"action.h"
#include	"queue.h"
#include	"gettext.h"
#include	"toplevel.h"

static struct changed_hander {
	GObject					*object;
	GCallback				func;
	gpointer				data;
	gint					block_flag;
	struct changed_hander 	*next;
} *changed_hander_list = NULL;

static Bool TimerFlag = FALSE;

extern	void
RegisterChangedHandler(
	GObject *object,
	GCallback func,
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
		g_signal_handlers_block_by_func (p->object, p->func, p->data);
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
			g_signal_handlers_unblock_by_func (p->object, p->func, p->data);
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
		g_source_remove (tag);	
	}
	/* Do not erase.  The interval is necessary in GtkCombo. */
	tag = g_idle_add(_GrabFocus, widget);
}

extern  void
ResetScrolledWindow(
    GtkWidget   *widget,
    gpointer    user_data)
{
    GtkAdjustment   *adj;

    if  (   GTK_IS_SCROLLED_WINDOW(widget)  ) {
		adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(widget));
		if	(	adj	) {
			adj->value = 0.0;
			gtk_adjustment_value_changed(adj);
		}
		adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget));
		if	(	adj	) {
			adj->value = 0.0;
			gtk_adjustment_value_changed(adj);
		}
    }
    if  (   GTK_IS_CONTAINER(widget)    ) {
        gtk_container_forall(GTK_CONTAINER(widget), ResetScrolledWindow, NULL);
    }
}

static	void
_RegistTimer(
	GtkWidget	*widget,
	gpointer	data)
{
	if(GTK_IS_CONTAINER(widget))
		gtk_container_forall(GTK_CONTAINER(widget), _RegistTimer, data);
	else if (GTK_IS_PANDA_TIMER(widget))
		g_hash_table_insert((GHashTable*)data, 
			(char *)gtk_widget_get_name(widget), widget);
}

static	void
_ResetTimer(
	gpointer	key,
	gpointer	value,
	gpointer	data)
{
	gtk_widget_show(GTK_WIDGET(value));
	gtk_panda_timer_reset (GTK_PANDA_TIMER(value));
}

static	void
_StopTimer(
	gpointer	key,
	gpointer	value,
	gpointer	data)
{
	gtk_widget_hide(GTK_WIDGET(value));
	gtk_panda_timer_stop(GTK_PANDA_TIMER(value));
}

extern	void
_AddChangedWidget(
	GtkWidget	*widget)
{
	char		*name;
	char		*wname;
	char		*key;
	char		*tail;
	GtkWidget	*window;
	WindowData	*wdata;

ENTER_FUNC;
	window = gtk_widget_get_toplevel(widget);
	name = (char *)glade_get_widget_long_name(widget);
	tail = strchr(name, '.');
	if (tail == NULL) {
		wname = StrDup(name);
	} else {
		wname = StrnDup(name, tail - name);
	}
	if ((wdata = g_hash_table_lookup(WindowTable,wname)) != NULL) {
		if (g_hash_table_lookup(wdata->ChangedWidgetTable, name) == NULL) {
			key = StrDup(name);
			g_hash_table_insert(wdata->ChangedWidgetTable, key, key);
		}
		if		(  TimerFlag  )	{
			g_hash_table_foreach(wdata->TimerWidgetTable, _ResetTimer, NULL);
		}
	}
	free(wname);
LEAVE_FUNC;
}

extern	void
AddChangedWidget(
	GtkWidget	*widget)
{
	if		(  !fInRecv  ) {
		_AddChangedWidget(widget);
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
	GSourceFunc	function,
	GtkWidget	*widget)
{
	GtkWidget	*window;
	static gint timeout_handler_id;
ENTER_FUNC;
	window = gtk_widget_get_toplevel(widget);
	g_object_set_data(G_OBJECT(window), "timeout_event", event);
	timeout_handler_id = g_timeout_add (timeout, function, widget);
	g_object_set_data(G_OBJECT(window), "timeout_handler_id", &timeout_handler_id);
	TimerFlag = TRUE;
LEAVE_FUNC;
}

extern	char	*
GetTimerEvent(
	GtkWindow	*window)
{
	static char *timeout_event;
ENTER_FUNC;
	timeout_event = (char *)g_object_get_data(G_OBJECT(window), "timeout_event");
LEAVE_FUNC;	
	return (timeout_event);
}

extern void
StopTimer(
		GtkWindow	*window)
{
	gint *timeout_handler_id;
ENTER_FUNC;	
	if (TimerFlag) {
		timeout_handler_id = g_object_get_data(G_OBJECT(window), 
												 "timeout_handler_id");
		if ((timeout_handler_id) && (*timeout_handler_id != 0)) {
			g_source_remove(*timeout_handler_id);
			*timeout_handler_id = 0;
		}
		g_object_set_data(G_OBJECT(window), "timeout_handler_id", 
							timeout_handler_id);
		TimerFlag = FALSE;
	}
LEAVE_FUNC;
}

extern	void
SetTitle(GtkWidget	*window,
	char *window_title)
{
	char		buff[SIZE_BUFF];

	if ( glSession->title != NULL && strlen(glSession->title) > 0 ) {
		snprintf(buff, sizeof(buff), "%s - %s", window_title, glSession->title);
	} else {
		snprintf(buff, sizeof(buff), "%s", window_title);
	}
	gtk_window_set_title (GTK_WINDOW(window), buff);
}

extern	void	
CreateWindow(
	char	*wname)
{
	char		*fname;
	GladeXML	*xml;
	WindowData	*wdata;
	GtkWidget	*window;
	GtkWidget	*child;

ENTER_FUNC;
	if (g_hash_table_lookup(WindowTable,wname) != NULL) {
		dbgprintf("%s already in WindowTable", wname);
		return;
	}
	fname = CacheFileName(wname);
	strcat(fname, ".utf8");
	xml = glade_xml_new(fname, NULL);
	if ( xml == NULL ) {
		dbgmsg("no xml");
		return;
	}
	window = glade_xml_get_widget_by_long_name(xml, wname);
	if (window == NULL) {
		Warning("Window %s not found in %s", wname, fname);
		return;
	}
	wdata = New(WindowData);
	wdata->xml = xml;
	wdata->name = StrDup(wname);
	wdata->title = StrDup(GTK_WINDOW (window)->title);
	wdata->fAccelGroup = FALSE;
	wdata->ChangedWidgetTable = NewNameHash();
	wdata->TimerWidgetTable = NewNameHash();
	wdata->UpdateWidgetQueue = NewQueue();
	glade_xml_signal_autoconnect(xml);
	g_hash_table_insert(WindowTable,strdup(wname),wdata);

	child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
	g_return_if_fail(child != NULL);
	gtk_widget_show_all(child);
	gtk_container_forall(GTK_CONTAINER(child), _RegistTimer, wdata->TimerWidgetTable);
	if (strstr(GTK_WINDOW(window)->wmclass_class, "dialog") != NULL) {
		dbgprintf("create dialog:%s\n", wname);
		gtk_container_add(GTK_CONTAINER(window), child); 
		wdata->fWindow = FALSE;
	} else {
		dbgprintf("create window:%s\n", wname);
		gtk_notebook_append_page(GTK_NOTEBOOK(TopNoteBook), 
			child, gtk_label_new(wname));
		wdata->fWindow = TRUE;
	}
LEAVE_FUNC;
}

static	void
SwitchWindow(
	GtkWidget *window)
{
	int			width, height;
	GtkWidget	*child;

ENTER_FUNC;
	child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
	g_return_if_fail(child != NULL);
	gtk_notebook_set_page(GTK_NOTEBOOK(TopNoteBook), 
		gtk_notebook_page_num(GTK_NOTEBOOK(TopNoteBook), child));

	gtk_window_get_default_size(GTK_WINDOW(window), &width, &height);
	gtk_window_set_default_size(GTK_WINDOW(TopWindow), width, height);
	gtk_widget_set_name(TopWindow, gtk_widget_get_name(window));
	gtk_window_set_resizable(GTK_WINDOW(TopWindow),
		GTK_WINDOW(window)->allow_grow);

	gtk_widget_show(TopWindow);
	gtk_widget_show(TopNoteBook);
	gtk_widget_show(child);
LEAVE_FUNC;
}

extern	void
CloseWindow(
	char	*wname)
{
	WindowData	*data;
	GtkWidget	*window;
	GtkWidget	*child;
	GSList		*list;

ENTER_FUNC;
	dbgprintf("close window:%s\n", wname);
	if ((data = g_hash_table_lookup(WindowTable,wname)) == NULL) {
		// FIXME sometimes comes here.
		fprintf(stderr,"%s:%d data %s is NULL\n", __FILE__, __LINE__, wname);
		return;
	}
	g_hash_table_foreach(data->TimerWidgetTable, _StopTimer, NULL);
	window = glade_xml_get_widget_by_long_name((GladeXML *)data->xml, wname);
	if (data->fWindow) {
		StopTimer(GTK_WINDOW(TopWindow));
		child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
		if (data->fAccelGroup) {
			for(list = ((GladeXML*)data->xml)->priv->accel_groups;
				list != NULL;
				list = list->next) {
				if (list->data != NULL) {
					gtk_window_remove_accel_group(GTK_WINDOW(TopWindow),
						(GtkAccelGroup*)(list->data));
				}
			}
			data->fAccelGroup = FALSE;
		}
	} else {
		StopTimer(GTK_WINDOW(window));
		gtk_widget_hide(window);
		gtk_window_set_modal(GTK_WINDOW(window), FALSE);
		DialogStack = g_list_remove(DialogStack, window);
	}
LEAVE_FUNC;
}

extern	void
ShowWindow(
	char	*wname)
{
	WindowData	*data;
	GtkWidget	*window;
	GSList		*list;

ENTER_FUNC;
	dbgprintf("show window:%s\n", wname);
	if ((data = g_hash_table_lookup(WindowTable,wname)) == NULL) {
		// FIXME sometimes comes here.
		fprintf(stderr,"%s:%d data is NULL\n", __FILE__, __LINE__);
		return;
	}
	window = glade_xml_get_widget_by_long_name((GladeXML *)data->xml, wname);
	g_return_if_fail(window != NULL);

	g_hash_table_foreach(data->TimerWidgetTable, _ResetTimer, NULL);
	if (data->fWindow) {
	dbgmsg("show primari window\n");
		SetTitle(TopWindow, data->title);
		if (strcmp(wname, gtk_widget_get_name(TopWindow))) {
			SwitchWindow(window);
			if (!data->fAccelGroup) {
				for(list = ((GladeXML*)data->xml)->priv->accel_groups;
					list != NULL;
					list = list->next) {
					if (list->data != NULL) {
					gtk_window_add_accel_group(GTK_WINDOW(TopWindow),
						(GtkAccelGroup*)(list->data));
					}
				}
				data->fAccelGroup = TRUE;
			}
		}
	} else {
	dbgmsg("show dialog\n");
		GtkWidget *parent = TopWindow;
		int i;

		gtk_widget_show(window);
		gtk_window_set_modal(GTK_WINDOW(window), TRUE);
		for(i = 0; i < g_list_length(DialogStack); i++) {
			if ((gpointer)window != g_list_nth_data(DialogStack, i)) {
				parent = (GtkWidget *)g_list_nth_data(DialogStack, i);
			}
		}
		gtk_window_set_transient_for(GTK_WINDOW(window), 
			GTK_WINDOW(parent));
		if (g_list_find(DialogStack, window) == NULL) {
			DialogStack = g_list_append(DialogStack, window);
		}
	}
LEAVE_FUNC;
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

extern	gpointer	*
GetObjectData(GtkWidget	*widget,
			  char *object_key)
{
	return g_object_get_data(G_OBJECT(widget), object_key);
}

extern	void
SetObjectData(GtkWidget	*widget,
			  char *object_key,
			  gpointer	*data)
{
	gpointer	*object;	
	object = g_object_get_data(G_OBJECT(widget), object_key);
	if ( object == NULL ) {
		object = xmalloc(sizeof(data));
	}
	memcpy(object, data, sizeof(data));
	g_object_set_data(G_OBJECT(widget), object_key, object);
}


extern	GtkWidget*
GetWidgetByLongName(char *name)
{
	WidgetData	*data;
	GtkWidget	*widget;
	
	widget = NULL;
	data = g_hash_table_lookup(WidgetDataTable, name);
	if (data != NULL) {
	    widget = glade_xml_get_widget_by_long_name(
			(GladeXML *)data->window->xml, name);
	}
	return widget;
}

extern	GtkWidget*
GetWidgetByName(char *name)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = g_hash_table_lookup(WindowTable,ThisWindowName);
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget((GladeXML *)wdata->xml, name);
	}
	return widget;
}

extern	GtkWidget*
GetWidgetByWindowNameAndLongName(char *windowName,
	char *widgetName)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = g_hash_table_lookup(WindowTable,windowName);
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget_by_long_name(
			(GladeXML *)wdata->xml, widgetName);
	}
	return widget;
}

extern	GtkWidget*
GetWidgetByWindowNameAndName(char *windowName,
	char *widgetName)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = g_hash_table_lookup(WindowTable,windowName);
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget(
			(GladeXML *)wdata->xml, widgetName);
	}
	return widget;
}
