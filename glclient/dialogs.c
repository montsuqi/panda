/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2005 Ogochan & JMA (Japan Medical Association).

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
