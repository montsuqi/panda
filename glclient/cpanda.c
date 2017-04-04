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
static int Rpc_id = 0;
static char *Session_id = NULL;
static char *Rpc_uri = NULL;
static char *Rest_uri = NULL;


static void
StartSession()
{
	json_object *obj;

	InitProtocol();
	RPC_StartSession();

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
	InitProtocol();
	RPC_GetWindow();
	printf("%s",json_object_to_json_string(SCREENDATA(Session)));
	FinalProtocol();
}

static void
ListDownloads()
{
	InitProtocol();
	RPC_ListDownloads();
	printf("%s",json_object_to_json_string(SCREENDATA(Session)));
	FinalProtocol();
}

static void
GetMessage()
{
	char *dialog,*popup,*abort;

	InitProtocol();
	RPC_GetMessage(&dialog,&popup,&abort);
	printf("%s:%s:%s\n",dialog,popup,abort);
	FinalProtocol();
    g_free(dialog);
    g_free(popup);
    g_free(abort);
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

	InitProtocol();
	RPC_SendEvent(obj);
	printf("%s",json_object_to_json_string(SCREENDATA(Session)));
	FinalProtocol();
}

static void
EndSession()
{
	InitProtocol();
	RPC_EndSession();
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

static GOptionEntry entries[] =
{
	{ "session_id",0,0,G_OPTION_ARG_STRING,&Session_id,
		"session_id",NULL},
	{ "rpc_id",0,0,G_OPTION_ARG_INT,&Rpc_id,
		"rpc_id",NULL},
	{ "rpc_uri",0,0,G_OPTION_ARG_STRING,&Rpc_uri,
		"rpc_uri",NULL},
	{ "rest_uri",0,0,G_OPTION_ARG_STRING,&Rest_uri,
		"rest_uri",NULL},
	{ NULL}
};

extern	int
main(
	int argc,
	char **argv)
{
	GOptionContext *ctx;

	setlocale(LC_CTYPE,"ja_JP.UTF-8");

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

	ConfDir =  g_strconcat(g_get_home_dir(), "/.glclient", NULL);
	gl_config_init();

	if (argc < 3) {
		PrintArgError();
		exit(1);
	}

	Session = g_new0(GLSession,1);
	InitMessage(NULL,NULL);

	RPCID(Session)     = Rpc_id;
	SESSIONID(Session) = Session_id;
	RPCURI(Session)    = Rpc_uri;
	RESTURI(Session)   = Rest_uri;

	conf_idx = GetConfigIndexByDesc(argv[1]);
	if (conf_idx == -1) {
		fprintf(stderr,"invalid config name\n");
		exit(1);
	}
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
	} else if (!strcmp(argv[2],"list_downloads")) {
		ListDownloads();
	} else if (!strcmp(argv[2],"get_message")) {
		GetMessage();
	} else {
		PrintArgError();
		exit(1);
	}
	return 0;
}
