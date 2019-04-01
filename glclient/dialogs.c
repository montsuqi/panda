/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Kouji TAKAO & JMA (Japan Medical Association).
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
#include <stdlib.h>
#include <stdarg.h>
#include <gtk/gtk.h>

#include "glclient.h"
#include "dialogs.h"
#include "gettext.h"
#include "logger.h"

#ifndef SIZE_BUFF
#define SIZE_BUFF 8192
#endif

static GtkWidget *_message_dialog(GtkMessageType type, const char *message) {
  return gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK,
                                "%s", message);
}

void MessageDialog(GtkMessageType type, const char *message) {
  GtkWidget *dialog;

  if (message == NULL || strlen(message) <= 0)
    return;
  dialog = _message_dialog(type, message);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void InfoDialog(const char *format, ...) {
  gchar *buf;
  va_list va;

  va_start(va, format);
  buf = g_strdup_vprintf(format, va);
  va_end(va);
  Info(buf);
  MessageDialog(GTK_MESSAGE_INFO, buf);
  g_free(buf);
}

void WarnDialog(const char *format, ...) {
  gchar *buf;
  va_list va;

  va_start(va, format);
  buf = g_strdup_vprintf(format, va);
  va_end(va);
  Warning(buf);
  MessageDialog(GTK_MESSAGE_WARNING, buf);
  g_free(buf);
}

void ErrorDialog(const char *format, ...) {
  gchar *buf;
  va_list va;

  va_start(va, format);
  buf = g_strdup_vprintf(format, va);
  va_end(va);
  MessageDialog(GTK_MESSAGE_ERROR, buf);
  _Error(buf);
}

void set_response(GtkDialog *dialog, gint response, gpointer data) {
  gint *ret;
  ret = data;
  *ret = response;
}

static void askpass_entry_activate(GtkEntry *entry, gpointer user_data) {
  GtkDialog *dialog;

  dialog = GTK_DIALOG(user_data);
  gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

char *AskPassDialog(const char *prompt) {
  GtkWidget *dialog, *entry;
  gint response;
  char *ret;

  ret = NULL;

  dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER,
                                  GTK_BUTTONS_OK_CANCEL, "%s", prompt);

  entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
  gtk_widget_grab_focus(entry);
  g_signal_connect(G_OBJECT(entry), "activate",
                   G_CALLBACK(askpass_entry_activate), dialog);

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     entry, TRUE, TRUE, 0);

  gtk_widget_show_all(dialog);
  response = gtk_dialog_run(GTK_DIALOG(dialog));

  if (response == GTK_RESPONSE_OK) {
    ret = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
  }
  gtk_widget_destroy(dialog);
  return ret;
}

char *AskPINDialog() {
  GtkWidget *dialog, *entry, *check, *vbox;
  gint response;
  char *ret;

  ret = NULL;

  dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER,
                                  GTK_BUTTONS_OK_CANCEL, "%s", _("pin:"));

  entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
  gtk_widget_grab_focus(entry);
  g_signal_connect(G_OBJECT(entry), "activate",
                   G_CALLBACK(askpass_entry_activate), dialog);

  check = gtk_check_button_new_with_label(_("save pin"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), check, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     vbox, TRUE, TRUE, 0);

  gtk_widget_show_all(dialog);
  response = gtk_dialog_run(GTK_DIALOG(dialog));

  if (response == GTK_RESPONSE_OK) {
    ret = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    fSavePIN = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
  }
  gtk_widget_destroy(dialog);
  return ret;
}

Bool ConfirmDialog(const char *message) {
  GtkWidget *dialog;
  gint response;

  dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER,
                                  GTK_BUTTONS_OK_CANCEL, "%s", message);
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  return(response == GTK_RESPONSE_OK);
}
