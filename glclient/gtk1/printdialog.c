/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2010 JMA (Japan Medical Association).
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
#include	<gnome.h>
#include	<unistd.h>
#include	<libmondai.h>
#include	<gtkpanda/gtkpanda.h>

#include	"glclient.h"
#include	"dialogs.h"
#include	"widgetcache.h"
#include	"printdialog.h"
#include	"utils.h"
#include	"toplevel.h"
#include	"message.h"
#include	"debug.h"

enum {
PREVIEW_CLICKED,
SAVE_CLICKED,
PRINT_CLICKED
};

static gboolean
preview_clicked_cb (GtkWidget * widget, int *ret)
{
	*ret = PREVIEW_CLICKED;
	gtk_main_quit ();
	return TRUE;
}

static gboolean
save_clicked_cb (GtkWidget * widget, int *ret)
{
	*ret = SAVE_CLICKED;
	gtk_main_quit ();
	return TRUE;
}

static gboolean
print_clicked_cb (GtkWidget * widget, int *ret)
{
	*ret = PRINT_CLICKED;
	gtk_main_quit ();
	return TRUE;
}

static void
preview_dialog(char *fname)
{
	GtkWidget *dialog;
	GtkWindow *parent;
	GtkWidget *close;
    GtkWidget *pandaps;

	if ((parent = (GtkWindow *)g_list_nth_data(DialogStack,g_list_length(DialogStack)-1)) == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}

	dialog = gtk_dialog_new();
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

	pandaps = gtk_panda_ps_new();
	gtk_panda_ps_load(GTK_PANDA_PS(pandaps),fname);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), pandaps,
		TRUE, TRUE, 0);
	gtk_widget_set_usize(pandaps,800,600);
	gtk_widget_show (pandaps);

	close = gtk_button_new_with_label(_("Close"));
	gtk_widget_set_usize (close, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		close,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (close), "clicked",
		GTK_SIGNAL_FUNC (gtk_main_quit),
		NULL);
	GTK_WIDGET_SET_FLAGS (close, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(close);
	gtk_widget_show (close);
	gtk_widget_show (pandaps);
	gtk_widget_show (dialog);
	gtk_main();

	gtk_widget_destroy(dialog);
}

static gboolean
SaveFile(
	GtkFileSelection *fs)
{
	char *newpath;
	GtkWidget *dialog;
	gboolean rc;
	char *oldpath;
	char buf[256];

	newpath = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	oldpath = (char *)gtk_object_get_data(GTK_OBJECT(fs), "oldpath");
	if(!rename(oldpath,newpath)) {
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	if (!rc){
		snprintf(buf, sizeof(buf), _("Can't save to %s \n%s"), 
			newpath, strerror(errno));
		dialog = message_dialog(buf, rc);
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
							  GTK_WINDOW (fs));
		gtk_window_set_modal (GTK_WINDOW(dialog), TRUE);
		gtk_widget_show (dialog);
	} else {
		SetWidgetCache("printdialog", g_dirname(newpath));
	}
	return (rc);
}

static gboolean
rewrite_yes_cb(
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
filesel_ok_cb (GtkWidget * widget, GtkFileSelection *fs)
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
			(GtkSignalFunc) rewrite_yes_cb, GTK_WIDGET(fs),
			GTK_WINDOW(fs));
	}
	return FALSE;
}

static void
filesel_cancel_cb(GtkWidget *widget,gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void
save_dialog(char *oldpath)
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
	dirname = GetWidgetCache("printdialog");
	if (dirname != NULL) {
		snprintf(path,sizeof(path),"%s/%s",dirname,"new.ps");
	} else {
		snprintf(path,sizeof(path),"%s","new.ps");
	}
	gtk_object_set_data(GTK_OBJECT(fs),"oldpath",oldpath);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs),path);

	gtk_signal_connect(GTK_OBJECT(fs), "destroy",
		GTK_SIGNAL_FUNC(filesel_cancel_cb), fs);
	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), 
		"clicked",
		GTK_SIGNAL_FUNC(filesel_ok_cb),
		GTK_OBJECT(fs));
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button), 
		"clicked",
		GTK_SIGNAL_FUNC(filesel_cancel_cb), GTK_OBJECT(fs));
	gtk_widget_show(fs);
}

static gboolean
check_printer(void)
{
	char buff[256], cmd[256];
	char *p;
	FILE *fp;
	gboolean ret = FALSE;
	
	snprintf(cmd, sizeof(cmd), "%s -a", LPSTAT_PATH);
	if ((fp = popen(cmd, "r")) != NULL) {
	  while(1) {
	    fgets(buff, sizeof(cmd), fp);
	    if (feof(fp))break;
	    p = strchr(buff, ' ');
	    if (p != NULL) *p = '\0';
		ret = TRUE;
	  }
	  pclose(fp);
	}
	return ret;
}

static void
append_printer_list(GtkWidget *combo)
{
	GList *items = NULL;
	char buff[256], cmd[256];
	char *p;
	FILE *fp;
	int i;
	
	snprintf(cmd, sizeof(cmd), "%s -a", LPSTAT_PATH);
	if ((fp = popen(cmd, "r")) != NULL) {
	  while(1) {
	    fgets(buff, sizeof(cmd), fp);
	    if (feof(fp))break;
	    p = strchr(buff, ' ');
	    if (p != NULL) *p = '\0';
	    items = g_list_append(items, strdup(buff));
	  }
	  pclose(fp);
	}
	if (items == NULL) {
		items = g_list_append(items, strdup(" "));
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), items);
	for(i = 0; i < g_list_length(items); i++) {
		free(g_list_nth_data(items,i));
	}
	g_list_free(items);
}

void
print_ok_cb (GtkWidget *widget, GtkCombo *combo)
{
	int pid;
	char *argv[5];
	char *printer;
	char *filename;

	printer = gtk_entry_get_text(GTK_ENTRY(combo->entry));
	if (printer == NULL || !strcmp(printer," ")) {
		gtk_main_quit();
		return;
	}
	
	filename = (char*)gtk_object_get_data(GTK_OBJECT(combo),"filename");
	if (filename != NULL) {
		signal  (SIGCHLD, SIG_IGN);
		argv[0] = LPR_PATH;
		argv[1] = "-P";
		argv[2] = gtk_entry_get_text(
			GTK_ENTRY(combo->entry));
		argv[3] = filename;
		argv[4] = (char *)NULL;
		if ((pid = fork()) == 0) {
			execv(LPR_PATH, argv);
			_exit(0);
		}
	}
	gtk_main_quit();
}

static void
select_printer_dialog(char *fname)
{
	GtkWidget *dialog;
	GtkWidget *combo;
	GtkWidget *label;
	GtkWidget *ok;
	GtkWidget *cancel;
	GtkWindow *parent;

	if ((parent = (GtkWindow *)g_list_nth_data(DialogStack,
		g_list_length(DialogStack)-1)) == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}

	dialog = gtk_dialog_new();
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_set_usize (dialog, 200, 100);

	label = gtk_label_new(_("Select the printer"));
	gtk_widget_set_usize(label,100,20);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		FALSE, FALSE, 0);

	combo = gtk_combo_new();
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry),FALSE);
	gtk_widget_set_usize (combo, 80, 20);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), combo,
		FALSE, FALSE, 0);
	gtk_object_set_data(GTK_OBJECT(combo),"filename",fname);
	append_printer_list(combo);

	ok = gtk_button_new_with_label(_("OK"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		ok,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		GTK_SIGNAL_FUNC(print_ok_cb),combo);
	GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("CANCEL"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		cancel,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		GTK_SIGNAL_FUNC(gtk_main_quit),NULL);

	gtk_widget_show(label);
	gtk_widget_show(combo);
	gtk_widget_show(ok);
	gtk_widget_show(cancel);
	gtk_widget_show(dialog);
	gtk_main();

	gtk_widget_destroy(dialog);
}

void
show_print_dialog(char *title,char *fname,size_t size)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *preview;
	GtkWidget *save;
	GtkWidget *print;
	GtkWidget *cancel;
	GtkWidget *pandaps;
	GtkWindow *parent;
	int	response = -1;
	char buf[SIZE_LONGNAME];
	char hbytes[64];

	pandaps = gtk_panda_ps_new();
    if (!gtk_panda_ps_load(GTK_PANDA_PS(pandaps),fname)) {
		gtk_widget_destroy(pandaps);
		return;
	}
	
	if ((parent = (GtkWindow *)g_list_nth_data(DialogStack,
			g_list_length(DialogStack)-1)) == NULL) {
		parent = GTK_WINDOW(TopWindow);
	}

	dialog = gtk_dialog_new();
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

	get_human_bytes(size, hbytes);
	snprintf(buf,sizeof(buf),
	_("There is the print data.\nDo you print the data?\n\n"
		"Title:%s\n"
		"Num of Pages:%d\n"
		"Size:%s"
		),title,gtk_panda_ps_get_page_count(GTK_PANDA_PS(pandaps)),hbytes);
	label = gtk_label_new (buf);
	gtk_misc_set_padding (GTK_MISC (label), 20, 20);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		TRUE, TRUE, 0);
	gtk_widget_show (label);
	gtk_widget_destroy(pandaps);

	preview = gtk_button_new_with_label (_("Preview"));
	gtk_widget_set_usize (preview, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		preview,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (preview), "clicked",
		GTK_SIGNAL_FUNC (preview_clicked_cb),
		&response);
	gtk_widget_show (preview);

	save = gtk_button_new_with_label (_("Save"));
	gtk_widget_set_usize (save, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		save,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (save), "clicked",
		GTK_SIGNAL_FUNC (save_clicked_cb),
		&response);
	gtk_widget_show (save);

	print = gtk_button_new_with_label (_("Print"));
	gtk_widget_set_usize (print, 80, -1);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		print,
		FALSE, FALSE, 14);
	gtk_signal_connect (GTK_OBJECT (print), "clicked",
		GTK_SIGNAL_FUNC (print_clicked_cb),
		&response);
	gtk_widget_show (print);
	if (!check_printer()) {
		gtk_widget_set_sensitive(print,FALSE);
	}

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
	gtk_widget_destroy(dialog);

	switch(response) {
	case PREVIEW_CLICKED:
		preview_dialog(fname);
		break;
	case SAVE_CLICKED:
		save_dialog(fname);
		break;
	case PRINT_CLICKED:
		select_printer_dialog(fname);
		break;
	}
}
