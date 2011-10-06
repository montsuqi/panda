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
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<glib.h>
#include	<gtk/gtk.h>
#include	<gtkpanda/gtkpanda.h>
#include	<errno.h>

#include	"glclient.h"
#include	"gettext.h"
#include	"interface.h"
#include	"dialogs.h"
#include	"desktop.h"
#include	"print.h"
#include	"widgetcache.h"
#include	"utils.h"
#include	"message.h"
#include	"debug.h"

static void
show_preview_dialog(
	char *title,
	char *fname)
{
	GtkWindow *parent;
	GtkWidget *dialog;
	GtkWidget *content;
	GtkWidget *pandapdf;

	parent = (GtkWindow *)g_list_nth_data(DialogStack,
		g_list_length(DialogStack)-1);
	if (parent == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}
	dialog = gtk_dialog_new_with_buttons(_("Preview"),parent,
		GTK_DIALOG_MODAL,
		GTK_STOCK_OK,
		GTK_RESPONSE_NONE,NULL);
	gtk_window_set_title(GTK_WINDOW(dialog),title);
	gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
	pandapdf = gtk_panda_pdf_new();
	gtk_panda_pdf_load(GTK_PANDA_PDF(pandapdf),fname);
	gtk_widget_set_usize(pandapdf,800,600);
	g_signal_connect_swapped(dialog,"response",
		G_CALLBACK(gtk_widget_destroy),dialog);
#if 0
	// gtk >= 2.14
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	content = GTK_DIALOG(dialog)->vbox;
#endif
	gtk_container_add(GTK_CONTAINER(content),pandapdf);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
}

static void
show_save_dialog(
	char *oldpath)
{
	GtkWindow *parent;
	GtkWidget *dialog;
	GtkWidget *error_dialog;
	char *dirname;
	char *newpath;

	parent = (GtkWindow *)g_list_nth_data(DialogStack,
		g_list_length(DialogStack)-1);
	if (parent == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}
	dialog = gtk_file_chooser_dialog_new (_("Specify filename..."),
		parent, GTK_FILE_CHOOSER_ACTION_SAVE,
    	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    	GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (
		GTK_FILE_CHOOSER (dialog), TRUE);
    
	dirname = GetWidgetCache("printdialog");
	if (dirname != NULL) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog), dirname);
	}
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "*.pdf");

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		newpath = gtk_file_chooser_get_filename(
			GTK_FILE_CHOOSER (dialog));
		if(!rename(oldpath,newpath)) {
			SetWidgetCache("printdialog", g_path_get_dirname(newpath));
		} else {
			error_dialog = gtk_message_dialog_new (GTK_WINDOW(dialog),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("Can't save to %s \n%s"),newpath, strerror(errno));
				gtk_dialog_run (GTK_DIALOG (error_dialog));
				gtk_widget_destroy (error_dialog);
		}
	}
	gtk_widget_destroy (dialog);
}

void
show_print_dialog(
	char	*title,
	char	*fname,
	size_t	size)
{
	GtkWindow *parent;
	GtkWidget *dialog;
	GtkWidget *pandapdf;
	char hbytes[64];
	int response;

	pandapdf = gtk_panda_pdf_new();
	if (!gtk_panda_pdf_load(GTK_PANDA_PDF(pandapdf),fname)) {
		gtk_widget_destroy(pandapdf);
		return;
	}
	
	parent = (GtkWindow *)g_list_nth_data(DialogStack,
		g_list_length(DialogStack)-1);
	if (parent == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}

	get_human_bytes(size, hbytes);
	dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL,
		GTK_MESSAGE_INFO,
		GTK_BUTTONS_NONE,
		_("There is the print data.\nDo you print the data?\n\n"
		"Title:%s\n"
		"Num of Pages:%d\n"
		"Size:%s"),
		title,gtk_panda_pdf_get_page_count(GTK_PANDA_PDF(pandapdf)),hbytes);
	gtk_widget_destroy(pandapdf);
	gtk_window_set_title(GTK_WINDOW(dialog),title);

	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
		GTK_STOCK_PRINT_PREVIEW, GTK_RESPONSE_YES,
		GTK_STOCK_SAVE, GTK_RESPONSE_NO,
		GTK_STOCK_PRINT, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (response) {
	case GTK_RESPONSE_YES:
		show_preview_dialog(title,fname);
		break;
	case GTK_RESPONSE_NO:
		show_save_dialog(fname);
		break;
	case GTK_RESPONSE_OK:
		gtk_panda_pdf_print(GTK_PANDA_PDF(pandapdf),TRUE);
		break;
	}
	gtk_widget_destroy(dialog);
	gtk_widget_destroy(pandapdf);
}

void
print_with_default_printer(
	char	*fname)
{
	GtkWidget *pandapdf;

	pandapdf = gtk_panda_pdf_new();
	if (!gtk_panda_pdf_load(GTK_PANDA_PDF(pandapdf),fname)) {
		gtk_widget_destroy(pandapdf);
		return;
	}
	gtk_panda_pdf_print(GTK_PANDA_PDF(pandapdf),FALSE);
	gtk_widget_destroy(pandapdf);
}
