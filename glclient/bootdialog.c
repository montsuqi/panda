/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2006 Kouji TAKAO
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

#include <stdio.h>
#include <string.h> /* strlen */
#include <errno.h> /* errno */
#include <ctype.h> /* isblank */
#include <sys/stat.h> /* mkdir */
#include <sys/types.h> /* mkdir */
#include <glib.h>
#include <gtk/gtk.h>

#include "port.h"
#include "const.h"
#include "bd_config.h"
#include "bootdialog.h"

static mode_t permissions = 0600;

gboolean
validate_isblank (gchar *str)
{
  gint i;
  extern int isblank(int c);

  if (str == NULL || str[0] == '\0')
    return TRUE;
  for (i = strlen (str) - 1; i >= 0; i--)
    {
      if (!isblank (str[i]))
        return FALSE;
    }
  return TRUE;
}

/*********************************************************************
 * EditDialog
 ********************************************************************/
typedef struct _EditDialog EditDialog;
struct _EditDialog {
  GtkWidget *dialog;
  GtkWidget *description;
  GtkWidget *host;
  GtkWidget *port;
  GtkWidget *application;
  GtkWidget *protocol_v1;
  GtkWidget *protocol_v2;
  GtkWidget *mlog;
  GtkWidget *cache;
  GtkWidget *style;
  GtkWidget *gtkrc;
  GtkWidget *user;
  GtkWidget *password;
  GtkWidget *keybuff;
#ifdef	USE_SSL
  GtkWidget *ssl;
  GtkWidget *key;
  GtkWidget *cert;
  GtkWidget *CApath;
  GtkWidget *CAfile;
  GtkWidget *ciphers;
#endif

  BDConfig *config;
  gchar *hostname;
  gboolean is_update;
};

static void
edit_dialog_set_value (EditDialog * self)
{
  BDConfigSection *section;

  if (self->hostname == NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (self->host), "localhost");
      gtk_entry_set_text (GTK_ENTRY (self->port), "8000");
      gtk_entry_set_text (GTK_ENTRY (self->application), "panda:");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v1), TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v2), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->mlog), TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->keybuff), FALSE);
      return;
    }

  g_return_if_fail (bd_config_exist_section (self->config, self->hostname));

  section = bd_config_get_section (self->config, self->hostname);
  
  gtk_entry_set_text (GTK_ENTRY (self->description),
                      bd_config_section_get_string (section, "description"));
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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->mlog),
                      bd_config_section_get_bool (section, "mlog"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->keybuff),
                      bd_config_section_get_bool (section, "keybuff"));
  gtk_entry_set_text (GTK_ENTRY (self->user),
                      bd_config_section_get_string (section, "user"));
  gtk_entry_set_text (GTK_ENTRY (self->password),
                      bd_config_section_get_string (section, "password"));
#ifdef	USE_SSL
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->ssl),
                                bd_config_section_get_bool (section, "ssl"));
  gtk_entry_set_text (GTK_ENTRY (self->CApath),
                      bd_config_section_get_string (section, "CApath"));
  gtk_entry_set_text (GTK_ENTRY (self->CAfile),
                      bd_config_section_get_string (section, "CAfile"));
  gtk_entry_set_text (GTK_ENTRY (self->key),
                      bd_config_section_get_string (section, "key"));
  gtk_entry_set_text (GTK_ENTRY (self->cert),
                      bd_config_section_get_string (section, "cert"));
  gtk_entry_set_text (GTK_ENTRY (self->ciphers),
		      bd_config_section_get_string (section, "ciphers"));
#endif
}

static void
edit_dialog_value_to_config (EditDialog * self)
{
  BDConfigSection *section;
  GString *hostname;
  gint i;
  gchar *password;
  gboolean savepassword;

  section = NULL;
  if (self->hostname != NULL)
    {
      g_return_if_fail (bd_config_exist_section (self->config, self->hostname));
      section = bd_config_get_section (self->config, self->hostname);
    }
  else
    {
      hostname = g_string_new (NULL);
      for (i = 1; i < 1000; i++)
        {
          g_string_sprintf (hostname, "host%03d", i);
          if (!bd_config_exist_section (self->config, hostname->str))
            {
              section = bd_config_append_section (self->config, hostname->str);
              break;
            }
        }
      g_string_free (hostname, TRUE);
    }
  g_return_if_fail (section != NULL);

  bd_config_section_set_string (section, "description",
                                gtk_entry_get_text (GTK_ENTRY (self->description)));
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
  bd_config_section_set_bool
    (section, "keybuff",
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->keybuff)));
  bd_config_section_set_string (section, "user",
                                gtk_entry_get_text (GTK_ENTRY (self->user)));
  password = gtk_entry_get_text (GTK_ENTRY (self->password));
  if (validate_isblank (password))
    savepassword = FALSE;
  else
    savepassword = TRUE;
  bd_config_section_set_string (section, "password", password);
  bd_config_section_set_bool (section, "savepassword", savepassword);
#ifdef	USE_SSL
  bd_config_section_set_bool
    (section, "ssl",
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->ssl)));
  bd_config_section_set_string (section, "CApath",
                                gtk_entry_get_text (GTK_ENTRY (self->CApath)));
  bd_config_section_set_string (section, "CAfile",
                                gtk_entry_get_text (GTK_ENTRY (self->CAfile)));
  bd_config_section_set_string (section, "key",
                                gtk_entry_get_text (GTK_ENTRY (self->key)));
  bd_config_section_set_string (section, "cert",
                                gtk_entry_get_text (GTK_ENTRY (self->cert)));
  bd_config_section_set_string (section, "ciphers",
                                gtk_entry_get_text (GTK_ENTRY (self->ciphers)));
#endif
}

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
      fprintf (stderr, "warning: Description is blank\n");
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
              fprintf (stderr, "warning: already used description\n");
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

typedef struct {
  GtkWidget *entry, *filesel;
} file_selection_data;


static void
destroy(GtkWidget *w, gpointer data)
{
  gtk_grab_remove(w);
  g_free(data);
}

static void
file_ok(GtkWidget *w, gpointer data)
{
  file_selection_data *localdata;
  char *tmp;

  localdata = (file_selection_data*)data;
  tmp = gtk_file_selection_get_filename(GTK_FILE_SELECTION(localdata->filesel));
  gtk_entry_set_text(GTK_ENTRY(localdata->entry), tmp);
  gtk_widget_destroy(localdata->filesel);

  return;
}

static void
open_file_selection(GtkWidget *w, gpointer entry)
{
  file_selection_data *data;
  GtkFileSelection *filew = NULL;

  filew = GTK_FILE_SELECTION(gtk_file_selection_new(""));
  data = g_malloc(sizeof(file_selection_data));
  data->entry = entry;
  data->filesel = GTK_WIDGET(filew);
  gtk_signal_connect(GTK_OBJECT(filew), "destroy",
		     (GtkSignalFunc)destroy, data);
  gtk_signal_connect(GTK_OBJECT(filew->ok_button), "clicked",
		     (GtkSignalFunc)file_ok, data);
  gtk_signal_connect(GTK_OBJECT(filew->cancel_button), "clicked",
		     (GtkSignalFunc)gtk_widget_destroy, (gpointer)filew);
  gtk_widget_show(GTK_WIDGET(filew));
  gtk_grab_add(GTK_WIDGET(filew));
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
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *alignment;
  GtkWidget *check;
  gint ypos;

  self = g_new0 (EditDialog, 1);

  self->hostname = hostname;
  self->config = config;
  self->is_update = FALSE;

  self->dialog = dialog = gtk_dialog_new ();
  if (self->hostname == NULL)
    title = "新規作成";
  else
    title = "編集";
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "edit", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (edit_dialog_on_delete_event), self);
  
  /* buttons */
  button = gtk_button_new_with_label ("Ok");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (edit_dialog_on_ok), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  button = gtk_button_new_with_label ("キャンセル");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (edit_dialog_on_cancel), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

  /* contents */
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);

  ypos = 0;

  label = gtk_label_new ("説明");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->description = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("ホスト(ポート)");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  self->host = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 110, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  self->port = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 40, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("アプリケーション");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->application = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("プロトコル");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  hbox = gtk_hbox_new (TRUE, 5);
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  self->protocol_v1 = check = gtk_check_button_new_with_label ("v1");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  self->protocol_v2 = check = gtk_check_button_new_with_label ("v2");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;
  
  label = gtk_label_new ("キャッシュ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->cache = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("スタイル");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->style = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("Gtkrc");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->gtkrc = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  self->mlog = check = gtk_check_button_new_with_label ("ログ出力");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  self->keybuff = check = gtk_check_button_new_with_label ("キーバッファを有効にする");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("ユーザ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->user = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("パスワード");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->password = entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

#ifdef USE_SSL
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  self->ssl = check = gtk_check_button_new_with_label ("SSL");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("CA証明書へのパス");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->CApath = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("CA証明書ファイル");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->CAfile = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("鍵ファイル名(pem/p12)");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->key = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("証明書ファイル名(pem/p12)");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->cert = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("暗号スイート");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->ciphers = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;
#endif

  return self;
}

static void
edit_dialog_free (EditDialog * self)
{
  g_free (self);
}

static gboolean
edit_dialog_run (BDConfig * config, gchar * hostname)
{
  EditDialog *self;
  gboolean is_update;

  self = edit_dialog_new (config, hostname);
  edit_dialog_set_value (self);
  
  gtk_widget_show_all (self->dialog);
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
  { "説明", "description" },
  { "ホスト",           "host" },
  { "ポート",           "port" },
  { "アプリケーション", "application"},
  { "ユーザ",           "user" },
};
static gint server_dialog_titles_count
= sizeof (server_dialog_titles) / sizeof (server_dialog_titles[0]);

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
  gtk_window_set_title (GTK_WINDOW (dialog), "サーバ設定");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "config", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (server_dialog_on_delete_event), self);

  /* buttons */
  button = gtk_button_new_with_label ("新規");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (server_dialog_on_new), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  self->edit = button = gtk_button_new_with_label ("編集");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (server_dialog_on_edit), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_set_sensitive (button, FALSE);
  self->delete = button = gtk_button_new_with_label ("削除");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (server_dialog_on_delete), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_set_sensitive (button, FALSE);
  button = gtk_button_new_with_label ("閉じる");
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
    titles[i] = server_dialog_titles[i].title;
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
static gchar *custom_label = "カスタム";

static void
boot_dialog_create_conf (BDConfig *config)
{
  BDConfigSection *section;
  gboolean is_create = FALSE;
  
  if (!bd_config_exist_section (config, "glclient"))
    {
      section = bd_config_append_section (config, "glclient");
      bd_config_section_append_value (section, "splash", "");
      bd_config_section_append_value (section, "caption", "glclient ランチャー");
      bd_config_section_append_value (section, "welcome", "glclient ランチャー");
      
      is_create = TRUE;
    }
  if (!bd_config_exist_section (config, "global"))
    {
      section = bd_config_append_section (config, "global");
      bd_config_section_append_value (section, "hostname", custom_label);
      bd_config_section_append_value (section, "host", "localhost");
      bd_config_section_append_value (section, "port", "8000");
      bd_config_section_append_value (section, "application", "panda:");
      bd_config_section_append_value (section, "protocol_v1", "true");
      bd_config_section_append_value (section, "protocol_v2", "false");
      bd_config_section_append_value (section, "cache", "./cache");
      bd_config_section_append_value (section, "style", "");
      bd_config_section_append_value (section, "gtkrc", "");
      bd_config_section_append_value (section, "mlog", "false");
      bd_config_section_append_value (section, "keybuff", "false");
      bd_config_section_append_value (section, "user", "");
      bd_config_section_append_value (section, "password", "");
      bd_config_section_append_value (section, "savepassword", "false");
#ifdef	USE_SSL
      bd_config_section_append_value (section, "ssl", "false");
      bd_config_section_append_value (section, "CApath", "");
      bd_config_section_append_value (section, "CAfile", "");
      bd_config_section_append_value (section, "key", "");
      bd_config_section_append_value (section, "cert", "");
      bd_config_section_append_value (section, "ciphers", "ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH");
#endif      
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
      gchar *dir;
      gchar *file;
      BDConfigSection *section;
      
      dir = g_strconcat(g_get_home_dir (), G_DIR_SEPARATOR_S, ".glclient", NULL);
      if (mkdir (dir, 0755) && errno != EEXIST)
        fprintf (stderr, "error: could not create per-user config directory\n");
      file = g_strconcat(dir, G_DIR_SEPARATOR_S, "glclient.conf", NULL);
      config_ = bd_config_new_with_filename (file);
      boot_dialog_create_conf (config_);
      
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
  GtkWidget *servers;
  GHashTable *servers_hash;
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
  GtkWidget *keybuff;
#ifdef	USE_SSL
  GtkWidget *ssl;
  GtkWidget *ssllabel;
  GtkWidget *sslcontainer;
  GtkWidget *CApath;
  GtkWidget *CAfile;
  GtkWidget *key;
  GtkWidget *cert;
  GtkWidget *ciphers;
#endif
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
  
  g_return_if_fail (bd_config_exist_section (config, "glclient"));
  g_return_if_fail (bd_config_exist_section (config, "global"));

  section = bd_config_get_section (config, "glclient");

  gtk_window_set_title (GTK_WINDOW (self->dialog),
                        bd_config_section_get_string (section, "caption"));
  gtk_label_set_text (GTK_LABEL (self->welcome),
                      bd_config_section_get_string (section, "welcome"));
  
  section = bd_config_get_section (config, "global");

  boot_dialog_servers_set_hostname (self,
                                    bd_config_section_get_string (section, "hostname"));
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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->keybuff),
                      bd_config_section_get_bool (section, "keybuff"));
  gtk_entry_set_text (GTK_ENTRY (self->password), password_->str);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->savepassword),
                                bd_config_section_get_bool (section, "savepassword"));
#ifdef	USE_SSL
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->ssl),
                                bd_config_section_get_bool (section, "ssl"));
  gtk_widget_set_sensitive(self->ssllabel, 
                                bd_config_section_get_bool (section, "ssl"));
  gtk_widget_set_sensitive(self->sslcontainer, 
                                bd_config_section_get_bool (section, "ssl"));
  gtk_entry_set_text (GTK_ENTRY (self->CApath),
                      bd_config_section_get_string (section, "CApath"));
  gtk_entry_set_text (GTK_ENTRY (self->CAfile),
                      bd_config_section_get_string (section, "CAfile"));
  gtk_entry_set_text (GTK_ENTRY (self->key),
                      bd_config_section_get_string (section, "key"));
  gtk_entry_set_text (GTK_ENTRY (self->cert),
                      bd_config_section_get_string (section, "cert"));
  gtk_entry_set_text (GTK_ENTRY (self->ciphers),
                      bd_config_section_get_string (section, "ciphers"));
#endif
}

static void
boot_dialog_get_value (BootDialog *self, BDConfig *config)
{
  BDConfigSection *section;
  gboolean savepassword;

  g_return_if_fail (bd_config_exist_section (config, "global"));

  section = bd_config_get_section (config, "global");

  bd_config_section_set_string (section, "hostname",
                                boot_dialog_servers_get_hostname (self));
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
  bd_config_section_set_bool
    (section, "keybuff",
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->keybuff)));
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
#ifdef	USE_SSL
  bd_config_section_set_bool (section, "ssl", 
               gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (self->ssl)));
  bd_config_section_set_string (section, "CApath",
                                gtk_entry_get_text (GTK_ENTRY (self->CApath)));
  bd_config_section_set_string (section, "CAfile",
                                gtk_entry_get_text (GTK_ENTRY (self->CAfile)));
  bd_config_section_set_string (section, "key",
                                gtk_entry_get_text (GTK_ENTRY (self->key)));
  bd_config_section_set_string (section, "cert",
                                gtk_entry_get_text (GTK_ENTRY (self->cert)));
  bd_config_section_set_string (section, "ciphers",
                                gtk_entry_get_text (GTK_ENTRY (self->ciphers)));
#endif
}

static void
boot_dialog_change_hostname (BootDialog * self, BDConfig * config, gboolean force)
{
  BDConfigSection *global, *section;
  gchar *hostname;
  gchar *password;

  g_return_if_fail (bd_config_exist_section (config, "global"));

  hostname = boot_dialog_servers_get_hostname (self);
  
  global = bd_config_get_section (config, "global");
  if (!force
      && strcmp (hostname, bd_config_section_get_string (global, "hostname")) == 0)
    return;
  if (strcmp (hostname, custom_label) == 0)
    {
      bd_config_section_set_string (global, "hostname", custom_label);
      return;
    }
  if (!bd_config_exist_section (config, hostname))
    {
      boot_dialog_servers_set_hostname (self, custom_label);
      bd_config_section_set_string (global, "hostname", custom_label);
      return;
    }
  
  section = bd_config_get_section (config, hostname);

  bd_config_section_set_string (global, "hostname", hostname);
  bd_config_section_set_string (global, "host",
                                bd_config_section_get_string (section, "host"));
  bd_config_section_set_string (global, "port",
                                bd_config_section_get_string (section, "port"));
  bd_config_section_set_string (global, "application",
                                bd_config_section_get_string (section, "application"));
  bd_config_section_set_bool (global, "protocol_v1",
                              bd_config_section_get_bool (section, "protocol_v1"));
  bd_config_section_set_bool (global, "protocol_v2",
                              bd_config_section_get_bool (section, "protocol_v2"));
  bd_config_section_set_string (global, "cache",
                                bd_config_section_get_string (section, "cache"));
  bd_config_section_set_string (global, "style",
                                bd_config_section_get_string (section, "style"));
  bd_config_section_set_string (global, "gtkrc",
                                bd_config_section_get_string (section, "gtkrc"));
  bd_config_section_set_bool (global, "mlog",
                              bd_config_section_get_bool (section, "mlog"));
  bd_config_section_set_string (global, "user",
                                bd_config_section_get_string (section, "user"));
  password = bd_config_section_get_string (section, "password");
  password_ = g_string_assign (password_, password);
  bd_config_section_set_string (global, "password", password);
  bd_config_section_set_bool (global, "savepassword",
                              bd_config_section_get_bool (section, "savepassword"));
#ifdef	USE_SSL
  bd_config_section_set_bool (global, "ssl",
                            bd_config_section_get_bool (section, "ssl"));
  bd_config_section_set_string (global, "CApath",
                            bd_config_section_get_string (section, "CApath"));
  bd_config_section_set_string (global, "CAfile",
                            bd_config_section_get_string (section, "CAfile"));
  bd_config_section_set_string (global, "key",
                            bd_config_section_get_string (section, "key"));
  bd_config_section_set_string (global, "cert",
                            bd_config_section_get_string (section, "cert"));
  bd_config_section_set_string (global, "ciphers",
                            bd_config_section_get_string (section, "ciphers"));
#endif
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
  server_list = g_list_append (server_list, custom_label);

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
      if (strcmp (hostname, custom_label) != 0)
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

#ifdef	USE_SSL
static void
boot_dialog_on_ssl_toggle (GtkWidget *widget, BootDialog *self)
{
  gboolean sensitive;

  sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->ssl));
  gtk_widget_set_sensitive(self->ssllabel, sensitive);
  gtk_widget_set_sensitive(self->sslcontainer, sensitive);
}
#endif

static BootDialog *
boot_dialog_new ()
{
  BootDialog *self;
  GtkWidget *dialog, *vbox, *welcome, *notebook, *table, *action_area, *button;
  GtkWidget *label, *entry, *hbox, *check, *alignment, *omenu;
  gint ypos;

  self = g_new0 (BootDialog, 1);
  self->servers_hash = NULL;
  self->is_connect = FALSE;

  dialog = gtk_dialog_new ();
  self->dialog = dialog;
  gtk_window_set_title (GTK_WINDOW (dialog), "glclient ランチャー");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "lancher", "glclient");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (boot_dialog_on_delete_event), self);
  
  vbox = GTK_DIALOG (dialog)->vbox;

  welcome = gtk_label_new ("glclient ランチャー");
  self->welcome = welcome;
  gtk_misc_set_alignment (GTK_MISC (welcome), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), welcome, TRUE, TRUE, 5);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new ("サーバ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  
  self->servers = omenu = gtk_option_menu_new ();
  gtk_widget_set_usize (omenu, 0, 30);
  boot_dialog_servers_update (self, config_);
  gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_homogeneous_tabs (GTK_NOTEBOOK (notebook), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 5);

  /* General options */
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  label = gtk_label_new ("基本");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  ypos = 0;

  label = gtk_label_new ("ホスト(ポート)");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 110, 0);
  self->host = entry;
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 40, 0);
  self->port = entry;
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("アプリケーション");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->application = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

#ifdef	USE_SSL
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label ("SSLを使う");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->ssl = check;
  gtk_signal_connect (GTK_OBJECT (check), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_ssl_toggle), self);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;
#endif

  label = gtk_label_new ("プロトコル");
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
  
  label = gtk_label_new ("ユーザ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->user = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("パスワード");
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
  check = gtk_check_button_new_with_label ("パスワードの保存");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->savepassword = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

#ifdef	USE_SSL
  /* SSL options */
  table = gtk_table_new (3, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  label = gtk_label_new ("SSL");
  self->sslcontainer = table;
  self->ssllabel = label;
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  ypos = 0;

  label = gtk_label_new ("CA証明書へのパス");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->CApath = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("CA証明書ファイル");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->CAfile = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("鍵ファイル名(pem/p12)");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->key = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("証明書ファイル名(pem/p12)");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->cert = entry = gtk_entry_new ();
  button = gtk_button_new_with_label("参照");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)open_file_selection, (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new ("暗号スイート");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->ciphers = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;
#endif

  /* Advanced options */
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  label = gtk_label_new ("詳細");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  ypos = 0;

  label = gtk_label_new ("キャッシュ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->cache = entry;
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("スタイル");
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
  check = gtk_check_button_new_with_label ("ログ出力");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->mlog = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label ("キーバッファを有効にする");
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->keybuff = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  action_area = GTK_DIALOG (dialog)->action_area;

  button = gtk_button_new_with_label ("接続");
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_connect), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);

  button = gtk_button_new_with_label ("閉じる");
  gtk_box_pack_start (GTK_BOX (action_area), button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (boot_dialog_on_close), self);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  
  button = gtk_button_new_with_label ("設定");
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
      fprintf (stderr, "error: permissions is not 0%o: %s\n",
               permissions, bd_config_get_filename (config_));
      return FALSE;
    }

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
      bd_config_save (config_, NULL, permissions);
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
  self->keybuff = bd_config_section_get_bool (section, "keybuff");
  self->user = bd_config_section_get_string (section, "user");
  self->password = password_->str;
#ifdef	USE_SSL
  self->ssl = bd_config_section_get_bool (section, "ssl");
  self->CApath = bd_config_section_get_string (section, "CApath");
  self->CAfile = bd_config_section_get_string (section, "CAfile");
  self->key = bd_config_section_get_string (section, "key");
  self->cert = bd_config_section_get_string (section, "cert");
  self->ciphers = bd_config_section_get_string (section, "ciphers");
#endif
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
  fprintf (fp, "keybuff     : %s\n", self->keybuff ? "TRUE" : "FALSE");
  fprintf (fp, "user        : %s\n", self->user);
  fprintf (fp, "password    : %s\n", self->password);
#ifdef	USE_SSL
  fprintf (fp, "ssl         : %s\n", self->ssl ? "TRUE" : "FALSE");
  fprintf (fp, "CApath      : %s\n", self->CApath);
  fprintf (fp, "CAfile      : %s\n", self->CAfile);
  fprintf (fp, "key         : %s\n", self->key);
  fprintf (fp, "cert        : %s\n", self->cert);
  fprintf (fp, "ciphers     : %s\n", self->ciphers);
#endif
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
