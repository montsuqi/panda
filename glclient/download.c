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
#include	"interface.h"
#include	"dialogs.h"
#include	"desktop.h"
#include	"utils.h"
#include	"download.h"
#include	"widgetcache.h"
#include	"message.h"
#include	"debug.h"

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

typedef struct {
	GtkWidget *dialog;
	GtkWidget *widget;
	char *filename;
	LargeByteString *binary;
}FileInfo;

void
cb_open(
	GtkWidget *button,
	FileInfo *info)
{
	OpenDesktop(info->filename,info->binary);
	gtk_widget_show_all(info->dialog);
}

void
cb_save(
	GtkWidget *button,
	FileInfo *info)
{
	show_save_dialog(info->widget,info->filename,info->binary);
	gtk_widget_show_all(info->dialog);
}

void
cb_close(
	GtkWidget *button,
	FileInfo *info)
{
	gtk_widget_hide(info->dialog);
	gtk_main_quit();
}

void
show_download_dialog(
	GtkWidget		*widget,
	char			*filename,
	LargeByteString	*binary)
{
	GtkWindow *parent;
	GtkWidget *dialog;
	GtkWidget *open_button;
	GtkWidget *save_button;
	GtkWidget *close_button;
	char hbytes[64];
	FileInfo *info;

	
	parent = (GtkWindow *)g_list_nth_data(DialogStack,
		g_list_length(DialogStack)-1);
	if (parent == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}

	get_human_bytes(LBS_Size(binary), hbytes);
	dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL,
		GTK_MESSAGE_INFO,
		GTK_BUTTONS_NONE,
		_("Do you open this file or save it?\n\n"
		"File Name:%s\n"
		"Size:%s"),filename,hbytes);

	info = g_new0(FileInfo,1);
	info->dialog = dialog;
	info->widget = widget;
	info->filename = filename;
	info->binary = binary;

	open_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
		GTK_STOCK_OPEN, GTK_RESPONSE_YES);
	save_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
		GTK_STOCK_SAVE, GTK_RESPONSE_NO);
	close_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	if (!CheckDesktop(filename)) {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
			GTK_RESPONSE_YES,FALSE);
	}
	g_signal_connect(G_OBJECT(open_button),"clicked",
		G_CALLBACK(cb_open),info);
	g_signal_connect(G_OBJECT(save_button),"clicked",
		G_CALLBACK(cb_save),info);
	g_signal_connect(G_OBJECT(close_button),"clicked",
		G_CALLBACK(cb_close),info);
	gtk_widget_show_all(dialog);
	gtk_main();
	g_free(info);
	gtk_widget_destroy(dialog);
}
