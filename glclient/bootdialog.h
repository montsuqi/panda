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
#ifdef USE_SSL
  gchar *key;
  gchar *cert;
  gboolean ssl;
  gchar *verifypeer;
  gchar *CApath;
  gchar *CAfile;
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
