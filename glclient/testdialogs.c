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
	dialog3 = gnome_question_dialog_parented("Already file Write?",
				GTK_SIGNAL_FUNC(question_clicked), NULL, GTK_WINDOW(dialog2));
	gtk_widget_show (dialog3);
#endif
	gtk_main();
	
	return 0;
}
