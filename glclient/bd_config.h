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

#ifndef __BD_CONFIG_H_INCLUDED__
#define __BD_CONFIG_H_INCLUDED__

#include <stdio.h>     /* FILE */
#include <glib.h>      /* gchar, gboolean */
#include <sys/types.h> /* mode_t */

typedef struct _BDConfig BDConfig;
typedef struct _BDConfigSection BDConfigSection;

void      bd_config_section_inspect      (BDConfigSection *self,
                                          FILE *fp);
gchar    *bd_config_section_get_name     (BDConfigSection *self);
gboolean  bd_config_section_get_bool     (BDConfigSection *self,
                                          gchar *name);
gchar    *bd_config_section_get_string   (BDConfigSection *self,
                                          gchar *name);
gboolean  bd_config_section_set_bool     (BDConfigSection *self,
                                          gchar *name,
                                          gboolean bool);
gboolean  bd_config_section_set_string   (BDConfigSection *self,
                                          gchar *name,
                                          gchar *str);
void      bd_config_section_append_value (BDConfigSection *self,
                                          gchar *name,
                                          gchar *contents);

BDConfig        *bd_config_new               ();
BDConfig        *bd_config_new_with_filename (gchar *filename);
void             bd_config_free              (BDConfig *self);
void             bd_config_inspect           (BDConfig *self,
                                              FILE *fp);
gchar           *bd_config_get_filename      (BDConfig *self);
BDConfigSection *bd_config_get_section       (BDConfig *self,
                                              gchar *name);
GList           *bd_config_get_sections      (BDConfig *self);
gboolean         bd_config_exist_section     (BDConfig *self,
                                              gchar *name);
BDConfigSection *bd_config_append_section    (BDConfig *self,
                                              gchar *name);
gchar           *bd_config_get_string        (BDConfig * self,
                                              gchar * section_name,
                                              gchar * value_name);
gboolean         bd_config_save              (BDConfig *self,
                                              gchar *filename,
                                              mode_t mode);
mode_t           bd_config_permissions       (BDConfig *self);

#endif /* #ifndef __BD_CONFIG_H_INCLUDED__ */

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
