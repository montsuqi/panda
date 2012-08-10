#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef _EVENT_HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif

#define MAIN

#include	"gettext.h"
#include	"const.h"
#include	"dirs.h"
#include	"socket.h"
#include	"port.h"
#include	"auth.h"
#include	"authstub.h"
#include	"monapi.h"
#include	"blobreq.h"
#include	"DDparser.h"
#include	"RecParser.h"
#include	"message.h"
#include	"debug.h"

static URL Auth;
static int ServicePort = 8888;
static char *PortSysData = SYSDATA_PORT;
static char *AuthURL = "glauth://localhost:" PORT_GLAUTH;
static GOptionEntry entries[] =
{
	{ "port",'p',0,G_OPTION_ARG_INT,&ServicePort,"waiting port",NULL},
	{ "sysdata",'s',0,G_OPTION_ARG_STRING,&PortSysData,"sysdata port",NULL},
	{ "auth",'a',0,G_OPTION_ARG_STRING,&AuthURL,"auth server",NULL},
	{ "record",'r',0,G_OPTION_ARG_STRING,&RecordDir,"record directory",NULL},
	{ NULL}
};

#define HTTP_CONTINUE 100
#define HTTP_SWITCHING_PROTOCOLS 101
#define HTTP_PROCESSING 102

#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_ACCEPTED 202
#define HTTP_NON_AUTHORITATIVE_INFORMATION 203
#define HTTP_NO_CONTENT 204
#define HTTP_RESET_CONTENT 205
#define HTTP_PARTIAL_CONTENT 206
#define HTTP_MULTI_STATUS 207

#define HTTP_MULTIPLE_CHOICES 300
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_FOUND 302
#define HTTP_SEE_OTHER 303
#define HTTP_NOT_MODIFIED 304
#define HTTP_USE_PROXY 305
#define HTTP_SWITCH_PROXY 306
#define HTTP_TEMPORARY_REDIRECT 307

#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_PAYMENT_REQUIRED 402
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_METHOD_NOT_ACCEPTABLE 406
#define HTTP_PROXY_AUTHENTICATION_REQUIRED 407
#define HTTP_REQUEST_TIMEOUT 408
#define HTTP_CONFLICT 409
#define HTTP_GONE 410
#define HTTP_LENGTH_REQUIRED 411
#define HTTP_PRECONDITION_FAILED 412
#define HTTP_REQUEST_ENTITY_TOO_LARGE 413
#define HTTP_REQUEST_URI_TOO_LONG 414
#define HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_REQUESTED_RANGE_NOT_SATISFIABLE 416
#define HTTP_EXPECTATION_FAILED 417
#define HTTP_UNPROCESSABLE_ENTITY 422
#define HTTP_LOCKED 423
#define HTTP_FAILED_DEPENDENCY 424
#define HTTP_UNORDERED_COLLECTION 425
#define HTTP_UPGRADE_REQUIRED 426
#define HTTP_RETRY_WITH 449

#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_NOT_IMPLEMENTED 501
#define HTTP_BAD_GATEWAY 502
#define HTTP_SERVICE_UNAVAILABLE 503
#define HTTP_GATEWAY_TIMEOUT 504
#define HTTP_HTTP_VERSION_NOT_SUPPORTED 505
#define HTTP_VARIANT_ALSO_NEGOTIATES 506
#define HTTP_INSUFFICIENT_STORAGE 507
#define HTTP_BANDWIDTH_LIMIT_EXCEEDED 509
#define HTTP_NOT_EXTENDED 510

#define HTTP_CODE2REASON(x,y) case x: return y; break;

char *
GetReasonPhrase(
	int code)
{
	switch(code) {
HTTP_CODE2REASON(HTTP_CONTINUE,"Continue")
HTTP_CODE2REASON(HTTP_SWITCHING_PROTOCOLS,"Switching Protocols")
HTTP_CODE2REASON(HTTP_PROCESSING,"Processing")
HTTP_CODE2REASON(HTTP_OK,"OK")
HTTP_CODE2REASON(HTTP_CREATED,"Created")
HTTP_CODE2REASON(HTTP_ACCEPTED,"Accepted")
HTTP_CODE2REASON(HTTP_NON_AUTHORITATIVE_INFORMATION,"Non-Authoritative Information")
HTTP_CODE2REASON(HTTP_NO_CONTENT,"No Content")
HTTP_CODE2REASON(HTTP_RESET_CONTENT,"Reset Content")
HTTP_CODE2REASON(HTTP_PARTIAL_CONTENT,"Partial Content")
HTTP_CODE2REASON(HTTP_MULTI_STATUS,"Multiple Choices")
HTTP_CODE2REASON(HTTP_MULTIPLE_CHOICES,"Multiple Choices")
HTTP_CODE2REASON(HTTP_MOVED_PERMANENTLY,"Moved Permanently")
HTTP_CODE2REASON(HTTP_FOUND,"Found")
HTTP_CODE2REASON(HTTP_SEE_OTHER,"See Other")
HTTP_CODE2REASON(HTTP_NOT_MODIFIED,"Not Modified")
HTTP_CODE2REASON(HTTP_USE_PROXY,"Use Proxy")
HTTP_CODE2REASON(HTTP_SWITCH_PROXY,"Switch Proxy")
HTTP_CODE2REASON(HTTP_TEMPORARY_REDIRECT,"Temporary Redirect")
HTTP_CODE2REASON(HTTP_BAD_REQUEST,"Bad Request")
HTTP_CODE2REASON(HTTP_UNAUTHORIZED,"Unauthorized")
HTTP_CODE2REASON(HTTP_PAYMENT_REQUIRED,"Payment Required")
HTTP_CODE2REASON(HTTP_FORBIDDEN,"Forbidden")
HTTP_CODE2REASON(HTTP_NOT_FOUND,"Not Found")
HTTP_CODE2REASON(HTTP_METHOD_NOT_ALLOWED,"Not Allowed")
HTTP_CODE2REASON(HTTP_METHOD_NOT_ACCEPTABLE,"Not Acceptable")
HTTP_CODE2REASON(HTTP_PROXY_AUTHENTICATION_REQUIRED,"Proxy Authentication Required")
HTTP_CODE2REASON(HTTP_REQUEST_TIMEOUT,"Request Timeout")
HTTP_CODE2REASON(HTTP_CONFLICT,"Conflict")
HTTP_CODE2REASON(HTTP_GONE,"Gone")
HTTP_CODE2REASON(HTTP_LENGTH_REQUIRED,"Length Required")
HTTP_CODE2REASON(HTTP_PRECONDITION_FAILED,"Precondition Failed")
HTTP_CODE2REASON(HTTP_REQUEST_ENTITY_TOO_LARGE,"Request Entity Too Large")
HTTP_CODE2REASON(HTTP_REQUEST_URI_TOO_LONG,"Request URI Too Long")
HTTP_CODE2REASON(HTTP_UNSUPPORTED_MEDIA_TYPE,"Unsupported Media Type")
HTTP_CODE2REASON(HTTP_REQUESTED_RANGE_NOT_SATISFIABLE,"Requested Range Not Satisfiable")
HTTP_CODE2REASON(HTTP_EXPECTATION_FAILED,"Expectation Failed")
HTTP_CODE2REASON(HTTP_UNPROCESSABLE_ENTITY,"Unprocessable Entity")
HTTP_CODE2REASON(HTTP_LOCKED,"Locked")
HTTP_CODE2REASON(HTTP_FAILED_DEPENDENCY,"Failed Dependency")
HTTP_CODE2REASON(HTTP_UNORDERED_COLLECTION,"Unordered Collection")
HTTP_CODE2REASON(HTTP_UPGRADE_REQUIRED,"Upgrade Required")
HTTP_CODE2REASON(HTTP_RETRY_WITH,"Retry With")
HTTP_CODE2REASON(HTTP_INTERNAL_SERVER_ERROR,"Internal Server Error")
HTTP_CODE2REASON(HTTP_NOT_IMPLEMENTED,"Not Implemented")
HTTP_CODE2REASON(HTTP_BAD_GATEWAY,"Bad Gateway")
HTTP_CODE2REASON(HTTP_SERVICE_UNAVAILABLE,"Service Unavailable")
HTTP_CODE2REASON(HTTP_GATEWAY_TIMEOUT,"Gateway Timeout")
HTTP_CODE2REASON(HTTP_HTTP_VERSION_NOT_SUPPORTED,"HTTP Version Not Supported")
HTTP_CODE2REASON(HTTP_VARIANT_ALSO_NEGOTIATES,"Variant Also Negotiates")
HTTP_CODE2REASON(HTTP_INSUFFICIENT_STORAGE,"Insufficient Storage")
HTTP_CODE2REASON(HTTP_BANDWIDTH_LIMIT_EXCEEDED,"Bandwidth Limit Exceeded")
HTTP_CODE2REASON(HTTP_NOT_EXTENDED,"Not Extended")
	default:
		return "Unsupported HTTP Code";
		break;
	}
}

typedef struct {
	struct evhttp_request *evhttp;
	int			           status;
	char                   *method;
	MonAPIData             *mondata;
	NETFILE		           *fpSysData;
} API_REQUEST;

API_REQUEST *
NewAPIRequest(
	struct evhttp_request *evhttp)
{
	API_REQUEST *req;
	Port *port;
	int fd;

	req = New(API_REQUEST);
	req->evhttp = evhttp;
	req->status = HTTP_OK;
	req->mondata = NewMonAPIData();

	port = ParPort(PortSysData, SYSDATA_PORT);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if ( fd > 0 ){
		req->fpSysData = SocketToNet(fd);
	} else {
		Error("cannot connect sysdata");
	}
	return req;
}

static void
FreeAPIRequest(API_REQUEST *req)
{
	CloseNet(req->fpSysData);
	FreeMonAPIData(req->mondata);
	free(req);
}

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void
dump_request_cb(struct evhttp_request *req, void *arg)
{
	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;

	switch (evhttp_request_get_command(req)) {
	case EVHTTP_REQ_GET: cmdtype = "GET"; break;
	case EVHTTP_REQ_POST: cmdtype = "POST"; break;
	case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
	case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
	case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
	case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
	case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
	case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
	case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
	default: cmdtype = "unknown"; break;
	}

	printf("Received a %s request for %s\nHeaders:\n",
	    cmdtype, evhttp_request_get_uri(req));

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		printf("  %s: %s\n", header->key, header->value);
	}
}

void
SendResponse(
	API_REQUEST *req)
{
	struct evbuffer *evb;
	unsigned char *body;
	size_t size;
	ValueStruct *value;
	ValueStruct *vstatus;
	ValueStruct *vctype;
	ValueStruct *vbody;
	MonObjectType obj;

	size = 0;
	body = NULL;
	obj = GL_OBJ_NULL;

	if (req->mondata->rec != NULL) {
        value = req->mondata->rec->value;
		vstatus = GetItemLongName(value,"httpstatus");
		if (vstatus != NULL) {
			req->status = ValueInteger(vstatus);
		} else {
			req->status = HTTP_OK;
		}
		vctype = GetItemLongName(value,"content_type");
		if (vctype != NULL) {
			evhttp_add_header(evhttp_request_get_output_headers(req->evhttp),
				"Content-Type",ValueToString(vctype,NULL));
		}
		if (req->status == HTTP_OK) {
			vbody = GetItemLongName(value, "body");
			if (vbody != NULL) {
				obj = ValueObjectId(vbody);
			}
			if (obj != GL_OBJ_NULL) {
				RequestReadBLOB(req->fpSysData, obj, &body, &size);
			}
		}
	}
	if (req->status == HTTP_OK) {
		if (body != NULL && size > 0) {
			evb = evbuffer_new();
			evbuffer_add(evb,body,size);
			evhttp_send_reply(req->evhttp,200,"OK",evb);
			evbuffer_free(evb);
			xfree(body);
		} else {
			evhttp_send_reply(req->evhttp,200,"OK",NULL);
		}
	} else {
		if (req->status == HTTP_UNAUTHORIZED) {
			evhttp_add_header(evhttp_request_get_output_headers(req->evhttp),
				"WWW-Authenticate","Basic realm=\"glserver\"");
		}
		evhttp_send_error(req->evhttp,req->status,GetReasonPhrase(req->status));
	}
	FreeAPIRequest(req);
}

#define MAX_REQBODY_SIZE 1024*1024+1

static void
CheckLD(
	API_REQUEST *req)
{
	const struct evhttp_uri *uri;
	const char *path;
	gchar **strs;
	gchar **list;
	gchar *fname;
	int i;

	uri = evhttp_request_get_evhttp_uri(req->evhttp);
	path = evhttp_uri_get_path(uri);
	strs = list = g_strsplit(path,"/",-1);
	for(i=0;*list!=NULL;i++,list++) {
		/* path -> /ld/window  */
		if (i==1)strncpy(req->mondata->ld,*list,sizeof(req->mondata->ld));
		if (i==2)strncpy(req->mondata->window,*list,sizeof(req->mondata->window));
	}
	g_strfreev(strs);
	if (req->mondata->ld == NULL || req->mondata->window == NULL) {
		req->status = HTTP_BAD_REQUEST; 
	}

	fname = g_strdup_printf("%s/%s.rec",RecordDir,req->mondata->window);
	if ((req->mondata->rec = ParseRecordFile(fname)) == NULL) {
		req->status = HTTP_NOT_FOUND;
	}
	g_free(fname);
}


static void
PackMonAPIData(
	API_REQUEST *req)
{
	const struct evhttp_uri *uri;
	const char *querystr;
	struct evkeyvalq queries;
	struct evkeyval *query;
	char lname[256];

	struct evkeyvalq *headers;
	const char *ctype;

	struct evbuffer *buf;
	ValueStruct *v;

	size_t size;
	char *body;
	MonObjectType obj;

	v = req->mondata->rec->value;

	SetValueString(GetItemLongName(v,"methodtype"),req->method,NULL);

	headers = evhttp_request_get_input_headers(req->evhttp);
	ctype = evhttp_find_header(headers,"content-type");

	if (ctype != NULL) {
		SetValueString(GetItemLongName(v,"content_type"),(char*)ctype,NULL);
	}

	/* pack query data */
	uri = evhttp_request_get_evhttp_uri(req->evhttp);
	querystr = evhttp_uri_get_query(uri);

	if (evhttp_parse_query_str(querystr,&queries) != -1) {
		for (query = queries.tqh_first; query;
		    query = query->next.tqe_next) {
			snprintf(lname,sizeof(lname),"argument.%s",query->key);
			SetValueString(GetItemLongName(v,lname),query->value,NULL);
		}
	}

	/* pack input data  */
	size = 0;
	buf = evhttp_request_get_input_buffer(req->evhttp);
	size = evbuffer_get_length(buf);
	if (size > MAX_REQBODY_SIZE) {
		req->status = HTTP_REQUEST_ENTITY_TOO_LARGE;
		return;
	}
	body = xmalloc(size);
	evbuffer_remove(buf, body, size);
	ValueObjectId(GetItemLongName(v,"body")) = obj = RequestNewBLOB(req->fpSysData,BLOB_OPEN_WRITE);
fprintf(stderr,"obj[%d]\n",(int)obj);
	if (obj != GL_OBJ_NULL) {
		RequestWriteBLOB(req->fpSysData,obj,body,size);
	}
	xfree(body);
}

static void
CheckAuth(
	API_REQUEST *req)
{
	struct evkeyvalq *headers;
	const char *authstr;
	char *head;
	char *tail;
	char *dec;
	char *user;
	char *pass;
	char *addr;
	ev_uint16_t port;
	gsize size;

	headers = evhttp_request_get_input_headers(req->evhttp);
	authstr = evhttp_find_header(headers,"authorization");

	if (authstr == NULL) {
		req->status = HTTP_UNAUTHORIZED;
		return;
	}

	head = (char*)authstr;
	tail = strstr(head, "Basic");
	if (tail == NULL) {
		req->status = HTTP_UNAUTHORIZED;
		Message("does not support Authorization method:%s", head);
		return;
	}
	head = tail + strlen("Basic");
	while (head[0] == ' ') { head++; }
	dec = (char *)g_base64_decode(head, &size);
	if (size <= 0 || dec == NULL) {
		req->status = HTTP_UNAUTHORIZED;
		Message("can not base64_decode :%s", head);
		return;
	}

	tail = strstr(dec, ":");
	if (tail == NULL) {
		req->status = HTTP_FORBIDDEN;
		Message("Invalid Basic Authorization data:%s", dec);
		g_free(dec);
		return;
	}
	user = g_strndup(dec, tail - dec);
	strncpy(req->mondata->user,user,sizeof(req->mondata->user));
	pass = g_strndup(tail + 1, size - (tail - dec + 1));
	g_free(dec);

	evhttp_connection_get_peer(evhttp_request_get_connection(req->evhttp),&addr,&port);
	snprintf(req->mondata->term,sizeof(req->mondata->term),"%s:%d",addr,(int)port);

	if (!AuthUser(&Auth, user, pass, NULL, NULL)) {
		MessageLogPrintf("[%s@%s] Authorization Error", user,req->mondata->term);
		req->status = HTTP_FORBIDDEN;
	}
	g_free(user);
	g_free(pass);
}

static void
ParseRequest(
	API_REQUEST *req)
{
    /* check HTTP method */
	switch (evhttp_request_get_command(req->evhttp)) {
	case EVHTTP_REQ_GET: 
		req->method = "GET";
		break;
	case EVHTTP_REQ_POST: 
		req->method = "POST";
		break;
	default: 
		req->status = HTTP_BAD_REQUEST; 
		return;
	}

    /* check auth */
	CheckAuth(req);
	if (req->status != HTTP_OK) {
		return;
	}

	/* parse uri path */
	CheckLD(req);
	if (req->status != HTTP_OK) {
		return;
	}

	/* pack data*/
	PackMonAPIData(req);
}

static void
CallAPI(
	struct evhttp_request *evhttp, 
	void *arg)
{
    API_REQUEST *req;
    PacketClass result;

#if 0
	dump_request_cb(evhttp,NULL);
#endif

	req = NewAPIRequest(evhttp);
	ParseRequest(req);
    if (req->status != HTTP_OK) {
		SendResponse(req);
		return;
	}
	result = CallMonAPI(req->mondata);
	switch(result) {
	case WFC_API_OK:
		SendResponse(req);
		break;
	case WFC_API_NOT_FOUND:
		req->status = HTTP_NOT_FOUND; 
		SendResponse(req);
		break;
	default:
		req->status = HTTP_INTERNAL_SERVER_ERROR; 
		SendResponse(req);
		break;
	}
	return;
}

int
main(int argc, char **argv)
{
	GOptionContext *ctx;
	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

	RecParserInit();
	ParseURL(&Auth,AuthURL,"file");

	unsigned short port = (unsigned short)ServicePort;
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return (1);

	base = event_base_new();
	if (!base) {
		Error("Couldn't create an event_base: exiting");
	}

	/* Create a new evhttp object to handle requests. */
	http = evhttp_new(base);
	if (!http) {
		Error("couldn't create evhttp. Exiting.");
	}

	/* We want to accept arbitrary requests, so we need to set a "generic"
	 * cb.  We can also add callbacks for specific paths. */
	evhttp_set_gencb(http, CallAPI, NULL);

	/* Now we tell the evhttp what port to listen on */
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	if (!handle) {
		Error("couldn't bind to port %d. Exiting.",(int)port);
	}
	event_base_dispatch(base);

	return 0;
}
