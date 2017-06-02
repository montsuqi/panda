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
#include	"const.h"
#include	"logger.h"
#include	"protocol.h"

static char *AUTHURI;
static char *RPCURI;
static char *RESTURI;
static char *SESSIONID;
static int  RPCID;

static char *USER;
static char *PASS;

#ifdef USE_SSL
static char *CERTFILE;
static char *KEYFILE;
static char *KEYPASS;
static char *CAFILE;
#endif


static void
StartSession(
	GLPctx *ctx)
{
	json_object *obj;

	RPC_StartSession(ctx);
	obj = json_object_new_object();
	json_object_object_add(obj,"session_id",json_object_new_string(GLP_GetSessionID(ctx)));
	json_object_object_add(obj,"app_rpc_endpoint_uri",json_object_new_string(GLP_GetRPCURI(ctx)));
	json_object_object_add(obj,"app_rest_api_uri_root",json_object_new_string(GLP_GetRESTURI(ctx)));
	printf("%s",json_object_to_json_string(obj));
	json_object_put(obj);
}

static void
GetWindow(
	GLPctx *ctx)
{
	json_object *obj;

	obj = RPC_GetWindow(ctx);
	printf("%s",json_object_to_json_string(obj));
	json_object_put(obj);
}

static void
ListDownloads(
	GLPctx *ctx)
{
	json_object *obj;

	obj = RPC_ListDownloads(ctx);
	printf("%s",json_object_to_json_string(obj));
	json_object_put(obj);
}

static void
GetMessage(
	GLPctx *ctx)
{
	char *dialog,*popup,*abort;

	RPC_GetMessage(ctx,&dialog,&popup,&abort);
	printf("%s:%s:%s\n",dialog,popup,abort);
    g_free(dialog);
    g_free(popup);
    g_free(abort);
}

static void
SendEvent(
	GLPctx *ctx,
	char *file)
{
	json_object *obj,*res;
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

	res = RPC_SendEvent(ctx,obj);
	printf("%s",json_object_to_json_string(res));
	json_object_put(obj);
	json_object_put(res);
}

static void
EndSession(
	GLPctx *ctx)
{
	RPC_EndSession(ctx);
}

static void
PrintArgError()
{
	fprintf(stderr,
		"$ cpanda [options] <command> [param.json]\n"
		"<command>: start_session | get_window | send_event | end_session\n\n"
	);
}

static GOptionEntry entries[] =
{
	{ "authuri",0,0,G_OPTION_ARG_STRING,&AUTHURI,
		"authuri",NULL},
	{ "rpcuri",0,0,G_OPTION_ARG_STRING,&RPCURI,
		"rpcuri",NULL},
	{ "resturi",0,0,G_OPTION_ARG_STRING,&RESTURI,
		"resturi",NULL},
	{ "seesionid",0,0,G_OPTION_ARG_STRING,&SESSIONID,
		"sessionid",NULL},
	{ "rpcid",0,0,G_OPTION_ARG_INT,&RPCID,
		"rpcid",NULL},
	{ "user",0,0,G_OPTION_ARG_STRING,&USER,
		"user",NULL},
	{ "pass",0,0,G_OPTION_ARG_STRING,&PASS,
		"pass",NULL},
#ifdef USE_SSL
	{ "certfile",0,0,G_OPTION_ARG_STRING,&CERTFILE,
		"certfile",NULL},
	{ "keyfile",0,0,G_OPTION_ARG_STRING,&KEYFILE,
		"keyfile",NULL},
	{ "keypass",0,0,G_OPTION_ARG_STRING,&KEYPASS,
		"keypass",NULL},
	{ "cafile",0,0,G_OPTION_ARG_STRING,&CAFILE,
		"cafile",NULL},
#endif
	{ NULL}
};

extern	int
main(
	int argc,
	char **argv)
{
	GOptionContext *goctx;
	char *command;
	GLPctx *ctx;

	setlocale(LC_CTYPE,"ja_JP.UTF-8");

	AUTHURI   = NULL;
	RPCURI    = NULL;
	RESTURI   = NULL;
	SESSIONID = NULL;
	RPCID     = 0;
	USER      = NULL;
	PASS      = NULL;
#ifdef USE_SSL
	CERTFILE  = NULL;
	KEYFILE   = NULL;
	KEYPASS   = NULL;
	CAFILE    = NULL;
#endif

	goctx = g_option_context_new("");
	g_option_context_add_main_entries(goctx, entries, NULL);
	g_option_context_parse(goctx,&argc,&argv,NULL);

	if (argc < 2) {
		PrintArgError();
		exit(1);
	}

	command = argv[1];

	ctx = InitProtocol(AUTHURI,USER,PASS);
	if (RPCURI != NULL) {
		GLP_SetRPCURI(ctx,RPCURI);
	}
	if (RESTURI != NULL) {
		GLP_SetRESTURI(ctx,RESTURI);
	}
	if (SESSIONID != NULL) {
		GLP_SetSessionID(ctx,SESSIONID);
	}
	if (RPCID != 0) {
		GLP_SetRPCID(ctx,RPCID);
	}
#ifdef USE_SSL
	if (CERTFILE != NULL && KEYFILE != NULL && KEYPASS != NULL && CAFILE != NULL) {
		GLP_SetSSL(ctx,CERTFILE,KEYFILE,KEYPASS,CAFILE);
	}
#endif

	if (!strcmp(command,"start_session")) {
		StartSession(ctx);
	} else if (!strcmp(command,"get_window")) {
		GetWindow(ctx);
	} else if (!strcmp(command,"send_event")) {
		if (argc < 3) {
			fprintf(stderr,"start_session need param.json\n");
			exit(1);
		}
		SendEvent(ctx,argv[2]);
	} else if (!strcmp(command,"end_session")) {
		EndSession(ctx);
	} else if (!strcmp(command,"list_downloads")) {
		ListDownloads(ctx);
	} else if (!strcmp(command,"get_message")) {
		GetMessage(ctx);
	} else {
		PrintArgError();
		exit(1);
	}
	FinalProtocol(ctx);
	return 0;
}
