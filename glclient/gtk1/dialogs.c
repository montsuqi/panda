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
#include	<gnome.h>

#include	"dialogs.h"

GtkWidget*
message_dialog(
	const char *message,
	gboolean message_type)
{
	GtkWidget *dialog, *button;
	if (message_type){
		dialog = gnome_ok_dialog(message);
	} else {
		dialog = gnome_error_dialog(message);
	}
	button = GNOME_DIALOG (dialog)->buttons->data;
	gtk_widget_grab_focus (button);
	return (dialog);
}

GtkWidget*
question_dialog(
	const char *message,
	GtkSignalFunc clicked_handler,
	GtkWidget *widget,
	GtkWindow *window)
{
	GtkWidget *dialog;
		
	dialog = gnome_question_dialog_modal_parented(message,
									GTK_SIGNAL_FUNC(clicked_handler), 
									GTK_WIDGET(widget), GTK_WINDOW(window));
	return (dialog);
}

void
show_message_dialog(
	const char *message)
{
	GtkWidget *dialog;
		
	if(strlen(message) <= 0) return;
	dialog = message_dialog(message, TRUE);
	gnome_dialog_run(GNOME_DIALOG(dialog));
}

void
show_error_dialog(
	const char *message)
{
	GtkWidget *dialog;
		
	if(strlen(message) <= 0) return;
	dialog = message_dialog(message, FALSE);
	gnome_dialog_run(GNOME_DIALOG(dialog));
	exit(1);
}

static gboolean
on_okbutton (GtkWidget * widget, gboolean *ret)
{
	*ret = TRUE;
	gtk_main_quit ();
	return TRUE;
}

int
askpass(
	char *buf,
	size_t	buflen,
	const char *prompt)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *okbutton;
	GtkWidget *cancelbutton;
	gboolean response;
	char *str;
	int ret;

	response = FALSE;

	dialog = gtk_dialog_new();
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

	label = gtk_label_new (_(prompt));
	gtk_misc_set_padding (GTK_MISC (label), 20, 20);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		TRUE, TRUE, 0);
	gtk_widget_show (label);

	entry = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry,
		TRUE, TRUE, 0);
	gtk_widget_show (entry);
	gtk_signal_connect(GTK_OBJECT (entry), "activate",
		GTK_SIGNAL_FUNC (on_okbutton),
		&response);

	okbutton = gtk_button_new_with_label (_("OK"));
	gtk_widget_set_usize (okbutton, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		okbutton,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (okbutton), "clicked",
		GTK_SIGNAL_FUNC (on_okbutton),
		&response);
	gtk_widget_show (okbutton);

	cancelbutton = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_set_usize (cancelbutton, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		cancelbutton,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (cancelbutton), "clicked",
		GTK_SIGNAL_FUNC (gtk_main_quit),
		NULL);
	gtk_widget_show (cancelbutton);

	GTK_WIDGET_SET_FLAGS (entry, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (entry);
	gtk_widget_grab_focus (entry);
	gtk_widget_show (dialog);
	gtk_main();

	if (response) {
		str = gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
		strncpy(buf, str, buflen);
		g_free(str);
		ret = strlen(buf);
	} else {
		ret = -1;
	}
	gtk_widget_destroy(dialog);
 	return ret;
}
