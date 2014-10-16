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
#include	<string.h>
#include	<setjmp.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<glib.h>

#include	"enum.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"socket.h"
#include	"port.h"
#include	"net.h"
#include	"comm.h"
#include	"auth.h"
#include	"authstub.h"
#include	"glserver.h"
#include	"glcomm.h"
#include	"term.h"
#include	"http.h"
#include	"driver.h"
#include	"monapi.h"
#include	"blobreq.h"
#include	"message.h"
#include	"debug.h"

#define MAX_REQ_SIZE 1024*1024+1

typedef struct {
	NETFILE		*fp;
	char		host[SIZE_HOST];
	PacketClass	method;
	int			buf_size;
	char		*buf;
	char		*head;
	char		*arguments;
	int			body_size;
	char		*body;
	GHashTable	*header_hash;
	char		*user;
	char		*pass;
	char		*ld;
	char		*window;
	int			status;
	ScreenData	*scr;
	NETFILE		*fpSysData;
} HTTP_REQUEST;

HTTP_REQUEST *
HTTP_Init(
	PacketClass klass,
	NETFILE *fp)
{
	HTTP_REQUEST *req;
	Port *port;
	int fd;

	req = New(HTTP_REQUEST);
	req->fp = fp;
	RemoteIP(fp->fd,req->host,SIZE_HOST);
	req->method = klass;
	req->buf_size = 0;
	req->buf = req->head = xmalloc(sizeof(char) * MAX_REQ_SIZE);
	memset(req->buf, 0x0, MAX_REQ_SIZE);
	req->body_size = 0;
	req->body = xmalloc(sizeof(char) * MAX_REQ_SIZE);
	memset(req->body, 0x0, MAX_REQ_SIZE);
	req->arguments = NULL;
	req->header_hash = NewNameiHash();
	req->user = NULL;
	req->pass = NULL;
	req->ld = NULL;
	req->window = NULL;
	req->status = HTTP_OK;
	req->scr = NewScreenData();

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

void
SendResponse(
	HTTP_REQUEST *req,
	MonAPIData *data)
{
	char buf[1024];
	char date[50];
	unsigned char *body;
	size_t size;
	struct tm cur, *cur_p;
	time_t t = time(NULL);
	ValueStruct *vstatus;
	ValueStruct *vbody;
	MonObjectType obj = GL_OBJ_NULL;

	size = 0;
	body = NULL;

	if (data != NULL && data->value != NULL) {
		vstatus = GetItemLongName(data->value,"httpstatus");
		if (vstatus != NULL) {
			req->status = ValueInteger(vstatus);
		} else {
			req->status = HTTP_OK;
		}
		
	}

	sprintf(buf, "HTTP/1.1 %d %s\r\n", 
		req->status, GetReasonPhrase(req->status));
	Send(req->fp, buf, strlen(buf)); 
	MessageLogPrintf("[%s@%s] %s", req->user, req->host ,buf);

	gmtime_r(&t, &cur);
	cur_p = &cur;
	if (strftime(date, sizeof(date),
			"%a, %d %b %Y %H:%M:%S GMT", cur_p) != 0) {
		sprintf(buf, "Date: %s\r\n", date);
		Send(req->fp, buf, strlen(buf)); 
	}
	
	sprintf(buf, "Server: glserver/%s\r\n", VERSION);
	Send(req->fp, buf, strlen(buf));

	if (data != NULL && data->value != NULL && req->status == HTTP_OK) {
		vbody = GetItemLongName(data->value, "body");
		if (vbody != NULL) {
			obj = ValueObjectId(vbody);
		}
		dbgprintf("obj:%d GL_OBJ_NULL:%d", (int)obj, (int)GL_OBJ_NULL);
		if (obj != GL_OBJ_NULL) {
			RequestReadBLOB(req->fpSysData, obj, &body, &size);
		}
		sprintf(buf, "Content-Type: %s\r\n", 
			ValueToString(GetItemLongName(data->value,"content_type"), NULL));
		Send(req->fp, buf, strlen(buf));
	}
	if (body != NULL && size > 0) {
		sprintf(buf, "Content-Length: %ld\r\n", (long)size);
		Send(req->fp, buf, strlen(buf));
	} else {
		sprintf(buf, "Content-Length: 0\r\n");
		Send(req->fp, buf, strlen(buf));
	}

	if (req->status == HTTP_UNAUTHORIZED) {
		const char *str = "WWW-Authenticate: Basic realm=\"glserver\"\r\n";
		Send(req->fp, (char *)str, strlen(str));
	}

	Send(req->fp, "\r\n", strlen("\r\n"));
	if (body != NULL && size > 0) {
		Send(req->fp, (char *)body, size);
		xfree(body);
	}
	Flush(req->fp);
}

int
TryRecv(
	HTTP_REQUEST *req)
{
	int size;

	if (req->buf_size >= MAX_REQ_SIZE) {
		req->status = HTTP_REQUEST_ENTITY_TOO_LARGE;
		SendResponse(req, NULL);
		Error("over max request size :%d", MAX_REQ_SIZE);
	}
	size = RecvAtOnce(req->fp, 
		req->buf + req->buf_size , 
		MAX_REQ_SIZE - req->buf_size);
	ON_IO_ERROR(req->fp,badio);	
	if (size >= 0) {
		req->buf_size += size;
		req->buf[req->buf_size] = '\0';
	} else {
		Error("can't read request");
	}
	return size;
badio:
	Error("client termination");
	return 0;
}

char *
GetNextLine(HTTP_REQUEST *req)
{
	char *p;
	char *ret;
	char *head;
	int len;

	while(1) {
		if (req->buf_size > 0) {
			head = req->head;
			p = strstr(head, "\r\n");
			if (p != NULL) {
				req->head = p + 2;
				len = p - head;
				if (len <= 0) {
					return NULL;
				}
				ret = StrnDup(head, len);
				return ret;
			}
		}
		TryRecv(req);
	}
}

void
ParseReqLine(HTTP_REQUEST *req)
{
	GRegex *re;
	GMatchInfo *match;
	char *line;

	line = GetNextLine(req);
	MessageLogPrintf("%s",line);

	/*api*/
	re = g_regex_new("^(et|ost)\\s+/([a-zA-Z0-9]+)/([a-zA-Z0-9]+)(/*|\\?(\\S+))\\s",G_REGEX_CASELESS,0,NULL);
	if (g_regex_match(re,line,0,&match)) {
		req->ld = g_match_info_fetch(match,2);
		req->window = g_match_info_fetch(match,3);
		req->arguments = g_match_info_fetch(match,5);
		g_match_info_free(match);
		return;
	} else {
		Warning("Invalid HTTP Request Line:%s", line);
		req->status = HTTP_BAD_REQUEST;
	}
	g_regex_unref(re);
	free(line);
}

gboolean
ParseReqHeader(HTTP_REQUEST *req)
{
	GRegex *re;
	GMatchInfo *match;
	gchar *line,*key,*value;
	gboolean ret;

	ret = FALSE;
	line =  GetNextLine(req);
	if (line == NULL) {
		return FALSE;
	}

	re = g_regex_new("^([\\w-]+)\\s*:\\s*(.+)",0,0,NULL);
	if (g_regex_match(re,line,0,&match)) {
		key = g_match_info_fetch(match,1);
		value = g_match_info_fetch(match,2);
		g_hash_table_insert(req->header_hash, key, value);
		g_match_info_free(match);
		ret = TRUE;
	} else {
		Message("invalid HTTP Header:%s", line);
		req->status = HTTP_BAD_REQUEST;
		ret = FALSE;
	}
	xfree(line);
	g_regex_unref(re);

	return ret;
}

void
ParseReqBody(HTTP_REQUEST *req)
{
	char *value;
	size_t size,left_size;

	value = (char *)g_hash_table_lookup(req->header_hash,"Content-Length");
	if (value == NULL) {
		req->status = HTTP_BAD_REQUEST;
		Message("invalid Content-Length:%s", value);
		return;
	}
	size = (size_t)atoi(value);
	if (size <= 0) {
		req->status = HTTP_BAD_REQUEST;
		Message("invalid Content-Length:%s", value);
		return;
	}
	if (size >= MAX_REQ_SIZE) {
		req->status = HTTP_REQUEST_ENTITY_TOO_LARGE;
		Message("invalid Content-Length:%s", value);
		return;
	}
	value = (char *)g_hash_table_lookup(req->header_hash,"Content-Type");
	if (value == NULL) {
		req->status = HTTP_BAD_REQUEST;
		Message("does not have content-type");
		return;
	}

	left_size = size - (req->buf_size - (req->head - req->buf));
	if (left_size>0) {
		while(left_size>0) {
			req->buf_size += TryRecv(req);
			left_size = size - (req->buf_size - (req->head - req->buf));
		}
	}
	memcpy(req->body, req->head, size);
	req->head += size;
	req->body_size = size;

	dbgprintf("body :%s\n", req->body);
}

void
ParseReqAuth(HTTP_REQUEST *req)
{
	GRegex *re;
	GMatchInfo *match;
	gchar *head,*base64,*userpass;
	gsize size;

#ifdef	USE_SSL
	if (fSsl && fVerifyPeer){
		if (!req->fp->peer_cert) {
			MessageLog("can not get peer certificate");
			req->status = HTTP_INTERNAL_SERVER_ERROR;
			return;
		}
		req->user = GetCommonNameFromCertificate(req->fp->peer_cert);
		return;
	}
#endif

	head = (gchar *)g_hash_table_lookup(req->header_hash,"authorization");
	if (head == NULL) {
		req->status = HTTP_UNAUTHORIZED;
		Message("does not have Authorization");
		return;
	}

	userpass = NULL;
	re = g_regex_new("^basic\\s+(.+)",G_REGEX_CASELESS,0,NULL);
	if (g_regex_match(re,head,0,&match)) {
		base64 = g_match_info_fetch(match,1);
		size = strlen(base64);
	    userpass = g_base64_decode(base64,&size);
		g_match_info_free(match);
	} else {
		g_regex_unref(re);
		req->status = HTTP_UNAUTHORIZED;
		return;
	}
	g_free(base64);
	g_regex_unref(re);

	if (userpass == NULL || strlen(userpass) <= 0) {
		Warning("Invalid userpass");
		req->status = HTTP_UNAUTHORIZED;
		return;
	}

	re = g_regex_new("^(\\w+):(\\S+)",0,0,NULL);
	if (g_regex_match(re,userpass,0,&match)) {
		req->user = g_match_info_fetch(match,1);
		req->pass = g_match_info_fetch(match,2);
		g_match_info_free(match);
	} else {
		g_free(userpass);
		g_regex_unref(re);
		Warning("Invalid userpass");
		req->status = HTTP_UNAUTHORIZED;
		return;
	}
	g_free(userpass);
	g_regex_unref(re);
}

void
ParseRequest(
	HTTP_REQUEST *req)
{
	ParseReqLine(req);
	while(ParseReqHeader(req));
	if (req->method == HTTP_POST) {
		ParseReqBody(req);
	}
	ParseReqAuth(req);
}

static	void
PackRequestRecord(
	ValueStruct		*value,
	HTTP_REQUEST	*req)
{
	char *head;
	char *tail;
	char *key;
	char *val;
	char buf[SIZE_BUFF+1];
	ValueStruct *e;
	char *p;
	MonObjectType obj;

ENTER_FUNC;
	e = value;
	InitializeValue(e);

	p = NULL;
	switch(req->method) {
	case 'G':
		SetValueString(GetItemLongName(e,"methodtype"), "GET",NULL);
		break;
	case 'P':
		SetValueString(GetItemLongName(e,"methodtype"), "POST",NULL);
		break;
	}
	if ( GetItemLongName(e,"httpstatus") ){
		ValueInteger(GetItemLongName(e,"httpstatus")) = HTTP_OK;
	}
	p = (char *)g_hash_table_lookup(req->header_hash, "content-cype");
	if (p != NULL) {
		SetValueString(GetItemLongName(e, "content_type"), p, NULL);
	}
	if (req->body != NULL && req->body_size > 0) {
		obj = RequestNewBLOB(req->fpSysData,BLOB_OPEN_WRITE);
		ValueObjectId(GetItemLongName(e,"body")) = obj;
		if (obj != GL_OBJ_NULL) {
			RequestWriteBLOB(req->fpSysData, obj, req->body, req->body_size);
		}
	}

	head = req->arguments;
	while(1) {
		if (head == NULL || head == '\0') {
			return;
		}
		tail = strstr(head, "=");
		if (tail == NULL) {
			return;
		}
		key = StrnDup(head, tail - head);
		snprintf(buf, sizeof(buf), "argument.%s", key);
		xfree(key);
		head = tail + 1;
		
		tail = strstr(head, "&");
		if (tail != NULL) {
			val = StrnDup(head, tail - head);
			SetValueString(GetItemLongName(e, buf), val, NULL);
			xfree(val);
			head = tail + 1;
		} else {
			SetValueString(GetItemLongName(e, buf), head, NULL);
			head = NULL;
		}
	}
LEAVE_FUNC;
}

MonAPIData *
MakeMonAPIData(
	HTTP_REQUEST *req)
{
	MonAPIData *data;
	ValueStruct *value;

	if (RegisterWindow(req->scr,req->window) == NULL) {
		return NULL;
	}
	value = GetWindowValue(req->scr,req->window);
	InitializeValue(value);
	data = NewMonAPIData();
	data->value = value;
	strncpy(data->ld, req->ld, sizeof(data->ld));
	strncpy(data->window, req->window, sizeof(data->window));
	strncpy(data->user, req->user, sizeof(data->user));
	strncpy(data->host, req->host, sizeof(data->host));
	PackRequestRecord(value, req);
	return data;
}

static void timeout(int i)
{
	Warning("request timeout");
	exit(0);
}

static gboolean
_HTTP_Method(
	HTTP_REQUEST *req)
{
	MonAPIData *data;
	PacketClass result;

	ParseRequest(req);
	alarm(0);

	if (req->status != HTTP_OK) {
		SendResponse(req, NULL);
		return FALSE;
	}

	if (fSsl && fVerifyPeer) {
		// SSL AUTH
	} else {
		if (!AuthUser(&Auth, req->user, req->pass, "api", NULL)) {
			MessageLogPrintf("[%s@%s] Authorization Error", req->user, req->host);
			req->status = HTTP_UNAUTHORIZED;
		}
	}
	if (req->status != HTTP_OK) {
		SendResponse(req, NULL);
		return FALSE;
	}
	data = MakeMonAPIData(req);
	if (data == NULL) {
		req->status = HTTP_NOT_FOUND; 
		SendResponse(req, NULL);
		return FALSE;
	}
	result = CallMonAPI(data);
	switch(result) {
	case WFC_API_OK:
		SendResponse(req, data);
		break;
	case WFC_API_NOT_FOUND:
		req->status = HTTP_NOT_FOUND; 
		SendResponse(req, NULL);
		break;
	default:
		req->status = HTTP_INTERNAL_SERVER_ERROR; 
		SendResponse(req, NULL);
		break;
	}
	FreeMonAPIData(data);
	return TRUE;
}

static void
XFree(char **ptr)
{
	xfree(*ptr);
	*ptr = NULL;
}

static gboolean
RemoveHeader(
	gpointer key,
	gpointer value,
	gpointer data)
{
	xfree(key);
	xfree(value);
	return TRUE;
}

static gboolean
PrepareNextRequest(
	HTTP_REQUEST *req)
{
	alarm(API_TIMEOUT_SEC);
	XFree(&(req->arguments));
	XFree(&(req->user));
	XFree(&(req->pass));
	XFree(&(req->ld));
	XFree(&(req->window));
	FreeScreenData(req->scr);
	req->scr = NewScreenData();
	req->status = HTTP_OK;
	req->body_size = 0;
	g_hash_table_foreach_remove(req->header_hash, RemoveHeader, NULL);

	if (req->head != NULL && strlen(req->head)> 1) {
		req->method = req->head[0];
		req->head ++;
		req->buf_size -= strlen(req->head);
		memmove(req->buf, req->head, req->buf_size);
		memset(req->head + 1, 0, MAX_REQ_SIZE - req->buf_size);
		req->buf[req->buf_size] = '\0';
		req->head = req->buf;
	} else {
		req->buf_size = 0;
		req->head = req->buf;
		memset(req->buf , 0, MAX_REQ_SIZE);

		req->method = RecvPacketClass(req->fp);
		ON_IO_ERROR(req->fp,badio);	
	}
	return TRUE;
badio:
	return FALSE;
}

void
HTTP_Method(
	PacketClass _klass,
	NETFILE *fpComm)
{
	HTTP_REQUEST *req;
	PacketClass klass;
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));  
	sa.sa_handler = timeout;
	sa.sa_flags |= SA_RESTART;
	sigemptyset (&sa.sa_mask);
	if(sigaction(SIGALRM, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	} 

	klass = _klass;
	req = HTTP_Init(klass, fpComm);
	if (_HTTP_Method(req)) {
		do {
			if (!PrepareNextRequest(req)) {
				break;
			}
		} while(_HTTP_Method(req));
	}
}
