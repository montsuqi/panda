/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Kouji TAKAO
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

#ifndef __BOOTDIALOG_H_INCLUDED__
#define __BOOTDIALOG_H_INCLUDED__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <glib.h>

typedef struct _BootProperty BootProperty;

struct _BootProperty
{
  gchar *host;
  gchar *port;
  gchar *application;
  gboolean protocol_v1;
  gboolean protocol_v2;
  gchar *cache;
  gchar *style;
  gchar *gtkrc;
  gchar *user;
  gchar *password;
  gboolean mlog;
  gboolean keybuff;
#ifdef USE_SSL
  gchar *key;
  gchar *cert;
  gboolean ssl;
  gchar *verifypeer;
  gchar *CApath;
  gchar *CAfile;
  gchar *ciphers;
#ifdef USE_PKCS11
  gboolean pkcs11;
  gchar *pkcs11_lib;
  gchar *slot;
#endif
#endif
};

gboolean boot_dialog_run ();
void boot_dialog_term ();

void boot_property_config_to_property (BootProperty *self);
void boot_property_inspect (BootProperty * self, FILE *fp);

#endif /* #ifndef __BOOTDIALOG_H_INCLUDED__ */

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
