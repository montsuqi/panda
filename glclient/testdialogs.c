#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif

#include "dialogs.h"

main(int argc, char *argv[])
{
	GtkWidget *dialog1, *dialog2;
		
#ifdef USE_GNOME
	gnome_init("glclient", VERSION, argc, argv);
#else
	gtk_init(&argc, &argv);
#endif
	dialog1 = message_dialog("Test message", TRUE);
	gtk_widget_show (dialog1);
	dialog2 = message_dialog("Error message", FALSE);
	gtk_widget_show (dialog2);
	gtk_main();
	
	return 0;
}
