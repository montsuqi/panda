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

#ifndef __BD_CONFIG_H_INCLUDED__
#define __BD_CONFIG_H_INCLUDED__

#include <stdio.h>     /* FILE */
#include <glib.h>      /* gchar, gboolean */
#include <sys/types.h> /* mode_t */

typedef struct _BDConfig BDConfig;
typedef struct _BDConfigSection BDConfigSection;

extern  void     bd_config_section_inspect      (BDConfigSection *self,
                                                 FILE *fp);
extern  gchar   *bd_config_section_get_name     (BDConfigSection *self);
extern  gboolean bd_config_section_get_bool     (BDConfigSection *self,
                                                 gchar *name);
extern  gboolean bd_config_section_get_bool_default 
                                                (BDConfigSection *self,
                                                 gchar *name,
                                                 gboolean value);
extern  gchar   *bd_config_section_get_string   (BDConfigSection *self,
                                                 gchar *name);
extern  gchar   *bd_config_section_get_string_default 
                                                (BDConfigSection *self,
                                                 gchar *name,
                                                 gchar *value);
extern  gboolean bd_config_section_set_bool     (BDConfigSection *self,
                                                 gchar *name,
                                                 gboolean data);
extern  gboolean bd_config_section_set_string   (BDConfigSection *self,
                                                 gchar *name,
                                                 const gchar *str);
extern  void     bd_config_section_append_value (BDConfigSection *self,
                                                 gchar *name,
                                                 const gchar *contents);
extern  gboolean bd_config_section_set_path     (BDConfigSection *self,
                                                 gchar *name, const gchar *str);

BDConfig        *bd_config_new               ();
BDConfig        *bd_config_new_with_filename (gchar *filename);
BDConfig        *bd_config_load_file         ();
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
gchar           *bd_config_get_string        (BDConfig * self,
                                              gchar * section_name,
                                              gchar * value_name);
gboolean         bd_config_save              (BDConfig *self,
                                              gchar *filename,
                                              mode_t mode);
mode_t           bd_config_permissions       (BDConfig *self);
gboolean         bd_config_remove_section    (BDConfig *self, gchar * name);
gboolean         bd_config_move_section      (BDConfig * self, gchar * name, gint to);

#endif /* #ifndef __BD_CONFIG_H_INCLUDED__ */

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
