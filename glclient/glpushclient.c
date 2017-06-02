#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>

#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <uuid/uuid.h>
#include <json.h>
#include <glib.h>

#include "glclient.h"
#include "protocol.h"
#include "logger.h"
#include "desktop.h"
#include "gettext.h"
#include "logger.h"
#include "utils.h"

#define BUF_SIZE (10*1024)

static volatile int force_exit;

/*option*/
static char *PusherURI;
static char *RestURI;
static char *ConfigName;
static char *SessionID;
static char *TempDir;
static char *LogFile;
static char *SubscribeID;

static int
websocket_write_back(
	struct lws *wsi_in, 
	char *str, 
	int str_size_in) 
{
	int n;
	int len;
	char *out = NULL;

	if (str == NULL || wsi_in == NULL) {
		return -1;
	}
	
	if (str_size_in < 1) {
		len = strlen(str);
	} else {
		len = str_size_in;
	}
	
	out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
	memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );
	n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
	free(out);
	
	return n;
}

static void
print_report(
	json_object *obj)
{
	json_object *child;
	char *printer,*oid,*title;
	gboolean showdialog;
#if 0
	LargeByteString *lbs;
#endif

	printer = NULL;
	title = "";
	showdialog = FALSE;

	child = json_object_object_get(obj,"object_id");
	if (!CheckJSONObject(child,json_type_string)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	child = json_object_object_get(obj,"printer");
	if (CheckJSONObject(child,json_type_string)) {
		printer = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"title");
	if (CheckJSONObject(child,json_type_string)) {
		title = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"showdialog");
	if (CheckJSONObject(child,json_type_boolean)) {
		showdialog = json_object_get_boolean(child);
	}
	if (getenv("GLCLIENT_PRINTREPORT_SHOWDIALOG")!=NULL) {
		showdialog = TRUE;
	}
	if (oid == NULL || strlen(oid) <= 0) {
		return;
	}
	if (printer == NULL || strlen(printer) <=0 ) {
		showdialog = TRUE;
	}

fprintf(stderr,"\nreport title:%s printer:%s showdialog:%d oid:%s\n",title,printer,(int)showdialog,oid);

#if 0
	lbs = REST_GetBLOB(oid);
	if (lbs != NULL) {
		if (LBS_Size(lbs) > 0) {
			if (showdialog) {
				ShowPrintDialog(title,lbs);
			} else {
				Print(oid,title,printer,lbs);
			}
		}
		FreeLBS(lbs);
	}
#endif
}

static void
download_file(
	json_object *obj)
{
	json_object *child;
	char *oid,*filename,*desc;
#if 0
	LargeByteString *lbs;
#endif

	filename = "";
	desc = "";

	child = json_object_object_get(obj,"object_id");
	if (!CheckJSONObject(child,json_type_string)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	child = json_object_object_get(obj,"filename");
	if (CheckJSONObject(child,json_type_string)) {
		filename = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"description");
	if (CheckJSONObject(child,json_type_string)) {
		desc = (char*)json_object_get_string(child);
	}

	if (oid == NULL || strlen(oid) <= 0) {
		return;
	}

fprintf(stderr,"\nmisc filename:%s description:%s oid:%s\n",filename,desc,oid);

#if 0
	lbs = REST_GetBLOB(oid);
	if (lbs != NULL) {
		if (LBS_Size(lbs) > 0) {
			ShowDownloadDialog(NULL,filename,desc,lbs);
		}
		FreeLBS(lbs);
	}
#endif
}

static void
client_data_ready_handler(
	json_object *obj)
{
	json_object *child;
	char *type;

	child = json_object_object_get(obj,"type");
	if (!CheckJSONObject(child,json_type_string)) {
		fprintf(stderr,"%s:%d type not found\n",__FILE__,__LINE__);
		return;
	}
	type = json_object_get_string(child);
	if (type != NULL) {
		if (!strcmp("report",type)) {
			print_report(obj);
		} else if (!strcmp("misc",type)) {
			download_file(obj);
		}
	}
}

static void
message_handler(
	char *in)
{
	json_object *obj,*child,*data;
	char *command,*event;

	obj = json_tokener_parse(in);
	if (!CheckJSONObject(obj,json_type_object)) {
		fprintf(stderr,"%s:%d invalid json message\n",__FILE__,__LINE__);
		return;
	}
	child = json_object_object_get(obj,"command");
	if (!CheckJSONObject(child,json_type_string)) {
		fprintf(stderr,"%s:%d command not found\n",__FILE__,__LINE__);
		goto message_handler_error;
	}
	command = json_object_get_string(child);
	if (command == NULL) {
		fprintf(stderr,"%s:%d command null\n",__FILE__,__LINE__);
		goto message_handler_error;
	}
	if (!strcmp(command,"subscribed")) {
	    child = json_object_object_get(obj,"sub.id");
    	if (!CheckJSONObject(child,json_type_string)) {
    	    fprintf(stderr,"%s:%d sub.id not found\n",__FILE__,__LINE__);
			goto message_handler_error;
    	}
		subid = strdup(json_object_get_string(child));
	} else if (!strcmp(command,"event")) {
	    data = json_object_object_get(obj,"data");
    	if (!CheckJSONObject(data,json_type_object)) {
    	    fprintf(stderr,"%s:%d data not found\n",__FILE__,__LINE__);
			goto message_handler_error;
    	}

	    child = json_object_object_get(data,"event");
    	if (!CheckJSONObject(child,json_type_string)) {
    	    fprintf(stderr,"%s:%d event not found\n",__FILE__,__LINE__);
			goto message_handler_error;
    	}
		event = json_object_get_string(child);

	    child = json_object_object_get(data,"body");
    	if (!CheckJSONObject(child,json_type_object)) {
    	    fprintf(stderr,"%s:%d body not found\n",__FILE__,__LINE__);
			goto message_handler_error;
    	}

fprintf(stderr,"event:%s\n",event);
		if (!strcmp(event,"client_data_ready")) {
			client_data_ready_handler(child);
		}
	}
message_handler_error:
	json_object_put(obj);
	return;
}

static int
callback_push_receive(
	struct lws *wsi,
	enum lws_callback_reasons reason,
	void *lws_user,
	void *in,
	size_t len)
{
	char buf[BUF_SIZE];

	switch (reason) {

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		fprintf(stderr,"LWS_CALLBACK_CLIENT_ESTABLISHED\n");
		/* subscribe */
		snprintf(buf,sizeof(buf)-1,"{\"command\":\"subscribe\",\"req.id\":\"%s\",\"event\":\"*\"}",reqid);
		websocket_write_back(wsi, buf, -1);
		break;

	case LWS_CALLBACK_CLOSED:
		fprintf(stderr,"LWS_CALLBACK_CLOSED\n");
		wsi_pr = NULL;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		fprintf(stderr,"LWS_CALLBACK_CLIENT_RECEIVE\n");
		fprintf(stderr,"%s\n", (char *)in);
		message_handler(in);

		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		if (wsi == wsi_pr) {
			fprintf(stderr,"LWS_CALLBACK_CLIENT_CONNECTION_ERROR:%s\n",(char*)in);
			wsi_pr = NULL;
		}
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		fprintf(stderr,"LWS_CALLBACK_CLIENT_WRITEABLE\n");
		break;

	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		{
			char **p,*userpass,*b64;

			p = (char **)in;
			if (len < 100) {
			    return 1;
			}
			if (user == NULL || pass == NULL) {
				return 0;
			}
			userpass = g_strdup_printf("%s:%s",user,pass);
			b64 = g_base64_encode(userpass,strlen(userpass));
			g_free(userpass);
			*p += sprintf(*p, "Authorization: Basic %s\x0d\x0a",b64);
			g_free(b64);
		}
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */
static struct lws_protocols protocols[] = {
	{
		"",
		callback_push_receive,
		0,
		BUF_SIZE,
	},
	{ NULL, NULL, 0, 0 } /* end */
};

static const struct lws_extension exts[] = {
	{ NULL, NULL, NULL /* terminator */ }
};

static int
ratelimit_connects(
	unsigned int *last, 
	unsigned int secs)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (tv.tv_sec - (*last) < secs)
		return 0;

	*last = tv.tv_sec;

	return 1;
}

static void
Init()
{
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	ConfDir =  g_strconcat(g_get_home_dir(), "/.glclient", NULL);

	PusherURI  = NULL;
	RestURI    = NULL;
	ConfigName = NULL;
	SessionID  = NULL;
	TempDir    = NULL;
	LogFile    = NULL;

	gl_config_init();

	if (PusherURI == NULL || RestURI == NULL || ConfigName == NULL) {
		fprintf(stderr,"puhser-uri or rest-uri or config-name = null\n");
		exit(1);
	}
	InitLogger_via_FileName(LogFile);
	InitDesktio();
}

static void
Execute()
{
}

static GOptionEntry entries[] =
{
	{ "pusher-uri",'p',0,G_OPTION_ARG_STRING,&PusherURI,
		"pusher uri",NULL},
	{ "rest-uri",'r',0,G_OPTION_ARG_STRING,&RestURI,
		"rest uri root",NULL},
	{ "config",'c',0,G_OPTION_ARG_STRING,&ConfigName,
		"config name",NULL},
	{ "sessionid",'i',0,G_OPTION_ARG_STRING,&SessionID,
		"session id",NULL},
	{ "tempdir",'t',0,G_OPTION_ARG_STRING,&TempDir,
		"temporary directory",NULL},
	{ "logfile",'l',0,G_OPTION_ARG_STRING,&LogFile,
		"logfile name",NULL},
	{ NULL}
};

int
main(
	int argc,
	char **argv)
{
	GOptionContext *ctx;

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

	Init();
	Execute();
	return 0;
}



	int n = 0, ret = 0 , use_ssl = 0, ietf_version = -1;
	unsigned int rl_pr = 0;
	struct lws_context_creation_info info;
	struct lws_client_connect_info i;
	struct lws_context *context;
	const char *prot, *p;
	char path[300], uri[256];
	uuid_t u;

	uuid_generate(u);
	uuid_unparse(u,reqid);

#if 0
	snprintf(uri,sizeof(uri)-1,"wss://sms.orca-ng.org:9400/ws");
	snprintf(uri,sizeof(uri)-1,"wss://pusher-proxy.orca.orcamo.jp:443/ws");
#endif
	snprintf(uri,sizeof(uri)-1,"ws://localhost:9400/ws");

#if 0
	user = NULL;
	pass = NULL;
#else
	user = "user";
	pass = "pass";
#endif

	memset(&info, 0, sizeof info);
	memset(&i, 0, sizeof(i));

	i.port = 9400;
	if (lws_parse_uri((char*)uri, &prot, &i.address, &i.port, &p)) {
		fprintf(stderr,"lws_parse_uri error\n");
		exit(1);
	}

	/* add back the leading / on path */
	path[0] = '/';
	strncpy(path + 1, p, sizeof(path) - 2);
	path[sizeof(path) - 1] = '\0';
	i.path = path;

	if (!strcmp(prot, "http") || !strcmp(prot, "ws")) {
		use_ssl = 0;
	}
	if (!strcmp(prot, "https") || !strcmp(prot, "wss")) {
		use_ssl = 1;
	}

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	info.ssl_ca_filepath          = "ca.crt";
	info.ssl_cert_filepath        = "client.crt";
	info.ssl_private_key_filepath = "client.pem";
	info.ssl_private_key_password = "pgrrTA3Y9UHepQGM";
	info.ssl_private_key_password = "";

	context = lws_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		return 1;
	}

	i.context = context;
	i.ssl_connection = use_ssl;
	i.host = i.address;
	i.origin = i.address;
	i.ietf_version_or_minus_one = ietf_version;
	i.client_exts = exts;

	while (!force_exit) {

		if (!wsi_pr && ratelimit_connects(&rl_pr, 2u)) {
			lwsl_notice("connecting\n");
			i.protocol = protocols[0].name;
			wsi_pr = lws_client_connect_via_info(&i);
		}

		lws_service(context, 500);
	}

	fprintf(stderr,"Exiting\n");
	lws_context_destroy(context);

	return ret;
}
