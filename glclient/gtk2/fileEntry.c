/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Noboru Saitou & JMA (Japan Medical Association).
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
#include	<gnome.h>

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
	gboolean rc;
    int		fd;

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
		message_dialog_printf(GTK_MESSAGE_INFO ,_("Succeeded in writing %s"), filename);
	} else {
		message_dialog_printf(GTK_MESSAGE_ERROR, _("cannot write %s \n%s"), strerror(errno));
	}
	return (rc);
}

static void
ask_overwrite(GtkDialog *dialog,
	gint response,
	gpointer data)
{
	gint	*ret;

	ret = (gint *)data;
	*ret = response;
}

static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
	GtkWidget 			*dialog;
	GtkFileSelection 	*fs;
	GnomeFileEntry 		*fentry;
	GtkWidget 			*entry;
    LargeByteString 	*binary;
	char				*filename;
	gint				response;

	fentry = GNOME_FILE_ENTRY(data);
	fs = GTK_FILE_SELECTION(fentry->fsw);

	g_signal_stop_emission_by_name(G_OBJECT(widget),"clicked");

	entry = gnome_file_entry_gtk_entry (fentry);
	gtk_entry_set_text (GTK_ENTRY (entry), 
		gtk_file_selection_get_filename (fs));
	/* Is this evil? */
	g_signal_emit_by_name(G_OBJECT(entry), "activate");

	filename = (char *)gnome_file_entry_get_full_path(fentry, FALSE);
	binary = g_object_get_data(G_OBJECT(fentry), "recvobject");

	if ((binary) && (LBS_Size(binary) > 0)){
		if (CheckAlreadyFile(filename)){
			if (SaveFile(filename, binary, widget)){
			}
		} else {
			dialog = gtk_message_dialog_new (
				GTK_WINDOW (gtk_widget_get_toplevel(widget)),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				"%s", _("File already exists. Rewrite?"));
			g_signal_connect(dialog, "response", 
				G_CALLBACK (ask_overwrite), &response);
			gtk_dialog_run(GTK_DIALOG (dialog));
			if (response == GTK_RESPONSE_YES) {
				SaveFile(filename, binary, widget);
			}
			gtk_widget_destroy (dialog);
		}
		gtk_widget_destroy (GTK_WIDGET (gtk_widget_get_toplevel(widget)));
	}
}

static void
browse_dialog_destroy (GtkWidget *widget, gpointer data)
{
    LargeByteString *binary;

	g_signal_stop_emission_by_name(G_OBJECT(widget),"clicked");
	binary = g_object_get_data(G_OBJECT(widget), "recvobject");
	FreeLBS(binary);
	gtk_widget_destroy (GTK_WIDGET (gtk_widget_get_toplevel(widget)));
}

extern void
browse_clicked(GnomeFileEntry *fentry, gpointer data)
{
	GtkFileSelection *fsw;

	if (!fentry->fsw) {
		return;
	}
	fsw = GTK_FILE_SELECTION(fentry->fsw);


	/* rebind browse_dialog_ok */
    g_signal_handlers_destroy(G_OBJECT (fsw->ok_button));
    g_signal_handlers_destroy(G_OBJECT (fsw->cancel_button));

    g_signal_connect(fsw->ok_button, "clicked",
       G_CALLBACK (browse_dialog_ok), fentry);
    g_signal_connect(fsw->cancel_button, "clicked",
       G_CALLBACK (browse_dialog_destroy), fentry);
}
