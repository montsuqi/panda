/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2006 Noboru Saitou
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif

#include	"comm.h"
#include	"fileEntry.h"

int 
main(int argc, char *argv[])
{
#ifdef USE_GNOME
	GtkWidget *fentry;
	GtkWidget *window;
    LargeByteString *binary;
		
	gnome_init("testfileentry", VERSION, argc, argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	fentry = gnome_file_entry_new (NULL, "Save As");
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(fentry), "/");
	binary = NewLBS();
	LBS_EmitString(binary, "Test Object file\n");
	gtk_object_set_data(GTK_OBJECT(fentry), "recvobject", binary);
	gtk_container_add(GTK_CONTAINER(window),GTK_WIDGET(fentry));

	gtk_signal_connect_after(GTK_OBJECT(fentry),"browse_clicked",
				 GTK_SIGNAL_FUNC(browse_clicked),
				 window);
	gtk_widget_show_all (window);
	gtk_signal_connect (GTK_OBJECT (window), "destroy",
						GTK_SIGNAL_FUNC(gtk_main_quit),
						NULL);
	gtk_signal_emit_by_name (GTK_OBJECT (fentry),
							 "browse_clicked");
	gtk_main();
#endif
	return 0;
}

