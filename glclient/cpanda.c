/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<dirent.h>
#include	<locale.h>

#define		MAIN
#include	"glclient.h"
#include	"protocol.h"
#include	"bd_config.h"
#include	"const.h"
#include	"message.h"
#include	"debug.h"

static int conf_idx;

static void
_LoadConfig()
{
	RPCID(Session)     =        gl_config_get_int   (conf_idx,"rpc_id");
	SESSIONID(Session) = (char*)gl_config_get_string(conf_idx,"session_id");
	RPCURI(Session)    = (char*)gl_config_get_string(conf_idx,"app_rpc_endpoint_uri");
	RESTURI(Session)   = (char*)gl_config_get_string(conf_idx,"app_rest_api_uri_root");
}

static void
_UpdateConfig()
{
	gl_config_set_int   (conf_idx,"rpc_id"               ,(int)RPCID(Session));
	gl_config_set_string(conf_idx,"session_id"           ,SESSIONID(Session));
	gl_config_set_string(conf_idx,"app_rpc_endpoint_uri" ,RPCURI(Session));
	gl_config_set_string(conf_idx,"app_rest_api_uri_root",RESTURI(Session));
	gl_config_save();
}

static void
StartSession()
{
	json_object *obj;

	InitProtocol();
	RPC_StartSession();
	_UpdateConfig();

	obj = json_object_new_object();
	json_object_object_add(obj,"session_id",json_object_new_string(SESSIONID(Session)));
	json_object_object_add(obj,"app_rpc_endpoint_uri",json_object_new_string(RPCURI(Session)));
	json_object_object_add(obj,"app_rest_api_uri_root",json_object_new_string(RESTURI(Session)));
	printf("%s",json_object_to_json_string(obj));
	json_object_put(obj);

	FinalProtocol();
}

static void
GetWindow()
{
	_LoadConfig();
	InitProtocol();
	RPC_GetWindow();
	_UpdateConfig();
	printf("%s",json_object_to_json_string(SCREENDATA(Session)));
	FinalProtocol();
}

static void
SendEvent(
	char *file)
{
	json_object *obj;
	gchar *buf,*jsonstr;
	gsize size;


	if(!g_file_get_contents(file,&buf,&size,NULL)) {
		fprintf(stderr,"cant read %s\n",file);
		exit(1);
	}

	jsonstr = malloc(size+1);
	memset(jsonstr,0,size+1);
	memcpy(jsonstr,buf,size);

	obj = json_tokener_parse(jsonstr);
	if (!CheckJSONObject(obj,json_type_object)) {
		fprintf(stderr,"json parse failure %s\n",file);
		exit(1);
	}

	_LoadConfig();
	InitProtocol();
	RPC_SendEvent(obj);
	_UpdateConfig();
	printf("%s",json_object_to_json_string(SCREENDATA(Session)));
	FinalProtocol();
}

static void
EndSession()
{
	_LoadConfig();
	InitProtocol();
	RPC_EndSession();
	RPCID(Session)     = 0;
	SESSIONID(Session) = "";
	RPCURI(Session)    = "";
	RESTURI(Session)   = "";
	_UpdateConfig();
	FinalProtocol();
}

static void
PrintArgError()
{
	fprintf(stderr,
		"$ cpanda <config_name> <command> [param.json]\n"
		"<command>: start_session | get_window | send_event | end_session\n\n"
	);
	ListConfig();
}

extern	int
main(
	int argc,
	char **argv)
{
	setlocale(LC_CTYPE,"ja_JP.UTF-8");
	ConfDir =  g_strconcat(g_get_home_dir(), "/.glclient", NULL);
	gl_config_init();

	if (argc < 3) {
		PrintArgError();
		exit(1);
	}

	Session = g_new0(GLSession,1);
	RPCID(Session) = 0;
	InitMessage(NULL,NULL);

	conf_idx = GetConfigIndexByDesc(argv[1]);
	LoadConfig(conf_idx);

	if (!strcmp(argv[2],"start_session")) {
		StartSession();
	} else if (!strcmp(argv[2],"get_window")) {
		GetWindow();
	} else if (!strcmp(argv[2],"send_event")) {
		if (argc < 4) {
			fprintf(stderr,"start_session need param.json\n");
			exit(1);
		}
		SendEvent(argv[3]);
	} else if (!strcmp(argv[2],"end_session")) {
		EndSession();
	} else {
		PrintArgError();
		exit(1);
	}
	return 0;
}
