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
#include    "port.h"
#include    "const.h"
#include    "bd_config.h"
#include    "bd_component.h"
#include    "bootdialog.h"
#include    "dialogs.h"
#include    "message.h"
#include    "debug.h"

static mode_t permissions = 0600;

/*********************************************************************
 * EditDialog
 ********************************************************************/
static gchar *
get_config_hostname_by_desc(BDConfig *config, gchar *desc)
{
  GList *p;
  gchar *_desc;
  gchar *host;

  if (desc == NULL) return "global";

  for (p = bd_config_get_sections (config); p != NULL; p = g_list_next (p)) {
      host = (char *) p->data;
      if (!strcmp (host, "glclient") || !strcmp (host, "global"))
        continue;
      _desc = bd_config_get_string (config, host, "description");
      if (!strcmp(_desc, desc)) {
        return host;
      }
  }
  return "global";
}


static gboolean
check_description(BDConfig *config, gchar *olddesc, gchar *newdesc)
{
  GList *p;
  gchar *desc;
  gchar *host;

  if (validate_isblank (newdesc))
    {
      message_dialog(GTK_MESSAGE_ERROR, _("warning: Description is blank\n"));
      return FALSE;
  }
  if (strcmp(olddesc, newdesc)) {
    for (p = bd_config_get_sections (config); p != NULL; p = g_list_next (p)) {
      host = (char *) p->data;
      if (strcmp (host, "glclient") == 0 || 
          strcmp (host, "global") == 0)
        continue;
      desc = bd_config_get_string (config, host, "description");
      if (strcmp (desc, newdesc) == 0) {
          message_dialog_printf(GTK_MESSAGE_ERROR,
            _("warning: already used description: %s\n"), newdesc);
          return FALSE;
      }
    }
  }
  return TRUE;
}

static void
edit_dialog_run (BDConfig * config, gchar * hostname, GtkWidget *parent)
{
  BDComponent *component;
  BDConfigSection *section;
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GString *newhostname;
  gchar *title;
  gchar *olddesc;
  gchar *newdesc;
  gint i;
  gboolean create_section = FALSE;

  if (hostname == NULL) {
    title = _("New");
    newhostname = g_string_new (NULL);
    for (i = 1; i < 1000; i++) {
      g_string_sprintf (newhostname, "host%03d", i);
      if (!bd_config_exist_section (config, newhostname->str)) {
        new_config_section(config, newhostname->str);
        create_section = TRUE;
        break;
      } 
    }
    hostname = newhostname->str;
  } else {
    title = _("Edit");
  }

  section = bd_config_get_section (config, hostname);
  if (section == NULL) {
    message_dialog_printf(GTK_MESSAGE_ERROR, 
      _("doesn't exist config:%s"), hostname);
    return;
  }
  olddesc = g_strdup(bd_config_section_get_string(section, "description"));

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
  label = gtk_label_new (_("Description"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY(entry), olddesc);

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

  bd_component_set_value(config, hostname, component);
  section = bd_config_get_section (config, hostname);

  /* run */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
    newdesc = (gchar *)gtk_entry_get_text(GTK_ENTRY(entry));
    if (check_description(config, olddesc, newdesc)) {
      section = bd_config_get_section (config, hostname);
      bd_config_section_set_string (section, "description", newdesc);
      bd_component_value_to_config(config, hostname, component);
      bd_config_save (config, NULL, permissions);
    } else {
      if (create_section)
        bd_config_remove_section (config, hostname);
    }
  } else {
      if (create_section)
        bd_config_remove_section (config, hostname);
  }
  g_free(olddesc);
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

  BDConfig *config;
  gint is_update;
};

static struct {
  gchar *title;
  gchar *value_name;
} server_dialog_titles[] = {
  { N_("Description"), "description" },
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
  gchar *hostname;
  BDConfigSection *section;
  gint i;
  
  g_return_if_fail (self != NULL);

  gtk_panda_clist_clear (GTK_PANDA_CLIST (self->server_list));

  text_list[server_dialog_titles_count] = NULL;
  for (p = bd_config_get_sections (self->config); 
    p != NULL; p = g_list_next (p)) {
    hostname = (gchar *) p->data;
    if (strcmp (hostname, "glclient") == 0 || 
      strcmp (hostname, "global") == 0) {
      continue;
    }
    section = bd_config_get_section (self->config, hostname);
    for (i = 0; i < server_dialog_titles_count; i++) {
      text_list[i] = bd_config_section_get_string (section, 
        server_dialog_titles[i].value_name);
    }
    gtk_panda_clist_append (GTK_PANDA_CLIST (self->server_list), text_list);
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
  edit_dialog_run (self->config, NULL, self->dialog);
  server_dialog_server_list_update (self);
#if 0
  gdk_window_raise (self->dialog->window);
#endif
}

static void
server_dialog_on_edit (GtkWidget * widget, ServerDialog * self)
{
  int i;
  gchar *hostname;
  gchar *desc;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;
  GValue value = {0};

  for (
    i = 0; 
    i < gtk_panda_clist_get_n_rows(GTK_PANDA_CLIST(self->server_list));
    i++) {
    if (gtk_panda_clist_row_is_selected(GTK_PANDA_CLIST(self->server_list),i)){
      path = gtk_tree_path_new_from_indices(i , -1);
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->server_list));
      if (gtk_tree_model_get_iter(model, &iter, path)) {
         gtk_tree_model_get_value(model, &iter, 0, &value);
         desc = (gchar *)g_value_get_string(&value);
         hostname = get_config_hostname_by_desc(self->config, desc);
         if (strcmp(hostname, "global")) {
           edit_dialog_run (self->config, hostname, self->dialog);
           server_dialog_server_list_update (self);
         }
         g_value_unset(&value);
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
  gchar *hostname;
  gchar *desc;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;
  GValue value = {0};

  for (
    i = 0; 
    i < gtk_panda_clist_get_n_rows(GTK_PANDA_CLIST(self->server_list));
    i++) {
    if (gtk_panda_clist_row_is_selected(GTK_PANDA_CLIST(self->server_list),i)){
      path = gtk_tree_path_new_from_indices(i , -1);
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->server_list));
      if (gtk_tree_model_get_iter(model, &iter, path)) {
         gtk_tree_model_get_value(model, &iter, 0, &value);
         desc = (gchar *)g_value_get_string(&value);
         hostname = get_config_hostname_by_desc(self->config, desc);
         if (strcmp(hostname,"global")) {
           bd_config_remove_section (self->config, hostname);
           bd_config_save (self->config, NULL, permissions);
           server_dialog_server_list_update (self);
         }
         g_value_unset(&value);
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
server_dialog_new (BDConfig * config, GtkWidget *parent)
{
  ServerDialog *self;
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *scroll;
  GtkWidget *clist;
  GtkTreeViewColumn *column;
  gint i;

  self = g_new0 (ServerDialog, 1);
  
  self->config = config;
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
server_dialog_run (BDConfig *config, GtkWidget *parent)
{
  ServerDialog *self;
  gboolean is_update;

  self = server_dialog_new (config, parent);
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

static BDConfig *config_ = NULL;
static gboolean is_boot_dialog_init = FALSE;
static gchar *custom_label = N_("Custom");

static void
boot_dialog_create_conf (BDConfig *config)
{
  BDConfigSection *section;
  gboolean is_create = FALSE;
  
  if (!bd_config_exist_section (config, "glclient"))
    {
      section = bd_config_append_section (config, "glclient");
      bd_config_section_append_value (section, "splash", "");
      bd_config_section_append_value (section, "caption", "glclient Launcher");
      bd_config_section_append_value (section, "welcome", "glclient Launcher");
      
      is_create = TRUE;
    }
  if (!bd_config_exist_section (config, "global"))
    {
      section = new_config_section (config, "global");
      bd_config_section_append_value (section, "hostname", _(custom_label));
      is_create = TRUE;
    }

  if (is_create)
    bd_config_save (config, NULL, permissions);

}

static void
boot_dialog_init ()
{
  if (!is_boot_dialog_init)
    {
      config_ = bd_config_load_file ();
      boot_dialog_create_conf (config_);
      is_boot_dialog_init = TRUE;
    }
}

void
boot_dialog_term ()
{
  bd_config_free (config_);
  config_ = NULL;
  is_boot_dialog_init = FALSE;
}

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
  BDConfigSection *global;
  BootDialog *self;
  gchar *desc;
  gchar *hostname;
  
  self = (BootDialog *)user_data;
#ifdef LIBGTK_3_0_0
  desc = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
#else
  desc = gtk_combo_box_get_active_text(widget);
#endif
  if (desc) {
    hostname = get_config_hostname_by_desc(config_, desc);
    bd_component_set_value(config_, hostname, self->component);

    global =  bd_config_get_section (config_, "global");
    bd_config_section_set_string (global, "hostname", hostname);

    g_free(desc);
  }
}

static void
boot_dialog_combo_update (BootDialog *self)
{
  GList *p;
  gchar *hostname;
  gchar *desc;
  gchar *selected;
  gint i;
  gint custom_index;
  gboolean update = FALSE;

  g_signal_handlers_block_by_func(GTK_COMBO_BOX(self->combo),
    boot_dialog_combo_changed, self->combo);

#ifdef LIBGTK_3_0_0
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(self->combo));
  selected = bd_config_get_string (config_, "global", "hostname");
  i = custom_index = 0;
  for (p = bd_config_get_sections (config_); p != NULL; p = g_list_next (p)) {
      hostname = (gchar *)p->data;
      if (!strcmp(hostname, "glclient")) {
        continue;
      } else if (!strcmp(hostname, "global")) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->combo), _(custom_label));
        custom_index = i;
      } else {
        desc = bd_config_get_string (config_, hostname, "description");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->combo), desc);
      }
      if (!strcmp(hostname, selected)) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(self->combo), i);
        bd_component_set_value(config_, hostname, self->component);
        update = TRUE;
      }
      i++;
  }
#else
  GtkListStore *store;
  store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(self->combo)));
  gtk_list_store_clear(store);

  selected = bd_config_get_string (config_, "global", "hostname");

  i = custom_index = 0;
  for (p = bd_config_get_sections (config_); p != NULL; p = g_list_next (p)) {
      hostname = (gchar *)p->data;
      if (!strcmp(hostname, "glclient")) {
        continue;
      } else if (!strcmp(hostname, "global")) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(self->combo), _(custom_label));
        custom_index = i;
      } else {
        desc = bd_config_get_string (config_, hostname, "description");
        gtk_combo_box_append_text(GTK_COMBO_BOX(self->combo), desc);
      }
      if (!strcmp(hostname, selected)) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(self->combo), i);
        bd_component_set_value(config_, hostname, self->component);
        update = TRUE;
      }
      i++;
  }
#endif
  if (!update) {
    bd_component_set_value(config_, "global", self->component);
    gtk_combo_box_set_active(GTK_COMBO_BOX(self->combo), custom_index);
  }
  g_signal_handlers_unblock_by_func(GTK_COMBO_BOX(self->combo),
    boot_dialog_combo_changed, self->combo);
}

static void
boot_dialog_on_connect (GtkWidget *connect, BootDialog *self)
{
  BDConfigSection *global;
  gchar *desc;
  gchar *hostname;
  self->is_connect = TRUE;
  
#ifdef LIBGTK_3_0_0
  desc = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self->combo));
#else
  desc = gtk_combo_box_get_active_text(GTK_COMBO_BOX(self->combo));
#endif
  hostname = get_config_hostname_by_desc(config_, desc);
  bd_component_value_to_config(config_, hostname, self->component);
  bd_component_value_to_config(config_, "global", self->component);

  global =  bd_config_get_section (config_, "global");
  bd_config_section_set_string (global, "hostname", hostname);
  bd_config_save (config_, NULL, permissions);
  
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
  server_dialog_run (config_, self->dialog);
  bd_config_save (config_, NULL, permissions);
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
gboolean
boot_dialog_run ()
{
  BootDialog *self;
  gboolean res;

  boot_dialog_init ();
  if (bd_config_permissions (config_) != permissions)
    {
      Warning(_("error: permissions is not 0%o: %s\n"),
               permissions, bd_config_get_filename (config_));
      return FALSE;
    }

  self = boot_dialog_new ();
  gtk_widget_show_all (self->dialog);
  //gtk_widget_hide(self->component->protocol_v2);

  gtk_widget_grab_focus (self->dialog);

  gtk_window_set_modal (GTK_WINDOW (self->dialog), TRUE);
  gtk_main ();
  gtk_window_set_modal (GTK_WINDOW (self->dialog), FALSE);

  res = self->is_connect;

  gtk_widget_destroy (self->dialog);
  boot_dialog_free (self);

  return res;
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
