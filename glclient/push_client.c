#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <uuid/uuid.h>
#include <json.h>
#include <sys/prctl.h>
#include <glib.h>
#include <libgen.h>
#include <libmondai.h>

#include "utils.h"
#include "logger.h"

#define BUF_SIZE		(10*1024)
#define CONN_WAIT_INIT	2
#define CONN_WAIT_MAX	600
#define PING_PERIOD		(10L)
#define PING_TIMEOUT	(30L)

static unsigned int ConnWait = CONN_WAIT_INIT;
static unsigned int fConnWarned = 0;
static struct lws *WSI;

/* option*/
static char     *GLPushAction;

/* env option*/
static char     *PusherURI;
static char     *RESTURI;
static char     *SessionID;
static char     *GroupID;
static char     *APIUser;
static char     *APIPass;
static gboolean fSSL;
static char     *Cert;
static char     *CertKey;
static char     *CertKeyPass;
static char     *CAFile;

static char     *SubID;
static char		*TempDir;

static time_t	PongLast;

static int
websocket_write_back(
	struct lws *wsi, 
	char *str, 
	int size,
	int protocol) 
{
	int n;
	int len;
	char *out = NULL;

	if (str == NULL || wsi == NULL) {
		return -1;
	}
	
	if (size < 1) {
		len = strlen(str);
	} else {
		len = size;
	}
	
	out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
	memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );
	n = lws_write(wsi, out + LWS_SEND_BUFFER_PRE_PADDING, len, protocol);
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
		Debug("%s fork",cmd);
	}
}

static void
event_handler(
	json_object *ent)
{
	json_object *obj;
	char *datafile,*lockfile,*buf,*tmp;
	int fd;
	size_t size;

	datafile = g_build_filename(TempDir,".client_data_ready.txt",NULL);
	lockfile = g_build_filename(TempDir,".client_data_ready.lock",NULL);

	fd = _flock(lockfile);
	if (g_file_get_contents(datafile,&buf,&size,NULL)) {
		tmp = realloc(buf,size+1);
		if (tmp == NULL) {
			Error("realloc(3) failure");
		} else {
			buf = tmp;
			memset(tmp+size,0,1);
		}
		obj = json_tokener_parse(buf);
		if (is_error(obj)) {
			obj = json_object_new_array();
			Warning("invalid json:|%s|",buf);
		}
		g_free(buf);
	} else {
		obj = json_object_new_array();
	}
	json_object_array_add(obj,ent);
	json_object_get(ent);
	buf = (char*)json_object_to_json_string(obj);
	g_file_set_contents(datafile,buf,strlen(buf),NULL);
	json_object_put(obj);
	_funlock(fd);

	g_free(datafile);
	g_free(lockfile);

	exec_cmd(GLPushAction);
}

static void
message_handler(
	char *in)
{
	json_object *obj,*child;
	const char *command;

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
		event_handler(obj);
	} else if (!strcmp(command,"error")) {
		WSI = NULL;
	}
message_handler_error:
	json_object_put(obj);
	return;
}

static void
warn_reconnect()
{
	json_object *obj,*data;

	if (!fConnWarned) {
		return;
	}
	fConnWarned = 0;

	obj = json_object_new_object();
	json_object_object_add(obj,"command",json_object_new_string("event"));
	data = json_object_new_object();
	json_object_object_add(obj,"command",json_object_new_string("event"));
	json_object_object_add(obj,"data",data);
	json_object_object_add(data,"body",json_object_new_object());
	json_object_object_add(data,"event",json_object_new_string("websocket_reconnect"));

	event_handler(obj);
	json_object_put(obj);
}

static void
warn_disconnect()
{
	json_object *obj,*data;

	if (fConnWarned) {
		return;
	}
	fConnWarned = 1;

	obj = json_object_new_object();
	json_object_object_add(obj,"command",json_object_new_string("event"));
	data = json_object_new_object();
	json_object_object_add(obj,"command",json_object_new_string("event"));
	json_object_object_add(obj,"data",data);
	json_object_object_add(data,"body",json_object_new_object());
	json_object_object_add(data,"event",json_object_new_string("websocket_disconnect"));

	event_handler(obj);
	json_object_put(obj);
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

	switch (reason) {

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		/* subscribe */
		uuid_generate(u);
		uuid_unparse(u,reqid);
		snprintf(buf,sizeof(buf)-1,"{"
			"\"command\"    : \"subscribe\","
			"\"req.id\"     : \"%s\","
			"\"event\"      : \"*\","
			"\"session_id\" : \"%s\""
		"}",reqid,SessionID);
		websocket_write_back(wsi,buf,-1,LWS_WRITE_TEXT);

		if (GroupID) {
			uuid_generate(u);
			uuid_unparse(u,reqid);
			snprintf(buf,sizeof(buf)-1,"{"
				"\"command\"    : \"subscribe\","
				"\"req.id\"     : \"%s\","
				"\"event\"      : \"*\","
				"\"group_id\" : \"%s\""
			"}",reqid,GroupID);
			websocket_write_back(wsi,buf,-1,LWS_WRITE_TEXT);
		}

		ConnWait = CONN_WAIT_INIT;
		PongLast = time(NULL);
		warn_reconnect();
		break;

	case LWS_CALLBACK_CLOSED:
		WSI = NULL;
		warn_disconnect();
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		Info("%s", (char *)in);
		message_handler(in);

		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		if (wsi == WSI) {
			Info("LWS_CALLBACK_CLIENT_CONNECTION_ERROR:%s",(char*)in);
			WSI = NULL;
		}
		warn_disconnect();
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
		Debug("WSPong");
		PongLast = time(NULL);
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
			*p += sprintf(*p, "Sec-WebSocket-Version: 13\x0d\x0a");
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
	time_t ping_last,ping_now;
	int ietf_version = -1;
	unsigned int conn_last = 0;
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

	ping_last = PongLast = time(NULL);

	while (1) {

		if (!WSI && ratelimit_connects(&conn_last, ConnWait)) {
			i.protocol = protocols[0].name;
			WSI = lws_client_connect_via_info(&i);
			ConnWait *= 2;
			if (ConnWait > CONN_WAIT_MAX) {
				ConnWait = CONN_WAIT_MAX;
			}
		}
		lws_service(context, 500);

		usleep(100);
		ping_now = time(NULL);
		if ((ping_now - ping_last) >= PING_PERIOD) {
			if (WSI != NULL) {
				ping_last = ping_now;
				if ((ping_now - PongLast) >= PING_TIMEOUT) {
					Warning("websocket ping timeout");
					WSI = NULL;
				} else {
					websocket_write_back(WSI,"ping",-1,LWS_WRITE_PING);
				}
			}
		}
	}

	Info("exit gl-push-client");
	lws_context_destroy(context);
}

static GOptionEntry entries[] =
{
	{ "gl-push-action",'a',0,G_OPTION_ARG_STRING,&GLPushAction,
		"specify gl-push-action command",NULL},
	{ NULL}
};

int
main(
	int argc,
	char **argv)
{
	char path[PATH_MAX];
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

	prctl(PR_SET_PDEATHSIG, SIGHUP);

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

	log = getenv("GLPUSH_LOGFILE");
	if (log != NULL) {
		InitLogger_via_FileName(log);
	} else {
		InitLogger("gl-push-client");
	}

	if (readlink("/proc/self/exe", path, sizeof(path)) <= 0){
		exit(1);
	}
	dname = dirname(path);
	GLPushAction    = g_strdup_printf("%s/gl-push-action",dname);
	if (!g_file_test(GLPushAction,G_FILE_TEST_IS_EXECUTABLE)) {
		Warning("%s does not exists",GLPushAction);
		exit(1);
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
	GroupID = getenv("GLPUSH_GROUP_ID");

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
	Info("GLPUSH_GROUP_ID:%s",GroupID);

	Cert        = getenv("GLPUSH_CERT");
	CertKey     = getenv("GLPUSH_CERT_KEY");
	CertKeyPass = getenv("GLPUSH_CERT_KEY_PASSWORD");
	CAFile      = getenv("GLPUSH_CA_FILE");
	fSSL        = Cert != NULL;

	TempDir = getenv("GLPUSH_TEMPDIR");
	if (TempDir == NULL) {
		_Error("set env GLPUSH_TEMPDIR");
	}

	Execute();

	return 0;
}
