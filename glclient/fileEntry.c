/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2005 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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
#include	<unistd.h>
#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif

#include "comm.h"
#include "dialogs.h"
#include "fileEntry.h"

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
    FILE *file;
	char	buf[SIZE_BUFF];

	if	((file = fopen(filename,"wb")) != NULL) {
		fchmod(fileno(file), 0600);
		fwrite(LBS_Body(binary), sizeof(byte), LBS_Size(binary), file);
		fclose(file);
		rc = TRUE;
		snprintf(buf, sizeof(buf), "%s 書き込み終了", filename);
		dialog = message_dialog(buf, rc);
		gtk_widget_destroy (GTK_WIDGET (gtk_widget_get_toplevel(widget)));
	} else {
		rc = FALSE;
		snprintf(buf, sizeof(buf), "%s ファイルに書き込めませんでした\n %s", filename, strerror(errno));
		dialog = message_dialog(buf, rc);
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
							GTK_WINDOW (gtk_widget_get_toplevel(widget)));
	}
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
			dialog = gnome_question_dialog_parented(
			"ファイルが既に存在します。書き換えますか？",
			GTK_SIGNAL_FUNC(question_clicked), GTK_WIDGET(fs), GTK_WINDOW(fs));
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

