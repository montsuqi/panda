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
#ifdef  USE_GNOME
#include    <gnome.h>
#else
#include    <gtk/gtk.h>
#include    "gettext.h"
#endif
#include    "port.h"
#include    "const.h"
#include    "bd_config.h"
#include    "bd_component.h"
#include    "bootdialog.h"
#include    "message.h"
#include    "debug.h"

static mode_t permissions = 0600;

/*********************************************************************
 * EditDialog
 ********************************************************************/
typedef struct _EditDialog EditDialog;
struct _EditDialog {
  GtkWidget *dialog;
  GtkWidget *description;
  BDComponent *component;
  BDConfig *config;
  gchar *hostname;
  gboolean is_update;
};

static gboolean
edit_dialog_validate (EditDialog * self)
{
  gchar *desc;
  gchar *hostname;
  gchar *str;
  GList *p;
  
  desc = gtk_entry_get_text (GTK_ENTRY (self->description));
  if (validate_isblank (desc))
    {
      Warning(_("warning: Description is blank\n"));
      return FALSE;
    }
  if (self->hostname == NULL)
    {
      for (p = bd_config_get_sections (self->config); p != NULL; p = g_list_next (p))
        {
          hostname = (gchar *) p->data;
          if (strcmp (hostname, "glclient") == 0 || strcmp (hostname, "global") == 0)
            continue;
          str = bd_config_get_string (self->config, hostname, "description");
          if (strcmp (str, desc) == 0)
            {
              Warning(_("warning: already used description\n"));
              return FALSE;
            }
        }
    }
  return TRUE;
}

static gboolean
edit_dialog_on_delete_event (GtkWidget * widget, EditDialog * self)
{
  self->is_update = FALSE;
  gtk_main_quit ();
  return TRUE;
}

static void
edit_dialog_value_to_config (EditDialog *self)
{
  BDConfigSection *section;
  section = NULL;
  g_return_if_fail (! (self->hostname == NULL));
  g_return_if_fail (bd_config_exist_section (self->config, self->hostname));
  section = bd_config_get_section (self->config, self->hostname);
  g_return_if_fail (! (section == NULL));
  bd_config_section_set_string (section, "description",
                                gtk_entry_get_text (GTK_ENTRY (self->description)));

  bd_component_value_to_config(self->config, self->hostname, self->component);
}

static void
edit_dialog_on_ok (GtkWidget * widget, EditDialog * self)
{
  if (!edit_dialog_validate (self))
    return;
  edit_dialog_value_to_config (self);
  self->is_update = TRUE;
  gtk_main_quit ();
}

static void
edit_dialog_on_cancel (GtkWidget * widget, EditDialog * self)
{
  self->is_update = FALSE;
  gtk_main_quit ();
}

static EditDialog *
edit_dialog_new (BDConfig * config, gchar * hostname)
{
  EditDialog *self;
  gchar *title;
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GString *newhostname;
  gint i;

  self = g_new0 (EditDialog, 1);

  if (hostname == NULL) {
    title = _("New");
    newhostname = g_string_new (NULL);
    for (i = 1; i < 1000; i++) {
      g_string_sprintf (newhostname, "host%03d", i);
      if (!bd_config_exist_section (config, newhostname->str)) {
        new_config_section(config, newhostname->str);
        break;
      } 
    }
    self->hostname = hostname = newhostname->str;
  } else {
    title = _("Edit");
    self->hostname = hostname;
  }

  self->config = config;
  self->is_update = FALSE;

  self->dialog = dialog = gtk_dialog_new ();

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "edit", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (edit_dialog_on_delete_event), self);

  /* buttons */
  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (edit_dialog_on_ok), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  button = gtk_button_new_with_label (_("Cancel"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (edit_dialog_on_cancel), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

  /* contents */
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  label = gtk_label_new (_("Description"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->description = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);

  self->component = bd_component_new();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), self->component->basictable, TRUE, TRUE, 0);
#ifdef USE_SSL
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), self->component->ssltable, TRUE, TRUE, 0);
#endif
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), self->component->othertable, TRUE, TRUE, 0);
  return self;
}

static void
edit_dialog_free (EditDialog * self)
{
  g_free (self->component);
  g_free (self);
}

static void
edit_dialog_set_value(EditDialog *self)
{
  BDConfigSection *section;

  bd_component_set_value(self->config, self->hostname, self->component);
  g_return_if_fail(!(self->hostname == NULL));

  section = bd_config_get_section (self->config, self->hostname);
  g_return_if_fail (! (section == NULL));
  gtk_entry_set_text (GTK_ENTRY (self->description),
    bd_config_section_get_string (section, "description"));
  bd_config_section_set_string (section, "description",
                                gtk_entry_get_text (GTK_ENTRY (self->description)));
}

static gboolean
edit_dialog_run (BDConfig * config, gchar * hostname)
{
  EditDialog *self;
  gboolean is_update;

  self = edit_dialog_new (config, hostname);
  edit_dialog_set_value (self);
  
  gtk_widget_show_all (self->dialog);
  gtk_widget_set_sensitive( self->component->protocol_v1, TRUE);
  gtk_widget_set_sensitive( self->component->protocol_v2, TRUE);
  gtk_widget_grab_focus (self->description);
  gtk_window_set_modal (GTK_WINDOW (self->dialog), TRUE);
  gtk_main ();
  gtk_window_set_modal (GTK_WINDOW (self->dialog), FALSE);
  gtk_widget_destroy (self->dialog);

  is_update = self->is_update;
  
  edit_dialog_free (self);

  return is_update;
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
  gint row;
  
  g_return_if_fail (self != NULL);

  gtk_clist_freeze (GTK_CLIST (self->server_list));
  gtk_clist_clear (GTK_CLIST (self->server_list));

  text_list[server_dialog_titles_count] = NULL;
  for (p = bd_config_get_sections (self->config); p != NULL; p = g_list_next (p))
    {
      hostname = (gchar *) p->data;
      if (strcmp (hostname, "glclient") == 0 || strcmp (hostname, "global") == 0)
        continue;
      section = bd_config_get_section (self->config, hostname);
      for (i = 0; i < server_dialog_titles_count; i++)
        text_list[i]
          = bd_config_section_get_string (section, server_dialog_titles[i].value_name);
      row = gtk_clist_append (GTK_CLIST (self->server_list), text_list);
      gtk_clist_set_row_data (GTK_CLIST (self->server_list), row, hostname);
    }
  gtk_clist_columns_autosize (GTK_CLIST (self->server_list));
  
  gtk_clist_thaw (GTK_CLIST (self->server_list));
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
  gint row;
  
  if (edit_dialog_run (self->config, NULL))
    {
      server_dialog_server_list_update (self);
      bd_config_save (self->config, NULL, permissions);
      row = GTK_CLIST (self->server_list)->rows - 1;
      gtk_clist_select_row (GTK_CLIST (self->server_list), row, 0);
      if (!gtk_clist_row_is_visible (GTK_CLIST (self->server_list), row))
        gtk_clist_moveto (GTK_CLIST (self->server_list), row, 0, 0.5, 0);
      self->is_update = TRUE;
    }
  gdk_window_raise (self->dialog->window);
}

static void
server_dialog_on_edit (GtkWidget * widget, ServerDialog * self)
{
  GList *selection, *p;
  gint row;
  gchar *hostname;
  gboolean is_update;

  g_return_if_fail (GTK_CLIST (self->server_list)->selection != NULL);
  
  selection = GTK_CLIST (self->server_list)->selection;
  is_update = FALSE;
  row = 0;
  for (p = selection; p != NULL; p = g_list_next (p))
    {
      row = GPOINTER_TO_INT (p->data);
      hostname = gtk_clist_get_row_data (GTK_CLIST (self->server_list), row);
      is_update = (edit_dialog_run (self->config, hostname) || is_update);
    }
  if (is_update)
    {
      server_dialog_server_list_update (self);
      bd_config_save (self->config, NULL, permissions);
      gtk_clist_select_row (GTK_CLIST (self->server_list), row, 0);
      if (!gtk_clist_row_is_visible (GTK_CLIST (self->server_list), row))
        gtk_clist_moveto (GTK_CLIST (self->server_list), row, 0, 0.5, 0);
      self->is_update = TRUE;
    }
  gdk_window_raise (self->dialog->window);
}

static void
server_dialog_on_delete (GtkWidget * widget, ServerDialog * self)
{
  GList *selection, *p;
  gint row;
  gchar *hostname;
  gboolean is_update;

  g_return_if_fail (GTK_CLIST (self->server_list)->selection != NULL);
  
  selection = GTK_CLIST (self->server_list)->selection;
  is_update = FALSE;
  row = 0;
  for (p = selection; p != NULL; p = g_list_next (p))
    {
      row = GPOINTER_TO_INT (p->data);
      hostname = gtk_clist_get_row_data (GTK_CLIST (self->server_list), row);
      bd_config_remove_section (self->config, hostname);
      is_update = TRUE;
    }
  
  if (is_update)
    {
      server_dialog_server_list_update (self);
      if (row >= GTK_CLIST (self->server_list)->rows)
        row = GTK_CLIST (self->server_list)->rows - 1;
      bd_config_save (self->config, NULL, permissions);
      gtk_clist_select_row (GTK_CLIST (self->server_list), row, 0);
      if (!gtk_clist_row_is_visible (GTK_CLIST (self->server_list), row))
        gtk_clist_moveto (GTK_CLIST (self->server_list), row, 0, 0.5, 0);
      self->is_update = TRUE;
    }
}

static void
server_dialog_on_close (GtkWidget * widget, ServerDialog * self)
{
  gtk_main_quit ();
}

static void
server_dialog_on_select_row (GtkWidget * widget, gint row, gint column,
                             GdkEvent * event, ServerDialog * self)
{
  gtk_widget_set_sensitive (self->edit, TRUE);
  gtk_widget_set_sensitive (self->delete, TRUE);
}

static void
server_dialog_on_unselect_row (GtkWidget * widget, gint row, gint column,
                               GdkEvent * event, ServerDialog * self)
{
  gtk_widget_set_sensitive (self->edit, FALSE);
  gtk_widget_set_sensitive (self->delete, FALSE);
}

static void
server_dialog_on_row_move (GtkWidget * widget, gint from, gint to,
                           ServerDialog * self)
{
  gchar *hostname;
  
  hostname = gtk_clist_get_row_data (GTK_CLIST (self->server_list), from);
  if (bd_config_move_section (self->config, hostname, to + 2)) /* 2 = glclient, global */
    {
      bd_config_save (self->config, NULL, permissions);
      self->is_update = TRUE;
    }
}

static ServerDialog *
server_dialog_new (BDConfig * config)
{
  ServerDialog *self;
  gchar *titles[server_dialog_titles_count];
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *scroll;
  GtkWidget *clist;
  gint i;

  self = g_new0 (ServerDialog, 1);
  
  self->config = config;
  self->is_update = FALSE;

  self->dialog = dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Server Setting"));
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "config", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (server_dialog_on_delete_event), self);

  /* buttons */
  button = gtk_button_new_with_label (_("New"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (server_dialog_on_new), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  self->edit = button = gtk_button_new_with_label (_("Edit"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (server_dialog_on_edit), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_set_sensitive (button, FALSE);
  self->delete = button = gtk_button_new_with_label (_("Delete"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (server_dialog_on_delete), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_set_sensitive (button, FALSE);
  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (server_dialog_on_close), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);

  /* contents */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize (scroll, 450, 200);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scroll, TRUE, TRUE, 5);

  for (i = 0; i < server_dialog_titles_count; i++)
    titles[i] = _(server_dialog_titles[i].title);
  self->server_list = clist = gtk_clist_new_with_titles (server_dialog_titles_count,
                                                         titles);
  gtk_clist_column_titles_show (GTK_CLIST (clist));
  gtk_clist_set_reorderable (GTK_CLIST (clist), TRUE);
  gtk_container_add (GTK_CONTAINER (scroll), clist);
  gtk_signal_connect (GTK_OBJECT (clist), "select_row",
                      GTK_SIGNAL_FUNC (server_dialog_on_select_row),
                      self);
  gtk_signal_connect (GTK_OBJECT (clist), "unselect_row",
                      GTK_SIGNAL_FUNC (server_dialog_on_unselect_row),
                      self);
  gtk_signal_connect (GTK_OBJECT (clist), "row_move",
                      GTK_SIGNAL_FUNC (server_dialog_on_row_move),
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
server_dialog_run (BDConfig *config)
{
  ServerDialog *self;
  gboolean is_update;

  self = server_dialog_new (config);
  
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
static GString *password_;
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
      bd_config_section_append_value (section, "caption", _("glclient Launcher"));
      bd_config_section_append_value (section, "welcome", _("glclient Launcher"));
      
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
      BDConfigSection *section;
      
      config_ = bd_config_load_file ();
      boot_dialog_create_conf (config_);
      
      section = bd_config_get_section (config_, "global");
      password_ = g_string_new (bd_config_section_get_string (section, "password"));
      is_boot_dialog_init = TRUE;
    }
}

void
boot_dialog_term ()
{
  bd_config_free (config_);
  config_ = NULL;
  
  g_string_free (password_, TRUE);
  password_ = NULL;
  
  is_boot_dialog_init = FALSE;
}

typedef struct _BootDialog BootDialog;

struct _BootDialog
{
  GtkWidget *dialog;

  GtkWidget *welcome;
  GtkWidget *servers;
  GHashTable *servers_hash;
  BDComponent *component;
  gboolean is_connect;
};

static void
boot_dialog_servers_set_hostname (BootDialog *self, gchar *hostname)
{
  gint current;
  GtkWidget *menu;

  current = GPOINTER_TO_INT (g_hash_table_lookup (self->servers_hash, hostname));
  gtk_option_menu_set_history (GTK_OPTION_MENU (self->servers), current);
  menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (self->servers));
  gtk_menu_set_active (GTK_MENU (menu), current);
}

static gchar *
boot_dialog_servers_get_hostname (BootDialog *self)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar *hostname;

  menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (self->servers));
  menuitem = gtk_menu_get_active (GTK_MENU (menu));
  hostname = gtk_object_get_user_data (GTK_OBJECT (menuitem));

  return hostname;
}

static void
boot_dialog_set_value (BootDialog *self, BDConfig *config)
{
  BDConfigSection *section;
  gchar *hostname;
  
  g_return_if_fail (bd_config_exist_section (config, "glclient"));
  g_return_if_fail (bd_config_exist_section (config, "global"));

  section = bd_config_get_section (config, "glclient");

  gtk_window_set_title (GTK_WINDOW (self->dialog),
    _(bd_config_section_get_string (section, "caption")));
  gtk_label_set_text (GTK_LABEL (self->welcome),
    _(bd_config_section_get_string (section, "welcome")));
  
  section = bd_config_get_section (config, "global");
  hostname = bd_config_section_get_string (section, "hostname");
  boot_dialog_servers_set_hostname (self, hostname);
  bd_component_set_value(config, hostname, self->component);
}

static void
boot_dialog_get_value (BootDialog *self, BDConfig *config)
{
  gchar *hostname;

  g_return_if_fail (bd_config_exist_section (config, "global"));

  hostname = boot_dialog_servers_get_hostname(self);

  bd_component_value_to_config(config, hostname, self->component);
  bd_component_value_to_config(config, "global", self->component);
  g_string_assign (password_, gtk_entry_get_text (GTK_ENTRY (self->component->password)));
}

static void
boot_dialog_change_hostname (BootDialog * self, BDConfig * config, gboolean force)
{
  BDConfigSection *global;
  gchar *hostname;

  g_return_if_fail (bd_config_exist_section (config, "global"));

  hostname = boot_dialog_servers_get_hostname (self);
  
  global = bd_config_get_section (config, "global");
  if (!force
      && strcmp (hostname, bd_config_section_get_string (global, "hostname")) == 0) { 
    return;
  }
  if (strcmp (hostname, _(custom_label)) == 0)
    {
      bd_config_section_set_string (global, "hostname", _(custom_label));
      return;
    }
  if (!bd_config_exist_section (config, hostname))
    {
      boot_dialog_servers_set_hostname (self, _(custom_label));
      bd_config_section_set_string (global, "hostname", _(custom_label));
      return;
    }

  bd_config_section_set_string (global, "hostname", hostname);
  bd_component_set_value(config, hostname, self->component);
  bd_component_value_to_config(config, "global", self->component);
  boot_dialog_set_value (self, config);
}

static void
boot_dialog_servers_on_selection_done (GtkWidget * widget, BootDialog * self)
{
  boot_dialog_change_hostname (self, config_, FALSE);
}

static void
boot_dialog_servers_update (BootDialog *self, BDConfig *config)
{
  GList *server_list, *p;
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar *hostname;
  gchar *desc;
  gint i;

  server_list = NULL;
  for (p = bd_config_get_sections (config); p != NULL; p = g_list_next (p))
    {
      hostname = (gchar *)p->data;
      if (strcmp (hostname, "glclient") == 0 || strcmp (hostname, "global") == 0)
        continue;
      server_list = g_list_append (server_list, hostname);
    }
  server_list = g_list_append (server_list, _(custom_label));

  gtk_option_menu_remove_menu (GTK_OPTION_MENU (self->servers));
  menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (self->servers), menu);
  gtk_signal_connect (GTK_OBJECT (menu), "selection-done",
                      GTK_SIGNAL_FUNC (boot_dialog_servers_on_selection_done),
                      self);
  if (self->servers_hash != NULL)
    g_hash_table_destroy (self->servers_hash);
  self->servers_hash = g_hash_table_new (g_str_hash, g_str_equal);
  for (p = server_list, i = 0; p != NULL; p = g_list_next (p), i++)
    {
      hostname = (gchar *) p->data;
      if (strcmp (hostname, _(custom_label)) != 0)
        desc = bd_config_get_string (config, hostname, "description");
      else
        desc = hostname;
      menuitem = gtk_menu_item_new_with_label (desc);
      gtk_object_set_user_data (GTK_OBJECT (menuitem), hostname);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
      g_hash_table_insert (self->servers_hash, hostname, GINT_TO_POINTER (i));
    }
  g_list_free (server_list);
  
  hostname = bd_config_get_string (config, "global", "hostname");
  boot_dialog_servers_set_hostname (self, hostname);
}

static void
boot_dialog_on_connect (GtkWidget *connect, BootDialog *self)
{
  self->is_connect = TRUE;
  boot_dialog_get_value (self, config_);
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
  if (server_dialog_run (config_))
    {
      boot_dialog_change_hostname (self, config_, TRUE);
      boot_dialog_servers_update (self, config_);
      bd_config_save (config_, NULL, permissions);
    }
  gdk_window_raise (self->dialog->window);
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
  GtkWidget *label, *hbox, *omenu;

  self = g_new0 (BootDialog, 1);
  self->servers_hash = NULL;
  self->is_connect = FALSE;
  self->component = bd_component_new();

  dialog = gtk_dialog_new ();
  self->dialog = dialog;
  gtk_window_set_title (GTK_WINDOW (dialog), _("glclient Launcher"));
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "lancher", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (boot_dialog_on_delete_event), self);
  
  vbox = GTK_DIALOG (dialog)->vbox;

  welcome = gtk_label_new (_("glclient Launcher"));
  self->welcome = welcome;
  gtk_misc_set_alignment (GTK_MISC (welcome), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), welcome, TRUE, TRUE, 5);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new (_("Server"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  
  self->servers = omenu = gtk_option_menu_new ();
  gtk_widget_set_usize (omenu, 0, 30);
  boot_dialog_servers_update (self, config_);
  gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_homogeneous_tabs (GTK_NOTEBOOK (notebook), TRUE);
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

  action_area = GTK_DIALOG (dialog)->action_area;

  button = gtk_button_new_with_label (_("Connect"));
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_connect), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);

  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_close), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

  button = gtk_button_new_with_label (_("Configuration"));
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_config), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

  boot_dialog_set_value (self, config_);

  return self;
}

static void
boot_dialog_free (BootDialog *self)
{
  if (self->servers_hash != NULL)
    g_hash_table_destroy (self->servers_hash);
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
  gtk_widget_set_sensitive( self->component->protocol_v1, TRUE);
  gtk_widget_set_sensitive( self->component->protocol_v1, TRUE);
  gtk_widget_grab_focus (self->dialog);
  gtk_window_set_modal (GTK_WINDOW (self->dialog), TRUE);
  gtk_main ();
  gtk_window_set_modal (GTK_WINDOW (self->dialog), FALSE);

  res = self->is_connect;
  if (res)
    {
      boot_dialog_get_value (self, config_);
      bd_config_save (config_, NULL, permissions);
    }

  gtk_widget_destroy (self->dialog);
  boot_dialog_free (self);

  return res;
}

char *
boot_dialog_get_password()
{
  return password_->str;
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
