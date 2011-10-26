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
#  include "config.h"
#endif

#include    <stdio.h>
#include    <string.h> /* strlen */
#include    <ctype.h> /* isblank */
#include    <glib.h>
#include    <gtk/gtk.h>
#include    "gettext.h"
#include	<gtkpanda/gtkpanda.h>
#include	"glclient.h"
#include    "port.h"
#include    "const.h"
#include    "bd_config.h"
#include    "bd_component.h"
#include    "bootdialog.h"
#include    "dialogs.h"
#include    "message.h"
#include    "debug.h"

/*********************************************************************
 * EditDialog
 ********************************************************************/
static void
edit_dialog_run(gchar *name, GtkWidget *parent)
{
  BDComponent *component;
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  gchar *title;
  gchar *newname;
  gboolean is_new;

  if (name == NULL) {
    title = _("New");
    name = "new";
    is_new = TRUE;
  } else {
    is_new = FALSE;
    title = _("Edit");
  }

  dialog = gtk_dialog_new_with_buttons (title,
    GTK_WINDOW(parent),
    GTK_DIALOG_MODAL,
    GTK_STOCK_OK,
    GTK_RESPONSE_OK,
    GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL,
    NULL);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "edit", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  /* contents */
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  label = gtk_label_new (_("ConfigName"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY(entry),name);

  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start(
    GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 
    table, TRUE, TRUE, 0);
  gtk_widget_show_all(table);

  component = bd_component_new();
  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 
    component->basictable, TRUE, TRUE, 0);
  gtk_widget_show_all(component->basictable);
  //gtk_widget_hide(component->protocol_v2);

#ifdef USE_SSL
  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 
    component->ssltable, TRUE, TRUE, 0);
  gtk_widget_show_all(component->ssltable);
#endif

  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 
    component->othertable, TRUE, TRUE, 0);
  gtk_widget_show_all(component->othertable);

  gl_config_set_config_name(name);
  bd_component_set_value(component);

  /* run */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
    newname = (gchar *)gtk_entry_get_text(GTK_ENTRY(entry));
    gl_config_set_config_name(newname);
    bd_component_value_to_config(component);
    if (!is_new && g_strcmp0(name,newname)) {
      /* rename */
      gl_config_remove_config(name);
    }
  } else {
  }
  gtk_widget_destroy (dialog);
}

/*********************************************************************
 * ServerDialog
 ********************************************************************/

typedef struct _ServerDialog ServerDialog;
struct _ServerDialog {
  GtkWidget *dialog;
  GtkWidget *server_list;
  GtkWidget *edit;
  GtkWidget *delete;

  gint is_update;
};

static struct {
  gchar *title;
  gchar *name;
} server_dialog_titles[] = {
  { N_("ConfigName"), "configname" },
  { N_("Host"),           "host" },
  { N_("Port"),           "port" },
  { N_("Application"), "application"},
  { N_("User"),           "user" },
};

static gint server_dialog_titles_count =
  sizeof (server_dialog_titles) / sizeof (server_dialog_titles[0]);

static void
server_dialog_server_list_update (ServerDialog * self)
{
  gchar *text_list[server_dialog_titles_count + 1];
  GList *p;
  gchar *confname;
  gint i;
  
  g_return_if_fail (self != NULL);

  gtk_panda_clist_clear (GTK_PANDA_CLIST (self->server_list));

  text_list[server_dialog_titles_count] = NULL;
  for (p = gl_config_list_config(); 
    p != NULL; p = g_list_next (p)) {
    confname = (gchar *) p->data;
    if (!g_strcmp0(confname, "_default")) {
      continue;
    }
    gl_config_set_config_name(confname);
    text_list[0] = confname;
    for (i = 1; i < server_dialog_titles_count; i++) {
      text_list[i] = gl_config_get_string(server_dialog_titles[i].name);
    }
    gtk_panda_clist_append (GTK_PANDA_CLIST(self->server_list), text_list);
  }
}

static gboolean
server_dialog_on_delete_event (GtkWidget * widget, ServerDialog * self)
{
  gtk_main_quit ();
  return TRUE;
}

static void
server_dialog_on_new (GtkWidget * widget, ServerDialog * self)
{
  edit_dialog_run (NULL, self->dialog);
  server_dialog_server_list_update (self);
#if 0
  gdk_window_raise (self->dialog->window);
#endif
}

static void
server_dialog_on_edit (GtkWidget * widget, ServerDialog * self)
{
  int i;
  gchar *confname;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;

  for (
    i = 0; 
    i < gtk_panda_clist_get_n_rows(GTK_PANDA_CLIST(self->server_list));
    i++) {
    if (gtk_panda_clist_row_is_selected(GTK_PANDA_CLIST(self->server_list),i)){
      path = gtk_tree_path_new_from_indices(i , -1);
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->server_list));
      if (gtk_tree_model_get_iter(model, &iter, path)) {
         gtk_tree_model_get(model, &iter, 0, &confname,-1);
         edit_dialog_run (confname, self->dialog);
         server_dialog_server_list_update(self);
      }
      gtk_tree_path_free(path);
      break;
    }
  }
#if 0
  gdk_window_raise (self->dialog->window);
#endif
}

static void
server_dialog_on_delete (GtkWidget * widget, ServerDialog * self)
{
  int i;
  gchar *confname;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;

  for (
    i = 0; 
    i < gtk_panda_clist_get_n_rows(GTK_PANDA_CLIST(self->server_list));
    i++) {
    if (gtk_panda_clist_row_is_selected(GTK_PANDA_CLIST(self->server_list),i)){
      path = gtk_tree_path_new_from_indices(i , -1);
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->server_list));
      if (gtk_tree_model_get_iter(model, &iter, path)) {
         gtk_tree_model_get(model, &iter, 0, &confname,-1);
         gl_config_remove_config(confname);
         server_dialog_server_list_update (self);
      }
      gtk_tree_path_free(path);
      break;
    }
  }
}

static void
server_dialog_on_close (GtkWidget * widget, ServerDialog * self)
{
  gtk_main_quit ();
}

static void
server_dialog_on_select_row (GtkWidget * widget, 
  gint row, 
  gint column,
  ServerDialog * self)
{
  gtk_widget_set_sensitive (self->edit, TRUE);
  gtk_widget_set_sensitive (self->delete, TRUE);
}

static void
server_dialog_on_unselect_row (GtkWidget * widget, 
  gint row, 
  gint column,
  ServerDialog * self)
{
  gtk_widget_set_sensitive (self->edit, FALSE);
  gtk_widget_set_sensitive (self->delete, FALSE);
}

static ServerDialog *
server_dialog_new (GtkWidget *parent)
{
  ServerDialog *self;
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *scroll;
  GtkWidget *clist;
  GtkTreeViewColumn *column;
  gint i;

  self = g_new0 (ServerDialog, 1);
  
  self->is_update = FALSE;

  self->dialog = dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Server Setting"));
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "config", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  g_signal_connect (G_OBJECT (dialog), "delete_event",
                      G_CALLBACK (server_dialog_on_delete_event), self);

  /* new button */
  button = gtk_button_new_with_label (_("New"));
  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
    button, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (server_dialog_on_new), self);
  gtk_widget_set_can_default(button,TRUE);

  /* edit button */
  self->edit = button = gtk_button_new_with_label (_("Edit"));
  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
    button, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (server_dialog_on_edit), self);
  gtk_widget_set_can_default(button,TRUE);
  gtk_widget_set_sensitive (button, FALSE);

  /* delete button */
  self->delete = button = gtk_button_new_with_label (_("Delete"));
  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
    button, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (server_dialog_on_delete), self);
  gtk_widget_set_can_default(button,TRUE);
  gtk_widget_set_sensitive (button, FALSE);

  /* close button */
  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
    button, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (server_dialog_on_close), self);
  gtk_widget_set_can_default(button,TRUE);
  gtk_widget_grab_default (button);

  /* contents */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scroll, 450, 200);
  gtk_box_pack_start (
    GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
    scroll, TRUE, TRUE, 5);

  self->server_list = clist = gtk_panda_clist_new();
  gtk_panda_clist_set_columns(GTK_PANDA_CLIST(clist),server_dialog_titles_count);
  gtk_panda_clist_set_selection_mode(GTK_PANDA_CLIST(clist),
    GTK_SELECTION_SINGLE);
  for (i = 0; i < server_dialog_titles_count; i++) {
    column = gtk_tree_view_get_column(GTK_TREE_VIEW(clist), i);
    if (column != NULL) {
      gtk_tree_view_column_set_resizable(column, TRUE);
      gtk_tree_view_column_set_title (column, _(server_dialog_titles[i].title));
    }
  }
  gtk_container_add (GTK_CONTAINER (scroll), clist);
  g_signal_connect (G_OBJECT (clist), "select_row",
    G_CALLBACK (server_dialog_on_select_row),
    self);
  g_signal_connect (G_OBJECT (clist), "unselect_row",
    G_CALLBACK (server_dialog_on_unselect_row),
    self);

  server_dialog_server_list_update (self);
    
  return self;
}

static void
server_dialog_free (ServerDialog * self)
{
  g_free (self);
}

static gboolean
server_dialog_run(GtkWidget *parent)
{
  ServerDialog *self;
  gboolean is_update;

  self = server_dialog_new (parent);
  gtk_widget_show_all (self->dialog);
  gtk_widget_grab_focus (self->dialog);
  gtk_window_set_modal (GTK_WINDOW (self->dialog), TRUE);
  gtk_main ();
  gtk_window_set_modal (GTK_WINDOW (self->dialog), FALSE);
  gtk_widget_destroy (self->dialog);

  is_update = self->is_update;
  
  server_dialog_free (self);

  return is_update;
}

/*********************************************************************
 * BootDialog
 ********************************************************************/

typedef struct _BootDialog BootDialog;

struct _BootDialog
{
  GtkWidget *dialog;

  GtkWidget *welcome;
  GtkWidget *combo;
  BDComponent *component;
  gboolean is_connect;
};

static void
boot_dialog_combo_changed(GtkComboBox *widget, gpointer user_data)
{
  BootDialog *self;
  gchar *conf;
  
  self = (BootDialog *)user_data;
#ifdef LIBGTK_3_0_0
  conf = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
#else
  conf = gtk_combo_box_get_active_text(widget);
#endif
  if (conf) {
    gl_config_set_config_name(conf);
    bd_component_set_value(self->component);
    g_free(conf);
  }
}

static void
boot_dialog_combo_update (BootDialog *self)
{
  GList *p;
  gchar *confname;
  gchar *selected;
  gint i;
  gboolean update = FALSE;

  g_signal_handlers_block_by_func(GTK_COMBO_BOX(self->combo),
    boot_dialog_combo_changed, self->combo);

  gl_config_get_config_name();
  selected = ConfigName;

#ifdef LIBGTK_3_0_0
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(self->combo));

  gtk_combo_box_append_text(GTK_COMBO_BOX(self->combo), "default");
  for (i = 0,p = gl_config_list_config(); p != NULL; p = g_list_next (p)) {
      confname = (gchar *)p->data;
      if (!strcmp(confname, "_default")||!strcmp(confname,"default")) {
        continue;
      }
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->combo),confname);
      if (!strcmp(confname, selected)) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(self->combo), i);
        gl_config_set_config_name(confname);
        bd_component_set_value(self->component);
        update = TRUE;
      }
      i++;
    }
  }
#else
  GtkListStore *store;
  store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(self->combo)));

  gtk_list_store_clear(store);
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->combo), "default");
  for (i = 0,p = gl_config_list_config(); p != NULL; p = g_list_next (p)) {
      confname = (gchar *)p->data;
      if (!strcmp(confname, "_default")||!strcmp(confname,"default")) {
        continue;
      } 
      gtk_combo_box_append_text(GTK_COMBO_BOX(self->combo), confname);
      if (!strcmp(confname, selected)) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(self->combo), i+1);
        gl_config_set_config_name(confname);
        bd_component_set_value(self->component);
        update = TRUE;
      }
      i++;
  }
#endif
  if (!update) {
    gl_config_set_config_name("default");
    bd_component_set_value(self->component);
    gtk_combo_box_set_active(GTK_COMBO_BOX(self->combo), 0);
  }
  bd_component_set_value(self->component);
  g_signal_handlers_unblock_by_func(GTK_COMBO_BOX(self->combo),
    boot_dialog_combo_changed, self->combo);
}

static void
boot_dialog_on_connect (GtkWidget *connect, BootDialog *self)
{
  gchar *confname;
  self->is_connect = TRUE;
  
#ifdef LIBGTK_3_0_0
  confname = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self->combo));
#else
  confname = gtk_combo_box_get_active_text(GTK_COMBO_BOX(self->combo));
#endif

  if (confname == NULL || strlen(confname) <= 0) {
    confname = "default";
  }

  gl_config_set_config_name(confname);
  gl_config_save_config_name(confname);
  bd_component_value_to_config(self->component);

  gtk_widget_hide (self->dialog);
  gtk_main_quit ();
}

static void
boot_dialog_on_close (GtkWidget *close, BootDialog *self)
{
  self->is_connect = FALSE;
  gtk_widget_hide (self->dialog);
  gtk_main_quit ();
}

static void
boot_dialog_on_config (GtkWidget * widget, BootDialog * self)
{
  server_dialog_run (self->dialog);
  boot_dialog_combo_update (self);
#if 0
  gdk_window_raise (self->dialog->window);
#endif
}

static gboolean
boot_dialog_on_delete_event (GtkWidget *widget, GdkEvent *event, BootDialog *self)
{
  self->is_connect = FALSE;
  gtk_widget_hide (self->dialog);
  gtk_main_quit ();
  return TRUE;
}

static BootDialog *
boot_dialog_new ()
{
  BootDialog *self;
  GtkWidget *dialog, *vbox, *welcome, *notebook, *action_area, *button;
  GtkWidget *label, *hbox, *combo;

  self = g_new0 (BootDialog, 1);
  self->is_connect = FALSE;
  self->component = bd_component_new();

  dialog = gtk_dialog_new ();
  self->dialog = dialog;
  gtk_window_set_title (GTK_WINDOW (dialog), _("glclient2 Launcher"));
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "lancher", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  g_signal_connect (G_OBJECT (dialog), "delete_event",
                      G_CALLBACK (boot_dialog_on_delete_event), self);
  
  vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  welcome = gtk_label_new (_("glclient2 Launcher"));
  self->welcome = welcome;
  gtk_misc_set_alignment (GTK_MISC (welcome), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), welcome, TRUE, TRUE, 5);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new (_("Server"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  
#ifdef LIBGTK_3_0_0
  self->combo = combo = gtk_combo_box_text_new();
#else
  self->combo = combo = gtk_combo_box_new_text();
#endif

  gtk_widget_set_size_request (combo, 0, 30);
  boot_dialog_combo_update (self);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  g_signal_connect(combo, "changed", 
    G_CALLBACK(boot_dialog_combo_changed), self);

  notebook = gtk_notebook_new ();
/* for gtk3 */
#if 0 
  gtk_notebook_set_homogeneous_tabs (GTK_NOTEBOOK (notebook), TRUE);
#endif
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 5);

  /* Basic tab */
  label = gtk_label_new (_("Basic"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), self->component->basictable, label);

#ifdef USE_SSL
  /* SSL tab */
  label = gtk_label_new (_("SSL"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), self->component->ssltable, label);
#endif

  /* Other tab */
  label = gtk_label_new (_("Details"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), self->component->othertable, label);

  action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

  button = gtk_button_new_with_label (_("Connect"));
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (boot_dialog_on_connect), self);
  gtk_widget_set_can_default(button,TRUE);
  gtk_widget_set_can_focus(button,TRUE);
  gtk_widget_grab_default (button);
  gtk_widget_grab_focus (button);

  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (boot_dialog_on_close), self);
  gtk_widget_set_can_default(button,TRUE);

  button = gtk_button_new_with_label (_("Configuration"));
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (boot_dialog_on_config), self);
  gtk_widget_set_can_default(button,TRUE);

  /* INFO tab */
  label = gtk_label_new (_("INFOMATION"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), self->component->infotable, label);

  return self;
}

static void
boot_dialog_free (BootDialog *self)
{
  g_free (self);
}

/*
  return: TRUE -> connect, FALSE -> close
 */
void
boot_dialog_run ()
{
  BootDialog *self;

  self = boot_dialog_new ();
  gtk_widget_show_all (self->dialog);
  //gtk_widget_hide(self->component->protocol_v2);

  gtk_widget_grab_focus (self->dialog);

  gtk_window_set_modal (GTK_WINDOW (self->dialog), TRUE);
  gtk_main ();
  gtk_window_set_modal (GTK_WINDOW (self->dialog), FALSE);


  gtk_widget_destroy (self->dialog);
  boot_dialog_free (self);

  if (!self->is_connect) {
    exit(0);
  }
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
