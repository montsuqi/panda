/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

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

#ifdef ENABLE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif
#include	<glade/glade.h>
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif
#include	<gdk/gdkkeysyms.h>

#include	"callbacks.h"
#include	"types.h"
#include	"value.h"
#include	"misc.h"
#include	"comm.h"
#include	"glclient.h"
#include	"protocol.h"
#include	"glterm.h"
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
dbgmsg(">ClearWindowTable");
	if		(  WindowTable  !=  NULL  ) {
		g_hash_table_foreach(WindowTable,(GHFunc)ClearWindowData,NULL);
	}
dbgmsg("<ClearWindowTable");
}


extern	gboolean
select_all(
	GtkWidget	*widget,
	GdkEventFocus	*event,
	gpointer		user_data)
{
	GtkEntry *entry = GTK_ENTRY (widget);
	gtk_entry_select_region(entry, 0, entry->text_length);
	GTK_EDITABLE (entry)->current_pos = 0;
	return (TRUE);
}

extern	gboolean
unselect_all(
	GtkWidget	*widget,
	GdkEventFocus	*event,
	gpointer		user_data)
{
	GtkEntry *entry = GTK_ENTRY (widget);
	gtk_entry_select_region (entry, 0, 0);
	return (TRUE);
}

 extern	gboolean
keypress_filter(
	GtkWidget	*widget,
	GdkEventKey	*event,
	char		*next)
{
	GtkWidget	*nextWidget;
	GtkWidget	*window;
	char		*wname;
	XML_Node	*node;

	if		(event->keyval == GDK_KP_Enter) {
		window = gtk_widget_get_toplevel(widget);
		wname = gtk_widget_get_name(window);
		if		(	( ( node = g_hash_table_lookup(WindowTable,wname) )       !=  NULL  )
				&&	(  ( nextWidget = glade_xml_get_widget(node->xml,next) )  !=  NULL  ) ) {
				gtk_widget_grab_focus (nextWidget);
		}
		gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_press_event");
	}
	return	(TRUE);
}

extern	gboolean
press_filter(
	GtkWidget	*widget,
	GdkEventButton	*event,
	gpointer		user_data)
{
	gboolean	rc;

	/* If WIDGET already has focus, do the default action */
	if		(GTK_WIDGET_HAS_FOCUS (widget)) {
		rc = FALSE;
	} else {
		/* Otherwise, just grab focus */
		gtk_widget_grab_focus (widget);
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");
		rc = TRUE;
	}
	return	(rc);
}

struct changed_hander {
	GtkObject       *object;
	GtkSignalFunc	func;
	gpointer	data;
	struct changed_hander *next;
} *changed_hander_list = NULL;

extern	void
RegisterChangedHander(
	GtkObject *object,
	GtkSignalFunc func,
	gpointer data)
{
  struct changed_hander *p = xmalloc (sizeof (struct changed_hander));
  p->object = object;
  p->func = func;
  p->data = data;
  p->next = changed_hander_list;
  changed_hander_list = p;
}

static void
BlockChangedHanders(void)
{
  struct changed_hander *p;
  for (p = changed_hander_list; p != NULL; p = p->next)
    gtk_signal_handler_block_by_func (p->object, p->func, p->data);
}

static void
UnblockChangedHanders(void)
{
  struct changed_hander *p;
  for (p = changed_hander_list; p != NULL; p = p->next)
    gtk_signal_handler_unblock_by_func (p->object, p->func, p->data);
}

extern	void
send_event(
	GtkWidget	*widget,
	char		*event)
{
	GtkWidget	*window;
	GdkWindow	*pane;
	GdkWindowAttr	attr;
	static int	ignore_event = FALSE;

	memset (&attr, 0, sizeof (GdkWindowAttr));
	attr.wclass = GDK_INPUT_ONLY;
	attr.window_type = GDK_WINDOW_CHILD;
	attr.cursor = gdk_cursor_new (GDK_WATCH);
	attr.x = attr.y = 0;
	attr.width = attr.height = 32767;

dbgmsg(">send_event");
	if		(  !fInRecv  &&  !ignore_event ) {
		/* show busy cursor */
		window = gtk_widget_get_toplevel(widget);
		pane = gdk_window_new(window->window, &attr, GDK_WA_CURSOR);
		gdk_window_show (pane);
		gdk_flush ();
		/* send event */
		SendEvent(fpComm,
			  gtk_widget_get_name(window),
			  gtk_widget_get_name(widget),
			  event);
		SendWindowData();
		BlockChangedHanders();
		if		(  GetScreenData(fpComm)  ) {
			ignore_event = TRUE;
			while	(  gtk_events_pending()  ) {
				gtk_main_iteration();
			}
			ignore_event = FALSE;
		}
		UnblockChangedHanders();
		/* clear busy cursor */
		gdk_window_destroy (pane);
	}
dbgmsg("<send_event");
}

extern	void
send_event_on_focus_out(
	GtkWidget		*widget,
	GdkEventFocus	*focus,
	char			*event)
{
	send_event (widget, event);
}

static char *timeout_event;
static gint timeout_hander_id = 0;

static gint
send_event_if_kana (gpointer widget)
{
  guchar *text = gtk_entry_get_text (GTK_ENTRY (widget));
  int len = strlen (text);
  if (len == 0 || text[len - 1] >= 0x80)
    {
      entry_changed (widget, timeout_event);
      send_event (widget, timeout_event);
    }
  timeout_hander_id = 0;
  return FALSE;
}

extern void
send_event_when_idle(
	GtkWidget	*widget,
	char		*event)
{
	static int registed = 0;
	static int timeout = -1;

	if (timeout_hander_id)
		gtk_timeout_remove (timeout_hander_id);

	if (!registed) {
		RegisterChangedHander (GTK_OBJECT (widget), send_event_when_idle, event);
		registed = 1;
	}
	if (timeout == -1) {
		char *s = getenv ("GL_SEND_EVENT_DELAY");
		if (s)
			timeout = atoi (s);
		else
			timeout = 1000;
	}

	if (timeout > 0) {
		timeout_event = event;
		timeout_hander_id = gtk_timeout_add (timeout, send_event_if_kana, widget);
	}
}

extern void
clist_send_event(
	GtkWidget	*widget,
	gint		row,
	gint		column,
	char		*event)
{
	send_event(widget, "SELECT");
}

extern void
activate_widget(GtkWidget *widget)
{
	send_event(widget, "ACTIVATE");
}

extern	void
entry_next_focus(
	GtkEntry	*entry,
	char		*next)
{	
	GtkWidget	*nextWidget;
	GtkWidget	*window;
	char		*wname;
	XML_Node	*node;

	window = gtk_widget_get_toplevel(GTK_WIDGET(entry));
	wname = gtk_widget_get_name(window);
	if		(	( ( node = g_hash_table_lookup(WindowTable,wname) )       !=  NULL  )
			&&	(  ( nextWidget = glade_xml_get_widget(node->xml,next) )  !=  NULL  ) ) {
		gtk_widget_grab_focus(nextWidget);
	}
}

static	void
UpdateWidget(
	GtkWidget	*widget,
	void		*user_data)
{
	const	char	*name;
	char	*wname;
	XML_Node	*node;
	GtkWidget	*window;

dbgmsg(">UpdateWidget");
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
dbgmsg("<UpdateWidget");
}

extern	void
changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	UpdateWidget((GtkWidget *)entry,NULL);
}

extern	void
entry_changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	UpdateWidget((GtkWidget *)entry,NULL);
}

extern	void
text_changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	UpdateWidget((GtkWidget *)entry,user_data);
}

extern	void
button_toggled(
	GtkWidget	*button,
	gpointer	user_data)
{
	UpdateWidget((GtkWidget *)button,user_data);
}

extern	void
selection_changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	UpdateWidget((GtkWidget *)entry,user_data);
}

extern	void
click_column(
	GtkWidget	*button,
	gpointer	user_data)
{
	UpdateWidget((GtkWidget *)button,user_data);
}

extern	void
entry_set_editable(
	GtkEntry	*entry,
	gpointer	user_data)
{

}

extern	void
map_event(
	GtkWidget	*widget,
	gpointer	user_data)
{
dbgmsg(">map_event");
	ClearWindowTable();
dbgmsg("<map_event");
}

extern	void
set_focus(
	GtkWidget	*widget,
	gpointer	user_data)
{
	XML_Node	*node;
	char		*name;

dbgmsg(">set_focus");
	name = gtk_widget_get_name(widget);

	if		(  ( node = g_hash_table_lookup(WindowTable,name) )  !=  NULL  ) {
		FocusedScreen = node;
	}
dbgmsg("<set_focus");
}

extern	void
day_selected(
	GtkCalendar	*widget,
	gpointer	user_data)
{
	UpdateWidget((GtkWidget *)widget,user_data);
#if	0
	printf("%d\n",(int)user_data);
	printf("year = %d\n",widget->year);
	printf("month = %d\n",widget->month+1);
	printf("day = %d\n",widget->selected_day);
#endif
}

extern	void
switch_page(
	GtkNotebook	*widget,
	char		*user_data)
{
	UpdateWidget((GtkWidget *)widget,user_data);
}

static	void
CheckWindow(
	char	*name,
	XML_Node	*node,
	Bool		*fOpen)
{
	if		(  node->window  !=  NULL  ) {
		*fOpen = TRUE;
	}
}

extern	void
window_close(
	GtkWidget	*widget,
	gpointer	user_data)
{
	XML_Node	*node;
	char		*name;
	Bool		fOpen;

	name = gtk_widget_get_name(widget);
	if		(  ( node = g_hash_table_lookup(WindowTable,name) )  !=  NULL  ) {
		gtk_widget_hide(GTK_WIDGET(node->window));
		if		(  !fInRecv  ) {
			fOpen = FALSE;
			g_hash_table_foreach(WindowTable,(GHFunc)CheckWindow,&fOpen);
			if		(  !fOpen  ) {
				gtk_main_quit();
			}
		}
	}
}

extern	void
window_destroy(
	GtkWidget	*widget,
	gpointer	user_data)
{
dbgmsg(">window_destroy");
	fInRecv = TRUE;
	gtk_main_quit();
dbgmsg("<window_destroy");
}

#ifdef	GTK_PANDA_TYPE_HTML
extern	void
open_browser(
	GtkPandaHTML	*html,
	XmHTMLAnchorCallbackStruct *cbs,
	void *data)
{
	/* build target URI */
	char	uri[FILENAME_MAX];
	char	*p;
	char	buff[SIZE_BUFF];

	if		(strncmp (cbs->href, "http:", 5) == 0)	{
		/* full URI */
		strcpy (uri, cbs->href);
	} else {
		strcpy (uri, gtk_panda_html_get_uri (html));
		if		( cbs->href[0]  ==  '/' )	{
			/* absolute path */
			p = strchr (uri + 7, '/');
			if	(  p  ==  NULL  ) {
				strcpy (p + 1, cbs->href + 1);
			} else {
				strcat (uri, cbs->href);
			}
		} else {
			/* relative path */
			p = strrchr (uri, '/');
			if	(  p  !=  NULL  ) {
				strcpy (p + 1, cbs->href);
			} else {
				strcat (uri, "/");
				strcat (uri, cbs->href);
			}
		}
	}

	/* open */
	if		(  cbs->target  ==  NULL  ) {
		gtk_panda_html_set_uri (html, uri);
	} else {
		sprintf (buff, "gnome-moz-remote %s", uri);
		system (buff);
	}
}
#endif
