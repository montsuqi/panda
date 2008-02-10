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

#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif

#include	"dialogs.h"

GtkWidget*
message_dialog(
	const char *message,
	gboolean message_type)
{
#ifdef USE_GNOME
	GtkWidget *dialog, *button;
	if (message_type){
		dialog = gnome_ok_dialog(message);
	} else {
		dialog = gnome_error_dialog(message);
	}
	button = GNOME_DIALOG (dialog)->buttons->data;
	gtk_widget_grab_focus (button);
#else
	GtkWidget *dialog, *label, *button;

	dialog = gtk_dialog_new();

	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

	label = gtk_label_new (message);
	gtk_misc_set_padding (GTK_MISC (label), 20, 20);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
						TRUE, TRUE, 0);
	gtk_widget_show (label);

	button = gtk_button_new_with_label ("OK");
	gtk_widget_set_usize (button, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button,
						FALSE, FALSE, 14);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_grab_focus (button);
	gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
							   GTK_SIGNAL_FUNC (gtk_widget_destroy),
							   GTK_OBJECT (dialog));
	gtk_widget_show (button);
#endif
	return (dialog);
}

#ifdef USE_GNOME
GtkWidget*
question_dialog(
	const char	*message,
	GtkSignalFunc	clicked_handler,
	GtkWidget	*widget,
	GtkWindow	*window)
{
	GtkWidget *dialog;
		
	dialog = gnome_question_dialog_modal_parented(message,
									GTK_SIGNAL_FUNC(clicked_handler), 
									GTK_WIDGET(widget), GTK_WINDOW(window));
	return (dialog);
}
#endif

extern void
GLError( const char *message)
{
	GtkWidget *dialog;
		
	if(strlen(message) <= 0) return;
	dialog = message_dialog(message, FALSE);
#ifdef USE_GNOME
	gnome_dialog_run(GNOME_DIALOG(dialog));
#else
	gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
						GTK_SIGNAL_FUNC(gtk_main_quit),
						NULL);
	gtk_widget_show (dialog);
	gtk_main();
#endif
	exit(0);
}

