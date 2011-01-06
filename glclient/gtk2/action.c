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
#include	"interface.h"
#include	"widgetcache.h"

static struct changed_hander {
	GObject					*object;
	GCallback				func;
	gpointer				data;
	gint					block_flag;
	struct changed_hander 	*next;
} *changed_hander_list = NULL;

static void ScaleWidget(GtkWidget *widget, gpointer data);
static void ScaleWindow(GtkWidget *widget);

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

static	void
_StopTimerWidgetAll(
	gpointer	key,
	gpointer	value,
	gpointer	data)
{
	WindowData *wdata;
	
	wdata = (WindowData*)value;
	g_hash_table_foreach(wdata->TimerWidgetTable,_StopTimer,NULL);
}

extern	void
StopTimerWidgetAll(void)
{
	g_hash_table_foreach(WindowTable,_StopTimerWidgetAll,NULL);
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
SetTitle(GtkWidget	*window)
{
	char		buff[SIZE_BUFF];
	WindowData *wdata;

	wdata = g_hash_table_lookup(WindowTable, (char *)gtk_widget_get_name(window));
	if (wdata == NULL||wdata->title == NULL) {
		return;
	} 

	if ( window == TopWindow && glSession->title != NULL && strlen(glSession->title) > 0 ) {
		snprintf(buff, sizeof(buff), "%s - %s", wdata->title, glSession->title);
	} else {
		snprintf(buff, sizeof(buff), "%s", wdata->title);
	}
	gtk_window_set_title (GTK_WINDOW(window), buff);
}

extern	void	
CreateWindow(
	char	*wname,
	int		size,
	char	*buff)
{
	GladeXML	*xml;
	WindowData	*wdata;
	GtkWidget	*window;
	GtkWidget	*child;

ENTER_FUNC;
	if (g_hash_table_lookup(WindowTable,wname) != NULL) {
		dbgprintf("%s already in WindowTable", wname);
		return;
	}
	xml = glade_xml_new_from_memory(buff,size,NULL,NULL);
	if ( xml == NULL ) {
		dbgmsg("no xml");
		return;
	}
	window = glade_xml_get_widget_by_long_name(xml, wname);
	if (window == NULL) {
		Warning("Window %s not found", wname);
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
	GtkWidget	*child;

ENTER_FUNC;
	child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
	g_return_if_fail(child != NULL);

	gtk_widget_set_size_request(TopNoteBook,1,1);
	ScaleWidget(child,NULL);

	gtk_notebook_set_page(GTK_NOTEBOOK(TopNoteBook), 
		gtk_notebook_page_num(GTK_NOTEBOOK(TopNoteBook), child));

	gtk_widget_set_name(TopWindow, gtk_widget_get_name(window));

	gtk_widget_show(TopWindow);
	gtk_widget_show(TopNoteBook);
	gtk_widget_show(child);

	gtk_window_set_resizable(GTK_WINDOW(TopWindow), TRUE);

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
	int			i;
	GList		*wlist;

ENTER_FUNC;
	dbgprintf("close window:%s\n", wname);
	if ((data = g_hash_table_lookup(WindowTable,wname)) == NULL) {
		// FIXME sometimes comes here.
		fprintf(stderr,"%s:%d data %s is NULL\n", __FILE__, __LINE__, wname);
		return;
	}
	window = glade_xml_get_widget_by_long_name((GladeXML *)data->xml, wname);
	if (data->fWindow) {
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
		gtk_widget_hide(window);
		gtk_window_set_modal(GTK_WINDOW(window), FALSE);
		wlist = g_list_find(DialogStack, window);
		if (wlist != NULL && wlist->next != NULL && wlist->next->data != NULL) {
			gtk_window_set_transient_for((GtkWindow *)wlist->next->data, NULL);
		}
		DialogStack = g_list_remove(DialogStack, window);
		for (i = g_list_length(DialogStack) - 1; i >= 1; i--) {
			gtk_window_set_transient_for(
				(GtkWindow *)g_list_nth_data(DialogStack, i),
				(GtkWindow *)g_list_nth_data(DialogStack, i - 1));
		}
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
		gtk_widget_show(TopWindow);
		_GrabFocus(TopWindow);
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
		SetTitle(TopWindow);
	} else {
	dbgmsg("show dialog\n");
		GtkWidget *parent = TopWindow;
		int i;

		ScaleWidget(window, NULL);
		ScaleWindow(window);

		gtk_widget_show(window);
		gtk_window_set_modal(GTK_WINDOW(window), TRUE);
		for(i = 0; i < g_list_length(DialogStack); i++) {
			if ((gpointer)window != g_list_nth_data(DialogStack, i)) {
				parent = (GtkWidget *)g_list_nth_data(DialogStack, i);
			}
		}
		if (g_list_find(DialogStack, window) == NULL) {
			DialogStack = g_list_append(DialogStack, window);
			gtk_window_set_transient_for(GTK_WINDOW(window), 
				GTK_WINDOW(parent));
		}
	}
LEAVE_FUNC;
}


extern	void
ShowBusyCursor(
	GtkWidget	*widget)
{
	static GdkCursor *busycursor = NULL;
	GtkWidget *window;
ENTER_FUNC;
	if (widget == NULL) {
		return;
	}
	if (busycursor == NULL) {
		busycursor = gdk_cursor_new (GDK_WATCH);
	}
	window = gtk_widget_get_toplevel(widget);
	gtk_window_set_title(GTK_WINDOW(window),_("Now loading..."));
	gdk_window_set_cursor(window->window,busycursor);
	gdk_flush ();
LEAVE_FUNC;
}

extern	void
HideBusyCursor(GtkWidget *widget)
{
ENTER_FUNC;
	GtkWidget	*window;
	if (widget == NULL) {
		return;
	}
	window = gtk_widget_get_toplevel(widget);
	SetTitle(window);
	gdk_window_set_cursor(window->window,NULL);
LEAVE_FUNC;
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

static  void
ScaleWidget(
    GtkWidget   *widget,
    gpointer    data)
{
	int *x, *y, *width, *height;

	x = g_object_get_data(G_OBJECT(widget),"x");
	y = g_object_get_data(G_OBJECT(widget),"y");
	width = g_object_get_data(G_OBJECT(widget),"width");
	height = g_object_get_data(G_OBJECT(widget),"height");


	if (GTK_IS_CONTAINER(widget) && ! GTK_IS_SCROLLED_WINDOW(widget)) {
		gtk_container_set_resize_mode(GTK_CONTAINER(widget),GTK_RESIZE_QUEUE);
		gtk_container_forall(GTK_CONTAINER(widget), ScaleWidget, data);
	}
	if (x != NULL && y != NULL && width != NULL && height != NULL) {
		int _x,_y,_width,_height;

		_x = (int)(*x * TopWindowScale.h);
		_y = (int)(*y * TopWindowScale.v);
		_width = (int)(*width * TopWindowScale.h);
		_height = (int)(*height * TopWindowScale.v);

#if 0
		fprintf(stderr,"[[%d,%d],[%d,%d]]->[[%d,%d],[%d,%d]]\n",
			*x,*y,*width,*height,
			_x,_y,_width,_height);
#endif
		gtk_widget_set_size_request(widget,_width,_height); 
#if 0
		gtk_widget_set_uposition(widget,_x,_y);
#else
		GtkWidget *parent = gtk_widget_get_parent(widget);
		if (parent != NULL && GTK_IS_FIXED(parent)) {
			gtk_fixed_move(GTK_FIXED(parent),widget,_x,_y);
		}
#endif
	} 
}

static	void
ScaleWindow(
	GtkWidget *widget)
{
	int *x, *y, *width, *height;

	x = g_object_get_data(G_OBJECT(widget),"x");
	y = g_object_get_data(G_OBJECT(widget),"y");
	width = g_object_get_data(G_OBJECT(widget),"width");
	height = g_object_get_data(G_OBJECT(widget),"height");

	if (x != NULL && y != NULL) {
		int _x,_y;

		gtk_window_get_position(GTK_WINDOW(TopWindow),&_x,&_y);
		_x += (int)(*x * TopWindowScale.h);
		_y += (int)(*y * TopWindowScale.v);
#if 0
		fprintf(stderr,"move window [%d,%d]->[%d,%d]\n",
			*x,*y,_x,_y);
#endif
		gtk_widget_set_uposition(widget,_x,_y);
	} 

	if (width != NULL && height != NULL) {
		int _width,_height;

		_width = (int)(*width * TopWindowScale.h);
		_height = (int)(*height * TopWindowScale.v);
#if 0
		fprintf(stderr,"scale window [%d,%d]->[%d,%d]\n",
			*width,*height,_width,_height);
#endif
		gtk_widget_set_size_request(widget,_width,_height); 
	} 
}

extern	void
ConfigureWindow(GtkWidget *widget,
	GdkEventConfigure *event,
	gpointer data)
{
	int x,y,width,height;
	char buf[16];

	gtk_window_get_position(GTK_WINDOW(widget), &x,&y);
	gtk_window_get_size(GTK_WINDOW(widget), &width,&height);

	TopWindowScale.h = (width * 1.0) / (DEFAULT_WINDOW_WIDTH);
	TopWindowScale.v = (height * 1.0) / 
		(DEFAULT_WINDOW_HEIGHT - DEFAULT_WINDOW_FOOTER);

#if 0
	fprintf(stderr,"[%d,%d][%d,%d][%f,%f]\n",
		x,y,width,height,TopWindowScale.h,TopWindowScale.v);
#endif
	gtk_widget_set_size_request(TopNoteBook,1,1); 
	gtk_container_forall(GTK_CONTAINER(widget), ScaleWidget, NULL);

	sprintf(buf,"%d",x);
	SetWidgetCache("glclient.topwindow.x",buf);
	sprintf(buf,"%d",y);
	SetWidgetCache("glclient.topwindow.y",buf);
	sprintf(buf,"%d",width);
	SetWidgetCache("glclient.topwindow.width",buf);
	sprintf(buf,"%d",height);
	SetWidgetCache("glclient.topwindow.height",buf);
}

extern	void
InitTopWindow(void)
{
	char *px, *py, *pwidth, *pheight;
	int x,y,width,height;

	px = GetWidgetCache("glclient.topwindow.x");
	py = GetWidgetCache("glclient.topwindow.y");
	if (px != NULL && py != NULL) {
		x = atoi(px); y = atoi(py);
	} else {
		x = 0; y = 0;
	}
	pwidth = GetWidgetCache("glclient.topwindow.width");
	pheight = GetWidgetCache("glclient.topwindow.height");
	if (pwidth != NULL && pheight != NULL) {
		width = atoi(pwidth); height = atoi(pheight);
	} else {
		width = DEFAULT_WINDOW_WIDTH;
		height = DEFAULT_WINDOW_HEIGHT - DEFAULT_WINDOW_FOOTER;
	}

	TopWindowScale.v = (width * 1.0) / (DEFAULT_WINDOW_WIDTH );
	TopWindowScale.h = (height * 1.0) / 
		(DEFAULT_WINDOW_HEIGHT - DEFAULT_WINDOW_FOOTER);

	TopWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_uposition(TopWindow,x,y); // not recommend
	gtk_widget_set_size_request(TopWindow,width, height);

	GdkGeometry geometry;
	geometry.min_width = DEFAULT_WINDOW_WIDTH;
	geometry.min_height = DEFAULT_WINDOW_HEIGHT - DEFAULT_WINDOW_FOOTER;
	gtk_window_set_geometry_hints(GTK_WINDOW(TopWindow),NULL,&geometry,
		GDK_HINT_MIN_SIZE);

	gtk_window_set_wmclass(GTK_WINDOW(TopWindow),"Glclient","Glclient");

	g_signal_connect(G_OBJECT(TopWindow), 
		"delete_event", G_CALLBACK(gtk_true), NULL);
	g_signal_connect(G_OBJECT(TopWindow), 
		"configure_event", G_CALLBACK(ConfigureWindow), NULL);

	TopNoteBook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(TopNoteBook), FALSE);
	gtk_container_add(GTK_CONTAINER(TopWindow), TopNoteBook);
	gtk_container_set_resize_mode(GTK_CONTAINER(TopNoteBook),GTK_RESIZE_IMMEDIATE);
	DialogStack = NULL;
}
