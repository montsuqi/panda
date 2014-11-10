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

static json_object *config_obj = NULL;

static	gboolean
check_json_object(
	json_object *config_obj,
	enum json_type type)
{
	if (config_obj == NULL) {
		return FALSE;
	}
	if (is_error(config_obj)) {
		return FALSE;
	}
	if (!json_object_is_type(config_obj,type)) {
		return FALSE;
	}
	return TRUE;
}

static json_object*
_new_server()
{
	json_object *child;

	child = json_object_new_object();
	json_object_object_add(child,"description",json_object_new_string("default"));
	json_object_object_add(child,"authuri",json_object_new_string("http://localhost:8000/rpc/"));
	json_object_object_add(child,"style",json_object_new_string("/usr/share/panda-client/jma-receipt.rc"));
	json_object_object_add(child,"gtkrc",json_object_new_string(""));
	json_object_object_add(child,"fontname",json_object_new_string("Takaoゴシック 10"));
	json_object_object_add(child,"mlog",json_object_new_boolean(FALSE));
	json_object_object_add(child,"keybuff",json_object_new_boolean(FALSE));
	json_object_object_add(child,"im_kana_off",json_object_new_boolean(FALSE));
	json_object_object_add(child,"user",json_object_new_string("ormaster"));
	json_object_object_add(child,"savepassword",json_object_new_boolean(TRUE));
	json_object_object_add(child,"password",json_object_new_string(""));
	json_object_object_add(child,"timer",json_object_new_boolean(TRUE));
	json_object_object_add(child,"timerperiod",json_object_new_int(1000));

	json_object_object_add(child,"ssl",json_object_new_boolean(FALSE));
	json_object_object_add(child,"cafile",json_object_new_string("/etc/ssl/certs/gl-cacert.pem"));
	json_object_object_add(child,"certfile",json_object_new_string(""));
	json_object_object_add(child,"ciphers",json_object_new_string("ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH"));
	json_object_object_add(child,"certpassword",json_object_new_string(""));
	json_object_object_add(child,"savecertpassword",json_object_new_boolean(TRUE));

	json_object_object_add(child,"pkcs11",json_object_new_boolean(FALSE));
	json_object_object_add(child,"pkcs11lib",json_object_new_string(""));

	return child;
}

static void
load_default()
{
	json_object *array;

	config_obj = json_object_new_object();
	array = json_object_new_array();
    json_object_object_add(config_obj,"list",array);
	json_object_array_add(array,_new_server());
	json_object_object_add(config_obj,"index",json_object_new_int(0));
}

static void
load_config(gchar* str)
{
	json_object *child;
	config_obj = json_tokener_parse(str);
	if (!check_json_object(config_obj,json_type_object)) {
		Error("invalid config json");
    }
	child = json_object_object_get(config_obj,"list");
	if (!check_json_object(child,json_type_array)) {
		Error("invalid config json");
    }
	child = json_object_object_get(config_obj,"index");
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

	index = json_object_object_get(config_obj,"index");
	return json_object_get_int(index);
}

void
gl_config_set_index(int i)
{
	json_object_object_add(config_obj,"index",json_object_new_int(i));
}

static json_object*
new_server(int i)
{
	json_object *child;
	char desc[256];

	sprintf(desc,"server%d",i);
	child = _new_server();
	json_object_object_add(child,"description",json_object_new_string(desc));
	return child;
}

int
gl_config_add_server()
{
	json_object *list,*server;
	int l;

	list = json_object_object_get(config_obj,"list");
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
	list = json_object_object_get(config_obj,"list");
	json_object_array_put_idx(list,i,NULL);
}

int
gl_config_get_config_nums(void)
{
	json_object *list;

	list = json_object_object_get(config_obj,"list");
	return json_object_array_length(list);
}

gboolean
gl_config_have_config(int i)
{
	json_object *list,*server;

	list = json_object_object_get(config_obj,"list");
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

	child = json_object_object_get(config_obj,"index");
	index = json_object_get_int(child);
	nindex = 0;
	list = json_object_object_get(config_obj,"list");
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
	json_object_object_add(config_obj,"list",nlist);
	json_object_object_add(config_obj,"index",json_object_new_int(nindex));

	jsonstr = json_object_to_json_string(config_obj);
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

	list = json_object_object_get(config_obj,"list");
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

	list = json_object_object_get(config_obj,"list");
	server = json_object_array_get_idx(list,i);
	if (!check_json_object(server,json_type_object)) {
		return "";
	}
	child = json_object_object_get(server,key);
	if (!check_json_object(child,json_type_string)) {
		return "";
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

	list = json_object_object_get(config_obj,"list");
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

	list = json_object_object_get(config_obj,"list");
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

	list = json_object_object_get(config_obj,"list");
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

	list = json_object_object_get(config_obj,"list");
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


extern	void
ListConfig()
{
	int i;

	for(i=0;i<gl_config_get_config_nums();i++) {
		if (gl_config_have_config(i)) {
		printf("------------------\n");
		printf("[%d]\n", i);
		printf("\tdescription:\t%s\n", 
			gl_config_get_string(i,"description"));
		printf("\tauthuri:\t%s\n", gl_config_get_string(i,"authuri"));
		printf("\tuser:\t\t%s\n",gl_config_get_string(i,"user"));
		}
	}
}

extern void
LoadConfig (
	int n)
{
	if (!gl_config_have_config(n)) {
		Warning("no server setting:%d",n);
		return;
	}

	AUTHURI(Session) = g_strdup(gl_config_get_string(n,"authuri"));
	Style = g_strdup(gl_config_get_string(n,"style"));
	Gtkrc = g_strdup(gl_config_get_string(n,"gtkrc"));
	fMlog = gl_config_get_boolean(n,"mlog");
	fKeyBuff = gl_config_get_boolean(n,"keybuff");
	User = g_strdup(gl_config_get_string(n,"user"));
	fIMKanaOff = gl_config_get_boolean(n,"im_kana_off");
	SavePass = gl_config_get_boolean(n,"savepassword");
	if (SavePass) {
		Pass = g_strdup(gl_config_get_string(n,"password"));
	} 

	fSSL = gl_config_get_boolean(n,"ssl");
	CAFile = g_strdup(gl_config_get_string(n,"cafile"));
	CertFile = g_strdup(gl_config_get_string(n,"certfile"));
	Ciphers = g_strdup(gl_config_get_string(n,"ciphers"));
	SaveCertPass = gl_config_get_boolean(n,"savecertpassword");
	if (SaveCertPass) {
		CertPass = g_strdup(gl_config_get_string(n,"certpassword"));
	}

	fPKCS11 = gl_config_get_boolean(n,"pkcs11");
	PKCS11Lib = g_strdup(gl_config_get_string(n,"pkcs11lib"));

	fTimer = gl_config_get_boolean(n,"timer");
	TimerPeriod = gl_config_get_int(n,"timerperiod");
	FontName = g_strdup(gl_config_get_string(n,"fontname"));
}

extern void
LoadConfigByDesc (
	const char *desc)
{
	int i,n = -1;

	for(i=0;i<gl_config_get_config_nums();i++) {
		if (gl_config_have_config(i)) {
			if (!strcmp(desc,gl_config_get_string(i,"description"))) {
				n = i;
				break;
			}
		}
	}
	if (n == -1) {
		Error("could not found setting:%s",desc);
	}
	LoadConfig(n);
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
