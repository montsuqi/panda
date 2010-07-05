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

#include	"glclient.h"
#include	"gettext.h"
#include	"toplevel.h"
#include	"dialogs.h"
#include	"desktop.h"
#include	"printdialog.h"
#include	"widgetcache.h"

void
show_print_dialog(char *title,char *fname,size_t size)
{
	//not implement
}

#if 0

static void
show_save_dialog(
	GtkWidget *widget,
	char *filename,
	LargeByteString *binary)
{
	GtkWindow *parent;
	GtkWidget *dialog;
	GtkWidget *error_dialog;
	GError *error = NULL;
	char *dirname;
	char *fname;

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
    
	dirname = GetWidgetCache((char*)glade_get_widget_long_name(widget));
	if (dirname != NULL) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog), dirname);
	}
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		fname = gtk_file_chooser_get_filename(
			GTK_FILE_CHOOSER (dialog));
        if (g_file_set_contents(fname, LBS_Body(binary), LBS_Size(binary),
			&error)) {
			SetWidgetCache(
				((char *)glade_get_widget_long_name(widget)),
				g_path_get_dirname(fname));
		} else {
			error_dialog = gtk_message_dialog_new (GTK_WINDOW(dialog),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s",
				error->message);
				gtk_dialog_run (GTK_DIALOG (error_dialog));
				gtk_widget_destroy (error_dialog);
				g_error_free(error);
        }
		g_free(fname);
	}
	gtk_widget_destroy (dialog);
}

static void
get_human_bytes(
	size_t size,
	char *str)
{
	const double KB = 1024;
	const double MB = KB * KB;
	const double GB = MB * MB;
	
	if (size > GB) {
		sprintf(str, "%.1lf GB",(double)size/GB);
	} else if (size > MB) {
		sprintf(str, "%.1lf MB",(double)size/MB);
	} else if (size > KB) {
		sprintf(str, "%.1lf KB",(double)size/KB);
	} else {
		sprintf(str, "%ld Bytes",(unsigned long)size);
	}
}

void
show_download_dialog(
	GtkWidget		*widget,
	char			*filename,
	LargeByteString	*binary)
{
	GtkWindow *parent;
	GtkWidget *dialog;
	char buff[SIZE_LONGNAME];
	char hbytes[64];
	int response;
	
	parent = (GtkWindow *)g_list_nth_data(DialogStack,
		g_list_length(DialogStack)-1);
	if (parent == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}

	get_human_bytes(LBS_Size(binary), hbytes);
	snprintf(buff,sizeof(buff),_("Do you open this file or save it?\n\n"
		"File Name:%s\n"
		"Size:%s"
		),filename,hbytes);
	dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL,
		GTK_MESSAGE_INFO,
		GTK_BUTTONS_NONE,
		_("Do you open this file or save it?\n\n"
		"File Name:%s\n"
		"Size:%s"),filename,hbytes);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
		GTK_STOCK_OPEN, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_NO,
		NULL);

	if (!CheckDesktop(filename)) {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
			GTK_RESPONSE_YES,FALSE);
	}

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (response) {
	case GTK_RESPONSE_YES:
		OpenDesktop(filename,binary);
		break;
	case GTK_RESPONSE_NO:
		show_save_dialog(widget,filename,binary);
		break;
	}
	gtk_widget_destroy(dialog);
}
#endif
