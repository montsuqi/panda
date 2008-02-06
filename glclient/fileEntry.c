/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Noboru Saitou
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
#include	<errno.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#ifdef USE_GNOME
#   include	<gnome.h>
#else
#	include	<gtk/gtk.h>
#	include	"gettext.h"
#endif

#include	"comm.h"
#include	"dialogs.h"
#include	"fileEntry.h"

static Bool
CheckAlreadyFile(
	char *filename)
{
	struct stat stbuf;
	uid_t euid;
	gid_t egid;
	gboolean rc = TRUE;

	euid = geteuid ();
	egid = getegid ();

    /* already file */
	if	(stat(filename, &stbuf) == 0 ){  
		rc = FALSE;
		/* Exception: not writable no error */	
		if (stbuf.st_uid == euid) {
			if (!(stbuf.st_mode & S_IWUSR)) {
				rc = TRUE;
			}
		} else if (stbuf.st_gid == egid) {
			if (!(stbuf.st_mode & S_IWGRP)) {
				rc = TRUE;
			}
		} else {
			if (!(stbuf.st_mode & S_IWOTH)) {
				rc = TRUE;
			}
		}
	}
	return (rc);
}

static Bool
SaveFile(
	char *filename,
	LargeByteString *binary,
	GtkWidget *widget)
{
	GtkWidget *dialog;
	gboolean rc;
    int		fd;
	char	buf[SIZE_BUFF];

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
	if (rc){
		snprintf(buf, sizeof(buf), _("Succeeded in writing %s"), filename);
		dialog = message_dialog(buf, rc);
		gtk_widget_destroy (GTK_WIDGET (gtk_widget_get_toplevel(widget)));
	} else {
		snprintf(buf, sizeof(buf), _("cannot write %s \n%s"), 
				 filename, strerror(errno));
		dialog = message_dialog(buf, rc);
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
							  GTK_WINDOW (gtk_widget_get_toplevel(widget)));
	}
	gtk_window_set_modal (GTK_WINDOW(dialog), TRUE);
	gtk_widget_show (dialog);
	return (rc);
}

#ifdef USE_GNOME
static void
question_clicked(
	gint reply,
	gpointer data)
{
	GtkFileSelection *fsw;
	GnomeFileEntry *fentry;
	GtkWidget *entry;
    LargeByteString *binary;
	char	*filename;

	if (reply == GNOME_YES) {
		fsw = GTK_FILE_SELECTION (data);
		fentry = GNOME_FILE_ENTRY (gtk_object_get_user_data (GTK_OBJECT (fsw)));
		entry = gnome_file_entry_gtk_entry (fentry);

		filename = (char *)gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(fentry), FALSE);
		binary = gtk_object_get_data(GTK_OBJECT(fentry), "recvobject");
		SaveFile(filename, binary, GTK_WIDGET (fsw));
	}
}


static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GtkFileSelection *fs;
	GnomeFileEntry *fentry;
	GtkWidget *entry;
    LargeByteString *binary;
	char	*filename;

	gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"clicked");

	fs = GTK_FILE_SELECTION (data);
	fentry = GNOME_FILE_ENTRY (gtk_object_get_user_data (GTK_OBJECT (fs)));
	entry = gnome_file_entry_gtk_entry (fentry);
	gtk_entry_set_text (GTK_ENTRY (entry),
			    gtk_file_selection_get_filename (fs));
	/* Is this evil? */
	gtk_signal_emit_by_name (GTK_OBJECT (entry), "activate");

	filename = (char *)gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(fentry), FALSE);
	binary = gtk_object_get_data(GTK_OBJECT(fentry), "recvobject");

	if ((binary) && (LBS_Size(binary) > 0)){
		if (CheckAlreadyFile(filename)){
			if (SaveFile(filename, binary, widget)){
			}
		} else {
			dialog = question_dialog(
				_("File already exists. Rewrite?"),
				(GtkSignalFunc) question_clicked, GTK_WIDGET(fs),
				GTK_WINDOW(fs));
		}
	} else {
		/* do nothing */
		gtk_widget_destroy (GTK_WIDGET (fs));
	}
}

static void
browse_dialog_destroy (GtkWidget *widget, gpointer data)
{
    LargeByteString *binary;

	binary = gtk_object_get_data(GTK_OBJECT(widget), "recvobject");
	FreeLBS(binary);
}

extern void
browse_clicked(GnomeFileEntry *fentry, gpointer data)
{
	GtkFileSelection *fsw;

	if(!fentry->fsw)
		return;
	fsw = GTK_FILE_SELECTION(fentry->fsw);
	/* rebind browse_dialog_ok */
	gtk_signal_handlers_destroy(GTK_OBJECT (fsw->ok_button));
	gtk_signal_connect (GTK_OBJECT (fsw->ok_button), "clicked",
					GTK_SIGNAL_FUNC(browse_dialog_ok),
					GTK_OBJECT(fsw));
	gtk_signal_connect(GTK_OBJECT(fsw), "destroy",
				   GTK_SIGNAL_FUNC(browse_dialog_destroy), NULL);
}
#endif

