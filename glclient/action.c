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
#include	<glade/glade.h>
#include	<json.h>

#include	"types.h"
#include	"glclient.h"
#define		ACTION_MAIN
#include	"bd_config.h"
#include	"action.h"
#include	"styleParser.h"
#include	"gettext.h"
#include	"widgetcache.h"
#include	"widgetOPS.h"
#include	"protocol.h"
#include	"notify.h"
#include	"download.h"
#include	"print.h"
#include	"logger.h"
#include	"dialogs.h"
#include	"icon.h"

static struct changed_hander {
	GObject					*object;
	GCallback				func;
	gpointer				data;
	gint					block_flag;
	struct changed_hander 	*next;
} *changed_hander_list = NULL;

static void ScaleWidget(GtkWidget *widget, gpointer data);
static void ScaleWindow(GtkWidget *widget);
static void ReDrawTopWindow(void);

static void
SetWindowIcon(GtkWindow *window)
{
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data (glclient2_icon);
	if (pixbuf) {
		gtk_window_set_icon (window, pixbuf);
		g_object_unref (pixbuf);
	}
}

extern	void
RegisterChangedHandler(
	GObject *object,
	GCallback func,
	gpointer data)
{
  struct changed_hander *p = xmalloc (sizeof (struct changed_hander));
	p->object = object;
	p->func = func;
	p->data = data;
	p->next = changed_hander_list;
	p->block_flag = FALSE;
	changed_hander_list = p;
}

extern void
BlockChangedHandlers(void)
{
	struct changed_hander *p;

	for (p = changed_hander_list; p != NULL; p = p->next) {
		p->block_flag = TRUE;		 
		g_signal_handlers_block_by_func (p->object, p->func, p->data);
	}
}

extern void
UnblockChangedHandlers(void)
{
	struct changed_hander *p;

	for (p = changed_hander_list; p != NULL; p = p->next) {
		if (p->block_flag) {
			g_signal_handlers_unblock_by_func (p->object, p->func, p->data);
		}
	}
}

extern	GtkWidget	*
GetWindow(
	GtkWidget	*widget)
{
	GtkWidget	*window;

	window = gtk_widget_get_toplevel(widget);
	return (window);
}

extern	char	*
GetWindowName(
	GtkWidget	*widget)
{
	static char	wname[SIZE_LONGNAME];
	/*	This logic is escape code for GTK bug.	*/
	strcpy(wname,glade_get_widget_long_name(widget));
	if (strchr(wname,'.')) {
		*(strchr(wname,'.')) = 0;
	}
	return (wname);
}

static	void
RegisterWidgets(
	GtkWidget *widget,
	gpointer data)
{
	WindowData *wdata;


	wdata = (WindowData*)data;
	if(GTK_IS_CONTAINER(widget)) {
		gtk_container_forall(GTK_CONTAINER(widget),RegisterWidgets,data);
	} 
	if (GTK_IS_PANDA_TIMER(widget)) {
		wdata->Timers = g_list_append(wdata->Timers,widget);
	}
}

extern	void
ResetTimers(
	char	*wname)
{
	WindowData *data;
	GList *l;

	if ((data = GetWindowData(wname)) == NULL) {
		// FIXME sometimes comes here.
		g_warning("%s:%d data is NULL for %s\n", __FILE__, __LINE__,wname);
		return;
	}
	for(l=data->Timers;l!=NULL;l=l->next) {
		gtk_widget_show(GTK_WIDGET(l->data));
		gtk_panda_timer_reset (GTK_PANDA_TIMER(l->data));
	}
}

static	void
_StopTimersAll(
	gpointer	key,
	gpointer	value,
	gpointer	data)
{
	WindowData *wdata;
	GList *l;

	wdata = (WindowData*)value;
	for(l=wdata->Timers;l!=NULL;l=l->next) {
		gtk_widget_hide(GTK_WIDGET(l->data));
		gtk_panda_timer_stop(GTK_PANDA_TIMER(l->data));
	}
}

extern	void
StopTimersAll(void)
{
	g_hash_table_foreach(WINDOWTABLE(Session),_StopTimersAll,NULL);
}

extern	void
_AddChangedWidget(
	GtkWidget	*widget)
{
	char		*name;
	char		*wname;
	char		*key;
	char		*tail;
	WindowData	*wdata;

	name = (char *)glade_get_widget_long_name(widget);
	tail = strchr(name, '.');
	if (tail == NULL) {
		wname = StrDup(name);
	} else {
		wname = StrnDup(name, tail - name);
	}
	if ((wdata = GetWindowData(wname)) != NULL) {
		if (g_hash_table_lookup(wdata->ChangedWidgetTable, name) == NULL) {
			key = StrDup(name);
			g_hash_table_insert(wdata->ChangedWidgetTable, key, key);
		}
	}
	free(wname);
}

extern	void
AddChangedWidget(
	GtkWidget	*widget)
{
	if (!ISRECV(Session)) {
		_AddChangedWidget(widget);
	}
}

extern	void
ClearKeyBuffer(void)
{
	GdkEvent	*event; 

	while( (event = gdk_event_get()) != NULL) {
		if ( (event->type == GDK_KEY_PRESS ||
			  event->type == GDK_KEY_RELEASE) ) {
 			gdk_event_free(event); 
		} else {
			gtk_main_do_event(event);
 			gdk_event_free(event); 
		}
	}
}

extern	void
SetTitle(GtkWidget	*window)
{
	char		buff[SIZE_BUFF];
	WindowData *wdata;

	wdata = GetWindowData((char *)gtk_widget_get_name(window));
	if (wdata == NULL||wdata->title == NULL) {
		return;
	} 

	if ( window == TopWindow && Session->title != NULL && strlen(Session->title) > 0 ) {
		snprintf(buff, sizeof(buff), "%s - %s", wdata->title, Session->title);
	} else {
		snprintf(buff, sizeof(buff), "%s", wdata->title);
	}
	gtk_window_set_title (GTK_WINDOW(window), buff);
}

extern	void
SetBGColor(GtkWidget *widget)
{
#ifdef LIBGTK_3_0_0
	GdkRGBA color;
	if (BGCOLOR(Session) != NULL) {
		if (gdk_rgba_parse(BGCOLOR(Session),&color)) {
			gtk_widget_override_background_color(widget,GTK_STATE_NORMAL,&color);
		} else {
			gtk_widget_override_background_color(widget,GTK_STATE_NORMAL,NULL);
		}
	} else {
			gtk_widget_override_background_color(widget,GTK_STATE_NORMAL,NULL);
	}
#else
	GdkColor color;
	if (BGCOLOR(Session) != NULL) {
		if (gdk_color_parse(BGCOLOR(Session),&color)) {
			gtk_widget_modify_bg(widget,GTK_STATE_NORMAL,&color);
		} else {
			gtk_widget_modify_bg(widget,GTK_STATE_NORMAL,NULL);
		}
	} else {
			gtk_widget_modify_bg(widget,GTK_STATE_NORMAL,NULL);
	}
#endif
}

static	WindowData*	
CreateWindow(
	const char	*wname,
	const char	*gladedata)
{
	GladeXML	*xml;
	WindowData	*wdata;
	GtkWidget	*window;
	GtkWidget	*child;

	if (GetWindowData(wname) != NULL) {
		return NULL;
	}
	xml = glade_xml_new_from_memory((char*)gladedata,strlen(gladedata),NULL,NULL);
	if ( xml == NULL ) {
		Error("invalid glade data");
	}
	window = glade_xml_get_widget_by_long_name(xml, wname);
	if (window == NULL) {
		Error("Window %s not found", wname);
	}
	wdata = New(WindowData);
	wdata->xml = xml;
	wdata->name = StrDup(wname);
	wdata->title = StrDup((char*)gtk_window_get_title(GTK_WINDOW(window)));
	wdata->fAccelGroup = FALSE;
	wdata->ChangedWidgetTable = NewNameHash();
	wdata->Timers = NULL;
	wdata->tmpl = NULL;
	glade_xml_signal_autoconnect(xml);
	g_hash_table_insert(WINDOWTABLE(Session),strdup(wname),wdata);

	child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
	if (child == NULL) {
		Error("invalid glade:no child");
	}
	g_object_ref(child);
	gtk_widget_show_all(child);
	RegisterWidgets(child,wdata);
	if (IsDialog(window)) {
		gtk_container_add(GTK_CONTAINER(window), child); 
		wdata->fWindow = FALSE;
		SetWindowIcon(GTK_WINDOW(window));
	} else {
		wdata->fWindow = TRUE;
	}
	return wdata;
}

static	void
SwitchWindow(
	GtkWidget *window)
{
	GtkWidget	*child,*org;

	child = (GtkWidget *)g_object_get_data(G_OBJECT(window), "child");
	g_return_if_fail(child != NULL);

	org = gtk_bin_get_child(GTK_BIN(TopWindow));
	if (org != NULL) {
		gtk_container_remove(GTK_CONTAINER(TopWindow),org);
	}
	gtk_container_add(GTK_CONTAINER(TopWindow),child);
	ScaleWidget(child,NULL);

	gtk_widget_set_name(TopWindow, gtk_widget_get_name(window));

	gtk_widget_show(TopWindow);
	gtk_widget_show(child);

#ifdef LIBGTK_3_0_0
	gtk_window_set_resizable(GTK_WINDOW(TopWindow), FALSE);
#else
	gtk_window_set_resizable(GTK_WINDOW(TopWindow), TRUE);
#endif
}

extern	void
CloseWindow(
	const char *wname)
{
	WindowData	*data;
	GtkWidget	*window;
	GSList		*list;
	int			i;
	GList		*wlist;

	if ((data = GetWindowData(wname)) == NULL) {
		// FIXME sometimes comes here.
		fprintf(stderr,"%s:%d data %s is NULL\n", __FILE__, __LINE__, wname);
		return;
	}
	window = glade_xml_get_widget_by_long_name((GladeXML *)data->xml, wname);
	if (data->fWindow) {
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
			gtk_window_set_transient_for(GTK_WINDOW(wlist->next->data), NULL);
		}
		DialogStack = g_list_remove(DialogStack, window);
		for (i = g_list_length(DialogStack) - 1; i >= 0; i--) {
			if (i >0) {
				gtk_window_set_transient_for(
					(GtkWindow *)g_list_nth_data(DialogStack, i),
					(GtkWindow *)g_list_nth_data(DialogStack, i - 1));
			} else {
				gtk_window_set_transient_for(
					GTK_WINDOW(DialogStack->data), GTK_WINDOW(TopWindow));
			}
		}
	}
}

static	void
ReDrawTopWindow(void)
{
#define WINC (1)
	static int i = 0;
	int width,height;
	
	gtk_window_get_size(GTK_WINDOW(TopWindow),&width,&height);
	g_signal_handlers_block_by_func(TopWindow,ConfigureWindow,NULL);
	if (i%2 == 0) {
		gtk_window_resize(GTK_WINDOW(TopWindow),width, height-WINC);
	} else {
		gtk_window_resize(GTK_WINDOW(TopWindow),width, height+WINC);
	}
	 g_signal_handlers_unblock_by_func(TopWindow,ConfigureWindow,NULL);
	i++;
}

extern	void
ShowWindow(
	const char *wname)
{
	WindowData	*data;
	GtkWidget	*window;
	GSList		*list;

	if ((data = GetWindowData(wname)) == NULL) {
		// FIXME sometimes comes here.
		g_warning("%s:%d data is NULL for %s\n", __FILE__, __LINE__,wname);
		return;
	}
	window = glade_xml_get_widget_by_long_name((GladeXML *)data->xml, wname);
	g_return_if_fail(window != NULL);

	if (data->fWindow) {
		gtk_widget_show(TopWindow);
		gtk_widget_grab_focus(TopWindow);
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
		SetBGColor(TopWindow);
		if (DelayDrawWindow) {
			ReDrawTopWindow();
		}
	} else {
		GtkWidget *parent = TopWindow;
		int i;

		ScaleWidget(window, NULL);
		ScaleWindow(window);
		SetBGColor(window);

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
		gtk_widget_show(window);
		gtk_window_set_modal(GTK_WINDOW(window), TRUE);
		if (DelayDrawWindow) {
			ReDrawTopWindow();
		}
	}
}


extern	void
ShowBusyCursor(
	GtkWidget	*widget)
{
	static GdkCursor *busycursor = NULL;
	GtkWidget *window;

	if (widget == NULL) {
		return;
	}
	if (busycursor == NULL) {
		busycursor = gdk_cursor_new (GDK_WATCH);
	}
	window = gtk_widget_get_toplevel(widget);
	gtk_window_set_title(GTK_WINDOW(window),_("Now loading..."));
#ifndef LIBGTK_3_0_0
	gdk_window_set_cursor(window->window,busycursor);
	gdk_flush ();
#endif
}

extern	void
HideBusyCursor(GtkWidget *widget)
{
	GtkWidget	*window;
	if (widget == NULL) {
		return;
	}
	window = gtk_widget_get_toplevel(widget);
	SetTitle(window);
#ifndef LIBGTK_3_0_0
	gdk_window_set_cursor(window->window,NULL);
#endif
}

static GtkWidget*
GetWidgetByWindowNameAndLongName(
	const char *windowName,
	const char *widgetName)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = GetWindowData(windowName);
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget_by_long_name(
			(GladeXML *)wdata->xml, widgetName);
	}
	return widget;
}

static GtkWidget*
GetWidgetByWindowNameAndName(
	const char *windowName,
	const char *widgetName)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = GetWindowData(windowName);
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget(
			(GladeXML *)wdata->xml, widgetName);
	}
	return widget;
}

extern	GtkWidget*
GetWidgetByLongName(const char *lname)
{
	char *wname,*p;
	GtkWidget *widget;
	
	wname = g_strdup(lname);
	for(p=wname;*p != 0;p++) {
		if (*p == '.') {
			*p = 0;
			break;
		}
	}
	widget = GetWidgetByWindowNameAndLongName(wname,lname);
	g_free(wname);
	return widget;
}

extern	GtkWidget*
GetWidgetByName(const char *name)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	
	widget = NULL;
	wdata = GetWindowData(THISWINDOW(Session));
	if (wdata != NULL && wdata->xml != NULL) {
	    widget = glade_xml_get_widget((GladeXML *)wdata->xml, name);
	}
	return widget;
}


static  void
ScaleWidget(
    GtkWidget   *widget,
    gpointer    data)
{
	int has_x,x,_x;
	int has_y,y,_y;
	int has_w,w,_w;
	int has_h,h,_h;

#ifdef LIBGTK_3_0_0
	return;
#endif
	if (GTK_IS_CONTAINER(widget) && ! GTK_IS_SCROLLED_WINDOW(widget)) {
		gtk_container_set_resize_mode(GTK_CONTAINER(widget),GTK_RESIZE_QUEUE);
		gtk_container_forall(GTK_CONTAINER(widget), ScaleWidget, data);
	} 
	has_x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_x"));
	has_y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_y"));
	has_w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_width"));
	has_h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"has_height"));

	if (!has_x || !has_y || !has_w || !has_h) {
      return;
    }

	x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"x"));
	y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"y"));
	w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"width"));
	h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"height"));

	if (w >= 0 && h >= 0) {
		_x = (int)(x * TopWindowScale.h);
		_y = (int)(y * TopWindowScale.v);
		_w = (int)(w * TopWindowScale.h);
		_h = (int)(h * TopWindowScale.v);

#if 0
		fprintf(stderr,"scalewidget [[%d,%d],[%d,%d]]->[[%d,%d],[%d,%d]]\n",
			x,y,w,h,
			_x,_y,_w,_h);
#endif
		if (!GTK_IS_WINDOW(widget)) {
			gtk_widget_set_size_request(widget,_w,_h); 
			GtkWidget *parent = gtk_widget_get_parent(widget);
			if (parent != NULL && GTK_IS_FIXED(parent)) {
				gtk_fixed_move(GTK_FIXED(parent),widget,_x,_y);
			}
		}
	}
}

static	void
ScaleWindow(
	GtkWidget *widget)
{

	int x,_x;
	int y,_y;
	int w,_w;
	int h,_h;

#ifdef LIBGTK_3_0_0
	return;
#endif
	x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"x"));
	y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"y"));
	w = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"width"));
	h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"height"));

	if (x != 0 || y != 0) {
		gtk_window_get_position(GTK_WINDOW(TopWindow),&_x,&_y);
			_x += (int)(x * TopWindowScale.h);
			_y += (int)(y * TopWindowScale.v);
#if 0
		fprintf(stderr,"move window [%d,%d]->[%d,%d]\n",
			x,y,_x,_y);
#endif
		gtk_window_move(GTK_WINDOW(widget),_x,_y);
	}

	if (w > 0 && h > 0) {
		_w = (int)(w * TopWindowScale.h);
		_h = (int)(h * TopWindowScale.v);
#if 0
		fprintf(stderr,"scale window [%d,%d]->[%d,%d]\n",
			w,h,_w,_h);
#endif
		gtk_widget_set_size_request(widget,_w,_h); 
	} 
}

extern	void
ConfigureWindow(GtkWidget *widget,
	GdkEventConfigure *event,
	gpointer data)
{
	static int old_width = 0, old_height = 0;
	static int old_x = 0, old_y = 0;
	int x,y,width,height;
	char buf[16];

	gtk_window_get_position(GTK_WINDOW(widget), &x,&y);
	gtk_window_get_size(GTK_WINDOW(widget), &width,&height);

	if (old_x != x || old_y != y) {
		if (x >= 0) {
			sprintf(buf,"%d",x);
			SetWidgetCache("glclient.topwindow.x",buf);
		}
		if (y >= 0) {
			sprintf(buf,"%d",y);
			SetWidgetCache("glclient.topwindow.y",buf);
		}
	}

#if 0
	fprintf(stderr,"configure window[%d,%d][%d,%d]->[%d,%d][%d,%d]\n",
		old_x,old_y,old_width,old_height,x,y,width,height);
#endif
	if (old_width != width || old_height != height) {
		TopWindowScale.h = (width  * 1.0) / ORIGIN_WINDOW_WIDTH;
		TopWindowScale.v = (height * 1.0) / ORIGIN_WINDOW_HEIGHT;
		ScaleWidget(widget,NULL);
		sprintf(buf,"%d",width);
		SetWidgetCache("glclient.topwindow.width",buf);
		sprintf(buf,"%d",height);
		SetWidgetCache("glclient.topwindow.height",buf);
	}
	old_x = x;
	old_y = y;
	old_width = width;
	old_height = height;
}

extern	void
InitTopWindow(void)
{
	const char *px, *py, *pwidth, *pheight;
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
		height = DEFAULT_WINDOW_HEIGHT;
	}

	TopWindowScale.h = (ORIGIN_WINDOW_WIDTH  * 1.0) / width;
	TopWindowScale.v = (ORIGIN_WINDOW_HEIGHT * 1.0) / height;

	if (CancelScaleWindow) {
		width = ORIGIN_WINDOW_WIDTH;
		height = ORIGIN_WINDOW_HEIGHT;
		TopWindowScale.h = 1.0;
		TopWindowScale.v = 1.0;
	}

	TopWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_move(GTK_WINDOW(TopWindow),x,y); 
	gtk_container_set_resize_mode(GTK_CONTAINER(TopWindow),GTK_RESIZE_QUEUE);
#if LIBGTK_3_0_0
#  if 0
    gtk_window_set_default_size(GTK_WINDOW(TopWindow),width,height);
#  else
    gtk_window_set_default_size(GTK_WINDOW(TopWindow),
		DEFAULT_WINDOW_WIDTH,
		DEFAULT_WINDOW_HEIGHT);
#  endif
#else
	gtk_widget_set_size_request(TopWindow,width, height);
	GdkGeometry geometry;
	geometry.min_width = DEFAULT_WINDOW_WIDTH;
	geometry.min_height = DEFAULT_WINDOW_HEIGHT;
	gtk_window_set_geometry_hints(GTK_WINDOW(TopWindow),NULL,&geometry,
		GDK_HINT_MIN_SIZE);
	gtk_window_set_wmclass(GTK_WINDOW(TopWindow),"Glclient","Glclient");
#endif

	g_signal_connect(G_OBJECT(TopWindow), 
		"delete_event", G_CALLBACK(gtk_true), NULL);
	if (!CancelScaleWindow) {
		g_signal_connect(G_OBJECT(TopWindow), 
			"configure_event", G_CALLBACK(ConfigureWindow), NULL);
	}

	SetWindowIcon(GTK_WINDOW(TopWindow));
	DialogStack = NULL;
}

extern	gboolean
IsDialog(GtkWidget *widget)
{
#ifdef LIBGTK_3_0_0
	if (g_object_get_data(G_OBJECT(widget),"IS_DIALOG") != NULL) {
#else
	if (strstr(GTK_WINDOW(widget)->wmclass_class, "dialog") != NULL) {
#endif
		return TRUE;
	} else {
		return FALSE;
	}
}

extern  gboolean    
IsWidgetName(char *name)
{
	return (GetWidgetByWindowNameAndLongName(THISWINDOW(Session), name) != NULL);
}

extern  gboolean    
IsWidgetName2(char *name)
{
	return (GetWidgetByWindowNameAndName(THISWINDOW(Session), name) != NULL);
}

static  void
GrabFocus(
	const char *windowName, 
	const char *widgetName)
{
	GtkWidget 	*widget;

	widget = GetWidgetByWindowNameAndName(windowName,widgetName);
	if (widget != NULL) {
		gtk_widget_grab_focus(widget);
	}
}

extern  void        
UI_Init(int argc, 
	char **argv)
{
	gtk_init(&argc, &argv);
#if 1
	/* set gtk-entry-select-on-focus */
	GtkSettings *set = gtk_settings_get_default();
    gtk_settings_set_long_property(set, "gtk-entry-select-on-focus", 0, 
		"glclient2");
#endif
	gtk_panda_init(&argc,&argv);
#ifndef LIBGTK_3_0_0
	gtk_set_locale();
#endif
	glade_init();
	SetErrorFunc(ErrorDialog);
}

extern	void
UI_Main(void)
{
	if (fIMKanaOff) {
		set_im_control_enabled(FALSE);
	}
	gtk_main();
}

extern	void
InitStyle(void)
{
	gchar *gltermrc;
	gchar *rcstr;

	StyleParserInit();
	gltermrc = g_strconcat(g_get_home_dir(),"/gltermrc",NULL);
	StyleParser(gltermrc);
	StyleParser("gltermrc");
	g_free(gltermrc);
	if (  Style  != NULL  ) {
		StyleParser(Style);
	}
	if (Gtkrc != NULL && strlen(Gtkrc)) {
		gtk_rc_parse(Gtkrc);
	} else {
		rcstr = g_strdup_printf(
			"style \"glclient2\" {"
			"  font_name = \"%s\""
			"  fg[NORMAL] = \"#000000\""
			"  fg[PRELIGHT] = \"#000000\""
			"  text[NORMAL] = \"#000000\""
			"  text[PRELIGHT] = \"#000000\""
			"}"
			"style \"tooltip\" {"
			"  fg[NORMAL] = \"#000000\""
			"  bg[NORMAL] = \"#ffffaf\""
			"}"
			"style \"panda-entry\" = \"entry\" {"
			"  text[INSENSITIVE] = @text_color"
			"  base[INSENSITIVE] = \"#ffffff\""
			"}"
			"style \"panda-clist\" {"
			"  text[ACTIVE] = @selected_fg_color"
			"  base[ACTIVE] = @selected_bg_color"
			"}"
			"widget_class \"*.*\" style \"glclient2\""
			"widget_class \"*<GtkPandaEntry>\" style \"panda-entry\""
			"widget_class \"*<GtkNumberEntry>\" style \"panda-entry\""
			"widget_class \"*<GtkPandaCList>\" style \"panda-clist\""
			"widget \"gtk-tooltip*\" style \"tooltip\""
			,FontName);
		gtk_rc_parse_string(rcstr);
		gtk_rc_reset_styles(gtk_settings_get_default());
		g_free(rcstr);
	}
}

extern	int
AskPass(char	*buf,
	size_t		buflen,
	const char	*prompt)
{
	return 0;
}

static void
Ping()
{
	char *dialog,*popup,*abort;

	RPC_GetMessage(GLP(Session),&dialog,&popup,&abort);
	if (strlen(abort)>0) {
		RPC_EndSession(GLP(Session));
		InfoDialog(abort);
		exit(1);
	} else if (strlen(dialog)>0) {
		InfoDialog(dialog);
	} else if (strlen(popup)>0) {
		Notify(_("glclient message notify"),popup,"gtk-dialog-info",0);
	}
	g_free(popup);
	g_free(dialog);
	g_free(abort);
}

static	void
PrintReport(
	json_object *obj)
{
	json_object *child;
	char *printer,*oid,*title;
	gboolean showdialog;
	LargeByteString *lbs;

	printer = NULL;
	title = "";
	showdialog = FALSE;

	if (!json_object_object_get_ex(obj,"object_id",&child)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	if (json_object_object_get_ex(obj,"printer",&child)) {
		printer = (char*)json_object_get_string(child);
	}

	if (json_object_object_get_ex(obj,"title",&child)) {
		title = (char*)json_object_get_string(child);
	}

	if (json_object_object_get_ex(obj,"showdialog",&child)) {
		showdialog = json_object_get_boolean(child);
	}
	if (getenv("GLCLIENT_PRINTREPORT_SHOWDIALOG")!=NULL) {
		showdialog = TRUE;
	}
	if (oid == NULL || strlen(oid) <= 0) {
		return;
	}
	if (printer == NULL || strlen(printer) <=0 ) {
		showdialog = TRUE;
	}

	lbs = REST_GetBLOB(GLP(Session),oid);
	if (lbs != NULL) {
		if (LBS_Size(lbs) > 0) {
			if (showdialog) {
				ShowPrintDialog(title,lbs);
			} else {
				Print(oid,title,printer,lbs);
			}
		}
		FreeLBS(lbs);
	}
}

static	void
DownloadFile(
	json_object *obj)
{
	json_object *child;
	char *oid,*filename,*desc;
	LargeByteString *lbs;

	filename = "foo.dat";
	desc = "";

	if (!json_object_object_get_ex(obj,"object_id",&child)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	if (json_object_object_get_ex(obj,"filename",&child)) {
		filename = (char*)json_object_get_string(child);
	}

	if (json_object_object_get_ex(obj,"description",&child)) {
		desc = (char*)json_object_get_string(child);
	}

	if (oid == NULL || strlen(oid) <= 0) {
		return;
	}
	lbs = REST_GetBLOB(GLP(Session),oid);
	if (lbs != NULL) {
		if (LBS_Size(lbs) > 0) {
			ShowDownloadDialog(filename,desc,lbs);
		}
		FreeLBS(lbs);
	}
}

static void
ListDownloads()
{
	int i;
	json_object *obj,*item,*result,*type;

	obj = RPC_ListDownloads(GLP(Session));
	if (!json_object_object_get_ex(obj,"result",&result)) {
		Error(_("invalid list_report response"));
	}

	for (i=0;i<json_object_array_length(result);i++) {
		item = json_object_array_get_idx(result,i);
		if (!CheckJSONObject(item,json_type_object)) {
			continue;
		}
		if (!json_object_object_get_ex(item,"type",&type)) {
			continue;
		}
		if (!strcmp(json_object_get_string(type),"report")) {
			PrintReport(item);
		} else {
			DownloadFile(item);
		}
	}
	json_object_put(obj);
}

static gint
PingTimerFunc(
	gpointer data)
{

	if (ISRECV(Session)) {
		return 1;
	}
	if (!UsePushClient) {
		ListDownloads();
	}
	Ping();
	return 1;
}

extern	void
SetPingTimerFunc()
{
	g_timeout_add(PingTimerPeriod,PingTimerFunc,NULL);
}

extern	WindowData *
GetWindowData(
	const char *wname)
{
	return (WindowData*)g_hash_table_lookup(WINDOWTABLE(Session),wname);
}

static	void
CheckCloseWindow(
	json_object *w,
	int idx)
{
	json_object *child;
	const char *put_type;
	const char *wname;

	if (!json_object_object_get_ex(w,"put_type",&child)) {
		Error("invalid json part:put_type");
	}
	put_type = (char*)json_object_get_string(child);

	if (!json_object_object_get_ex(w,"window",&child)) {
		Error("invalid json part:window");
	}
	wname = json_object_get_string(child);

	if (GetWindowData(wname) == NULL) {
		return;
	}
	if (strcmp("new",put_type) && strcmp("current",put_type)) {
		Info("close window[%s] put_type[%s]",wname,put_type);
		CloseWindow(wname);
	}
}

static json_object *
CopyJSON(
	json_object *obj)
{
	json_object_iter iter;
	json_object *ret;
	int i,length;

	ret = NULL;
	switch(json_object_get_type(obj)) {
	case json_type_boolean:
		ret = json_object_new_boolean(json_object_get_boolean(obj));
		break;
	case json_type_double:
		ret = json_object_new_double(json_object_get_double(obj));
		break;
	case json_type_int:
		ret = json_object_new_int(json_object_get_int(obj));
		break;
	case json_type_string:
		ret = json_object_new_string(json_object_get_string(obj));
		break;
	case json_type_array:
		length = json_object_array_length(obj);
		ret = json_object_new_array();
		for(i=0;i<length;i++) {
			json_object_array_add(ret,CopyJSON(json_object_array_get_idx(obj,i)));
		}
		break;
	case json_type_object:
		ret = json_object_new_object();
		json_object_object_foreachC(obj,iter) {
			json_object_object_add(ret,iter.key,CopyJSON(iter.val));
		}
		break;
	default:
		ret = json_object_new_object();
		break;
	}

	return ret;
}


json_object *
UpdateTemplate(
	json_object *tmpl,
	json_object *obj,
	int level)
{
	json_object_iter iter;
	json_object *ret,*c1,*c2;
	int i;


	ret = NULL;
	if (tmpl == NULL) {
		return NULL;
	}
	switch(json_object_get_type(tmpl)) {
	case json_type_boolean:
		if (CheckJSONObject(obj,json_type_boolean)) {
			ret = json_object_new_boolean(json_object_get_boolean(obj));
		} else {
			ret = json_object_new_boolean(TRUE);
		}
		break;
	case json_type_double:
		if (CheckJSONObject(obj,json_type_double)) {
			ret = json_object_new_double(json_object_get_double(obj));
		} else {
			ret = json_object_new_double(0.0);
		}
		break;
	case json_type_int:
		if (CheckJSONObject(obj,json_type_int)) {
			ret = json_object_new_int(json_object_get_int(obj));
		} else {
			ret = json_object_new_int(0);
		}
		break;
	case json_type_string:
		if (CheckJSONObject(obj,json_type_string)) {
			ret = json_object_new_string(json_object_get_string(obj));
		} else {
			ret = json_object_new_string("");
		}
		break;
	case json_type_array:
		ret = json_object_new_array();
		if (CheckJSONObject(obj,json_type_array)) {
			for(i=0;i<json_object_array_length(tmpl);i++) {
				c1 = json_object_array_get_idx(tmpl,i);
				c2 = json_object_array_get_idx(obj,i);
				json_object_array_add(ret,UpdateTemplate(c1,c2,level+1));
			}
		} else {
			for(i=0;i<json_object_array_length(tmpl);i++) {
				c1 = json_object_array_get_idx(tmpl,i);
				json_object_array_add(ret,UpdateTemplate(c1,NULL,level+1));
			}
		}
		break;
	case json_type_object:
		ret = json_object_new_object();
		if (CheckJSONObject(obj,json_type_object)) {
			json_object_object_foreachC(tmpl,iter) {
				c1 = iter.val;
				json_object_object_get_ex(obj,iter.key,&c2);
				json_object_object_add(ret,iter.key,UpdateTemplate(c1,c2,level+1));
			}
		} else {
			json_object_object_foreachC(tmpl,iter) {
				c1 = iter.val;
				json_object_object_add(ret,iter.key,UpdateTemplate(c1,NULL,level+1));
			}
		}
	default:
		break;
	}
	return ret;
}

static	void
UpdateWindow(
	json_object *w,
	int idx)
{
	json_object *child,*obj,*result,*old;
	const char *put_type;
	const char *wname;
	const char *gladedata;
	WindowData *wdata;

	if (!json_object_object_get_ex(w,"put_type",&child)) {
		Error("invalid json part:put_type");
	}
	put_type = (char*)json_object_get_string(child);

	if (!json_object_object_get_ex(w,"window",&child)) {
		Error("invalid json part:window");
	}
	wname = json_object_get_string(child);

	wdata = GetWindowData(wname);
	if (wdata == NULL) {
		obj = RPC_GetScreenDefine(GLP(Session),wname);
		json_object_object_get_ex(obj,"result",&result);
		if (!json_object_object_get_ex(result,"screen_define",&child)) {
			Error("can't get screen define:%s",wname);
		}
		gladedata = json_object_get_string(child);
		wdata = CreateWindow(wname,gladedata);
		json_object_put(obj);
	}

	if (json_object_object_get_ex(w,"screen_data",&child)) {
		if (wdata->tmpl == NULL) {
			if (json_object_object_length(child) > 0) {
				wdata->tmpl = CopyJSON(child);
			}
		} else {
			old = wdata->tmpl;
			wdata->tmpl = UpdateTemplate(old,child,0);
			json_object_put(old);
		}
	}

	if (!strcmp("new",put_type)||!strcmp("current",put_type)) {
		if (child == NULL ||is_error(child)) {
			Error("invalid json part:screeen_data");
		}
		UpdateWidget(GetWidgetByLongName(wname),wdata->tmpl);
		if (!strcmp(wname,FOCUSEDWINDOW(Session))) {
			ShowWindow(wname);
		}
		ResetTimers((char*)wname);
		Info("show window[%s] put_type[%s]",wname,put_type);
	}
}

extern	void
UpdateScreen()
{
	json_object *result,*window_data,*windows,*child;
	const char *f_window,*f_widget;
	int i;

	Info("----");
	
	json_object_object_get_ex(SCREENDATA(Session),"result",&result);
	if (!json_object_object_get_ex(result,"window_data",&window_data)) {
		Error("invalid json part:window_data");
	}

	if (!json_object_object_get_ex(window_data,"focused_window",&child)) {
		Error("invalid json part:focused_window");
	}
	f_window = json_object_get_string(child);
	if (THISWINDOW(Session) != NULL) {
		g_free(THISWINDOW(Session));
	}
	THISWINDOW(Session) = g_strdup(f_window);
	FOCUSEDWINDOW(Session) = (char*)f_window;
	Info("focused_window[%s]",f_window);

	if (!json_object_object_get_ex(window_data,"focused_widget",&child)) {
		Error("invalid json part:focused_widget");
	}
	f_widget = json_object_get_string(child);
	FOCUSEDWIDGET(Session) = (char*)f_widget;

	if (!json_object_object_get_ex(window_data,"windows",&windows)) {
		Error("invalid json part:windows");
	}
	for(i=0;i<json_object_array_length(windows);i++) {
		child = json_object_array_get_idx(windows,i);
		CheckCloseWindow(child,i);
	}
	for(i=0;i<json_object_array_length(windows);i++) {
		child = json_object_array_get_idx(windows,i);
		UpdateWindow(child,i);
	}
	if (f_window != NULL) {
#ifdef GTK_2_24_23
		if (!fKeyBuff) {
			while(gtk_events_pending()) {
				gtk_main_iteration();
			}
		}
#endif
		GrabFocus(f_window,f_widget);
		PandaTableFocusCell(f_widget);
	}
}

extern	void
TimeSet(
	const char *str)
{
	static struct timeval t0;
	struct timeval t1;
	struct timeval d;
	long ms;
	
	gettimeofday(&t1,NULL);
	timersub(&t1,&t0,&d);
	t0 = t1;
	ms = d.tv_sec * 1000L + d.tv_usec/1000L;
	fprintf(stderr,"%s[%ld]\n",str,ms);
}
