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

#ifndef __BDCOMPONENT_H_INCLUDED__
#define __BDCOMPONENT_H_INCLUDED__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

typedef struct _BDComponent BDComponent;
struct _BDComponent {
// basic
  GtkWidget *basictable;
  GtkWidget *host;
  GtkWidget *port;
  GtkWidget *application;
  GtkWidget *protocol_v1;
  GtkWidget *protocol_v2;
  GtkWidget *user;
  GtkWidget *password;
  GtkWidget *savepassword;
// ssl
#ifdef USE_SSL
  GtkWidget *ssltable;
  GtkWidget *ssl_container;
  GtkWidget *ssl;
  GtkWidget *key;
  GtkWidget *cert;
  GtkWidget *CApath;
  GtkWidget *CAfile;
  GtkWidget *ciphers;
#ifdef  USE_PKCS11
  GtkWidget *pkcs11;
  GtkWidget *pkcs11_lib;
  GtkWidget *pkcs11_container;
  GtkWidget *slot;
#endif
#endif
// other 
  GtkWidget *othertable;
  GtkWidget *mlog;
  GtkWidget *cache;
  GtkWidget *style;
  GtkWidget *gtkrc;
  GtkWidget *keybuff;
  GtkWidget *timer;
  GtkWidget *timerperiod;
  GtkWidget *timer_container;
// info
  GtkWidget *infotable;
};

BDConfigSection * new_config_section(BDConfig *config, char *hostname);

gboolean validate_isblank (gchar *str);

typedef struct {
  GtkWidget *entry, *filesel;
} file_selection_data;

void file_ok(GtkWidget *w, gpointer data); 
void file_cancel(GtkWidget *w, gpointer data);
void file_cancel(GtkWidget *w, gpointer data);

void bd_component_set_value(
	BDConfig *config, 
	char *configName, 
	BDComponent *self);

void bd_component_value_to_config(
	BDConfig *config, 
	char *configName, 
	BDComponent *self);

BDComponent * bd_component_new();

#endif /* #ifndef __BDCOMPONENT_H_INCLUDED__ */

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
