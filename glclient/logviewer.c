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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h> /* strlen */
#include <ctype.h>  /* isblank */
#include <glib.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>

#include "gettext.h"
#include "glclient.h"
#include "const.h"
#include "bd_config.h"
#include "bd_component.h"
#include "logviewer.h"
#include "action.h"
#include "dialogs.h"
#include "logger.h"

static gboolean on_delete(GtkWidget *widget,void *data) {
  gtk_main_quit();
  return TRUE;
}

static void LogView(const char *fname) {
  GtkWidget *dialog, *button, *scroll;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), _("LogViewer"));
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
                   G_CALLBACK(on_delete), NULL);

  /* csv out button */
  button = gtk_button_new_with_label(_("CSV Out"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(on_csv), self);
  gtk_widget_set_can_default(button, TRUE);

  /* close button */
  button = gtk_button_new_with_label(_("Close"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(on_delete), NULL);
  gtk_widget_set_can_default(button, TRUE);
  gtk_widget_grab_default(button);

  /* contents */
  scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scroll, 850, 750);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     scroll, TRUE, TRUE, 5);

  model = (GtkTreeModel *)gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_INT);
  self->list = gtk_tree_view_new_with_model(model);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->list));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(self->list), -1,
                                              N_("Description"), renderer,
                                              "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(self->list), -1, N_("AuthURI"), renderer, "text", 1, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(self->list), -1, N_("User"), renderer, "text", 2, NULL);

  gtk_container_add(GTK_CONTAINER(scroll), self->list);

  server_dialog_server_list_update(self);
}

void LogViewerRun() {
  GtkWidget *fcd;
  char *fname;

  fcd = gtk_file_chooser_dialog_new(
      _("Specify filename..."), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
      GTK_RESPONSE_ACCEPT, NULL);
  gtk_window_set_keep_above(GTK_WINDOW(fcd), TRUE);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fcd), GetLogDir());

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    LogView(fname);
    g_free(fname);
  }
  gtk_widget_destroy(GTK_WIDGET(fcd));
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
