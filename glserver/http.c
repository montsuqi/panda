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
#include	<stdarg.h>
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
#include	<json.h>

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
#include	"term.h"
#include	"http.h"
#include	"blobreq.h"
#include	"sysdataio.h"
#include	"wfcio.h"
#include	"dirs.h"
#include	"message.h"
#include	"debug.h"

#define MAX_REQ_SIZE 1024*1024+1

#define REQUEST_TYPE_NONE        0
#define REQUEST_TYPE_JSONRPC     1
#define REQUEST_TYPE_BLOB_IMPORT 2
#define REQUEST_TYPE_BLOB_EXPORT 3
#define REQUEST_TYPE_API         4

typedef struct {
	NETFILE			*fp;
	int 			type;
	char			host[SIZE_HOST];
	char			*server_host;
	PacketClass		method;
	size_t			buf_size;
	char			*buf;
	char			*head;
	char			*arguments;
	int				body_size;
	char			*body;
	GHashTable		*header_hash;
	char			*user;
	char			*pass;
	char			*ld;
	char			*window;
	char			*session_id;
	char			*oid;
	gboolean		require_auth;
	int				status;
} HTTP_REQUEST;

HTTP_REQUEST *
HTTP_Init(
	NETFILE *fp)
{
	HTTP_REQUEST *req;

	req = New(HTTP_REQUEST);
	req->fp = fp;
	RemoteIP(fp->fd,req->host,SIZE_HOST);
	req->type = REQUEST_TYPE_NONE;
	req->method = 0;
	req->buf_size = 0;
	req->buf = req->head = xmalloc(sizeof(char) * MAX_REQ_SIZE);
	memset(req->buf, 0x0, MAX_REQ_SIZE);
	req->body_size = 0;
	req->body = NULL;
	req->arguments = NULL;
	req->header_hash = NewNameiHash();
	req->user = NULL;
	req->pass = NULL;
	req->ld = NULL;
	req->window = NULL;
	req->status = HTTP_OK;
	req->session_id = NULL;
	req->oid = 0;
	req->require_auth = FALSE;
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
	int status,
	char *body,
	size_t body_size,
	...)
{
	va_list ap;
	char buf[1024],date[50],*header,*h,*v;
	struct tm cur, *cur_p;
	time_t t = time(NULL);

	sprintf(buf, "HTTP/1.1 %d %s\r\n",status,GetReasonPhrase(status));
	Send(req->fp,buf,strlen(buf)); 
	if (fDebug) {
		Warning("%s",buf);
	}

	gmtime_r(&t, &cur);
	cur_p = &cur;
	if (strftime(date, sizeof(date),
			"%a, %d %b %Y %H:%M:%S GMT", cur_p) != 0) {
		sprintf(buf, "Date: %s\r\n", date);
		Send(req->fp, buf, strlen(buf)); 
	}
	
	sprintf(buf, "Server: glserver/%s\r\n", VERSION);
	Send(req->fp, buf, strlen(buf));

	if (body != NULL && body_size > 0) {
		sprintf(buf, "Content-Length: %zd\r\n", body_size);
		Send(req->fp, buf, strlen(buf));
	} else {
		sprintf(buf, "Content-Length: 0\r\n");
		Send(req->fp, buf, strlen(buf));
	}

	if (status == HTTP_UNAUTHORIZED) {
		const char *str = "WWW-Authenticate: Basic realm=\"glserver\"\r\n";
		Send(req->fp, (char *)str, strlen(str));
	}

	va_start(ap,NULL);
	while(1) {
		h = va_arg(ap,char*);
		if (h == NULL) {
			break;
		}
		v = va_arg(ap,char*);
		if (v == NULL) {
			break;
		}
		header = g_strdup_printf("%s: %s\r\n",h,v);
		Send(req->fp, header, strlen(header));
		g_free(header);
	}
	va_end(ap);

	Send(req->fp, "\r\n", strlen("\r\n"));
	if (body != NULL && body_size > 0) {
		Send(req->fp, (char *)body, body_size);
	}
	Flush(req->fp);
}
	
int
TryRecv(
	HTTP_REQUEST *req)
{
	int size;

	if (req->buf_size >= MAX_REQ_SIZE) {
		SendResponse(req,HTTP_REQUEST_ENTITY_TOO_LARGE,NULL,0,NULL);
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
#if 0
	Error("client termination");
#else
	exit(0);
#endif
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

	if (fDebug) {
		Warning("%s",line);
	}

	/*jsonrpc*/
	if (g_regex_match_simple("^post\\s+/rpc/*\\s",line,G_REGEX_CASELESS,0)) {
		req->type = REQUEST_TYPE_JSONRPC;
		req->method = HTTP_POST;
		free(line);
		return;
	}

	/*blob export*/
	re = g_regex_new("^get\\s+/rest/sessions/([a-zA-Z0-9-]+)/blob/(\\d+)",G_REGEX_CASELESS,0,NULL);
	if (g_regex_match(re,line,0,&match)) {
		req->session_id = g_match_info_fetch(match,1);
		req->oid = g_match_info_fetch(match,2);
		req->type = REQUEST_TYPE_BLOB_EXPORT;
		req->method = HTTP_GET;
		g_match_info_free(match);
		g_regex_unref(re);
		free(line);
		return;
	}
	g_regex_unref(re);

	/*blob import*/
	re = g_regex_new("^post\\s+/rest/sessions/([a-zA-Z0-9-]+)/blob/*\\s",G_REGEX_CASELESS,0,NULL);
	if (g_regex_match(re,line,0,&match)) {
		req->session_id = g_match_info_fetch(match,1);
		req->type = REQUEST_TYPE_BLOB_IMPORT;
		req->method = HTTP_POST;
		g_match_info_free(match);
		g_regex_unref(re);
		free(line);
		return;
	}
	g_regex_unref(re);

	/*api*/
	re = g_regex_new("^(get|post)\\s+(/rest)?/([a-zA-Z0-9]+)/([a-zA-Z0-9]+)(/*|\\?(\\S+))\\s",G_REGEX_CASELESS,0,NULL);
	if (g_regex_match(re,line,0,&match)) {
		if (*line == 'g' || *line == 'G') {
			req->method = HTTP_GET;
		} else {
			req->method = HTTP_POST;
		}
		req->ld = g_match_info_fetch(match,3);
		req->window = g_match_info_fetch(match,4);
		req->arguments = g_match_info_fetch(match,6);
		req->type = REQUEST_TYPE_API;
		req->require_auth = TRUE;
		g_match_info_free(match);
		g_regex_unref(re);
		free(line);
		return;
	}
	g_regex_unref(re);
	Warning("Invalid HTTP Request Line:%s", line);
	req->status = HTTP_BAD_REQUEST;
	free(line);
}

gboolean
ParseReqHeader(HTTP_REQUEST *req)
{
	GRegex *re;
	GMatchInfo *match;
	gchar *line,*key,*value;

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
	} else {
		Message("invalid HTTP Header:%s", line);
		req->status = HTTP_BAD_REQUEST;
		free(line);
		g_regex_unref(re);
		return FALSE;
	}
	xfree(line);
	g_regex_unref(re);

	return TRUE;
}

void
ParseReqBody(HTTP_REQUEST *req)
{
	char *value;
	size_t body_size,size,left;

	value = (char *)g_hash_table_lookup(req->header_hash,"Content-Length");
	if (value == NULL) {
		req->status = HTTP_BAD_REQUEST;
		Message("invalid Content-Length:%s", value);
		return;
	}
	body_size = (size_t)atoi(value);
	if ((body_size + req->buf_size) >= MAX_REQ_SIZE) {
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

	left = body_size - (req->buf_size - (req->head - req->buf));
	if (left > 0) {
		while (left > 0) {
			size = TryRecv(req);
			if (left < size) {
				left = 0;
			} else {
				left -= size;
			}
		}
	}

 	req->body = req->head;
	req->body_size = body_size;
	req->head += body_size;

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
			req->status = HTTP_FORBIDDEN;
			return;
		}
	}
#endif

	head = (gchar *)g_hash_table_lookup(req->header_hash,"authorization");
	if (head == NULL) {
		req->status = HTTP_UNAUTHORIZED;
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
		req->status = HTTP_FORBIDDEN;
		return;
	}

	re = g_regex_new("^(\\w+):(.*)",0,0,NULL);
	if (g_regex_match(re,userpass,0,&match)) {
		req->user = g_match_info_fetch(match,1);
		req->pass = g_match_info_fetch(match,2);
		g_match_info_free(match);
	} else {
		g_free(userpass);
		g_regex_unref(re);
		Warning("Invalid userpass");
		req->status = HTTP_FORBIDDEN;
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

	while(ParseReqHeader(req)){};

	if (req->method == HTTP_POST) {
		ParseReqBody(req);
	}
	ParseReqAuth(req);

	req->server_host = (gchar *)g_hash_table_lookup(req->header_hash,"host");
	if (req->server_host == NULL) {
		req->status = HTTP_BAD_REQUEST;
		return;
	}
}

static void timeout(int i)
{
	Warning("request timeout");
	exit(0);
}

static	json_object *
ParseReqArguments(
	char *args)
{
	json_object *obj;
	gchar **kvs,**kv;
	int i;
ENTER_FUNC;
	obj = json_object_new_object();
	if (args != NULL && strlen(args) > 0) {
		kvs = g_strsplit(args,"&",128);
		for(i=0;kvs[i]!=NULL;i++) {
			kv = g_strsplit(kvs[i],"=",2);
			if (kv[0] != NULL || kv[1] != NULL) {
				json_object_object_add(obj,kv[0],json_object_new_string(kv[1]));
			}
			g_strfreev(kv);
		}
		g_strfreev(kvs);
	}
LEAVE_FUNC;
	return obj;
}

json_object *
MakeAPIReqJSON(
	HTTP_REQUEST *req)
{
	json_object *obj,*params,*meta,*arguments;
	gchar *ctype,oid[256];
	MonObjectType mon;

	obj = json_object_new_object();
	json_object_object_add(obj,"jsonrpc",json_object_new_string("2.0"));
	json_object_object_add(obj,"id",json_object_new_int(0));
	json_object_object_add(obj,"method",json_object_new_string("panda_api"));

	params = json_object_new_object();

	meta = json_object_new_object();
	json_object_object_add(meta,"user",json_object_new_string(req->user));
	json_object_object_add(meta,"ld",json_object_new_string(req->ld));
	json_object_object_add(meta,"window",json_object_new_string(req->window));
	json_object_object_add(meta,"host",json_object_new_string(req->host));
	json_object_object_add(params,"meta",meta);

	arguments = ParseReqArguments(req->arguments);
	json_object_object_add(params,"arguments",arguments);
	if (req->method == HTTP_POST) {
		json_object_object_add(params,"http_method",json_object_new_string("POST"));
	} else {
		json_object_object_add(params,"http_method",json_object_new_string("GET"));
	}
	json_object_object_add(params,"http_status",json_object_new_int(200));
	ctype = (gchar *)g_hash_table_lookup(req->header_hash,"Content-Type");
	if (ctype) {
		json_object_object_add(params,"content_type",json_object_new_string(ctype));
	} else {
		json_object_object_add(params,"content_type",json_object_new_string(""));
	}

	mon = GL_OBJ_NULL;
	if (req->method == HTTP_POST) {
		mon = GLImportBLOB(req->body,req->body_size);
	}
	sprintf(oid,"%lu",mon);
	json_object_object_add(params,"body",json_object_new_string(oid));
	json_object_object_add(obj,"params",params);

	return obj;
}

void
APISendResponse(
	HTTP_REQUEST *req,
	json_object *obj)
{
	json_object *result,*http_status,*body,*ctype;
	int status;
	char *blob;
	size_t blob_size;
	MonObjectType mon;

	if (!CheckJSONObject(obj,json_type_object)) {
		Error("panda_api response json is invalid");
	}
	result = json_object_object_get(obj,"result");
	if (!CheckJSONObject(result,json_type_object)) {
		Error("panda_api response json result is invalid");
	}
	http_status = json_object_object_get(result,"http_status");
	if (!CheckJSONObject(http_status,json_type_int)) {
		Error("panda_api response json http_status is invalid");
	}
	status = json_object_get_int(http_status);
	if (status == 200) {
		ctype = json_object_object_get(result,"content_type");
		if (!CheckJSONObject(ctype,json_type_string)) {
			Error("panda_api response json content_type is invalid");
		}
		body = json_object_object_get(result,"body");
		if (!CheckJSONObject(body,json_type_string)) {
			Error("panda_api response json body is invalid");
		}
		mon = (MonObjectType)atoll(json_object_get_string(body));
		GLExportBLOB(mon,&blob,&blob_size);
		SendResponse(req,status,blob,blob_size,"Content-Type",json_object_get_string(ctype),NULL);
		xfree(blob);
	} else {
		SendResponse(req,status,NULL,0,NULL);
	}
	json_object_put(obj);

	MessageLogPrintf("api %d /%s/%s/%s %s@%s",status,req->ld,req->window,req->arguments,req->user,req->host);
}

static gboolean
APIHandler(
	HTTP_REQUEST *req)
{
	json_object *obj,*res;

	obj = MakeAPIReqJSON(req);
	res = WFCIO_JSONRPC(obj);
	json_object_put(obj);
	APISendResponse(req,res);
	json_object_put(res);
	return TRUE;
}

static gboolean
BLOBImportHandler(
	HTTP_REQUEST *req)
{
	MonObjectType obj;
	char oid[256];

	if (!CheckSession(req->session_id)) {
		SendResponse(req,HTTP_FORBIDDEN,NULL,0,NULL);
		MessageLogPrintf("invalid session id %s@%s %s",req->user,req->host,req->session_id);
		return FALSE;
	}

	obj = GLImportBLOB(req->body,req->body_size);
	if (obj == GL_OBJ_NULL) {
		SendResponse(req,HTTP_INTERNAL_SERVER_ERROR,NULL,0,NULL);
		return TRUE;
	}
	sprintf(oid,"%lu",obj);
	SendResponse(req,HTTP_OK,NULL,0,"X-BLOB-ID",oid,NULL);
	return TRUE;
}

static gboolean
BLOBExportHandler(
	HTTP_REQUEST *req)
{
	MonObjectType obj;
	char *body;
	size_t size;

	if (!CheckSession(req->session_id)) {
		SendResponse(req,HTTP_FORBIDDEN,NULL,0,NULL);
		MessageLogPrintf("invalid session id %s@%s %s",req->user,req->host,req->session_id);
		return FALSE;
	}

	obj = (MonObjectType)atoll(req->oid);
	if (obj == GL_OBJ_NULL) {
		SendResponse(req,HTTP_NOT_FOUND,NULL,0,NULL);
		return TRUE;
	}
	GLExportBLOB(obj,&body,&size);
	if (body == NULL || size == 0) {
		SendResponse(req,HTTP_NOT_FOUND,NULL,0,NULL);
		return TRUE;
	} else {
		SendResponse(req,HTTP_OK,body,size,NULL);
		xfree(body);
	}
	return TRUE;
}

static	json_object*
MakeJSONResponseTemplate(
	json_object *obj)
{
	json_object *res,*child;

	res = json_object_new_object();
	json_object_object_add(res,"jsonrpc",json_object_new_string("2.0"));
	child = json_object_object_get(obj,"id");
	json_object_object_add(res,"id",json_object_new_int(json_object_get_int(child)));

	return res;
}

static	json_object*
GetServerInfo(
	json_object *obj)
{
	json_object *result,*res;
ENTER_FUNC;
	res = MakeJSONResponseTemplate(obj);

	result = json_object_new_object();
	json_object_object_add(result,"protocol_version",json_object_new_string("1.0"));
	json_object_object_add(result,"application_version",json_object_new_string("4.8.0"));
	json_object_object_add(result,"server_type",json_object_new_string("glserver"));
	json_object_object_add(res,"result",result);
LEAVE_FUNC;
	return res;
}

static	char*
_GetScreenDefine(
	const char *wname)
{
	static gchar **dirs = NULL;
	gchar *fname,*ret,*buff;
	size_t size;
	int i;
ENTER_FUNC;
	ret = NULL;
	if (dirs == NULL) {
		dirs = g_strsplit_set(ScreenDir,":",-1);
	}
	for (i=0; dirs[i]!=NULL; i++) {
		fname = g_strdup_printf("%s/%s.glade",dirs[i],wname);
		if(g_file_get_contents(fname,&buff, &size, NULL)) {
			ret = strndup(buff,size);
			g_free(buff);
			g_free(fname);
			break;
		}
		g_free(fname);
	}
	return ret;
LEAVE_FUNC;
}

static	json_object*
GetScreenDefine(
	json_object *obj)
{
	json_object *params,*child,*result,*res,*error;
	char *scrdef,*window;
ENTER_FUNC;
	params = json_object_object_get(obj,"params");
	child = json_object_object_get(params,"window");
	if (CheckJSONObject(child,json_type_string)) {
		window = (char*)json_object_get_string(child);
	} else {
		window = "";
	}

	res = MakeJSONResponseTemplate(obj);
	scrdef = _GetScreenDefine(window);
	if (scrdef != NULL) {
		result = json_object_new_object();
		json_object_object_add(result,"screen_define",json_object_new_string(scrdef));
		json_object_object_add(res,"result",result);
		free(scrdef);
	} else {
		error = json_object_new_object();
		json_object_object_add(error,"code",json_object_new_int(-20003));
		json_object_object_add(error,"message",json_object_new_string("invalid window"));
		json_object_object_add(res,"error",error);
	}
LEAVE_FUNC;
	return res;
}

static	json_object*
GetMessage(
	json_object *obj)
{
	json_object *params,*meta,*child,*result,*res,*error;
	char *session_id,*popup,*dialog,*abort;
ENTER_FUNC;
	params = json_object_object_get(obj,"params");
	meta = json_object_object_get(params,"meta");
	session_id = NULL;
	if (CheckJSONObject(meta,json_type_object)) {
		child = json_object_object_get(meta,"session_id");
		if (CheckJSONObject(child,json_type_string)) {
			session_id = (char*)json_object_get_string(child);
		}
	}
	res = MakeJSONResponseTemplate(obj);
	if (session_id != NULL) {
		GetSessionMessage(session_id,&popup,&dialog,&abort);
		result = json_object_new_object();
		json_object_object_add(result,"popup",json_object_new_string(popup));
		json_object_object_add(result,"dialog",json_object_new_string(dialog));
		json_object_object_add(result,"abort",json_object_new_string(abort));
		g_free(popup);
		g_free(dialog);
		g_free(abort);
		json_object_object_add(res,"result",result);
		ResetSessionMessage(session_id);
	} else {
		error = json_object_new_object();
		json_object_object_add(error,"code",json_object_new_int(-20004));
		json_object_object_add(error,"message",json_object_new_string("invalid session"));
		json_object_object_add(res,"error",error);
	}
LEAVE_FUNC;
	return res;
}

static gboolean
JSONRPCHandler(
	HTTP_REQUEST *req)
{
	char *reqjson,*resjson,*method,*prefix;
	json_object *obj,*params,*meta,*child,*res;

	reqjson = StrnDup(req->body,req->body_size);
	obj = json_tokener_parse(reqjson);
	xfree(reqjson);
	if (!CheckJSONObject(obj,json_type_object)) {
		Warning("invalid json");
		SendResponse(req,HTTP_BAD_REQUEST,NULL,0,NULL);
		json_object_put(obj);
		return FALSE;
	}
	child = json_object_object_get(obj,"jsonrpc");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("invalid json");
		SendResponse(req,HTTP_BAD_REQUEST,NULL,0,NULL);
		json_object_put(obj);
		return FALSE;
	}
	if (strcmp(json_object_get_string(child),"2.0")) {
		Warning("invalid json");
		SendResponse(req,HTTP_BAD_REQUEST,NULL,0,NULL);
		json_object_put(obj);
		return FALSE;
	}
	child = json_object_object_get(obj,"id");
	if (!CheckJSONObject(child,json_type_int)) {
		Warning("invalid json");
		SendResponse(req,HTTP_BAD_REQUEST,NULL,0,NULL);
		json_object_put(obj);
		return FALSE;
	}
	child = json_object_object_get(obj,"method");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("invalid json");
		SendResponse(req,HTTP_BAD_REQUEST,NULL,0,NULL);
		json_object_put(obj);
		return FALSE;
	}
	method = (char*)json_object_get_string(child);
	params = json_object_object_get(obj,"params");
	if (!CheckJSONObject(params,json_type_object)) {
		Warning("invalid json");
		SendResponse(req,HTTP_BAD_REQUEST,NULL,0,NULL);
		json_object_put(obj);
		return FALSE;
	}
	if (fDebug) {
		Warning("jsonrpc:%s",method);
	}
	if (!strcmp(method,"get_server_info")) {
		res = GetServerInfo(obj);
		resjson = (char*)json_object_to_json_string(res);
		SendResponse(req,200,resjson,strlen(resjson),"Content-Type","application/json",NULL);
		json_object_put(res);
	} else if (!strcmp(method,"get_screen_define")) {
		res = GetScreenDefine(obj);
		resjson = (char*)json_object_to_json_string(res);
		SendResponse(req,200,resjson,strlen(resjson),"Content-Type","application/json",NULL);
		json_object_put(res);
	} else if (!strcmp(method,"get_message")) {
		res = GetMessage(obj);
		resjson = (char*)json_object_to_json_string(res);
		SendResponse(req,200,resjson,strlen(resjson),"Content-Type","application/json",NULL);
		json_object_put(res);
	} else {
		meta = json_object_object_get(params,"meta");
		if (!CheckJSONObject(meta,json_type_object)) {
			SendResponse(req,HTTP_BAD_REQUEST,NULL,0,NULL);
			json_object_put(obj);
			return FALSE;
		}

#ifdef  USE_SSL
		if (fSsl) {
			prefix = g_strdup_printf("https://%s",req->server_host);
		} else {
			prefix = g_strdup_printf("http://%s",req->server_host);
		}
#else
		prefix = g_strdup_printf("http://%s",req->server_host);
#endif

		json_object_object_add(meta,"host",json_object_new_string(req->host));
		json_object_object_add(meta,"user",json_object_new_string(req->user));
		json_object_object_add(meta,"server_url_prefix",json_object_new_string(prefix));
		g_free(prefix);

		if ((res = WFCIO_JSONRPC(obj)) != NULL) {
			resjson = (char*)json_object_to_json_string(res);
			SendResponse(req,200,resjson,strlen(resjson),"Content-Type","application/json",NULL);
			json_object_put(res);
		} else {
			SendResponse(req,HTTP_INTERNAL_SERVER_ERROR,NULL,0,NULL);
		}
	}
	json_object_put(obj);
	return TRUE;
}

static gboolean
AuthAPI(
	const char *user,
	const char *password)
{
	json_object *obj,*params,*meta,*arguments;
	json_object *res,*result,*http_status;
	int status;

	obj = json_object_new_object();
	json_object_object_add(obj,"jsonrpc",json_object_new_string("2.0"));
	json_object_object_add(obj,"id",json_object_new_int(1));
	json_object_object_add(obj,"method",json_object_new_string("panda_api"));

	params = json_object_new_object();

	meta = json_object_new_object();
	json_object_object_add(meta,"user",json_object_new_string(""));
	json_object_object_add(meta,"ld",json_object_new_string("session"));
	json_object_object_add(meta,"window",json_object_new_string("session_start"));
	json_object_object_add(meta,"host",json_object_new_string(""));
	json_object_object_add(params,"meta",meta);

	arguments = json_object_new_object();
	json_object_object_add(arguments,"user",json_object_new_string(user));
	json_object_object_add(arguments,"password",json_object_new_string(password));
	json_object_object_add(arguments,"session_type",json_object_new_string(""));
	json_object_object_add(params,"arguments",arguments);

	json_object_object_add(params,"http_method",json_object_new_string("GET"));
	json_object_object_add(params,"content_type",json_object_new_string(""));

	json_object_object_add(params,"body",json_object_new_string("0"));
	json_object_object_add(obj,"params",params);

	res = WFCIO_JSONRPC(obj);
	json_object_put(obj);

	if (!CheckJSONObject(res,json_type_object)) {
		Error("panda_api response json is invalid");
	}
	result = json_object_object_get(res,"result");
	if (!CheckJSONObject(result,json_type_object)) {
		Error("panda_api response json result is invalid");
	}
	http_status = json_object_object_get(result,"http_status");
	if (!CheckJSONObject(http_status,json_type_int)) {
		Error("panda_api response json http_status is invalid");
	}
	status = json_object_get_int(http_status);
	json_object_put(res);
	return status == 200;
}

static gboolean
GLAuth(
	HTTP_REQUEST *req)
{
#ifdef	USE_SSL
	if (fSsl && fVerifyPeer){
        if (!req->fp->peer_cert) return FALSE;
	}
#endif
	if (!strncmp(Auth.protocol,"api",strlen("api"))) {
		return AuthAPI(req->user,req->pass);
	} else {
		return AuthUser(&Auth,req->user,req->pass,"",NULL);
	}
}

void
CheckJSONRPCMethod(
	HTTP_REQUEST *req)
{
	char *reqjson,*method;
	json_object *obj,*child;

	reqjson = StrnDup(req->body,req->body_size);
	obj = json_tokener_parse(reqjson);
	xfree(reqjson);
	child = json_object_object_get(obj,"method");
	if (CheckJSONObject(child,json_type_string)) {
		method = (char*)json_object_get_string(child);
		if (!strcmp(method,"start_session")) {
			req->require_auth = TRUE;
		} else {
			req->require_auth = FALSE;
		}
	}
	json_object_put(obj);
}

static gboolean
_HTTP_Method(
	HTTP_REQUEST *req)
{
	ParseRequest(req);
	alarm(0);

	if (req->status != HTTP_OK) {
		SendResponse(req,req->status,NULL,0,NULL);
		return FALSE;
	}

	if (req->type == REQUEST_TYPE_JSONRPC) {
		CheckJSONRPCMethod(req);
	}

	if (req->require_auth) {
		if (!GLAuth(req)) {
			MessageLogPrintf("[%s@%s] Authorization Error", req->user, req->host);
			req->status = HTTP_FORBIDDEN;
			SendResponse(req,req->status,NULL,0,NULL);
			return FALSE;
		}
	}

	switch(req->type) {
	case REQUEST_TYPE_API:
		return APIHandler(req);
	case REQUEST_TYPE_JSONRPC:
		return JSONRPCHandler(req);
	case REQUEST_TYPE_BLOB_IMPORT:
		return BLOBImportHandler(req);
	case REQUEST_TYPE_BLOB_EXPORT:
		return BLOBExportHandler(req);
	}
	Error("do not reach");
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
	XFree(&(req->session_id));
	req->status = HTTP_OK;
	req->body_size = 0;
	req->require_auth = FALSE;
	g_hash_table_foreach_remove(req->header_hash, RemoveHeader, NULL);

	if (req->head != NULL && strlen(req->head)> 1) {
		req->buf_size -= strlen(req->head);
		memmove(req->buf, req->head, req->buf_size);
		memset(req->head + 1, 0, MAX_REQ_SIZE - req->buf_size);
		req->buf[req->buf_size] = '\0';
		req->head = req->buf;
	} else {
		req->buf_size = 0;
		req->head = req->buf;
		memset(req->buf , 0, MAX_REQ_SIZE);
		ON_IO_ERROR(req->fp,badio);	
	}
	return TRUE;
badio:
	return FALSE;
}

void
HTTP_Method(
	NETFILE *fpComm)
{
	HTTP_REQUEST *req;
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));  
	sa.sa_handler = timeout;
	sa.sa_flags |= SA_RESTART;
	sigemptyset (&sa.sa_mask);
	if(sigaction(SIGALRM, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	} 

	req = HTTP_Init(fpComm);
	while (_HTTP_Method(req)) {
		if (!PrepareNextRequest(req)) {
			break;
		}
	}
}
