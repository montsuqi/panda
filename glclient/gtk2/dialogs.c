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
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<gtk/gtk.h>
#include	"gettext.h"

#include	"dialogs.h"

#ifndef SIZE_BUFF
#define SIZE_BUFF               8192 
#endif

static GtkWidget*
_message_dialog(
	GtkMessageType type,
	const char *message)
{
	return gtk_message_dialog_new(NULL, 
			GTK_DIALOG_MODAL, 
			type,
			GTK_BUTTONS_OK,
			"%s",message);
}

void
message_dialog(
	GtkMessageType type,
	const char *message)
{
	GtkWidget *dialog;
		
	if(strlen(message) <= 0) return;
	dialog = _message_dialog(type, message);
	gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void
show_message_dialog(
	const char *message)
{
    message_dialog(GTK_MESSAGE_INFO, message);
}

void
show_error_dialog(
	const char *message)
{
    message_dialog(GTK_MESSAGE_ERROR, message);
	exit(1);
}

void
message_dialog_printf(
	GtkMessageType type,
	const char *format,
    ...)
{
    va_list va;
    char buff[SIZE_BUFF];

    va_start(va, format);
    vsnprintf(buff, sizeof(buff), format, va);
    va_end(va);
	message_dialog(type, buff);
}

void
error_dialog_printf(
	const char *format,
    ...)
{
    va_list va;
    char buff[SIZE_BUFF];

    va_start(va, format);
    vsnprintf(buff, sizeof(buff), format, va);
    va_end(va);
	message_dialog(GTK_MESSAGE_ERROR, buff);
	exit(0);
}

void
set_response(GtkDialog *dialog,
    gint response,
    gpointer data)
{
	gint	*ret;
	ret = data;
	*ret = response;
}

static void
askpass_entry_activate(GtkEntry *entry, gpointer user_data)
{
	GtkDialog	*dialog;

	dialog = GTK_DIALOG(user_data);
	gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

int
askpass(
	char *buf,
	size_t	buflen,
	const char *prompt)
{
	GtkWidget 		*dialog;
	GtkWidget		*entry;
	gint			response;
	char			*str;
    int				ret;

	dialog = gtk_message_dialog_new(NULL,
		GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_OK_CANCEL,
		prompt);
	entry = gtk_entry_new();
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
		entry, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
	gtk_widget_grab_focus(entry);
	g_signal_connect(G_OBJECT(entry), "activate", 
		G_CALLBACK(askpass_entry_activate), dialog);
    response = gtk_dialog_run(GTK_DIALOG (dialog));

    ret = -1;
	if (response == GTK_RESPONSE_OK) {
		str = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
		strncpy(buf, str, buflen);
		g_free(str);
		ret = strlen(buf);
	}
    gtk_widget_destroy(dialog);
    return ret;
}
