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
#include    <errno.h> /* errno */
#include    <ctype.h> /* isblank */
#include    <sys/stat.h> /* mkdir */
#include    <sys/types.h> /* mkdir */
#include    <glib.h>
#include    <gtk/gtk.h>
#include    "gettext.h"
#include	"glclient.h"
#include    "port.h"
#include    "const.h"
#include    "bd_config.h"
#include    "bd_component.h"

/*********************************************************************
 * config function
 ********************************************************************/
BDConfigSection *
new_config_section(
  BDConfig *config,
  char *hostname)
{
  BDConfigSection *section;
  char *cachename = g_strconcat(g_get_home_dir(), DEFAULT_CACHE_PATH, NULL);

  section = bd_config_append_section (config, hostname);
  bd_config_section_append_value (section, "host", DEFAULT_HOST);
  bd_config_section_append_value (section, "port", DEFAULT_PORT);
  bd_config_section_append_value (section, "application", DEFAULT_APPLICATION);
  bd_config_section_append_value (section, "protocol_v1", DEFAULT_PROTOCOL_V1_STR);
  bd_config_section_append_value (section, "protocol_v2", DEFAULT_PROTOCOL_V2_STR);
  bd_config_section_append_value (section, "cache", cachename);
  bd_config_section_append_value (section, "style", DEFAULT_STYLE);
  bd_config_section_append_value (section, "gtkrc", DEFAULT_GTKRC);
  bd_config_section_append_value (section, "mlog",  DEFAULT_MLOG_STR);
  bd_config_section_append_value (section, "keybuff", DEFAULT_KEYBUFF_STR);
  bd_config_section_append_value (section, "timer", DEFAULT_TIMER_STR);
  bd_config_section_append_value (section, "timerperiod", DEFAULT_TIMERPERIOD);
  bd_config_section_append_value (section, "user", DEFAULT_USER);
  bd_config_section_append_value (section, "password", DEFAULT_PASSWORD);
  bd_config_section_append_value (section, "savepassword", DEFAULT_SAVEPASSWORD_STR);
#ifdef  USE_SSL
  bd_config_section_append_value (section, "ssl", DEFAULT_SSL_STR);
  bd_config_section_append_value (section, "CApath", DEFAULT_CAPATH);
  bd_config_section_append_value (section, "CAfile", DEFAULT_CAFILE);
  bd_config_section_append_value (section, "key", DEFAULT_KEY);
  bd_config_section_append_value (section, "cert", DEFAULT_CERT);
  bd_config_section_append_value (section, "ciphers", DEFAULT_CIPHERS);
#ifdef  USE_PKCS11
  bd_config_section_append_value (section, "pkcs11", DEFAULT_PKCS11_STR);
  bd_config_section_append_value (section, "pkcs11_lib", DEFAULT_PKCS11_LIB);
  bd_config_section_append_value (section, "slot", DEFAULT_SLOT);
#endif
#endif
  g_free(cachename);
  return section;
}

/*********************************************************************
 * misc function
 ********************************************************************/
gboolean
validate_isblank (gchar *str)
{
  gint i;
#if 0
  extern int isblank(int c);
#endif

  if (str == NULL || str[0] == '\0')
    return TRUE;
  for (i = strlen (str) - 1; i >= 0; i--)
    {
      if (!isblank (str[i]))
        return FALSE;
    }
  return TRUE;
}

void
open_file_chooser(GtkWidget *w, gpointer entry)
{
  GtkWidget *dialog;
  
  dialog = gtk_file_chooser_dialog_new (_("Open File"),
    NULL,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename;
    
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_entry_set_text(GTK_ENTRY(entry), filename);
    g_free (filename);
  }
  
  gtk_widget_destroy (dialog);
}

void
open_dir_chooser(GtkWidget *w, gpointer entry)
{
  GtkWidget *dialog;
  
  dialog = gtk_file_chooser_dialog_new (_("Open Directory"),
    NULL,
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename;
    
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_entry_set_text(GTK_ENTRY(entry), filename);
    g_free (filename);
  }
  
  gtk_widget_destroy (dialog);
}

#ifdef USE_SSL
static void
on_ssl_toggle (GtkWidget *widget, BDComponent *self)
{
  gboolean sensitive;

  sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->ssl));
  gtk_widget_set_sensitive(self->ssl_container, sensitive);
}

#ifdef  USE_PKCS11
static void
on_pkcs11_toggle (GtkWidget *widget, BDComponent *self) {
  gboolean sensitive;

  sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->pkcs11));
  gtk_widget_set_sensitive(self->pkcs11_container, sensitive);
}
#endif 
#endif

static void
on_timer_toggle (GtkWidget *widget, BDComponent *self) {
  gboolean sensitive;

  sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->timer));
  gtk_widget_set_sensitive(self->timer_container, sensitive);
}

/*********************************************************************
 * boot dialog component
 ********************************************************************/
void
bd_component_set_value (
  BDConfig   *config, 
  char       *hostname,
  BDComponent *self) 
{
  BDConfigSection *section;
  char *host;
  char *port;

  g_return_if_fail( hostname != NULL );

  if (!bd_config_exist_section (config, hostname)) {
    section = bd_config_get_section (config, "global");
  } else {
    section = bd_config_get_section (config, hostname);
  }

  // basic
  gtk_entry_set_text (GTK_ENTRY (self->host), 
    bd_config_section_get_string (section, "host"));
  gtk_entry_set_text (GTK_ENTRY (self->port), 
    bd_config_section_get_string (section, "port"));

  host = bd_config_section_get_string (section, "host");
  port = bd_config_section_get_string (section, "port");
  PortNumber = g_strconcat(host, ":", port, NULL);

  gtk_entry_set_text (GTK_ENTRY (self->application), 
    bd_config_section_get_string (section, "application"));
  CurrentApplication = bd_config_section_get_string (section, "application");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v1), 
    bd_config_section_get_bool (section, "protocol_v1"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v2), 
    bd_config_section_get_bool (section, "protocol_v2"));
  gtk_entry_set_text (GTK_ENTRY (self->user),
    bd_config_section_get_string (section, "user"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->savepassword), 
    bd_config_section_get_bool (section, "savepassword"));
  if ( bd_config_section_get_bool (section, "savepassword") ) {
    gtk_entry_set_text (GTK_ENTRY (self->password),
      bd_config_section_get_string (section, "password"));
  } else {
    gtk_entry_set_text (GTK_ENTRY (self->password), "");
  }

#ifdef USE_SSL
  // ssl
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->ssl),
    bd_config_section_get_bool (section, "ssl"));
  gtk_widget_set_sensitive(self->ssl_container, 
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
#ifdef USE_PKCS11
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->pkcs11),
    bd_config_section_get_bool (section, "pkcs11"));  
  gtk_widget_set_sensitive(self->pkcs11_container, 
    bd_config_section_get_bool (section, "pkcs11"));  
  gtk_entry_set_text (GTK_ENTRY (self->pkcs11_lib),
    bd_config_section_get_string (section, "pkcs11_lib"));
  gtk_entry_set_text (GTK_ENTRY (self->slot),
    bd_config_section_get_string (section, "slot")); 
#endif
#endif

  // other
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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->timer),
    bd_config_section_get_bool_default (section, "timer", DEFAULT_TIMER));
  gtk_entry_set_text (GTK_ENTRY (self->timerperiod),
    bd_config_section_get_string_default (section, "timerperiod", 
      DEFAULT_TIMERPERIOD));
  gtk_widget_set_sensitive(self->timer_container, 
    bd_config_section_get_bool_default (section, "timer", DEFAULT_TIMER));  
}

void
bd_component_value_to_config (
  BDConfig   *config,
  char       *hostname,
  BDComponent *self)
{
  BDConfigSection *section;
  gchar *password;
  gboolean savepassword;
  char *host;
  char *port;

  section = NULL;
  if (hostname != NULL) {
      if (! bd_config_exist_section (config, hostname)) {
        return;
      }
      section = bd_config_get_section (config, hostname);
  }
  g_return_if_fail (section != NULL);

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
  bd_config_section_set_string (section, "user",
                                gtk_entry_get_text (GTK_ENTRY (self->user)));
  password = strdup(gtk_entry_get_text (GTK_ENTRY (self->password)));
  savepassword = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->savepassword));
  if (savepassword) {
    bd_config_section_set_string (section, "password", password);
  } else {
    bd_config_section_set_string (section, "password", "");
  }
  bd_config_section_set_bool (section, "savepassword", savepassword);

  host = strdup(gtk_entry_get_text (GTK_ENTRY (self->host)));
  port = strdup(gtk_entry_get_text (GTK_ENTRY (self->port)));
  PortNumber = g_strconcat(host, ":", port, NULL);
  CurrentApplication = strdup(gtk_entry_get_text (GTK_ENTRY (self->application)));
  Protocol1 = gtk_toggle_button_get_active(
    GTK_TOGGLE_BUTTON (self->protocol_v1));
  Protocol2 = gtk_toggle_button_get_active(
    GTK_TOGGLE_BUTTON (self->protocol_v2));
  User = strdup(gtk_entry_get_text (GTK_ENTRY (self->user)));
  Pass = strdup(gtk_entry_get_text (GTK_ENTRY (self->password)));
  SavePass = gtk_toggle_button_get_active (
    GTK_TOGGLE_BUTTON (self->savepassword));

#ifdef USE_SSL
  // ssl
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

  fSsl = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->ssl));
  CA_Path = strdup(gtk_entry_get_text (GTK_ENTRY (self->CApath)));
  if (!strcmp("", CA_Path)) CA_Path = NULL;
  CA_File = strdup(gtk_entry_get_text (GTK_ENTRY (self->CAfile)));
  if (!strcmp("", CA_File)) CA_File = NULL;
  KeyFile = strdup(gtk_entry_get_text (GTK_ENTRY (self->key)));
  if (!strcmp("", KeyFile)) KeyFile = NULL;
  CertFile = strdup(gtk_entry_get_text (GTK_ENTRY (self->cert)));
  if (!strcmp("", CertFile)) CertFile = NULL;
  Ciphers = strdup(gtk_entry_get_text (GTK_ENTRY (self->ciphers)));

#ifdef  USE_PKCS11
  bd_config_section_set_bool
    (section, "pkcs11", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->pkcs11)));
  bd_config_section_set_string (section, "pkcs11_lib",
    gtk_entry_get_text (GTK_ENTRY (self->pkcs11_lib)));
  bd_config_section_set_string (section, "slot",
    gtk_entry_get_text (GTK_ENTRY (self->slot)));

  fPKCS11 = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->pkcs11));
  PKCS11_Lib = strdup(gtk_entry_get_text (GTK_ENTRY (self->pkcs11_lib)));
  if (!strcmp("", PKCS11_Lib)) PKCS11_Lib = NULL;
  Slot = strdup(gtk_entry_get_text (GTK_ENTRY (self->slot)));
#endif
#endif

  // other
  bd_config_section_set_path (section, "cache",
    gtk_entry_get_text (GTK_ENTRY (self->cache)));
  bd_config_section_set_path (section, "style",
    gtk_entry_get_text (GTK_ENTRY (self->style)));
  bd_config_section_set_path (section, "gtkrc",
    gtk_entry_get_text (GTK_ENTRY (self->gtkrc)));
  bd_config_section_set_bool
    (section, "mlog",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->mlog)));
  bd_config_section_set_bool
    (section, "keybuff",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->keybuff)));
  bd_config_section_set_bool
    (section, "timer",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->timer)));
  bd_config_section_set_path (section, "timerperiod",
    gtk_entry_get_text (GTK_ENTRY (self->timerperiod)));

  Cache = strdup(gtk_entry_get_text (GTK_ENTRY (self->cache)));
  Style = strdup(gtk_entry_get_text (GTK_ENTRY (self->style)));
  Gtkrc = strdup(gtk_entry_get_text (GTK_ENTRY (self->gtkrc)));
  fMlog = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->mlog));
  fKeyBuff = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->keybuff));
  fTimer = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->timer));
  TimerPeriod = strdup(gtk_entry_get_text (GTK_ENTRY (self->timerperiod)));
}

BDComponent *
bd_component_new()
{
  BDComponent *self;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *alignment;
  GtkWidget *check;
  gint ypos;

  self = g_new0(BDComponent, 1);

  // basic
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  self->basictable = table;

  ypos = 0;

  label = gtk_label_new (_("Host(Port)"));
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

  label = gtk_label_new (_("Application"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->application = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new (_("Protocol"));
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

  label = gtk_label_new (_("User"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->user = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new (_("Password"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->password = entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label (_("Remember Password"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->savepassword = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

#ifdef USE_SSL
  // ssl
  table = gtk_table_new (3, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  self->ssltable = table;


  ypos = 0;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label (_("Use SSL"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  g_signal_connect (G_OBJECT (check), "clicked",
                      G_CALLBACK (on_ssl_toggle), self);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  self->ssl = check;
  ypos++;

  table = gtk_table_new (3, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  self->ssl_container = table;
  gtk_table_attach (GTK_TABLE(self->ssltable), table, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos = 0;

  label = gtk_label_new (_("CA Certificate Path"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->CApath = entry = gtk_entry_new ();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_dir_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new (_("CA Certificate File"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->CAfile = entry = gtk_entry_new ();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_file_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new (_("SSL Key File(pem)"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->key = entry = gtk_entry_new ();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_file_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new (_("Certificate(pem/p12)"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->cert = entry = gtk_entry_new ();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_file_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new (_("Cipher Suite"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->ciphers = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

#ifdef  USE_PKCS11
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label (_("Use Security Device"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->pkcs11 = check;
  g_signal_connect (G_OBJECT (check), "clicked",
                      G_CALLBACK (on_pkcs11_toggle), self);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  /* pkcs11 container */
  table = gtk_table_new (3, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  self->pkcs11_container = table;
  gtk_table_attach (GTK_TABLE(self->ssl_container), table, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos = 0;

  label = gtk_label_new (_("PKCS#11 Library"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->pkcs11_lib = entry = gtk_entry_new ();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_file_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new (_("Slot ID"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->slot = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

#endif
#endif

  // other
  table = gtk_table_new (3, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  self->othertable = table;

  ypos = 0;

  label = gtk_label_new (_("Cache"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->cache = entry;
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_dir_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new (_("Style"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->style = entry;
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_file_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new (_("Gtkrc"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  entry = gtk_entry_new ();
  self->gtkrc = entry;
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked",
             G_CALLBACK(open_file_chooser), (gpointer)entry);
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label (_("Output Logfile"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->mlog = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label (_("Enable Keybuffer"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->keybuff = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label (_("Enable Timer"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->timer = check;
  g_signal_connect (GTK_OBJECT (check), "clicked",
                      G_CALLBACK (on_timer_toggle), self);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  /* timer container */
  table = gtk_table_new (3, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  self->timer_container = table;
  gtk_table_attach (GTK_TABLE(self->othertable), table, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos = 0;

  label = gtk_label_new (_("Timer Period(ms)"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  self->timerperiod = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  return self;
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
