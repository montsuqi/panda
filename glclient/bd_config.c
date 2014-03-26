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
#  include <config.h>
#endif
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <json.h>

#include "glclient.h"
#include "gettext.h"
#include "bd_config.h"
#include "message.h"
#include "debug.h"

static json_object *obj = NULL;

static	gboolean
check_json_object(
	json_object *obj,
	enum json_type type)
{
	if (obj == NULL) {
		return FALSE;
	}
	if (is_error(obj)) {
		return FALSE;
	}
	if (!json_object_is_type(obj,type)) {
		return FALSE;
	}
	return TRUE;
}

#define DEFAULT_DESCRIPTION		"default"
#define DEFAULT_AUTHURI			"http://localhost:8000/rpc/"
#define DEFAULT_STYLE 			"/usr/share/panda-client/jma-receipt.rc"
#define DEFAULT_GTKRC 			""
#define DEFAULT_FONTNAME 		"Takaoゴシック 10"
#define DEFAULT_MLOG 			FALSE
#define DEFAULT_KEYBUFF 		FALSE
#define DEFAULT_IM_KANA_OFF 	FALSE
#define DEFAULT_USER 			"ormaster"
#define DEFAULT_SAVEPASSWORD 	TRUE
#define DEFAULT_PASSWORD 		""
#define DEFAULT_TIMER 			TRUE
#define DEFAULT_TIMERPERIOD		1000

static void
load_default()
{
	json_object *array,*child;

	obj = json_object_new_object();
	array = json_object_new_array();
    json_object_object_add(obj,"list",array);

	child = json_object_new_object();
	json_object_array_add(array,child);
	json_object_object_add(child,"description",json_object_new_string(DEFAULT_DESCRIPTION));
	json_object_object_add(child,"authuri",json_object_new_string(DEFAULT_AUTHURI));
	json_object_object_add(child,"style",json_object_new_string(DEFAULT_STYLE));
	json_object_object_add(child,"gtkrc",json_object_new_string(DEFAULT_GTKRC));
	json_object_object_add(child,"fontname",json_object_new_string(DEFAULT_FONTNAME));
	json_object_object_add(child,"mlog",json_object_new_boolean(DEFAULT_MLOG));
	json_object_object_add(child,"keybuff",json_object_new_boolean(DEFAULT_KEYBUFF));
	json_object_object_add(child,"im_kana_off",json_object_new_boolean(DEFAULT_IM_KANA_OFF));
	json_object_object_add(child,"user",json_object_new_string(DEFAULT_USER));
	json_object_object_add(child,"savepassword",json_object_new_boolean(DEFAULT_SAVEPASSWORD));
	json_object_object_add(child,"password",json_object_new_string(DEFAULT_PASSWORD));
	json_object_object_add(child,"timer",json_object_new_boolean(DEFAULT_TIMER));
	json_object_object_add(child,"timerperiod",json_object_new_int(DEFAULT_TIMERPERIOD));

	json_object_object_add(obj,"index",json_object_new_int(0));
}

static void
load_config(gchar* str)
{
	json_object *child;
	obj = json_tokener_parse(str);
	if (!check_json_object(obj,json_type_object)) {
		Error("invalid config json");
    }
	child = json_object_object_get(obj,"list");
	if (!check_json_object(child,json_type_array)) {
		Error("invalid config json");
    }
	child = json_object_object_get(obj,"index");
	if (!check_json_object(child,json_type_int)) {
		Error("invalid config json");
    }
}

void
gl_config_init(void) 
{
	gchar *path,*buf;
	size_t size;

	path = g_strdup_printf("%s/config.json",ConfDir);
	if (!g_file_get_contents(path,&buf,&size,NULL)) {
		load_default();
	} else {
		load_config(buf);
		g_free(buf);
	}
	g_free(path);
}

int
gl_config_get_index(void)
{
	json_object *index;

	index = json_object_object_get(obj,"index");
	return json_object_get_int(index);
}

void
gl_config_set_index(int i)
{
	json_object_object_add(obj,"index",json_object_new_int(i));
}

static json_object*
new_server(int i)
{
	json_object *child;
	char desc[256];

	sprintf(desc,"server%d",i);
	child = json_object_new_object();
	json_object_object_add(child,"description",json_object_new_string(DEFAULT_DESCRIPTION));
	json_object_object_add(child,"authuri",json_object_new_string(DEFAULT_AUTHURI));
	json_object_object_add(child,"style",json_object_new_string(DEFAULT_STYLE));
	json_object_object_add(child,"gtkrc",json_object_new_string(DEFAULT_GTKRC));
	json_object_object_add(child,"fontname",json_object_new_string(DEFAULT_FONTNAME));
	json_object_object_add(child,"mlog",json_object_new_boolean(DEFAULT_MLOG));
	json_object_object_add(child,"keybuff",json_object_new_boolean(DEFAULT_KEYBUFF));
	json_object_object_add(child,"im_kana_off",json_object_new_boolean(DEFAULT_IM_KANA_OFF));
	json_object_object_add(child,"user",json_object_new_string(DEFAULT_USER));
	json_object_object_add(child,"savepassword",json_object_new_boolean(DEFAULT_SAVEPASSWORD));
	json_object_object_add(child,"password",json_object_new_string(DEFAULT_PASSWORD));
	json_object_object_add(child,"timer",json_object_new_boolean(DEFAULT_TIMER));
	json_object_object_add(child,"timerperiod",json_object_new_int(DEFAULT_TIMERPERIOD));
	
	return child;
}

int
gl_config_add_server()
{
	json_object *list,*server;
	int l;

	list = json_object_object_get(obj,"list");
	l = json_object_array_length(list);
	server = new_server(l);
	json_object_array_add(list,server);
	return l;
}

void
gl_config_del_server(int i)
{
	json_object *list;

	if (i == 0) {
		Warning("MUST NOT delete default server settings");
		return;
	}
	list = json_object_object_get(obj,"list");
	json_object_array_put_idx(list,i,NULL);
}

int
gl_config_get_config_nums(void)
{
	json_object *list;

	list = json_object_object_get(obj,"list");
	return json_object_array_length(list);
}

gboolean
gl_config_have_config(int i)
{
	json_object *list,*server;

	list = json_object_object_get(obj,"list");
	server = json_object_array_get_idx(list,i);
	return check_json_object(server,json_type_object);
}

void
gl_config_save(void)
{
	json_object *list,*nlist,*child;
	gchar *path;
	const char *jsonstr;
	int i,j,index,nindex;

	child = json_object_object_get(obj,"index");
	index = json_object_get_int(child);
	nindex = 0;
	list = json_object_object_get(obj,"list");
	nlist = json_object_new_array();
	for(i=0,j=0;i<json_object_array_length(list);i++) {
		child = json_object_array_get_idx(list,i);
		if (child == NULL || is_error(child)) {
		} else {
			json_object_get(child); /* ref++ for child(array) put */
			json_object_array_add(nlist,child);
			if (i == index) {
				nindex = j;
			}
			j++;
		}
	}
	json_object_object_add(obj,"list",nlist);
	json_object_object_add(obj,"index",json_object_new_int(nindex));

	jsonstr = json_object_to_json_string(obj);
	path = g_strdup_printf("%s/config.json",ConfDir);
	if (!g_file_set_contents(path,jsonstr,strlen(jsonstr),NULL)) {
		Error("could not create %s",path);
	}
	g_free(path);
}

void
gl_config_set_string(
	int i,
	const char *key,
	const char *val)
{
	json_object *list,*server;

	list = json_object_object_get(obj,"list");
	server = json_object_array_get_idx(list,i);
	if (!check_json_object(server,json_type_object)) {
		return;
	}
	json_object_object_add(server,key,json_object_new_string(val));
}

const char*
gl_config_get_string(
	int i,
	const char *key)
{
	json_object *list,*server,*child;

	list = json_object_object_get(obj,"list");
	server = json_object_array_get_idx(list,i);
	if (!check_json_object(server,json_type_object)) {
		return NULL;
	}
	child = json_object_object_get(server,key);
	if (!check_json_object(child,json_type_string)) {
		return NULL;
	}
	return json_object_get_string(child);
}

void
gl_config_set_int(
	int i,
	const char *key,
	int val)
{
	json_object *list,*server;

	list = json_object_object_get(obj,"list");
	server = json_object_array_get_idx(list,i);
	if (!check_json_object(server,json_type_object)) {
		return;
	}
	json_object_object_add(server,key,json_object_new_int(val));
}

int
gl_config_get_int(
	int i,
	const char *key)
{
	json_object *list,*server,*child;

	list = json_object_object_get(obj,"list");
	server = json_object_array_get_idx(list,i);
	if (!check_json_object(server,json_type_object)) {
		return 0;
	}
	child = json_object_object_get(server,key);
	if (!check_json_object(child,json_type_int)) {
		return 0;
	}
	return json_object_get_int(child);
}

void
gl_config_set_boolean(
	int i,
	const char *key,
	gboolean val)
{
	json_object *list,*server;

	list = json_object_object_get(obj,"list");
	server = json_object_array_get_idx(list,i);
	if (!check_json_object(server,json_type_object)) {
		return;
	}
	json_object_object_add(server,key,json_object_new_boolean(val));
}

gboolean
gl_config_get_boolean(
	int i,
	const char *key)
{
	json_object *list,*server,*child;

	list = json_object_object_get(obj,"list");
	server = json_object_array_get_idx(list,i);
	if (!check_json_object(server,json_type_object)) {
		return FALSE;
	}
	child = json_object_object_get(server,key);
	if (!check_json_object(child,json_type_boolean)) {
		return FALSE;
	}
	return json_object_get_boolean(child);
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
