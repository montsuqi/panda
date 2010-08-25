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
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<signal.h>

#include	<gnome.h>
#include	<glade/glade.h>
#include	<gtkpanda/gtkpanda.h>
#include	<gdk/gdkkeysyms.h>

#include	"callbacks.h"
#include	"types.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"glclient.h"
#include	"action.h"
#include	"protocol.h"
#include	"glterm.h"
#include	"debug.h"

/* #define GTK1_OPEN_BROWSER_COMMAND "/usr/bin/gnome-moz-remote" */
#define GTK1_OPEN_BROWSER_COMMAND "/usr/bin/x-www-browser"

extern	gboolean
select_all(
	GtkWidget	*widget,
	GdkEventFocus	*event,
	gpointer		user_data)
{
	GtkEntry *entry = GTK_ENTRY (widget);
ENTER_FUNC;
	gtk_entry_select_region(entry, 0, entry->text_length);
	GTK_EDITABLE (entry)->current_pos = 0;
LEAVE_FUNC;
	return (TRUE);
}

extern	gboolean
unselect_all(
	GtkWidget	*widget,
	GdkEventFocus	*event,
	gpointer		user_data)
{
	GtkEntry *entry = GTK_ENTRY (widget);
ENTER_FUNC;
	gtk_entry_select_region (entry, 0, 0);
LEAVE_FUNC;
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

ENTER_FUNC;
	if		(event->keyval == GDK_KP_Enter) {
		window = gtk_widget_get_toplevel(widget);
		nextWidget = GetWidgetByLongName(next);
		if (nextWidget != NULL) {
			gtk_widget_grab_focus (nextWidget);
		}
		gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_press_event");
	}
LEAVE_FUNC;
	return	(TRUE);
}

extern	gboolean
press_filter(
	GtkWidget	*widget,
	GdkEventButton	*event,
	gpointer		user_data)
{
	gboolean	rc;
ENTER_FUNC;
	/* If WIDGET already has focus, do the default action */
	if		(GTK_WIDGET_HAS_FOCUS (widget)) {
		rc = FALSE;
	} else {
		/* Otherwise, just grab focus */
		gtk_widget_grab_focus (widget);
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");
		rc = TRUE;
	}
LEAVE_FUNC;
	return	(rc);
}

static GtkWidget*
find_widget_ancestor(
	GtkWidget	*w,
	GtkType		t)
{
	GtkWidget	*widget;
	GtkType		type;
	widget = w;
	while	(	widget	) {
		type = (long)(((GtkTypeObject *)widget)->klass->type);
		if	(	type == t	){
			return widget;
		}
		widget = widget->parent;
	}
	return NULL;
}

extern	void
set_focus_child_container(
	GtkContainer	*container,
	GtkWidget		*widget,
	gpointer		*user_data)
{
	GtkViewport		*vp;
	GtkAdjustment	*adj;
	GtkAllocation	alloc;

#define	FOCUS_MARGIN	10

ENTER_FUNC;
	if	(	!container || !widget	) return;
	vp = GTK_VIEWPORT(find_widget_ancestor(
			GTK_WIDGET(container), 
			GTK_TYPE_VIEWPORT));
	if	(	vp	) {
		alloc = widget->allocation;
		if	(	(adj = gtk_viewport_get_vadjustment(vp))	){
			if	(	alloc.y < adj->value	){
				if	( alloc.y - FOCUS_MARGIN < adj->lower ) {
					adj->value = alloc.y;
				} else {
					adj->value = alloc.y - FOCUS_MARGIN;
				}
				gtk_adjustment_value_changed(adj);
			}
			if 	(	alloc.y + alloc.height > adj->value + adj->page_size ) {
				if	(	alloc.y + alloc.height + FOCUS_MARGIN > adj->upper	) {
					adj->value = adj->upper - adj->page_size;
				} else {
					adj->value = alloc.y + alloc.height - adj->page_size 
									+ FOCUS_MARGIN;
				}
				gtk_adjustment_value_changed(adj);
			}
		}
		if	(	(adj = gtk_viewport_get_hadjustment(vp))	){
         	if  (   alloc.x < adj->value    ){
                if  ( alloc.x - FOCUS_MARGIN < adj->lower ) {
                    adj->value = alloc.x;
                } else {
                    adj->value = alloc.x - FOCUS_MARGIN;
                }
                gtk_adjustment_value_changed(adj);
            }
            if  (   alloc.x + alloc.width > adj->value + adj->page_size ) {
				if	(	alloc.x + alloc.width + FOCUS_MARGIN > adj->upper	) {
					adj->value = alloc.x + alloc.width - adj->page_size;
				} else {
					adj->value = alloc.x + alloc.width - adj->page_size
									+ FOCUS_MARGIN;
				}
                gtk_adjustment_value_changed(adj);
            }
		}
	}
LEAVE_FUNC;
}

static int TimerID = 0;
static char *TimerEvent = "";

static	void
StartEventTimer(
	char		*event,
	int			timeout,
	GtkFunction function,
	GtkWidget	*widget)
{
ENTER_FUNC;
	TimerEvent = event;
    TimerID = gtk_timeout_add (timeout, function, widget);
LEAVE_FUNC;
}

static	void
StopEventTimer(void)
{
ENTER_FUNC;	
    if (TimerID != 0) {
		gtk_timeout_remove(TimerID);
		TimerID = 0;
	}
LEAVE_FUNC;
}

extern	void
send_event(
	GtkWidget	*widget,
	char		*event)
{
	char		*wname;
	static Bool	ignore_event = FALSE;

ENTER_FUNC;
	if		(  !fInRecv &&  !ignore_event ) {
		fInRecv = TRUE;

		StopEventTimer();
		StopTimerWidgetAll();

		ShowBusyCursor(widget);

		wname = GetWindowName(widget);
		/* send event */
		if		(  event  !=  NULL  ) {
			SendEvent(FPCOMM(glSession),
					  wname,
					  gtk_widget_get_name(widget),
					  event);
		} else {
			SendEvent(FPCOMM(glSession),
					  wname,
					  gtk_widget_get_name(widget),
					  gtk_widget_get_name(widget));
		}
		SendWindowData();
		BlockChangedHandlers();
		GetScreenData(FPCOMM(glSession));
		UnblockChangedHandlers();
		if	( ! fKeyBuff  ) {
			ignore_event = TRUE;
			ClearKeyBuffer();
			ignore_event = FALSE;
		}
		HideBusyCursor(widget); 
		fInRecv = FALSE;
	}
LEAVE_FUNC;
}

extern	void
send_event_on_focus_out(
	GtkWidget		*widget,
	GdkEventFocus	*focus,
	char			*event)
{
	send_event (widget, event);
}

static gint
send_event_if_kana (gpointer widget)
{
	GtkWidget	*window;
	guchar *text = gtk_entry_get_text (GTK_ENTRY (widget));
	int len = strlen (text);
ENTER_FUNC;
	window = gtk_widget_get_toplevel(widget);
	if (len == 0 || text[len - 1] >= 0x80)
	{
		entry_changed (widget, TimerEvent);
		send_event (widget, TimerEvent);
	}
LEAVE_FUNC;
  return FALSE;
}

extern void
send_event_when_idle(
	GtkWidget	*widget,
	char		*event)
{
	static int registed = 0;
	static int timeout = -1;
	static int openchanged = 0;
ENTER_FUNC;
	StopEventTimer();
	if (!registed) {
		RegisterChangedHandler (GTK_OBJECT (widget), send_event_when_idle, event);
		registed = 1;
	}
	if (timeout == -1) {
		if (fTimer)
			timeout = atoi (TimerPeriod);
		else
			timeout = 0;
	}

	if (timeout > 0) {
		if ( openchanged == 0 ) {
			openchanged += 1;
		} else {
			StartEventTimer(event, timeout, send_event_if_kana, widget);
		}
	} else {
		entry_changed (widget, event);
	}
LEAVE_FUNC;
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
clist_select(
	GtkWidget	*widget,
	gint		row,
	gint		column,
	GdkEventButton	*dummy,
	char		*event)
{
	AddChangedWidget(widget);
	send_event(widget, event);
}

extern	gboolean
notebook_send_event(
	GtkNotebook	*widget,
	GtkNotebookPage *page,
	gint			now_pageno,
	char			*event)
{
	int			*pageno;
	gboolean	rc;

	pageno = (int *)gtk_object_get_data(GTK_OBJECT(widget), "pageno");
	AddChangedWidget((GtkWidget *)widget);
	if ( now_pageno != *pageno ){
		*pageno = now_pageno;
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "switch_page");
		send_event(GTK_WIDGET(widget), event);
		rc = TRUE;	
	} else {
		rc = FALSE;
	}
	return rc;
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

ENTER_FUNC;
	window = gtk_widget_get_toplevel(GTK_WIDGET(entry));
	nextWidget = GetWidgetByLongName(next);
	if (nextWidget != NULL) {
		gtk_widget_grab_focus(nextWidget);
	}
LEAVE_FUNC;
}

extern	void
changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	AddChangedWidget((GtkWidget *)entry);
}

extern	void
entry_changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	AddChangedWidget((GtkWidget *)entry);
}

extern	void
text_changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	AddChangedWidget((GtkWidget *)entry);
}

extern	void
button_toggled(
	GtkWidget	*button,
	gpointer	user_data)
{
	AddChangedWidget((GtkWidget *)button);
}

extern	void
selection_changed(
	GtkWidget	*entry,
	gpointer	user_data)
{
	AddChangedWidget((GtkWidget *)entry);
}

extern void
fileentry_changed(
	GtkWidget	*widget,
	gpointer	user_data)
{
	GtkWidget	*fileentry;

    fileentry = find_widget_ancestor(widget, GNOME_TYPE_FILE_ENTRY);
    if (fileentry != NULL) {
	  AddChangedWidget(fileentry);
	  AddChangedWidget(widget);
    }
}

extern	void
click_column(
	GtkWidget	*button,
	gpointer	user_data)
{
	AddChangedWidget((GtkWidget *)button);
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
	//fprintf(stderr,"map_event\n");
}

extern	void
set_focus(
	GtkWidget	*widget,
	gpointer	user_data)
{
ENTER_FUNC;
LEAVE_FUNC;
}

extern	void
day_selected(
	GtkCalendar	*widget,
	gpointer	user_data)
{
	AddChangedWidget((GtkWidget *)widget);
#ifdef	DEBUG
	printf("%d\n",(int)user_data);
	printf("year = %d\n",widget->year);
	printf("month = %d\n",widget->month+1);
	printf("day = %d\n",widget->selected_day);
#endif
}

extern	gboolean
switch_page(
	GtkNotebook	*widget,
	GtkNotebookPage *page,
	gint			now_pageno,
	char			*user_data)
{
	gboolean	rc;
	int			*pageno;

	pageno = (int *)gtk_object_get_data(GTK_OBJECT(widget), "pageno");
	AddChangedWidget((GtkWidget *)widget);
	if ((user_data != NULL ) &&
		(pageno != NULL)	&&
		(now_pageno != *pageno)){
		*pageno = now_pageno;
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "switch_page");
		rc = TRUE;	
	} else {
		rc = FALSE;
	}
	return rc;
}

extern	void
window_close(
	GtkWidget	*widget,
	gpointer	user_data)
{
	GtkWidget	*window;
	char		*name;

ENTER_FUNC;
	name = gtk_widget_get_name(widget);
	window = GetWidgetByLongName(name);
	if (window != NULL) {
		gtk_widget_hide(GTK_WIDGET(window));
	}
LEAVE_FUNC;
}

extern	void
window_destroy(
	GtkWidget	*widget,
	gpointer	user_data)
{
	fInRecv = TRUE;
	gtk_main_quit();
}

#ifdef	GTK_PANDA_TYPE_HTML
extern void
new_browser_window(char *uri)
{
	struct sigaction	sa, old_sa;
	
	int err;
	pid_t	pid;
	char *argv[3];
	extern char **environ;
	
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_NOCLDWAIT;
	if (sigaction(SIGCHLD, &sa, &old_sa) != 0) {
		Error("sigaction(2) failure");
	}
	
	if ((pid = fork()) == 0) { 
		argv[0] = GTK1_OPEN_BROWSER_COMMAND;
		argv[1] = uri;
		argv[2] = NULL;
		printf("%s %s\n", argv[0], argv[1]);
	 	err = execve(argv[0], argv, environ);
		fprintf(stderr, "error: %s", strerror(errno));
		_exit(1);
	} else if (pid < 0 ) { 
	 	Warning("fork errror"); 
	} 
	sigaction(SIGCHLD, &old_sa, NULL);
}

extern	void
open_browser(
	GtkPandaHTML	*html,
	XmHTMLAnchorCallbackStruct *cbs,
	void *data)
{
	/* build target URI */
	char	uri[FILENAME_MAX];
	char	*p;

ENTER_FUNC;
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
		new_browser_window(uri);		
	}
LEAVE_FUNC;
}
#endif
