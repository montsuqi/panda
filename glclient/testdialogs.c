/*
PANDA -- a simple transaction monitor
Copyright (C) 2004-2005 Kouji TAKAO

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif

#include "dialogs.h"

#ifdef USE_GNOME
static void
question_clicked(gint reply, gpointer data)
{
  gchar * sdata = (gchar *)data;
  gchar * s = NULL;

  if (reply == GNOME_YES) {
    s = g_strconcat(_("The user chose Yes/OK with data:\n"),
		       sdata, NULL);
  }
  else if (reply == GNOME_NO) {
    s = g_strconcat(_("The user chose No/Cancel with data:\n"),
		       sdata, NULL);
  }
  g_print(s);
  g_print("\n");
}
#endif

int 
main(int argc, char *argv[])
{
	GtkWidget *dialog1, *dialog2, *dialog3;
		
#ifdef USE_GNOME
	gnome_init("glclient", VERSION, argc, argv);
#else
	gtk_init(&argc, &argv);
#endif
	dialog1 = message_dialog("Test message", TRUE);
	gtk_signal_connect (GTK_OBJECT (dialog1), "destroy",
						GTK_SIGNAL_FUNC(gtk_main_quit),
						NULL);
	gtk_widget_show (dialog1);
	dialog2 = message_dialog("Error message", FALSE);
	gtk_widget_show (dialog2);
#ifdef USE_GNOME
	dialog3 = question_dialog("Already file Write?",
				(GtkSignalFunc) question_clicked, NULL,
							  GTK_WINDOW(dialog2));
	gtk_widget_show (dialog3);
#endif
	gtk_main();
	
	return 0;
}
