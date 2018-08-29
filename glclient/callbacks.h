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

#ifndef _CALLBACKS_H
#define _CALLBACKS_H

#include <gtk/gtk.h>

extern gboolean select_all(GtkWidget *widget, GdkEventFocus *event,
                           gpointer user_data);
extern gboolean unselect_all(GtkWidget *widget, GdkEventFocus *event,
                             gpointer user_data);
extern gboolean press_filter(GtkWidget *widget, GdkEventButton *event,
                             gpointer user_data);
extern gboolean keypress_filter(GtkWidget *widget, GdkEventKey *event,
                                char *next);
extern gboolean switch_page(GtkNotebook *widget, gpointer *page,
                            gint now_pageno, char *user_data);
extern void send_event(GtkWidget *widget, char *event);
extern void send_event_if_changed(GtkWidget *widget, GdkEventFocus *focus,
                                  char *event);
extern void send_event_when_idle(GtkWidget *widget, char *event);
extern void entry_next_focus(GtkEntry *entry, char *next);
extern void entry_changed(GtkWidget *entry, gpointer user_data);
extern void togglebutton_toggled(GtkWidget *entry, gpointer user_data);
extern void entry_set_editable(GtkEntry *entry, gpointer user_data);
extern void map_event(GtkWidget *widget, gpointer user_data);
extern void window_close(GtkWidget *widget, gpointer user_data);
extern void clist_send_event(GtkWidget *widget, gint row, gint column,
                             char *event);
extern void table_send_event(GtkWidget *widget, gint row, gint column,
                             gchar *value, gchar *event);
extern void changed(GtkWidget *entry, gpointer user_data);
extern void text_changed(GtkWidget *entry, gpointer user_data);
extern void button_toggled(GtkWidget *button, gpointer user_data);
extern void selection_changed(GtkWidget *widget, gpointer user_data);
extern void click_column(GtkWidget *button, gpointer user_data);
extern void set_focus(GtkWidget *widget, gpointer user_data);
extern void day_selected(GtkCalendar *widget, gpointer user_data);
extern void window_destroy(GtkWidget *widget, gpointer user_data);
#ifdef GTK_PANDA_TYPE_HTML
extern void open_browser(GtkPandaHTML *html, void *data1, void *data2);
#endif

#endif
