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
#include	"const.h"
#include	"message.h"
#include	"debug.h"

static char *OpURI       = NULL;
static char *OpSessionID = NULL;
static gboolean OpGinbee = FALSE;

static void
StartSession()
{
	json_object *obj;

	AUTHURI(Session) = OpURI;

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
	if (OpSessionID == NULL) {
		fprintf(stderr,"specify sessionid. use -s option\n");
		exit(1);
	}
	RPCURI(Session) = OpURI;
	SESSIONID(Session) = OpSessionID;

	InitProtocol();
	RPC_GetWindow();
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

	if (OpSessionID == NULL) {
		fprintf(stderr,"specify sessionid. use -s option\n");
		exit(1);
	}
	RPCURI(Session) = OpURI;
	SESSIONID(Session) = OpSessionID;

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
	if (OpSessionID == NULL) {
		fprintf(stderr,"specify sessionid. use -s option\n");
		exit(1);
	}
	RPCURI(Session) = OpURI;
	SESSIONID(Session) = OpSessionID;

	InitProtocol();
	RPC_EndSession();
	FinalProtocol();
}

static GOptionEntry entries[] =
{
	{ "user",'u',0,G_OPTION_ARG_STRING,&User,
		"user",NULL},
	{ "password",'p',0,G_OPTION_ARG_STRING,&Pass,
		"password",NULL},
	{ "URI",'U',0,G_OPTION_ARG_STRING,&OpURI,
		"password",NULL},
	{ "sessionid",'i',0,G_OPTION_ARG_STRING,&OpSessionID,
		"sessionid",NULL},
	{ "ginbee",'g',0,G_OPTION_ARG_NONE,&OpGinbee,
		"ginbee",NULL},
	{ NULL}
};

static void
PrintArgError()
{
	fprintf(stderr,
		"$ cpanda <command> options\n"
		"<command>: start_session | get_window | send_event | end_session\n"
	);
}

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

	Session = g_new0(GLSession,1);
	RPCID(Session) = 0;

	InitMessage(NULL,NULL);
	if (argc < 2) {
		PrintArgError();
		exit(1);
	}

	if (OpURI == NULL) {
		fprintf(stderr,"specify URL. use -U option\n");
		exit(1);
	}
	if (!strcmp(argv[1],"start_session")) {
		StartSession();
	} else if (!strcmp(argv[1],"get_window")) {
		GetWindow();
	} else if (!strcmp(argv[1],"send_event")) {
		if (argc < 3) {
			fprintf(stderr,"$ cpanda send_event param.json\n");
			exit(1);
		}
		SendEvent(argv[2]);
	} else if (!strcmp(argv[1],"end_session")) {
		EndSession();
	} else {
		PrintArgError();
		exit(1);
	}
	return 0;
}
