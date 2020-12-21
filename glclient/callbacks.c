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
#include <config.h>
#endif

#include <stdio.h>

#include <gtk/gtk.h>
#include "gettext.h"
#include <gtkpanda/gtkpanda.h>
#include <gdk/gdkkeysyms.h>

#include "callbacks.h"
#include "types.h"
#include "libmondai.h"
#include "glclient.h"
#include "action.h"
#include "widgetOPS.h"
#include "protocol.h"
#include "dialogs.h"
#include "utils.h"
#include "logger.h"

extern gboolean select_all(GtkWidget *widget, GdkEventFocus *event,
                           gpointer user_data) {
  GtkSettings *set = gtk_widget_get_settings(widget);
  gtk_settings_set_long_property(set, "gtk-entry-select-on-focus", 1,
                                 "glclient2");
  return (FALSE);
}

extern gboolean unselect_all(GtkWidget *widget, GdkEventFocus *event,
                             gpointer user_data) {

  GtkSettings *set = gtk_widget_get_settings(widget);
  gtk_settings_set_long_property(set, "gtk-entry-select-on-focus", 0,
                                 "glclient2");
  return (FALSE);
}

extern gboolean keypress_filter(GtkWidget *widget, GdkEventKey *event,
                                char *next) {
  GtkWidget *nextWidget;

#ifdef LIBGTK_3_0_0
  if (event->keyval == GDK_KEY_KP_Enter) {
#else
  if (event->keyval == GDK_KP_Enter) {
#endif
    nextWidget = GetWidgetByLongName(next);
    if (nextWidget != NULL) {
      gtk_widget_grab_focus(nextWidget);
    }
    g_signal_stop_emission_by_name(G_OBJECT(widget), "key_press_event");
  }
  return (FALSE);
}

extern gboolean press_filter(GtkWidget *widget, GdkEventButton *event,
                             gpointer user_data) {
  gboolean rc;
  /* If WIDGET already has focus, do the default action */
  if (gtk_widget_has_focus(widget)) {
    rc = FALSE;
  } else {
    /* Otherwise, just grab focus */
    gtk_widget_grab_focus(widget);
    g_signal_stop_emission_by_name(G_OBJECT(widget), "button_press_event");
    rc = TRUE;
  }
  return (rc);
}

static int TimerID = 0;
static char *TimerEvent = "";

static void StartEventTimer(char *event, int timeout, GSourceFunc function,
                            GtkWidget *widget) {
  TimerEvent = event;
  TimerID = g_timeout_add(timeout, function, widget);
}

static void StopEventTimer(void) {
  if (TimerID != 0) {
    g_source_remove(TimerID);
    TimerID = 0;
  }
}

static json_object *MakeEventData(const char *window, const char *widget,
                                  const char *event) {
  json_object *params, *event_data;

  event_data = json_object_new_object();
  json_object_object_add(event_data, "window", json_object_new_string(window));
  json_object_object_add(event_data, "widget", json_object_new_string(widget));
  json_object_object_add(event_data, "event", json_object_new_string(event));
  json_object_object_add(event_data, "screen_data", MakeScreenData(window));

  params = json_object_new_object();
  json_object_object_add(params, "event_data", event_data);
  if (SCREENDATA(Session) != NULL) {
    json_object_put(SCREENDATA(Session));
  }

  Info("window:%s widget:%s event:%s", window, widget, event);

  return params;
}

extern void send_event(GtkWidget *widget, char *event) {
  char *window_name, *widget_name;
  unsigned long t1, t2, t3, t4, t5, rpc, server, app;
  json_object *params;

  window_name = GetWindowName(widget);
  widget_name = (char *)gtk_widget_get_name(widget);
  event = event != NULL ? event : widget_name;
  if (strcmp(window_name, THISWINDOW(Session))) {
#if 0
		Warning("window name unmatched glade[%s],session[%s]",
			window_name,THISWINDOW(Session));
#endif
    return;
  }
  if (!ISRECV(Session)) {
    ISRECV(Session) = TRUE;
    t1 = now();

    StopEventTimer();
    StopTimersAll();

    ShowBusyCursor(TopWindow);
    BlockChangedHandlers();

    params = MakeEventData(window_name, widget_name, event);
    t2 = now();

    SCREENDATA(Session) = RPC_SendEvent(GLP(Session), params);
    rpc = GLP(Session)->RPCExecTime;
    server = GLP(Session)->TotalExecTime;
    app = GLP(Session)->AppExecTime;

    t3 = now();
    UpdateScreen();
    t4 = now();

    if (!fKeyBuff) {
      ClearKeyBuffer();
    }
    UnblockChangedHandlers();
    HideBusyCursor(TopWindow);

    ISRECV(Session) = FALSE;
    t5 = now();

    Info("[send_event] "
         "total:%lums "
         "make_event_data:%lums "
         "rpc_total:%lums "
         "server_total:%lums "
         "server_app:%lums "
         "update_screen:%lums",
         (t5 - t1), (t2 - t1), (rpc), (server), (app), (t4 - t3));
  }
}

static gint send_event_if_kana(gpointer widget) {
  guchar *text = (guchar *)gtk_entry_get_text(GTK_ENTRY(widget));
  int len = strlen(text);
  if (len == 0 || text[len - 1] >= 0x80) {
    entry_changed(widget, TimerEvent);
    send_event(widget, TimerEvent);
  }
  return FALSE;
}

extern void send_event_when_idle(GtkWidget *widget, char *event) {
  static int registed = 0;
  static int timeout = -1;
  static int openchanged = 0;

  StopEventTimer();
  if (!registed) {
    RegisterChangedHandler(G_OBJECT(widget), G_CALLBACK(send_event_when_idle),
                           event);
    registed = 1;
  }
  if (timeout == -1) {
    if (fTimer)
      timeout = TimerPeriod;
    else
      timeout = 0;
  }

  if (timeout > 0) {
    if (openchanged == 0) {
      openchanged += 1;
    } else {
      StartEventTimer(event, timeout, send_event_if_kana, widget);
    }
  } else {
    entry_changed(widget, event);
  }
}

extern void clist_send_event(GtkWidget *widget, gint row, gint column,
                             char *event) {
  send_event(widget, "SELECT");
}

extern void table_send_event(GtkWidget *widget, gint row, gint column,
                             gchar *value, gchar *event) {
  gchar *oldvalue;

  oldvalue = (gchar *)g_object_get_data(G_OBJECT(widget), "changed_value");
  if (oldvalue != NULL) {
    g_free(oldvalue);
  }

  g_object_set_data(G_OBJECT(widget), "changed_value", g_strdup(value));

  AddChangedWidget(widget);
  send_event(widget, event);
}

extern void changed(GtkWidget *entry, gpointer user_data) {
  AddChangedWidget((GtkWidget *)entry);
}

extern void entry_changed(GtkWidget *entry, gpointer user_data) {
  AddChangedWidget((GtkWidget *)entry);
}

extern void text_changed(GtkWidget *entry, gpointer user_data) {
  AddChangedWidget((GtkWidget *)entry);
}

extern void button_toggled(GtkWidget *button, gpointer user_data) {
  AddChangedWidget((GtkWidget *)button);
}

extern void selection_changed(GtkWidget *widget, gpointer user_data) {
  AddChangedWidget(widget);
}

extern void click_column(GtkWidget *button, gpointer user_data) {
  AddChangedWidget((GtkWidget *)button);
}

extern void map_event(GtkWidget *widget, gpointer user_data) {
  // fprintf(stderr,"map_event\n");
}

extern void set_focus(GtkWidget *widget, gpointer user_data) {}

extern void day_selected(GtkCalendar *widget, gpointer user_data) {
  AddChangedWidget((GtkWidget *)widget);
#ifdef DEBUG
  printf("%d\n", (int)user_data);
  printf("year = %d\n", widget->year);
  printf("month = %d\n", widget->month + 1);
  printf("day = %d\n", widget->selected_day);
#endif
}

extern gboolean no_switch_page(GtkNotebook *widget, gpointer *page,
                               gint new_pageno, char *user_data) {
  if (ISRECV(Session)) {
    return FALSE;
  } else {
    g_signal_stop_emission_by_name(G_OBJECT(widget), "switch_page");
    return TRUE;
  }
}

extern gboolean switch_page(GtkNotebook *widget, gpointer *page,
                            gint new_pageno, char *user_data) {
  int pageno;
  gboolean rc;

  pageno = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "pageno"));
  AddChangedWidget((GtkWidget *)widget);
  if (pageno != new_pageno) {
    g_signal_stop_emission_by_name(G_OBJECT(widget), "switch_page");
    g_object_set_data(G_OBJECT(widget), "pageno", GINT_TO_POINTER(new_pageno));
    rc = TRUE;
  } else {
    rc = FALSE;
  }
  return rc;
}

extern gboolean notebook_send_event(GtkNotebook *widget, gpointer *page,
                                    gint new_pageno, char *user_data) {
  AddChangedWidget(GTK_WIDGET(widget));
  g_object_set_data(G_OBJECT(widget), "new_pageno",
                    GINT_TO_POINTER(new_pageno));
  send_event(GTK_WIDGET(widget), "SWITCH");
  return FALSE;
}

extern void window_close(GtkWidget *widget, gpointer user_data) {
  GtkWidget *window;
  char *name;

  name = (char *)gtk_widget_get_name(widget);
  window = GetWidgetByLongName(name);
  if (window != NULL) {
    gtk_widget_hide(GTK_WIDGET(window));
  }
}

extern void window_destroy(GtkWidget *widget, gpointer user_data) {
  ISRECV(Session) = TRUE;
  gtk_main_quit();
}

#ifdef GTK_PANDA_TYPE_HTML
extern void open_browser(GtkPandaHTML *html, void *data1, void *data2) {
  // do nothing
}
#endif
