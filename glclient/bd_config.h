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

/* gconf tool */
void gl_config_init();
int gl_config_get_config_nums();
gboolean gl_config_have_config(int i);
int gl_config_get_index();
void gl_config_set_index(int);
int gl_config_add_server();
void gl_config_del_server(int);
void gl_config_save();
void gl_config_set_string(int,const char*,const char*);
const char* gl_config_get_string(int,const char*);
void gl_config_set_int(int,const char*,int);
int gl_config_get_int(int,const char*);
void gl_config_set_boolean(int,const char*,gboolean);
gboolean gl_config_get_boolean(int,const char*);

void ListConfig();
void LoadConfig(int n);
void LoadConfigByDesc(const char *configname);

#endif /* #ifndef __BD_CONFIG_H_INCLUDED__ */

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
