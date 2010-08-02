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

#include	<errno.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<gnome.h>
#include	<glib.h>
#include	<libmondai.h>
#include	<glade/glade.h>

#include	"glclient.h"
#include	"dialogs.h"
#include	"toplevel.h"
#include	"widgetcache.h"
#include	"desktop.h"
#include	"utils.h"
#include	"download.h"

static Bool
SaveFile(
	GtkFileSelection *fs)
{
	char *filename;
	LargeByteString *binary;
	GtkWidget *dialog;
	GtkWidget *widget;
	gboolean rc;
    int	fd;
	char buf[SIZE_BUFF];

	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	binary = (LargeByteString *)gtk_object_get_data(GTK_OBJECT(fs), "binary");
	widget = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(fs), "widget");

	fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd > 0) {
		if ( write(fd, LBS_Body(binary), LBS_Size(binary) ) > 0){
			close(fd);
			rc = TRUE;
		} else {
			rc = FALSE;
		}
	} else {
		rc = FALSE;
	}
	if (!rc){
		snprintf(buf, sizeof(buf), _("cannot write %s \n%s"), 
				 filename, strerror(errno));
		dialog = message_dialog(buf, rc);
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
							  GTK_WINDOW (fs));
		gtk_window_set_modal (GTK_WINDOW(dialog), TRUE);
		gtk_widget_show (dialog);
	} else {
		SetWidgetCache(
			(char *)glade_get_widget_long_name(widget),
			g_dirname(filename));
	}
	return (rc);
}

static gboolean
on_rewrite(
	gint reply,
	gpointer data)
{
	GtkFileSelection *fs;

	fs = GTK_FILE_SELECTION (data);
	if (reply == GNOME_YES) {
		SaveFile(fs);
		gtk_widget_destroy(GTK_WIDGET(fs));
	}
	return FALSE;
}

static gboolean
on_ok_filesel (GtkWidget * widget, GtkFileSelection *fs)
{
	GtkWidget *dialog;
	char *filename;

	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	if (CheckAlreadyFile(filename)){
		SaveFile(fs);
		gtk_widget_destroy(GTK_WIDGET(fs));
	} else {
		dialog = question_dialog(
			_("File already exists. Rewrite?"),
			(GtkSignalFunc) on_rewrite, GTK_WIDGET(fs),
			GTK_WINDOW(fs));
	}
	return FALSE;
}

static void
fs_destroy(GtkWidget *widget,gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void
show_save_dialog(GtkWidget *widget,char *filename, LargeByteString *binary)
{
	char path[SIZE_LONGNAME];
	char *dirname;
	GtkWidget *fs;
	GtkWindow *parent;


	fs = gtk_file_selection_new(_("Please select a file for saving"));
	if ((parent = (GtkWindow *)g_list_nth_data(DialogStack,g_list_length(DialogStack)-1)) == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}
	gtk_window_set_transient_for(GTK_WINDOW(fs), parent);
	dirname = GetWidgetCache((char *)glade_get_widget_long_name(widget));
	if (dirname != NULL) {
		snprintf(path,sizeof(path),"%s/%s",dirname,filename);
	} else {
		snprintf(path,sizeof(path),"%s",filename);
	}
	gtk_object_set_data(GTK_OBJECT(fs),"binary",binary);
	gtk_object_set_data(GTK_OBJECT(fs),"widget",widget);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs),path);

	gtk_signal_connect(GTK_OBJECT(fs), "destroy",
		GTK_SIGNAL_FUNC(fs_destroy), fs);
	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_ok_filesel),
		GTK_OBJECT(fs));
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button), 
		"clicked",
		GTK_SIGNAL_FUNC(fs_destroy), GTK_OBJECT(fs));
	gtk_widget_show(fs);
}

static gboolean
on_open (GtkWidget * widget, int *ret)
{
	*ret = 0;
	gtk_main_quit ();
	return TRUE;
}

static gboolean
on_save (GtkWidget * widget, int *ret)
{
	*ret = 1;
	gtk_main_quit ();
	return TRUE;
}

void
show_download_dialog(
	GtkWidget		*widget,
	char			*filename,
	LargeByteString	*binary)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *open;
	GtkWidget *save;
	GtkWidget *cancel;
	GtkWindow *parent;
	int	response = -1;
	char buff[SIZE_LONGNAME];
	char hbytes[64];
	
	if ((parent = (GtkWindow *)g_list_nth_data(DialogStack,g_list_length(DialogStack)-1)) == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}

	dialog = gtk_dialog_new();
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

	get_human_bytes(LBS_Size(binary), hbytes);
	
	snprintf(buff,sizeof(buff),_("Do you open this file or save it?\n\n"
		"File Name:%s\n"
		"Size:%s"
		),filename,hbytes);
	label = gtk_label_new (buff);
	gtk_misc_set_padding (GTK_MISC (label), 20, 20);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		TRUE, TRUE, 0);
	gtk_widget_show (label);

	open = gtk_button_new_with_label (_("Open with App"));
	gtk_widget_set_usize (open, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		open,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (open), "clicked",
		GTK_SIGNAL_FUNC (on_open),
		&response);
	gtk_widget_show (open);
	if (!CheckDesktop(filename)) {
		gtk_widget_set_sensitive(open,FALSE);
	}

	save = gtk_button_new_with_label (_("Save"));
	gtk_widget_set_usize (save, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		save,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (save), "clicked",
		GTK_SIGNAL_FUNC (on_save),
		&response);
	gtk_widget_show (save);

	cancel = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_set_usize (cancel, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		cancel,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (cancel), "clicked",
		GTK_SIGNAL_FUNC (gtk_main_quit),
		NULL);
	gtk_widget_show (cancel);

	gtk_widget_show (dialog);
	gtk_main();

	if (response == 0) {
		OpenDesktop(filename,binary);
	} else if (response == 1) {
		show_save_dialog(widget,filename,binary);
	}
	gtk_widget_destroy(dialog);
}
