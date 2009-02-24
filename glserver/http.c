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
#include	<glib.h>

#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"auth.h"
#include	"authstub.h"
#include	"glserver.h"
#include	"glcomm.h"
#include	"front.h"
#include	"term.h"
#include	"message.h"
#include	"debug.h"
#include	"http.h"
#include	"monapi.h"

#define MAX_REQ_SIZE 1024*1024

typedef struct {
	NETFILE		*fp;
	PacketClass	method;
	int			buf_size;
	char		buf[MAX_REQ_SIZE];
	char		*head;
	int			body_size;
	char		body[MAX_REQ_SIZE];
	char		*arguments;
	GHashTable 	*header_hash;
	char		*headers;
	char		*user;
	char		*pass;
	char		*ld;
	int			status;
} HTTP_REQUEST;

HTTP_REQUEST *
HTTP_Init(
	PacketClass klass,
	NETFILE *fp)
{
	HTTP_REQUEST *req;

	req = New(HTTP_REQUEST);
	req->fp = fp;
	req->method = klass;
	req->buf_size = 0;
	req->buf[0] = '\0';
	req->head = req->buf;
	req->body_size = 0;
	req->body[0] = '\0';
	req->arguments = NULL;
	req->header_hash = NewNameHash();
	req->headers = NULL;
	req->user = NULL;
	req->pass = NULL;
	req->ld = NULL;
	req->status = HTTP_OK;
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
	NETFILE *fp,
	int status,
	LargeByteString *headers,
	LargeByteString *body)
{
	char buf[256];
	char date[50];
	struct tm cur, *cur_p;
	time_t t = time(NULL);

	sprintf(buf, "HTTP/1.1 %d %s\r\n", 
		status, GetReasonPhrase(status));
	Send(fp, buf, strlen(buf));
	Message("%s", buf);

	gmtime_r(&t, &cur);
	cur_p = &cur;
	if (strftime(date, sizeof(date),
			"%a, %d %b %Y %H:%M:%S GMT", cur_p) != 0) {
		sprintf(buf, "Date: %s\r\n", date);
		Send(fp, buf, strlen(buf));
	}
	
	sprintf(buf, "Server: glserver/%s\r\n", VERSION);
	Send(fp, buf, strlen(buf));

	if (headers != NULL && LBS_Body(headers) != NULL) {
		Send(fp, LBS_Body(headers), LBS_Size(headers));
	}

	Send(fp, "\r\n", strlen("\r\n"));

	if (body != NULL && LBS_Body(body) != NULL) {
		Send(fp, LBS_Body(body), LBS_Size(body));
	}
}

int
TryRecv(
	HTTP_REQUEST *req)
{
	int size;

	if (req->buf_size >= MAX_REQ_SIZE) {
		req->status = HTTP_UNPROCESSABLE_ENTITY;
		SendResponse(req->fp, req->status, 0, NULL);
		Error("over max request size :%d", MAX_REQ_SIZE);
	}

	size = RecvAtOnce(req->fp, 
		req->buf + req->buf_size , 
		MAX_REQ_SIZE - req->buf_size);
	if (size >= 0) {
		req->buf_size += size;
		req->buf[req->buf_size] = '\0';
	} else {
		Error("can't read request");
	}
	return size;
}

char *
GetNextLine(HTTP_REQUEST *req)
{
	char *p;
	char *ret;
	int len;

	while(1) {
		p = strstr(req->head, "\r\n");
		if (p != NULL) {
			len =  p - req->head;
			if (len <= 0) {
				return NULL;
			}
			ret = strndup(req->head, len);
			req->head = p + strlen("\r\n");
			return ret;
		}
		TryRecv(req);
	}
}

#if 0
static char *
ParseReqArgument(HTTP_REQUEST *req, char *argument)
{
	char *head;
	char *tail;
	char *key;
	char *value;

	head = argument;

	while(1) {
		tail = strstr(head, "=");
		if (tail == NULL) {
			Message("Invalid HTTP Argument :%s", argument);
			req->status = HTTP_BAD_REQUEST;
			return head;
		}
		key = strndup(head, tail - head);
		head = tail + 1;
		
		tail = strstr(head, "&");
		if (tail != NULL) {
				value = strndup(head, tail - head);
				head = tail + 1;
				g_hash_table_insert(req->arguments, key, value);
				dbgprintf("argument key:%s value:%s\n", key, value);
		} else {
			tail = strstr(head, " ");
			if (tail == NULL) {
				Message("Invalid HTTP Argument :%s", argument);
				req->status = HTTP_BAD_REQUEST;
			} else {
				value = strndup(head, tail - head);
				head = tail + 1;
				g_hash_table_insert(req->arguments, key, value);
				dbgprintf("argument key:%s value:%s\n", key, value);
			}
			return head;
		}
	}
}
#endif

void
ParseReqLine(HTTP_REQUEST *req)
{
	char *head;
	char *tail;
	char *line;
	int cmp = 1;

	line = head = GetNextLine(req);
	Message("%s", line);

	tail = strstr(head, " ");
	if (tail == NULL) {
		Warning("Invalid HTTP Method :%s", line);
		req->status = HTTP_BAD_REQUEST;
		return;
	}
	switch(req->method) {
	case HTTP_GET:
		cmp = strncmp("ET", head, strlen("ET"));
		break;
	case HTTP_HEAD:
		cmp = strncmp("EAD", head, strlen("EAD"));
		break;
	case HTTP_POST:
		cmp = strncmp("OST", head, strlen("OST"));
		break;
	}
	if (cmp != 0) {
		Warning("Invalid HTTP Method :%s", line);
		req->status = HTTP_BAD_REQUEST;
		return;
	}
	head = tail + strlen(" ");
	while (head[0] == ' ') { head++; }

	tail = strstr(head, "/");
	if (tail == NULL) {
		Warning("Invalid URI :%s", line);
		req->status = HTTP_BAD_REQUEST;
		return;
	}
	head = tail + strlen("/");
	
	tail = strstr(head, "?");
	if (tail == NULL) {
		tail = strstr(head, " ");
		if (tail == NULL) {
			Warning("Invalid Panda Command :%s", line);
			req->status = HTTP_BAD_REQUEST;
			return;
		}
		req->ld = strndup(head, tail - head);
		head = tail + 1;
	} else {
		req->ld = strndup(head, tail - head);
		head = tail + 1;
		tail = strstr(head, " ");
		if (tail == NULL) {
			Warning("Missing HTTP-Version :%s", line);
			req->status = HTTP_BAD_REQUEST;
			return;
		}
		req->arguments = strndup(head, tail - head);
		head = tail + 1;
	}
	while (head[0] == ' ') { head++; }
	dbgprintf("ld :%s", req->ld);
	dbgprintf("arguments :%s", req->arguments);

	tail = strstr(head, "HTTP/1.1");
	if (tail == NULL) {
		Message("Invalid HTTP Version :%s", head);
		req->status = HTTP_BAD_REQUEST;
	}
	free(line);
}

gboolean
ParseReqHeader(HTTP_REQUEST *req)
{
	char *line;
	char *head;
	char *tail;
	char *key;
	char *value;

	line = head = GetNextLine(req);
	if ( line == NULL) {
		return FALSE;
	}
	Message("%s", line);

	tail = strstr(head, ":");
	if (tail == NULL) {
		Message("invalid HTTP Header:%s", line);
		req->status = HTTP_BAD_REQUEST;
		return FALSE;
	}
	key = strndup(head, tail - head);
	head = tail + 1;
	while(head[0] == ' '){ head++; }

	value = strdup(head);
	g_hash_table_insert(req->header_hash, key, value);
	dbgprintf("header key:%s value:%s\n", key, value);

	if (req->headers != NULL) {
		tail = xmalloc(strlen(req->headers) + strlen(line) + strlen("\r\n") + 1);
		sprintf(tail, "%s\r\n%s", req->headers, line);
		xfree(req->headers);
		req->headers = tail;
	} else {
		req->headers = strdup(line);
	}

	xfree(line);
	return TRUE;
}

void
ParseReqBody(HTTP_REQUEST *req)
{
	char *value;
	int size;
	
	value = (char *)g_hash_table_lookup(req->header_hash,"Content-Length");
	size = atoi(value);
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

	// FIXME ; delete this check
	if (strcmp("application/xml", value)) {
		req->status = HTTP_BAD_REQUEST;
		Message("invalid Content-Type:%s", value);
		return;
	}

	req->body_size = Recv(req->fp, req->body, size);
	dbgprintf("body :%s\n", req->body);
}

void
ParseReqAuth(HTTP_REQUEST *req)
{
	char *head;
	char *tail;
	char *dec;
	gsize size;

	head = (char *)g_hash_table_lookup(req->header_hash,"Authorization");
	if (head == NULL) {
		req->status = HTTP_BAD_REQUEST;
		Message("does not have Authorization");
		return;
	}
	tail = strstr(head, "Basic");
	if (tail == NULL) {
		req->status = HTTP_BAD_REQUEST;
		Message("does not support Authorization method:%s", head);
		return;
	}
	head = tail + strlen("Basic");
	while (head[0] == ' ') { head++; }
	dec = (char *)g_base64_decode(head, &size);
	if (size <= 0 || dec == NULL) {
		req->status = HTTP_BAD_REQUEST;
		Message("can not base64_decode :%s", head);
		return;
	}

	tail = strstr(dec, ":");
	if (tail == NULL) {

		req->status = HTTP_BAD_REQUEST;
		Message("Invalid Basic Authorization data:%s", dec);
		return;
	}
	req->user = strndup(dec, tail - dec);
	req->pass = strndup(dec, size - (tail - dec + 1));
	dbgprintf("Basic Auth user:%s pass:%s\n", req->user, req->pass);
	g_free(dec);
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

MonAPIData *
MakeMonAPIData(
	HTTP_REQUEST *req)
{
	MonAPIData * data;
	int size;

	data = NewMonAPIData();
	strncpy(data->ld, req->ld, sizeof(data->ld));
	strncpy(data->user, req->user, sizeof(data->user));
	strncpy(data->term, TermName(req->fp->fd), sizeof(data->term));
	data->method = req->method;

	if (req->arguments != NULL &&
		(size = strlen(req->arguments)) > 0) {
		LBS_ReserveSize(data->arguments, size + 1,FALSE);
		strcpy(LBS_Body(data->arguments),req->arguments);
	}
	if (req->headers != NULL &&
		(size = strlen(req->headers)) > 0) {
		LBS_ReserveSize(data->headers, size + 1,FALSE);
		strcpy(LBS_Body(data->headers),req->headers);
	}
	if (req->body_size > 0) {
		LBS_ReserveSize(data->body, req->body_size, FALSE);
		memcpy(LBS_Body(data->body), req->body, req->body_size);
	}
	return data;
}

void
HTTP_Method(
	PacketClass klass,
	NETFILE *fpComm)
{
	HTTP_REQUEST *req;
	MonAPIData *data;
	int status = HTTP_OK;

	req = HTTP_Init(klass, fpComm);

	ParseRequest(req);
	if (!AuthUser(&Auth, req->user, req->pass, NULL, NULL)) {
		Message("Auth Error user:%s pass:%s", req->user, req->pass);
		req->status = HTTP_UNAUTHORIZED;
	}
	if (req->status != HTTP_OK) {
		SendResponse(req->fp, req->status, NULL , NULL);
		return;
	}
	data = MakeMonAPIData(req);

	if (!CallMonAPI(data)) { 
		status = HTTP_INTERNAL_SERVER_ERROR; 
		SendResponse(req->fp, req->status, NULL , NULL);
		return;
	}

	SendResponse(req->fp, status, 
		data->headers, data->body);
}
