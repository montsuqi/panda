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
#include <glib.h>

#define	DEFAULT_HOST 				"localhost"
#define	DEFAULT_PORT 				"8000"
#define	DEFAULT_APPLICATION 		"panda:"
#define DEFAULT_CACHE_PATH			"/.glclient/cache"
#define	DEFAULT_PROTOCOL_V1 		TRUE
#define	DEFAULT_PROTOCOL_V1_STR 	"true"
#define	DEFAULT_PROTOCOL_V2 		TRUE
#define	DEFAULT_PROTOCOL_V2_STR 	"true"
#define	DEFAULT_MLOG 				TRUE
#define	DEFAULT_MLOG_STR 			"true"
#define	DEFAULT_KEYBUFF 			FALSE
#define	DEFAULT_KEYBUFF_STR 		"false"
#define	DEFAULT_STYLE 				"/usr/share/panda-client/jma-receipt.rc"
#define	DEFAULT_GTKRC 				""
#define	DEFAULT_USER 				"user"
#define	DEFAULT_PASSWORD 			""
#define	DEFAULT_SAVEPASSWORD 		FALSE
#define	DEFAULT_SAVEPASSWORD_STR	"false"
#ifdef USE_SSL
#define	DEFAULT_SSL 				FALSE
#define	DEFAULT_SSL_STR 			"false"
#define	DEFAULT_CAPATH 				""
#define	DEFAULT_CAFILE 				""
#define	DEFAULT_KEY 				""
#define	DEFAULT_CERT 				""
#define	DEFAULT_CIPHERS 			"ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH"
#ifdef USE_PKCS11
#define	DEFAULT_PKCS11 				FALSE
#define	DEFAULT_PKCS11_STR 			"false"
#define	DEFAULT_PKCS11_LIB			""
#define	DEFAULT_SLOT				""
#endif
#endif

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
};

gboolean validate_isblank (gchar *str);

typedef struct {
  GtkWidget *entry, *filesel;
} file_selection_data;

BDConfigSection * new_config_section(BDConfig *config, char *hostname);
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
