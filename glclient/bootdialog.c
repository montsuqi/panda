/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA, written by Kouji TAKAO <kouji@netlab.jp>.

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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <errno.h> /* errno */

#include "port.h"
#include "const.h"
#include "bd_config.h"
#include "bootdialog.h"

/*********************************************************************
 * BootDialog
 ********************************************************************/

static BDConfig *config_ = NULL;
static gboolean is_boot_dialog_init = FALSE;
static GString *password_;

static void
boot_dialog_init ()
{
  if (!is_boot_dialog_init)
    {
      gchar *dir;
      gchar *file;
      BDConfigSection *section;
      
      dir = g_strconcat(g_get_home_dir (), G_DIR_SEPARATOR_S, ".glclient", NULL);
      if (mkdir (dir, 0755) && errno != EEXIST)
        fprintf (stderr, "error: could not create per-user config directory\n");
      file = g_strconcat(dir, G_DIR_SEPARATOR_S, "glclient.conf", NULL);
      config_ = bd_config_new_with_filename (file);
      if (!bd_config_exist_section (config_, "glclient"))
        {
          section = bd_config_append_section (config_, "glclient");
          bd_config_section_append_value (section, "splash", "");
          bd_config_section_append_value (section, "caption", "glclient ランチャー");
          bd_config_section_append_value (section, "welcome", "glclient ランチャー");
        }
      if (!bd_config_exist_section (config_, "global"))
        {
          section = bd_config_append_section (config_, "global");
          bd_config_section_append_value (section, "hostname", "カスタム");
          bd_config_section_append_value (section, "host", "localhost");
          bd_config_section_append_value (section, "port", "8000");
          bd_config_section_append_value (section, "application", "panda:");
          bd_config_section_append_value (section, "protocol_v1", "true");
          bd_config_section_append_value (section, "protocol_v2", "false");
          bd_config_section_append_value (section, "cache", "./cache");
          bd_config_section_append_value (section, "style", "");
          bd_config_section_append_value (section, "gtkrc", "");
          bd_config_section_append_value (section, "mlog", "false");
          bd_config_section_append_value (section, "user", "");
          bd_config_section_append_value (section, "password", "");
          bd_config_section_append_value (section, "savepassword", "false");
        }
      
      section = bd_config_get_section (config_, "global");
      password_ = g_string_new (bd_config_section_get_string (section, "password"));

      g_free (file);
      g_free (dir);
      
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
  GtkWidget *host;
  GtkWidget *port;
  GtkWidget *application;
  GtkWidget *protocol_v1;
  GtkWidget *protocol_v2;
  GtkWidget *user;
  GtkWidget *password;
  GtkWidget *savepassword;
  GtkWidget *cache;
  GtkWidget *style;
  GtkWidget *gtkrc;
  GtkWidget *mlog;

  gboolean is_connect;
};

static void
boot_dialog_on_connect (GtkWidget *connect, BootDialog *self)
{
  self->is_connect = TRUE;
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

static gboolean
boot_dialog_on_delete_event (GtkWidget *widget, GdkEvent *event, BootDialog *self)
{
  self->is_connect = FALSE;
  gtk_widget_hide (self->dialog);
  gtk_main_quit ();
  return TRUE;
}

static void
boot_dialog_set_value (BootDialog *self, BDConfig *config)
{
  BDConfigSection *section;

  g_return_if_fail (bd_config_exist_section (config, "glclient"));
  g_return_if_fail (bd_config_exist_section (config, "global"));

  section = bd_config_get_section (config, "glclient");

  gtk_window_set_title (GTK_WINDOW (self->dialog),
                        bd_config_section_get_string (section, "caption"));
  gtk_label_set_text (GTK_LABEL (self->welcome),
                      bd_config_section_get_string (section, "welcome"));
  
  section = bd_config_get_section (config, "global");
  
  gtk_entry_set_text (GTK_ENTRY (self->host),
                      bd_config_section_get_string (section, "host"));
  gtk_entry_set_text (GTK_ENTRY (self->port),
                      bd_config_section_get_string (section, "port"));
  gtk_entry_set_text (GTK_ENTRY (self->application),
                      bd_config_section_get_string (section, "application"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v1),
                                bd_config_section_get_bool (section, "protocol_v1"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v2),
                                bd_config_section_get_bool (section, "protocol_v2"));
  gtk_entry_set_text (GTK_ENTRY (self->cache),
                      bd_config_section_get_string (section, "cache"));
  gtk_entry_set_text (GTK_ENTRY (self->style),
                      bd_config_section_get_string (section, "style"));
  gtk_entry_set_text (GTK_ENTRY (self->gtkrc),
                      bd_config_section_get_string (section, "gtkrc"));
  gtk_entry_set_text (GTK_ENTRY (self->user),
                      bd_config_section_get_string (section, "user"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->mlog),
                                bd_config_section_get_bool (section, "mlog"));
  gtk_entry_set_text (GTK_ENTRY (self->password), password_->str);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->savepassword),
                                bd_config_section_get_bool (section, "savepassword"));
}

static void
boot_dialog_get_value (BootDialog *self, BDConfig *config)
{
  BDConfigSection *section;
  gboolean savepassword;

  g_return_if_fail (bd_config_exist_section (config, "global"));

  section = bd_config_get_section (config, "global");

  bd_config_section_set_string (section, "host",
                                gtk_entry_get_text (GTK_ENTRY (self->host)));
  bd_config_section_set_string (section, "port",
                                gtk_entry_get_text (GTK_ENTRY (self->port)));
  bd_config_section_set_string (section, "application",
                                gtk_entry_get_text (GTK_ENTRY (self->application)));
  bd_config_section_set_bool
    (section, "protocol_v1",
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->protocol_v1)));
  bd_config_section_set_bool
    (section, "protocol_v2",
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->protocol_v2)));
  bd_config_section_set_string (section, "cache",
                                gtk_entry_get_text (GTK_ENTRY (self->cache)));
  bd_config_section_set_string (section, "style",
                                gtk_entry_get_text (GTK_ENTRY (self->style)));
  bd_config_section_set_string (section, "gtkrc",
                                gtk_entry_get_text (GTK_ENTRY (self->gtkrc)));
  bd_config_section_set_bool
    (section, "mlog",
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->mlog)));
  bd_config_section_set_string (section, "user",
                                gtk_entry_get_text (GTK_ENTRY (self->user)));
  g_string_assign (password_, gtk_entry_get_text (GTK_ENTRY (self->password)));
  savepassword = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->savepassword));
  bd_config_section_set_bool (section, "savepassword", savepassword);
  if (savepassword)
    bd_config_section_set_string (section, "password", password_->str);
  else
    {
      gtk_entry_set_text (GTK_ENTRY (self->password), "");
      bd_config_section_set_string (section, "password", "");
    }
}

static BootDialog *
boot_dialog_new ()
{
  BootDialog *self;
  GtkWidget *dialog, *vbox, *welcome, *notebook, *table, *action_area, *connect, *close;
  GtkWidget *label, *entry, *hbox, *check, *alignment;
  gint ypos;

  self = g_new0 (BootDialog, 1);
  self->is_connect = FALSE;

  dialog = gtk_dialog_new ();
  self->dialog = dialog;
  gtk_window_set_title (GTK_WINDOW (dialog), "glclient lancher");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "lancher", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (boot_dialog_on_delete_event), self);
  
  vbox = GTK_DIALOG (dialog)->vbox;

  welcome = gtk_label_new ("glclient lancher");
  self->welcome = welcome;
  gtk_misc_set_alignment (GTK_MISC (welcome), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), welcome, TRUE, TRUE, 5);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_homogeneous_tabs (GTK_NOTEBOOK (notebook), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 5);

  /* General options */
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  label = gtk_label_new ("General");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  ypos = 0;

  label = gtk_label_new ("Host(Port)");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 100, 0);
  self->host = entry;
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 50, 0);
  self->port = entry;
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("Application");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->application = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("Protocol");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  hbox = gtk_hbox_new (TRUE, 5);
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label ("v1");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
  self->protocol_v1 = check;
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label ("v2");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
  self->protocol_v2 = check;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;
  
  label = gtk_label_new ("User");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->user = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("Password");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  self->password = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label ("Saved password");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->savepassword = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  /* Advanced options */
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  label = gtk_label_new ("Advanced");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  ypos = 0;

  label = gtk_label_new ("Cache");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->cache = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("Style");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->style = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("Gtkrc");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->gtkrc = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label ("Logging");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->mlog = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  action_area = GTK_DIALOG (dialog)->action_area;

  connect = gtk_button_new_with_label ("connect");
  gtk_box_pack_start (GTK_BOX (action_area), connect, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (connect), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_connect), self);

  close = gtk_button_new_with_label ("close");
  gtk_box_pack_start (GTK_BOX (action_area), close, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (close), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_close), self);

  boot_dialog_set_value (self, config_);
    
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

  self = boot_dialog_new ();
  gtk_widget_show_all (self->dialog);
  gtk_widget_grab_focus (self->dialog);
  gtk_window_set_modal (GTK_WINDOW (self->dialog), TRUE);
  gtk_main ();
  gtk_window_set_modal (GTK_WINDOW (self->dialog), FALSE);

  res = self->is_connect;
  if (res)
    {
      boot_dialog_get_value (self, config_);
      bd_config_save (config_, NULL);
    }

  gtk_widget_destroy (self->dialog);
  boot_dialog_free (self);

  return res;
}

/*********************************************************************
 * BootProperty
 ********************************************************************/

void
boot_property_config_to_property (BootProperty *self)
{
  BDConfigSection *section;

  g_return_if_fail (bd_config_exist_section (config_, "global"));
  
  section = bd_config_get_section (config_, "global");
  
  self->host = bd_config_section_get_string (section, "host");
  self->port = bd_config_section_get_string (section, "port");
  self->application = bd_config_section_get_string (section, "application");
  self->protocol_v1 = bd_config_section_get_bool (section, "protocol_v1");
  self->protocol_v2 = bd_config_section_get_bool (section, "protocol_v2");
  self->cache = bd_config_section_get_string (section, "cache");
  self->style = bd_config_section_get_string (section, "style");
  self->gtkrc = bd_config_section_get_string (section, "gtkrc");
  self->mlog = bd_config_section_get_bool (section, "mlog");
  self->user = bd_config_section_get_string (section, "user");
  self->password = password_->str;
}

void
boot_property_inspect (BootProperty * self, FILE *fp)
{
  if (fp == NULL)
    fp = stdout;
  
  fprintf (fp, "host(port)  : %s:%s\n", self->host, self->port);
  fprintf (fp, "application : %s\n", self->application);
  fprintf (fp, "protocol v1 : %s\n", self->protocol_v1 ? "TRUE" : "FALSE");
  fprintf (fp, "protocol v2 : %s\n", self->protocol_v2 ? "TRUE" : "FALSE");
  fprintf (fp, "cache       : %s\n", self->cache);
  fprintf (fp, "style       : %s\n", self->style);
  fprintf (fp, "gtkrc       : %s\n", self->gtkrc);
  fprintf (fp, "mlog        : %s\n", self->mlog ? "TRUE" : "FALSE");
  fprintf (fp, "user        : %s\n", self->user);
  fprintf (fp, "password    : %s\n", self->password);
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
