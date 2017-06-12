#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <uuid/uuid.h>
#include <json.h>
#include <glib.h>
#include <libgen.h>
#include <libmondai.h>

#include "utils.h"
#include "logger.h"

#define BUF_SIZE (10*1024)

static volatile int force_exit;
static struct lws *wsi_pr;

/* option*/
static char     *GLPUSHPRINT;
static char     *GLPUSHDOWNLOAD;

/* env option*/
static char     *PusherURI;
static char     *RESTURI;
static char     *SessionID;
static char     *APIUser;
static char     *APIPass;
static gboolean fSSL;
static char     *Cert;
static char     *CertKey;
static char     *CertKeyPass;
static char     *CAFile;

static char     *SubID;

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
exec_cmd(
	const char *cmd)
{
	pid_t	pid;

	pid = fork();
	if (pid == 0) {
		execl(cmd,cmd,NULL);
	} else if (pid < 0) {
		Warning("fork error:%s",strerror(errno));
	} else {
		Info("%s fork",cmd);
	}
}

static void
print_report(
	json_object *obj)
{
	json_object *child;
	char *printer,*oid,*title;
	gboolean showdialog;

	printer = "";
	title = "";
	showdialog = FALSE;

	if (!json_object_object_get_ex(obj,"object_id",&child)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	if (json_object_object_get_ex(obj,"printer",&child)) {
		printer = (char*)json_object_get_string(child);
	}
	if (json_object_object_get_ex(obj,"title",&child)) {
		title = (char*)json_object_get_string(child);
	}
	if (json_object_object_get_ex(obj,"showdialog",&child)) {
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

	Info("report title:%s printer:%s showdialog:%d oid:%s",title,printer,(int)showdialog,oid);

	setenv("GLPUSH_TITLE",title,1);
	setenv("GLPUSH_PRINTER",printer,1);
	if (showdialog) {
	setenv("GLPUSH_SHOW_DIALOG","T",1);
	} else {
	setenv("GLPUSH_SHOW_DIALOG","F",1);
	}
	setenv("GLPUSH_OID",oid,1);
	exec_cmd(GLPUSHPRINT);
}

static void
download_file(
	json_object *obj)
{
	json_object *child;
	char *oid,*filename,*desc;

	filename = "";
	desc = "";

	if (!json_object_object_get_ex(obj,"object_id",&child)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	if (json_object_object_get_ex(obj,"filename",&child)) {
		filename = (char*)json_object_get_string(child);
	}
	if (json_object_object_get_ex(obj,"description",&child)) {
		desc = (char*)json_object_get_string(child);
	}

	if (oid == NULL || strlen(oid) <= 0) {
		return;
	}

	Info("misc filename:%s description:%s oid:%s",filename,desc,oid);

	setenv("GLPUSH_FILENAME",filename,1);
	setenv("GLPUSH_DESCRIPTION",desc,1);
	setenv("GLPUSH_OID",oid,1);
	exec_cmd(GLPUSHDOWNLOAD);
}

static void
client_data_ready_handler(
	json_object *obj)
{
	json_object *child;
	const char *type;

	if (!json_object_object_get_ex(obj,"type",&child)) {
		Warning("%s:%d type not found",__FILE__,__LINE__);
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
	const char *command,*event;

	obj = json_tokener_parse(in);
	if (!CheckJSONObject(obj,json_type_object)) {
		Warning("%s:%d invalid json message",__FILE__,__LINE__);
		return;
	}
	if (!json_object_object_get_ex(obj,"command",&child)) {
		Warning("%s:%d command not found",__FILE__,__LINE__);
		goto message_handler_error;
	}
	command = json_object_get_string(child);
	if (command == NULL) {
		Warning("%s:%d command null",__FILE__,__LINE__);
		goto message_handler_error;
	}
	if (!strcmp(command,"subscribed")) {
	    if (!json_object_object_get_ex(obj,"sub.id",&child)) {
    	    Warning("%s:%d sub.id not found",__FILE__,__LINE__);
			goto message_handler_error;
    	}
		SubID = strdup(json_object_get_string(child));
	} else if (!strcmp(command,"event")) {
	    if (!json_object_object_get_ex(obj,"data",&data)) {
    	    Warning("%s:%d data not found",__FILE__,__LINE__);
			goto message_handler_error;
    	}

	    if (!json_object_object_get_ex(data,"event",&child)) {
    	    Warning("%s:%d event not found",__FILE__,__LINE__);
			goto message_handler_error;
    	}
		event = json_object_get_string(child);

	    if (!json_object_object_get_ex(data,"body",&child)) {
    	    Warning("%s:%d body not found",__FILE__,__LINE__);
			goto message_handler_error;
    	}

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
	char buf[BUF_SIZE],reqid[64];
	uuid_t u;

	uuid_generate(u);
	uuid_unparse(u,reqid);

	switch (reason) {

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		Info("LWS_CALLBACK_CLIENT_ESTABLISHED\n");
		/* subscribe */
		snprintf(buf,sizeof(buf)-1,"{\"command\":\"subscribe\",\"req.id\":\"%s\",\"event\":\"*\"}",reqid);
		websocket_write_back(wsi, buf, -1);
		break;

	case LWS_CALLBACK_CLOSED:
		Info("LWS_CALLBACK_CLOSED");
		wsi_pr = NULL;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		Info("LWS_CALLBACK_CLIENT_RECEIVE");
		Info("%s", (char *)in);
		message_handler(in);

		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		if (wsi == wsi_pr) {
			Info("LWS_CALLBACK_CLIENT_CONNECTION_ERROR:%s",(char*)in);
			wsi_pr = NULL;
		}
		break;

	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		{
			char **p,*userpass,*b64;

			p = (char **)in;
			if (len < 100) {
			    return 1;
			}
			userpass = g_strdup_printf("%s:%s",APIUser,APIPass);
			b64 = g_base64_encode(userpass,strlen(userpass));
			g_free(userpass);
			*p += sprintf(*p, "Authorization: Basic %s\x0d\x0a",b64);
			*p += sprintf(*p, "X-GINBEE-TENANT-ID: 1\x0d\x0a");
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
Execute()
{
	int ietf_version = -1;
	unsigned int rl_pr = 0;
	struct lws_context_creation_info info;
	struct lws_client_connect_info i;
	struct lws_context *context;
	const char *prot, *p;
	char path[512];

	memset(&info, 0, sizeof info);
	memset(&i, 0, sizeof(i));

	i.port = 9400;
	if (lws_parse_uri(PusherURI, &prot, &i.address, &i.port, &p)) {
		_Error("lws_parse_uri error");
	}

	/* add back the leading / on path */
	path[0] = '/';
	strncpy(path + 1, p, sizeof(path) - 2);
	path[sizeof(path) - 1] = '\0';
	i.path = path;

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	if (fSSL) {
		info.ssl_ca_filepath          = CAFile;
		info.ssl_cert_filepath        = Cert;
		info.ssl_private_key_filepath = CertKey;
		info.ssl_private_key_password = CertKeyPass;
	}

	context = lws_create_context(&info);
	if (context == NULL) {
		_Error("Creating libwebsocket context failed");
	}

	i.context = context;
	i.ssl_connection = fSSL;
	i.host = i.address;
	i.origin = i.address;
	i.ietf_version_or_minus_one = ietf_version;
	i.client_exts = exts;

	while (!force_exit) {

		if (!wsi_pr && ratelimit_connects(&rl_pr, 2u)) {
			i.protocol = protocols[0].name;
			wsi_pr = lws_client_connect_via_info(&i);
		}

		lws_service(context, 500);
	}

	Info("exit gl-push-client");
	lws_context_destroy(context);
}

static GOptionEntry entries[] =
{
	{ "gl-push-print",'p',0,G_OPTION_ARG_STRING,&GLPUSHPRINT,
		"specify glprint command",NULL},
	{ "gl-push-download",'d',0,G_OPTION_ARG_STRING,&GLPUSHDOWNLOAD,
		"specify gldownload command",NULL},
	{ NULL}
};

int
main(
	int argc,
	char **argv)
{
	char *dname,*log;
	GOptionContext *ctx;
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags |= SA_RESTART;
	sigemptyset (&sa.sa_mask);
	if (sigaction(SIGCHLD, &sa, NULL) != 0) {
		fprintf(stderr,"sigaction(2) failure: %s",strerror(errno));
	}

	dname = dirname(argv[0]);
	GLPUSHPRINT    = g_strdup_printf("%s/gl-push-print",dname);
	GLPUSHDOWNLOAD = g_strdup_printf("%s/gl-push-download",dname);

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

	log = getenv("GLPUSH_LOGFILE");
	if (log != NULL) {
		InitLogger_via_FileName(log);
	} else {
		InitLogger("gl-push-client");
	}

	PusherURI = getenv("GLPUSH_PUSHER_URI");
	if (PusherURI == NULL) {
		_Error("set env GLPUSH_PUSHER_URI");
	}
	RESTURI = getenv("GLPUSH_REST_URI");
	if (RESTURI == NULL) {
		_Error("set env GLPUSH_REST_URI");
	}
	SessionID = getenv("GLPUSH_SESSION_ID");
	if (SessionID == NULL) {
		_Error("set env GLSPUSH_SESSION_ID");
	}
	APIUser = getenv("GLPUSH_API_USER");
	if (APIUser == NULL) {
		_Error("set env GLPUSH_API_USER");
	}
	APIPass = getenv("GLPUSH_API_PASSWORD");
	if (APIPass == NULL) {
		_Error("set env GLPUSH_API_PASSWORD");
	}

	Info("GLPUSH_PUSHER_URI:%s",PusherURI);
	Info("GLPUSH_REST_URI:%s",RESTURI);
	Info("GLPUSH_SESSION_ID:%s",SessionID);

	fSSL         = FALSE;
	Cert        = getenv("GLPUSH_CERT");
	CertKey     = getenv("GLPUSH_CERT_KEY");
	CertKeyPass = getenv("GLPUSH_CERT_KEY_PASSWORD");
	CAFile      = getenv("GLPUSH_CA_FILE");

	Execute();

	return 0;
}
