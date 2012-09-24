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
  gchar *default_path;

  dialog = gtk_file_chooser_dialog_new (_("Open File"),
    NULL,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL);
  default_path = (gchar *)gtk_entry_get_text ((GtkEntry *)entry);
  if (default_path != NULL) {
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), default_path);
  }
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
bd_component_set_value (BDComponent *self,gchar *serverkey) 
{
  gchar *value;

  // basic
  value = gl_config_get_string(serverkey,"host");
  gtk_entry_set_text (GTK_ENTRY (self->host), value);
  g_free(value);

  value = gl_config_get_string(serverkey,"port");
  gtk_entry_set_text (GTK_ENTRY (self->port),value);
  g_free(value);

  value = gl_config_get_string(serverkey,"application");
  gtk_entry_set_text (GTK_ENTRY (self->application),value);
  g_free(value);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v1), 
    gl_config_get_bool(serverkey,"protocol_v1"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->protocol_v2), 
    gl_config_get_bool(serverkey,"protocol_v2"));

  value = gl_config_get_string(serverkey,"user");
  gtk_entry_set_text (GTK_ENTRY (self->user),value);
  g_free(value);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->savepassword), 
    gl_config_get_bool(serverkey,"savepassword"));
  if ( gl_config_get_bool(serverkey,"savepassword") ) {
    value = gl_config_get_string(serverkey,"password");
    gtk_entry_set_text (GTK_ENTRY (self->password),value);
    g_free(value);
  } else {
    gtk_entry_set_text (GTK_ENTRY (self->password), "");
  }

#ifdef USE_SSL
  // ssl
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->ssl),
    gl_config_get_bool(serverkey,"ssl"));
  gtk_widget_set_sensitive(self->ssl_container, 
    gl_config_get_bool(serverkey,"ssl"));  

  value = gl_config_get_string(serverkey,"CAfile");
  gtk_entry_set_text (GTK_ENTRY (self->CAfile),value);
  g_free(value);

  value = gl_config_get_string(serverkey,"cert");
  gtk_entry_set_text (GTK_ENTRY (self->cert),value);
  g_free(value);

  value = gl_config_get_string(serverkey,"ciphers");
  gtk_entry_set_text (GTK_ENTRY (self->ciphers),value);
  g_free(value);
#ifdef USE_PKCS11
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->pkcs11),
    gl_config_get_bool(serverkey,"pkcs11"));  
  gtk_widget_set_sensitive(self->pkcs11_container, 
    gl_config_get_bool(serverkey,"pkcs11"));  

  value = gl_config_get_string(serverkey,"pkcs11_lib");
  gtk_entry_set_text (GTK_ENTRY (self->pkcs11_lib),value);
  g_free(value);

  value = gl_config_get_string(serverkey,"slot"); 
  gtk_entry_set_text (GTK_ENTRY (self->slot),value);
  g_free(value);
#endif
#endif

  // other
  value = gl_config_get_string(serverkey,"style");
  gtk_entry_set_text (GTK_ENTRY (self->style),value);
  g_free(value);

  value = gl_config_get_string(serverkey,"gtkrc");
  gtk_entry_set_text (GTK_ENTRY (self->gtkrc),value);
  g_free(value);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->mlog),
    gl_config_get_bool(serverkey,"mlog"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->keybuff),
    gl_config_get_bool(serverkey,"keybuff"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->timer),
    gl_config_get_bool(serverkey,"timer"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->imkanaoff),
    gl_config_get_bool(serverkey,"im_kana_off"));

  value = gl_config_get_string(serverkey,"timerperiod");
  gtk_entry_set_text (GTK_ENTRY (self->timerperiod),value);
  g_free(value);

  gtk_widget_set_sensitive(self->timer_container, 
    gl_config_get_bool(serverkey,"timer"));  
}

void
bd_component_value_to_config(BDComponent *self,gchar *serverkey)
{
  gchar *password;
  gboolean savepassword;

  gl_config_set_string(serverkey,"host",
    gtk_entry_get_text (GTK_ENTRY (self->host)));
  gl_config_set_string(serverkey,"port",
    gtk_entry_get_text (GTK_ENTRY (self->port)));
  gl_config_set_string(serverkey,"application",
    gtk_entry_get_text (GTK_ENTRY (self->application)));
  gl_config_set_bool(serverkey,"protocol_v1",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->protocol_v1)));
  gl_config_set_bool(serverkey,"protocol_v2",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->protocol_v2)));
  gl_config_set_string(serverkey,"user",
    gtk_entry_get_text (GTK_ENTRY (self->user)));
  password = strdup(gtk_entry_get_text (GTK_ENTRY (self->password)));
  savepassword = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->savepassword));
  if (savepassword) {
    gl_config_set_string(serverkey,"password", password);
  } else {
    gl_config_set_string(serverkey,"password", "");
  }
  gl_config_set_bool(serverkey,"savepassword", savepassword);

  Host = strdup(gtk_entry_get_text (GTK_ENTRY (self->host)));
  PortNum = strdup(gtk_entry_get_text (GTK_ENTRY (self->port)));
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
  gl_config_set_bool(serverkey,"ssl",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->ssl)));
  gl_config_set_string(serverkey,"CAfile",
    gtk_entry_get_text (GTK_ENTRY (self->CAfile)));
  gl_config_set_string(serverkey,"cert",
    gtk_entry_get_text (GTK_ENTRY (self->cert)));
  gl_config_set_string(serverkey,"ciphers",
    gtk_entry_get_text (GTK_ENTRY (self->ciphers)));

  fSsl = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->ssl));
  CA_File = strdup(gtk_entry_get_text (GTK_ENTRY (self->CAfile)));
  if (!strcmp("", CA_File)) CA_File = NULL;
  CertFile = strdup(gtk_entry_get_text (GTK_ENTRY (self->cert)));
  if (!strcmp("", CertFile)) CertFile = NULL;
  Ciphers = strdup(gtk_entry_get_text (GTK_ENTRY (self->ciphers)));

#ifdef  USE_PKCS11
  gl_config_set_bool(serverkey,"pkcs11", 
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->pkcs11)));
  gl_config_set_string(serverkey,"pkcs11_lib",
    gtk_entry_get_text (GTK_ENTRY (self->pkcs11_lib)));
  gl_config_set_string(serverkey,"slot",
    gtk_entry_get_text (GTK_ENTRY (self->slot)));

  fPKCS11 = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->pkcs11));
  PKCS11_Lib = strdup(gtk_entry_get_text (GTK_ENTRY (self->pkcs11_lib)));
  if (!strcmp("", PKCS11_Lib)) PKCS11_Lib = NULL;
  Slot = strdup(gtk_entry_get_text (GTK_ENTRY (self->slot)));
#endif
#endif

  // other
  gl_config_set_string(serverkey,"style",
    gtk_entry_get_text (GTK_ENTRY (self->style)));
  gl_config_set_string(serverkey,"gtkrc",
    gtk_entry_get_text (GTK_ENTRY (self->gtkrc)));
  gl_config_set_bool(serverkey,"mlog",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->mlog)));
  gl_config_set_bool(serverkey,"keybuff",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->keybuff)));
  gl_config_set_bool(serverkey,"im_kana_off",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->imkanaoff)));
  gl_config_set_bool(serverkey,"timer",
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->timer)));
  gl_config_set_string(serverkey,"timerperiod",
    gtk_entry_get_text (GTK_ENTRY (self->timerperiod)));

  Style = strdup(gtk_entry_get_text (GTK_ENTRY (self->style)));
  Gtkrc = strdup(gtk_entry_get_text (GTK_ENTRY (self->gtkrc)));
  fMlog = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->mlog));
  fKeyBuff = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->keybuff));
  fIMKanaOff = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->imkanaoff));
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
  char buff[256];
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
  gtk_widget_set_size_request (entry, 110, -1);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  self->port = entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 40, -1);
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

  label = gtk_label_new (_("Certificate(*.p12)"));
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
  check = gtk_check_button_new_with_label (_("Disable IM control and Kana input"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->imkanaoff = check;
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label (_("Enable Timer"));
  gtk_container_add (GTK_CONTAINER (alignment), check);
  self->timer = check;
  g_signal_connect (G_OBJECT (check), "clicked",
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

  // info
  table = gtk_table_new (2, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  self->infotable = table;

  ypos = 0;
  
  sprintf(buff, "glclient2 ver %s", PACKAGE_VERSION);
  label = gtk_label_new (buff);
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  gtk_container_add (GTK_CONTAINER (alignment), label);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new ("Copyright (C) 2011 ORCA Project");
  alignment = gtk_alignment_new (0.5, 0.5, 0, 1);
  gtk_container_add (GTK_CONTAINER (alignment), label);
  gtk_table_attach (GTK_TABLE (table), alignment, 0, 2, ypos, ypos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

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
