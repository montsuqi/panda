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
#include "bootdialog.h"
#include "action.h"
#include "dialogs.h"
#include "logviewer.h"

/*********************************************************************
 * EditDialog
 ********************************************************************/
static void edit_dialog_run(GtkWidget *parent, int n) {
  BDComponent *component;
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  gchar *title;
  const gchar *desc;
  gboolean new;

  if (n == -1) {
    title = _("New");
    n = gl_config_add_server();
    new = TRUE;
  } else {
    title = _("Edit");
    new = FALSE;
  }

  dialog = gtk_dialog_new_with_buttons(
      title, GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_STOCK_OK,
      GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "edit", "glclient");
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

  /* contents */
  table = gtk_table_new(2, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  label = gtk_label_new(_("ConfigName"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  entry = gtk_entry_new();

  desc = gl_config_get_string(n, "description");
  gtk_entry_set_text(GTK_ENTRY(entry), desc);

  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL,
                   GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
                   GTK_FILL, 0, 0);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     table, TRUE, TRUE, 0);
  gtk_widget_show_all(table);

  component = bd_component_new();
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     component->basictable, TRUE, TRUE, 0);
  gtk_widget_show_all(component->basictable);

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     component->ssltable, TRUE, TRUE, 0);
  gtk_widget_show_all(component->ssltable);

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     component->othertable, TRUE, TRUE, 0);
  gtk_widget_show_all(component->othertable);

  bd_component_set_value(component, n);

  /* run */
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    gl_config_set_string(n, "description",
                         gtk_entry_get_text(GTK_ENTRY(entry)));
    bd_component_value_to_config(component, n);
  } else {
    if (new) {
      gl_config_del_server(n);
    }
  }
  gtk_widget_destroy(dialog);
}

/*********************************************************************
 * ServerDialog
 ********************************************************************/

typedef struct _ServerDialog ServerDialog;
struct _ServerDialog {
  GtkWidget *dialog;
  GtkWidget *list;
  GtkWidget *edit;
  GtkWidget *delete;

  gint is_update;
};

static void server_dialog_server_list_update(ServerDialog *self) {
  GtkListStore *model;
  GtkTreeIter iter;
  int i;

  g_return_if_fail(self != NULL);

  model = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(self->list));
  gtk_list_store_clear(model);

  for (i = 0; i < gl_config_get_config_nums(); i++) {
    if (gl_config_have_config(i)) {
      gtk_list_store_append(model, &iter);
      gtk_list_store_set(model, &iter, 0,
                         gl_config_get_string(i, "description"), -1);
      gtk_list_store_set(model, &iter, 1, gl_config_get_string(i, "authuri"),
                         -1);
      gtk_list_store_set(model, &iter, 2, gl_config_get_string(i, "user"), -1);
      gtk_list_store_set(model, &iter, 3, i, -1);
    }
  }
}

static gboolean server_dialog_on_delete_event(GtkWidget *widget,
                                              ServerDialog *self) {
  gtk_main_quit();
  return TRUE;
}

static void server_dialog_on_new(GtkWidget *widget, ServerDialog *self) {
  edit_dialog_run(self->dialog, -1);
  server_dialog_server_list_update(self);
}

static void server_dialog_on_edit(GtkWidget *widget, ServerDialog *self) {
  GtkTreeModel *model;
  GtkTreeIter iter;
  int n;

  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->list));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->list));

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 3, &n, -1);
    edit_dialog_run(self->dialog, n);
    server_dialog_server_list_update(self);
  }
}

static void server_dialog_on_delete(GtkWidget *widget, ServerDialog *self) {
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  int n;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->list));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->list));

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 3, &n, -1);
    gl_config_del_server(n);
    server_dialog_server_list_update(self);
  }
}

static void server_dialog_on_close(GtkWidget *widget, ServerDialog *self) {
  gl_config_save();
  gtk_main_quit();
}

static ServerDialog *server_dialog_new(GtkWidget *parent) {
  ServerDialog *self;
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *scroll;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkCellRenderer *renderer;

  self = g_new0(ServerDialog, 1);

  self->is_update = FALSE;

  self->dialog = dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), _("Server Setting"));
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "config", "glclient");
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
                   G_CALLBACK(server_dialog_on_delete_event), self);

  /* new button */
  button = gtk_button_new_with_label(_("New"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(server_dialog_on_new), self);
  gtk_widget_set_can_default(button, TRUE);

  /* edit button */
  self->edit = button = gtk_button_new_with_label(_("Edit"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(server_dialog_on_edit), self);
  gtk_widget_set_can_default(button, TRUE);

  /* delete button */
  self->delete = button = gtk_button_new_with_label(_("Delete"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(server_dialog_on_delete), self);
  gtk_widget_set_can_default(button, TRUE);

  /* close button */
  button = gtk_button_new_with_label(_("Close"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(server_dialog_on_close), self);
  gtk_widget_set_can_default(button, TRUE);
  gtk_widget_grab_default(button);

  /* contents */
  scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scroll, 450, 200);
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

  return self;
}

static void server_dialog_free(ServerDialog *self) { g_free(self); }

static gboolean server_dialog_run(GtkWidget *parent) {
  ServerDialog *self;
  gboolean is_update;

  self = server_dialog_new(parent);
  gtk_widget_show_all(self->dialog);
  gtk_widget_grab_focus(self->dialog);
  gtk_window_set_modal(GTK_WINDOW(self->dialog), TRUE);
  gtk_main();
  gtk_window_set_modal(GTK_WINDOW(self->dialog), FALSE);
  gtk_widget_destroy(self->dialog);

  is_update = self->is_update;

  server_dialog_free(self);

  return is_update;
}

/*********************************************************************
 * BootDialog
 ********************************************************************/

typedef struct _BootDialog BootDialog;

struct _BootDialog {
  GtkWidget *dialog;

  GtkWidget *welcome;
  GtkWidget *combo;
  BDComponent *component;
  gboolean is_connect;
};

static void boot_dialog_combo_changed(GtkComboBox *widget, gpointer user_data) {
  BootDialog *self;
  GtkTreeModel *model;
  GtkTreeIter iter;
  int n;

  self = (BootDialog *)user_data;
  if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(self->combo), &iter)) {
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(self->combo));
    gtk_tree_model_get(model, &iter, 1, &n, -1);
    bd_component_set_value(self->component, n);
  }
}

static void boot_dialog_combo_update(BootDialog *self) {
  GtkListStore *model;
  GtkComboBox *combo;
  GtkTreeIter iter;
  int i, index;

  combo = GTK_COMBO_BOX(self->combo);

  g_signal_handlers_block_by_func(combo, boot_dialog_combo_changed,
                                  self->combo);

  model = (GtkListStore *)gtk_combo_box_get_model(combo);
  gtk_list_store_clear(model);

  index = gl_config_get_index();
  for (i = 0; i < gl_config_get_config_nums(); i++) {
    if (gl_config_have_config(i)) {
      gtk_list_store_append(model, &iter);
      gtk_list_store_set(model, &iter, 0,
                         gl_config_get_string(i, "description"), 1, i, -1);
      if (i == index) {
        gtk_combo_box_set_active_iter(combo, &iter);
      }
    }
  }
  bd_component_set_value(self->component, index);
  g_signal_handlers_unblock_by_func(self->combo, boot_dialog_combo_changed,
                                    self->combo);
}

static void boot_dialog_on_connect(GtkWidget *connect, BootDialog *self) {
  GtkComboBox *combo;
  GtkTreeIter iter;
  GtkTreeModel *model;
  int n;

  self->is_connect = TRUE;

  combo = GTK_COMBO_BOX(self->combo);

  if (gtk_combo_box_get_active_iter(combo, &iter)) {
    model = gtk_combo_box_get_model(combo);
    gtk_tree_model_get(model, &iter, 1, &n, -1);
    gl_config_set_index(n);
    bd_component_value_to_config(self->component, n);
    gl_config_save();
  }
  gtk_widget_hide(self->dialog);
  gtk_main_quit();
}

static void boot_dialog_on_close(GtkWidget *close, BootDialog *self) {
  self->is_connect = FALSE;
  gtk_widget_hide(self->dialog);
  gtk_main_quit();
}

static void boot_dialog_on_logviewer(GtkWidget *close, BootDialog *self) {
  LogViewerRun();
}

static void boot_dialog_on_config(GtkWidget *widget, BootDialog *self) {
  server_dialog_run(self->dialog);
  boot_dialog_combo_update(self);
#if 0
  gdk_window_raise (self->dialog->window);
#endif
}

static gboolean boot_dialog_on_delete_event(GtkWidget *widget, GdkEvent *event,
                                            BootDialog *self) {
  self->is_connect = FALSE;
  gtk_widget_hide(self->dialog);
  gtk_main_quit();
  return TRUE;
}

static BootDialog *boot_dialog_new() {
  BootDialog *self;
  GtkWidget *dialog, *vbox, *welcome, *notebook, *action_area, *button;
  GtkWidget *label, *hbox, *combo;
  GtkTreeModel *model;
  GtkCellRenderer *renderer;

  self = g_new0(BootDialog, 1);
  self->is_connect = FALSE;
  self->component = bd_component_new();

  dialog = gtk_dialog_new();
  self->dialog = dialog;
  gtk_window_set_title(GTK_WINDOW(dialog), _("glclient2 Launcher"));
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "launcher", "glclient");
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
                   G_CALLBACK(boot_dialog_on_delete_event), self);

  vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  welcome = gtk_label_new(_("glclient2 Launcher"));
  self->welcome = welcome;
  gtk_misc_set_alignment(GTK_MISC(welcome), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(vbox), welcome, TRUE, TRUE, 5);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new(_("Server"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

  model = GTK_TREE_MODEL(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT));
  self->combo = combo = gtk_combo_box_new_with_model(model);
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0,
                                 NULL);

  gtk_widget_set_size_request(combo, 0, 30);
  boot_dialog_combo_update(self);
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
  g_signal_connect(combo, "changed", G_CALLBACK(boot_dialog_combo_changed),
                   self);

  notebook = gtk_notebook_new();
/* for gtk3 */
#if 0 
  gtk_notebook_set_homogeneous_tabs (GTK_NOTEBOOK (notebook), TRUE);
#endif
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 5);

  /* Basic tab */
  label = gtk_label_new(_("Basic"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), self->component->basictable,
                           label);

#ifdef USE_SSL
  /* ssl tab */
  label = gtk_label_new(_("SSL"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), self->component->ssltable,
                           label);
#endif

  /* printer tab */
  label = gtk_label_new(_("Printer"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           self->component->printertable, label);

  /* Other tab */
  label = gtk_label_new(_("Details"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), self->component->othertable,
                           label);

  action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

  button = gtk_button_new_with_label(_("Connect"));
  gtk_box_pack_start(GTK_BOX(action_area), button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(boot_dialog_on_connect), self);
  gtk_widget_set_can_default(button, TRUE);
  gtk_widget_set_can_focus(button, TRUE);
  gtk_widget_grab_default(button);
  gtk_widget_grab_focus(button);

  button = gtk_button_new_with_label(_("Close"));
  gtk_box_pack_start(GTK_BOX(action_area), button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(boot_dialog_on_close), self);
  gtk_widget_set_can_default(button, TRUE);

  button = gtk_button_new_with_label(_("Configuration"));
  gtk_box_pack_start(GTK_BOX(action_area), button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(boot_dialog_on_config), self);
  gtk_widget_set_can_default(button, TRUE);

  button = gtk_button_new_with_label(_("LogViewer"));
  gtk_box_pack_start(GTK_BOX(action_area), button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(boot_dialog_on_logviewer), self);
  gtk_widget_set_can_default(button, TRUE);

  /* INFO tab */
  label = gtk_label_new(_("INFOMATION"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), self->component->infotable,
                           label);

  return self;
}

static void boot_dialog_free(BootDialog *self) { g_free(self); }

/*
  return: TRUE -> connect, FALSE -> close
 */
void BootDialogRun() {
  BootDialog *self;

  self = boot_dialog_new();
  gtk_widget_show_all(self->dialog);
  // gtk_widget_hide(self->component->protocol_v2);

  gtk_widget_grab_focus(self->dialog);

  gtk_window_set_modal(GTK_WINDOW(self->dialog), TRUE);
  gtk_main();
  gtk_window_set_modal(GTK_WINDOW(self->dialog), FALSE);

  gtk_widget_destroy(self->dialog);

  if (!self->is_connect) {
    exit(0);
  }

  boot_dialog_free(self);
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
