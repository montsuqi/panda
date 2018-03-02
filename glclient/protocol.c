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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <json.h>
#include <curl/curl.h>
#include <uuid.h>
#include <errno.h>
#ifdef USE_SSL
#include <libp11.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#include <openssl/engine.h>
#endif
#include <libmondai.h>

#include "protocol.h"
#include "gettext.h"
#include "const.h"
#include "logger.h"
#include "tempdir.h"
#include "utils.h"

#define SIZE_URL_BUF 256
#define TYPE_AUTH 0
#define TYPE_APP  1

static LargeByteString *readbuf;
static LargeByteString *writebuf;
static gboolean Logging = FALSE;
static char *LogDir;

size_t 
write_data(
	void *buf,
	size_t size,
	size_t nmemb,
	void *userp)
{
	size_t buf_size;
	LargeByteString *lbs;
	unsigned char *p;
	int i;

	lbs = (LargeByteString*)userp;
	buf_size = size * nmemb;
	for(i=0,p=(unsigned char*)buf;i<buf_size;i++,p++) {
		LBS_EmitChar(lbs,*p);
	}
	return buf_size;
}

size_t 
read_text_data(
	void *buf,
	size_t size,
	size_t nmemb,
	void *userp)
{
	size_t read_size;
	char *p;
	int i;
	LargeByteString *lbs;

	p = (char*)buf;
	lbs = (LargeByteString*)userp;
	read_size = 0;

	for (i=0;i<size*nmemb;i++) {
		*p = (char)LBS_FetchChar(lbs);
		if (*p == 0) {
			break;
		}
		p++;
		read_size++;
	}
	return read_size;
}

size_t 
read_binary_data(
	void *buf,
	size_t size,
	size_t nmemb,
	void *userp)
{
	size_t read_size,buf_size,rest_size;
	char *p;
	LargeByteString *lbs;

	p = (char*)buf;
	lbs = (LargeByteString*)userp;
	read_size = 0;

	buf_size = size*nmemb;
	rest_size = LBS_Size(lbs) - LBS_GetPos(lbs);
	if (rest_size <= 0) {
		return 0;
	}

	if (buf_size > rest_size) {
		read_size = rest_size;
	} else {
		read_size = buf_size;
	}
	memcpy(p,LBS_Ptr(lbs),read_size);
	LBS_SetPos(lbs,LBS_GetPos(lbs)+read_size);

	return read_size;
}

size_t
HeaderPostBLOB(
	void *ptr,
	size_t size,
	size_t nmemb,
	void *userdata)
{
	size_t all = size * nmemb;
	char *oid,*p,*q,*target = "X-BLOB-ID:";
	int i;

	oid = (char*)userdata;
	if (all <= strlen(target)) {
		return all;
	}
	if (strncasecmp(ptr,target,strlen(target))) {
		return all;
	}
	for(p=ptr+strlen(target);isspace(*p);p++);
	q = oid;
	for(i=0;isdigit(*p)&&i<SIZE_NAME;p++,i++) {
		*q = *p;
		q++;
		if (i == SIZE_NAME) {
			return all;
		}
	}

	return all;
}


char *
REST_PostBLOB(
	GLProtocol *ctx,
	LargeByteString *lbs)
{
	struct curl_slist *headers = NULL;
	char *oid,url[SIZE_URL_BUF+1],clength[256],buf[256];
	long http_code;
	CURLcode res;

	oid = malloc(SIZE_NAME+1);
	memset(oid,0,SIZE_NAME+1);
	snprintf(url,sizeof(url)-1,"%ssessions/%s/blob/",ctx->RESTURI,ctx->SessionID);
	url[sizeof(url)-1] = 0;

	snprintf(buf,sizeof(buf),"User-Agent: glclient2_%s_%s",PACKAGE_VERSION,PACKAGE_DATE);
	headers = curl_slist_append(headers, buf);
	headers = curl_slist_append(headers,"Content-Type: application/octet-stream");
	snprintf(clength,sizeof(clength),"Content-Length: %ld",LBS_Size(lbs));
	headers = curl_slist_append(headers, clength);

	LBS_SetPos(lbs,0);

	curl_easy_setopt(ctx->Curl, CURLOPT_URL, url);
	curl_easy_setopt(ctx->Curl, CURLOPT_POST, 1);
	curl_easy_setopt(ctx->Curl, CURLOPT_READDATA,(void*)lbs);
	curl_easy_setopt(ctx->Curl, CURLOPT_READFUNCTION, read_binary_data);

	curl_easy_setopt(ctx->Curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(ctx->Curl, CURLOPT_HEADERFUNCTION,HeaderPostBLOB);
	curl_easy_setopt(ctx->Curl, CURLOPT_WRITEHEADER,(void*)oid);

	res = curl_easy_perform(ctx->Curl);
	if (res != CURLE_OK) {
		Error(_("comm error:%s"),curl_easy_strerror(res));
	}
	http_code = 0;
	res = curl_easy_getinfo(ctx->Curl,CURLINFO_RESPONSE_CODE,&http_code);
	if (res != CURLE_OK) {
		Error(_("comm error:%s"),curl_easy_strerror(res));
	}
	curl_slist_free_all(headers);

	switch (http_code) {
	case 200:
		break;
	case 401:
	case 403:
		Error(_("authentication error:incorrect user or password"));
		break;
	case 503:
		Error(_("server maintenance error"));
		break;
	default:
		Error(_("http status code[%ld]"),http_code);
		break;
	}

	return oid;
}

LargeByteString*
REST_GetBLOB(
	GLProtocol *ctx,
	const char *oid)
{
	struct curl_slist *headers = NULL;
	char url[SIZE_URL_BUF+1],buf[256];
	LargeByteString *lbs;
	long http_code;
	CURLcode res;

	if (oid == NULL || !strcmp(oid,"0")) {
		return NULL;
	}

	lbs = NewLBS();

	snprintf(url,sizeof(url)-1,"%ssessions/%s/blob/%s",ctx->RESTURI,ctx->SessionID,oid);
	url[sizeof(url)-1] = 0;
	Debug("REST_GetBLOB:%s",url);

	snprintf(buf,sizeof(buf),"User-Agent: glclient2_%s_%s",PACKAGE_VERSION,PACKAGE_DATE);
	headers = curl_slist_append(headers, buf);

	curl_easy_setopt(ctx->Curl, CURLOPT_URL, url);
	curl_easy_setopt(ctx->Curl, CURLOPT_POST,0);
	curl_easy_setopt(ctx->Curl, CURLOPT_WRITEDATA,(void*)lbs);
	curl_easy_setopt(ctx->Curl, CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(ctx->Curl, CURLOPT_HTTPHEADER, headers);

	res = curl_easy_perform(ctx->Curl);
	if (res != CURLE_OK) {
		Error(_("comm error:%s"),curl_easy_strerror(res));
	}
	http_code = 0;
	res = curl_easy_getinfo(ctx->Curl,CURLINFO_RESPONSE_CODE,&http_code);
	if (res != CURLE_OK) {
		Error(_("comm error:%s"),curl_easy_strerror(res));
	}
	curl_slist_free_all(headers);

	switch (http_code) {
	case 200:
		break;
	case 401:
	case 403:
		Warning(_("authentication error:incorrect user or password"));
		return NULL;
	case 503:
		Error(_("server maintenance error"));
		break;
	default:
		Warning(_("http status code[%ld]"),http_code);
		return NULL;
	}

	return lbs;
}

static	json_object*
MakeJSONRPCRequest(
	GLProtocol *ctx,
	const char *method,
	json_object *params)
{
	json_object *obj;

	obj = json_object_new_object();
	json_object_object_add(obj,"jsonrpc",json_object_new_string("2.0"));
	json_object_object_add(obj,"id",json_object_new_int(ctx->RPCID));
	json_object_object_add(obj,"method",json_object_new_string(method));
	json_object_object_add(obj,"params",params);
	ctx->RPCID++;
	return obj;
}

static	void
CheckJSONRPCResponse(
	GLProtocol *ctx,
	json_object *obj)
{
	json_object *child,*child2,*child3;
	int code,id;
	char *message,*jsonstr,*path;

	ctx->TotalExecTime = 0;
	ctx->AppExecTime = 0;

	if (!json_object_object_get_ex(obj,"jsonrpc",&child)) {
		Error(_("invalid jsonrpc"));
	}
	if (strcmp("2.0",json_object_get_string(child))) {
		Error(_("invalid jsonrpc version"));
	}

	if (!json_object_object_get_ex(obj,"id",&child)) {
		Error(_("invalid jsonrpc id"));
	}
	id = json_object_get_int(child);
	if (id != ctx->RPCID - 1) {
		Error(_("invalid jsonrpc id"));
	}

	if (json_object_object_get_ex(obj,"error",&child)) {
		code = 0;
		message = NULL;
		if (json_object_object_get_ex(child,"code",&child2)) {
			code = json_object_get_int(child2);
		}
		if (json_object_object_get_ex(child,"message",&child2)) {
			message = (char*)json_object_get_string(child2);
		}
		Error(_("jsonrpc error code:%d message:%s"),code,message);
	}

	if (json_object_object_get_ex(obj,"result",&child)) {
		if (json_object_object_get_ex(child,"meta",&child2)) {
			if (json_object_object_get_ex(child2,"total_exec_time",&child3)) {
				ctx->TotalExecTime = (unsigned long)json_object_get_int(child3);
			}
			if (json_object_object_get_ex(child2,"app_exec_time",&child3)) {
				ctx->AppExecTime = (unsigned long)json_object_get_int(child3);
			}
		}
	} else {
		Error(_("no result object"));
	}

	if (Logging) {
		jsonstr = (char*)json_object_to_json_string(obj);
		path = g_strdup_printf("%s/res_%05d.json",LogDir,ctx->RPCID-1);
		if (!g_file_set_contents(path,jsonstr,strlen(jsonstr),NULL)) {
			Error("could not create %s",path);
		}
		fprintf(stderr,"----\n");
		fprintf(stderr,"%s\n",jsonstr);
		g_free(path);
	}
}


static	json_object*
JSONRPC(
	GLProtocol *ctx,
	int type,
	json_object *obj)
{
	struct curl_slist *headers = NULL;
	char *ctype,*url,*jsonstr,*path;
	char buf[256];
	long http_code;
	json_object *ret;
	CURLcode res;

	ctx->RPCExecTime = now();

	if (type == TYPE_AUTH) {
		url = ctx->AuthURI;
	} else {
		url = ctx->RPCURI;
	}
	if (readbuf == NULL) {
		readbuf = NewLBS();
	}
	jsonstr = (char*)json_object_to_json_string(obj);
	ctx->ReqSize = strlen(jsonstr);

	if (Logging) {
		path = g_strdup_printf("%s/req_%05d.json",LogDir,ctx->RPCID);
		if (!g_file_set_contents(path,jsonstr,ctx->ReqSize,NULL)) {
			Error("could not create %s",path);
		}
		g_free(path);
	}

	LBS_EmitStart(readbuf);
	LBS_EmitString(readbuf,jsonstr);
	LBS_EmitEnd(readbuf);
	LBS_SetPos(readbuf,0);

	json_object_put(obj);

	if (writebuf == NULL) {
		writebuf = NewLBS();
	}
	LBS_EmitStart(writebuf);
	LBS_SetPos(writebuf,0);

	snprintf(buf,sizeof(buf),"User-Agent: glclient2_%s_%s",PACKAGE_VERSION,PACKAGE_DATE);
	headers = curl_slist_append(headers, buf);
	headers = curl_slist_append(headers, "Content-Type: application/json");
	snprintf(buf,sizeof(buf),"Content-Length: %ld",ctx->ReqSize);
	headers = curl_slist_append(headers, buf);
	snprintf(buf,sizeof(buf),"Expect:");
	headers = curl_slist_append(headers, buf);

	curl_easy_setopt(ctx->Curl, CURLOPT_URL, url);
	curl_easy_setopt(ctx->Curl, CURLOPT_POST,1);
	curl_easy_setopt(ctx->Curl, CURLOPT_WRITEDATA,(void*)writebuf);
	curl_easy_setopt(ctx->Curl, CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(ctx->Curl, CURLOPT_READDATA,(void*)readbuf);
	curl_easy_setopt(ctx->Curl, CURLOPT_READFUNCTION, read_text_data);
	curl_easy_setopt(ctx->Curl, CURLOPT_HTTPHEADER, headers);

	{
		ctx->RPCExecTime = now();
		res = curl_easy_perform(ctx->Curl);
		if (res != CURLE_OK) {
			Error(_("comm error:%s"),curl_easy_strerror(res));
		}
		ctx->RPCExecTime = now() - ctx->RPCExecTime;
	}
	http_code = 0;
	res = curl_easy_getinfo(ctx->Curl,CURLINFO_RESPONSE_CODE,&http_code);
	if (res != CURLE_OK) {
		Error(_("comm error:%s"),curl_easy_strerror(res));
	}
	res = curl_easy_getinfo(ctx->Curl,CURLINFO_CONTENT_TYPE,&ctype);
	if (res != CURLE_OK) {
		Error(_("comm error:%s"),curl_easy_strerror(res));
	}
	if (ctype == NULL) {
		Error(_("invalid content type:%s"),ctype);
	}
	if (g_regex_match_simple("json",ctype,G_REGEX_CASELESS,0) || g_regex_match_simple("text",ctype,G_REGEX_CASELESS,0)) {
		// do nothing
	} else {
		Error(_("invalid content type:%s"),ctype);
	}
	curl_slist_free_all(headers);
	LBS_EmitEnd(writebuf);

	switch (http_code) {
	case 200:
		break;
	case 401:
	case 403:
		if (!strcmp("NOT PERMITTED CERTIFICATE",LBS_Body(writebuf))) {
			Error(_("NOT PERMITTED CERTIFICATE"));
		} else {
			Error(_("authentication error:incorrect user or password"));
		}
		break;
	case 503:
		Error(_("server maintenance error"));
		break;
	default:
		Error(_("http status code[%d]"),http_code);
		break;
	}

	ret = json_tokener_parse(LBS_Body(writebuf));
	if (is_error(ret)) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(ctx,ret);
	ctx->ResSize = strlen(LBS_Body(writebuf));

	return ret;
}

#define JSONRPC_AUTH(ctx,obj) JSONRPC(ctx,TYPE_AUTH,obj)
#define JSONRPC_APP(ctx,obj ) JSONRPC(ctx,TYPE_APP ,obj)

void
RPC_GetServerInfo(
	GLProtocol *ctx)
{
	json_object *obj,*params,*child,*result;
	char *type;

	params = json_object_new_object();
	obj = MakeJSONRPCRequest(ctx,"get_server_info",params);
	obj = JSONRPC_AUTH(ctx,obj);
	json_object_object_get_ex(obj,"result",&result);
	if (!json_object_object_get_ex(result,"protocol_version",&child)) {
		Error(_("no protocol_version object"));
	}
	ctx->ProtocolVersion = g_strdup(json_object_get_string(child));

	if(!json_object_object_get_ex(result,"application_version",&child)) {
		Error(_("no application_version object"));
	}
	ctx->AppVersion = g_strdup(json_object_get_string(child));

	if(!json_object_object_get_ex(result,"server_type",&child)) {
		Error(_("no server_type object"));
	}
	type = (char*)json_object_get_string(child);
	if (!strcmp(type,"ginbee")) {
		ctx->fGinbee = TRUE;
	} else {
		ctx->fGinbee = FALSE;
	}
	json_object_put(obj);
}

void
RPC_StartSession(
	GLProtocol *ctx)
{
	json_object *obj,*params,*child,*result,*meta;

	Info("start_session %s",ctx->AuthURI);
	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(params,"meta",child);
	obj = MakeJSONRPCRequest(ctx,"start_session",params);
	obj = JSONRPC_AUTH(ctx,obj);
	json_object_object_get_ex(obj,"result",&result);
	if (!json_object_object_get_ex(result,"meta",&meta)) {
		Error(_("no meta object"));
	}

	if (json_object_object_get_ex(meta,"tenant_id",&child)) {
		ctx->TenantID = g_strdup(json_object_get_string(child));
	}
	if (json_object_object_get_ex(meta,"group_id",&child)) {
		ctx->GroupID = g_strdup(json_object_get_string(child));
	}

	if (!json_object_object_get_ex(meta,"session_id",&child)) {
		Error(_("no session_id object"));
	}
	ctx->SessionID = g_strdup(json_object_get_string(child));

	if (json_object_object_get_ex(result,"startup_message",&child)) {
		ctx->StartupMessage = g_strdup((char*)json_object_get_string(child));
	}

	if (!json_object_object_get_ex(result,"app_rpc_endpoint_uri",&child)) {
		Error(_("no jsonrpc_uri object"));
	}
	ctx->RPCURI = g_strdup((char*)json_object_get_string(child));

	if (!json_object_object_get_ex(result,"app_rest_api_uri_root",&child)) {
		Error(_("no rest_uri object"));
	}
	ctx->RESTURI = g_strdup((char*)json_object_get_string(child));

	if (json_object_object_get_ex(result,"pusher_uri",&child)) {
		ctx->PusherURI = g_strdup((char*)json_object_get_string(child));
	} else {
		ctx->PusherURI = NULL;
	}

	fprintf(stderr,"SessionID: %s\n",ctx->SessionID);
	Info("TenantID : %s",ctx->TenantID);
	Info("GroupID  : %s",ctx->GroupID);
	Info("SessionID: %s",ctx->SessionID);
	Info("RPCURI   : %s",ctx->RPCURI);
	Info("RESTURI  : %s",ctx->RESTURI);
	Info("PusherURI: %s",ctx->PusherURI);
	json_object_put(obj);
	if (ctx->fGinbee) {
		curl_easy_setopt(ctx->Curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(ctx->Curl, CURLOPT_USERPWD, NULL);
	}
}

void
RPC_EndSession(
	GLProtocol *ctx)
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",
		json_object_new_string(ctx->SessionID));
	json_object_object_add(params,"meta",child);
	obj = MakeJSONRPCRequest(ctx,"end_session",params);
	obj = JSONRPC_APP(ctx,obj);
	json_object_put(obj);
	Info("end_session");
}

json_object *
RPC_GetWindow(
	GLProtocol *ctx)
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",
		json_object_new_string(ctx->SessionID));
	json_object_object_add(params,"meta",child);

	obj = MakeJSONRPCRequest(ctx,"get_window",params);
	obj = JSONRPC_APP(ctx,obj);

	return obj;
}

json_object *
RPC_GetScreenDefine(
	GLProtocol *ctx,
	const char*wname)
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",json_object_new_string(ctx->SessionID));
	json_object_object_add(params,"meta",child);
	json_object_object_add(params,"window",json_object_new_string(wname));
	obj = MakeJSONRPCRequest(ctx,"get_screen_define",params);
	obj = JSONRPC_APP(ctx,obj);
	return obj;
}

json_object *
RPC_SendEvent(
	GLProtocol *ctx,
	json_object *params)
{
	json_object *obj,*meta;

	meta = json_object_new_object();
	json_object_object_add(meta,"client_version",json_object_new_string(VERSION));
	json_object_object_add(meta,"session_id",json_object_new_string(ctx->SessionID));
	json_object_object_add(params,"meta",meta);
	obj = MakeJSONRPCRequest(ctx,"send_event",params);
	obj = JSONRPC_APP(ctx,obj);

	return obj;
}

void
RPC_GetMessage(
	GLProtocol *ctx,
	char **dialog,
	char **popup,
	char **abort)
{
	json_object *obj,*params,*child,*result;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",json_object_new_string(ctx->SessionID));
	json_object_object_add(params,"meta",child);
	obj = MakeJSONRPCRequest(ctx,"get_message",params);
	obj = JSONRPC_APP(ctx,obj);
	json_object_object_get_ex(obj,"result",&result);

	if (!json_object_object_get_ex(result,"dialog",&child)) {
		Error(_("invalid message data:dialog"));
	}
	*dialog = g_strdup((char*)json_object_get_string(child));

	if (!json_object_object_get_ex(result,"popup",&child)) {
		Error(_("invalid message data:popup"));
	}
	*popup = g_strdup((char*)json_object_get_string(child));

	if (!json_object_object_get_ex(result,"abort",&child)) {
		Error(_("invalid message data:abort"));
	}
	*abort = g_strdup((char*)json_object_get_string(child));
	json_object_put(obj);
}

json_object *
RPC_ListDownloads(
	GLProtocol *ctx)
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version", json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id", json_object_new_string(ctx->SessionID));
	json_object_object_add(params,"meta",child);

	obj = MakeJSONRPCRequest(ctx,"list_downloads",params);
	obj = JSONRPC_APP(ctx,obj);

	return obj;
}


void 
GLP_SetRPCURI(
	GLProtocol *ctx,
	const char *uri)
{
	if (ctx->RPCURI != NULL) {
		g_free(ctx->RPCURI);
	}
	ctx->RPCURI = g_strdup(uri);
}

char*
GLP_GetRPCURI(
	GLProtocol *ctx)
{
	return ctx->RPCURI;
}

void 
GLP_SetRESTURI(
	GLProtocol *ctx,
	const char *uri)
{
	if (ctx->RESTURI != NULL) {
		g_free(ctx->RESTURI);
	}
	ctx->RESTURI = g_strdup(uri);
}

char*
GLP_GetRESTURI(
	GLProtocol *ctx)
{
	return ctx->RESTURI;
}

void 
GLP_SetPusherURI(
	GLProtocol *ctx,
	const char *uri)
{
	if (ctx->PusherURI != NULL) {
		g_free(ctx->PusherURI);
	}
	ctx->PusherURI = g_strdup(uri);
}

char*
GLP_GetPusherURI(
	GLProtocol *ctx)
{
	return ctx->PusherURI;
}

char*
GLP_GetStartupMessage(
	GLProtocol *ctx)
{
	return ctx->StartupMessage;
}

char*
GLP_GetTenantID(
	GLProtocol *ctx)
{
	return ctx->TenantID;
}

char*
GLP_GetGroupID(
	GLProtocol *ctx)
{
	return ctx->GroupID;
}

void 
GLP_SetSessionID(
	GLProtocol *ctx,
	const char *sid)
{
	if (ctx->SessionID != NULL) {
		g_free(ctx->SessionID);
	}
	ctx->SessionID = g_strdup(sid);
}

char*
GLP_GetSessionID(
	GLProtocol *ctx)
{
	return ctx->SessionID;
}

gboolean
GLP_GetfGinbee(
	GLProtocol *ctx)
{
	return ctx->fGinbee;
}

void 
GLP_SetRPCID(
	GLProtocol *ctx,
	int id)
{
	ctx->RPCID = id;
}

int
GLP_GetRPCID(
	GLProtocol *ctx)
{
	return ctx->RPCID;
}

LargeByteString*
REST_GetBLOB_via_ENV()
{
	GLProtocol *proto;
	char *RESTURI,*SessionID,*APIUser,*APIPass,*OID;
	char *Cert,*CertKey,*CertKeyPass,*CAFile;
	LargeByteString *lbs;

	RESTURI = getenv("GLPUSH_REST_URI");
	if (RESTURI == NULL) {
		_Error("set env GLPUSH_REST_URI\n");
	}
	SessionID = getenv("GLPUSH_SESSION_ID");
	if (SessionID == NULL) {
		_Error("set env GLSPUSH_SESSION_ID\n");
	}
	APIUser = getenv("GLPUSH_API_USER");
	if (APIUser == NULL) {
		_Error("set env GLPUSH_API_USER\n");
	}
	APIPass = getenv("GLPUSH_API_PASSWORD");
	if (APIPass == NULL) {
		_Error("set env GLPUSH_API_PASSWORD\n");
	}
	Cert        = getenv("GLPUSH_CERT");
	CertKey     = getenv("GLPUSH_CERT_KEY");
	CertKeyPass = getenv("GLPUSH_CERT_KEY_PASSWORD");
	CAFile      = getenv("GLPUSH_CA_FILE");

	OID = getenv("GLPUSH_OID");
	if (OID == NULL) {
		_Error("set env GLPUSH_OID\n");
	}

	proto = InitProtocol("http://localhost",APIUser,APIPass);
	GLP_SetRESTURI(proto,RESTURI);
	GLP_SetSessionID(proto,SessionID);
	if (Cert != NULL && CertKey != NULL && CertKeyPass != NULL && CAFile != NULL) {
		GLP_SetSSL(proto,Cert,CertKey,CertKeyPass,CAFile);
	}
    lbs = REST_GetBLOB(proto,OID);
	FinalProtocol(proto);

	if (lbs == NULL) {
		_Error("REST_GetBLOB failure");
	}

	return lbs;
}

#ifdef USE_SSL
void
GLP_SetSSLPKCS11(
	GLProtocol *ctx,
	const char *p11lib,
	const char *pin)
{
	int i,rc,fd;
	unsigned int nslots,nslot,ncerts;
	char part[3],*id,*p,*certid,*cafile;
	PKCS11_CTX *p11ctx;
	PKCS11_SLOT *slots, *slot;
	PKCS11_CERT *certs,*cert;
	BIO *out;
	size_t size;


	nslot = 0;
	p11ctx = PKCS11_CTX_new();

	/* load pkcs #11 module */
	rc = PKCS11_CTX_load(p11ctx,p11lib);
	if (rc) {
		Error("PKCS11_CTX_load failure");
	}

	/* get information on all slots */
	rc = PKCS11_enumerate_slots(p11ctx, &slots, &nslots);
	if (rc < 0) {
		Error("PKCS11_enumerate_slots failure");
	}

	/* get first slot with a token */
	slot = PKCS11_find_token(p11ctx, slots, nslots);
	if (!slot || !slot->token) {
		Error("PKCS11_find_token failure");
	}
	for(i=0;i<nslots;i++) {
		if (&slots[i] == slot) {
			nslot = i + 1;
		}
	}

	fprintf(stderr,"Slot manufacturer......: %s\n", slot->manufacturer);
	fprintf(stderr,"Slot description.......: %s\n", slot->description);
	fprintf(stderr,"Slot token label.......: %s\n", slot->token->label);
	fprintf(stderr,"Slot token manufacturer: %s\n", slot->token->manufacturer);
	fprintf(stderr,"Slot token model.......: %s\n", slot->token->model);
	fprintf(stderr,"Slot token serialnr....: %s\n", slot->token->serialnr);

	rc = PKCS11_open_session(slot, 0);
	if (rc != 0) {
		Error("PKCS11_open_session failure");
	}

	rc = PKCS11_login(slot, 0, pin);
	if (rc != 0) {
		Error("PKCS11_login failure");
	}

	id = getenv("GLCLIENT_PKCS11_CERTID");
	if (id == NULL) {
		id = "cert";
	}

	size = strlen("slot_9999-id_") + strlen(id) * 2 + 1;
	certid = malloc(size);
	memset(certid,0,size);
	snprintf(certid,size-1,"slot_%d-id_",nslot);
	for(i=0,p=id;i<strlen(id);i++) {
		snprintf(part,sizeof(part),"%02x",*(p+i));
		strcat(certid,part);
	}
	fprintf(stderr,"%s\n",certid);

	/* get all certs */
	rc = PKCS11_enumerate_certs(slot->token, &certs, &ncerts);
	if (rc) {
		Error("PKCS11_enumerate_certs");
	}
	fprintf(stderr,"ncerts:%d\n",ncerts);
	cafile = g_strconcat(GetTempDir(),"/ca.pem",NULL);

	/* write cacertfile */
	if ((fd = creat(cafile,0600)) == -1) {
		Error("creat failure");
	}
	out = BIO_new_fd(fd,BIO_CLOSE);
	for(i=0;i<ncerts;i++) {
		cert=(PKCS11_CERT*)&certs[i];
		PEM_write_bio_X509(out,cert->x509);
	}
	BIO_free(out);

	PKCS11_release_all_slots(p11ctx, slots, nslots);
	PKCS11_CTX_unload(p11ctx);
	PKCS11_CTX_free(p11ctx);

	/* engine init */
	ENGINE_load_dynamic();
	ENGINE_load_builtin_engines();
	ctx->Engine = ENGINE_by_id("dynamic");
	if (!ctx->Engine) {
		Error("ENIGNE_by_id failure");
	}
	if (!ENGINE_ctrl_cmd_string(ctx->Engine, "SO_PATH", ENGINE_PKCS11_PATH, 0)||
		!ENGINE_ctrl_cmd_string(ctx->Engine, "ID", "pkcs11", 0) ||
		!ENGINE_ctrl_cmd_string(ctx->Engine, "LIST_ADD", "1", 0) ||
		!ENGINE_ctrl_cmd_string(ctx->Engine, "LOAD", NULL, 0) ||
		!ENGINE_ctrl_cmd_string(ctx->Engine, "MODULE_PATH", p11lib, 0) ||
		!ENGINE_ctrl_cmd_string(ctx->Engine, "PIN", pin, 0) ) {
		Error("ENGINE_ctrl_cmd_string failure");
	}
	if (!ENGINE_init(ctx->Engine)) {
		Error("ENGINE_init failure");
	}

	curl_easy_setopt(ctx->Curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
	curl_easy_setopt(ctx->Curl,CURLOPT_SSL_VERIFYPEER,1);
	curl_easy_setopt(ctx->Curl,CURLOPT_SSL_VERIFYHOST,2);
	curl_easy_setopt(ctx->Curl,CURLOPT_SSLCERT,certid);
	curl_easy_setopt(ctx->Curl,CURLOPT_SSLKEY,certid);
	curl_easy_setopt(ctx->Curl,CURLOPT_SSLENGINE,"pkcs11");
	curl_easy_setopt(ctx->Curl,CURLOPT_SSLKEYTYPE,"ENG");
	curl_easy_setopt(ctx->Curl,CURLOPT_SSLCERTTYPE,"ENG");
	curl_easy_setopt(ctx->Curl,CURLOPT_SSLENGINE_DEFAULT,1L);
	curl_easy_setopt(ctx->Curl,CURLOPT_CAINFO,cafile);

	g_free(cafile);
	free(certid);
}

void
GLP_SetSSL(
	GLProtocol *ctx,
	const char *cert,
	const char *key,
	const char *pass,
	const char *cafile)
{
	curl_easy_setopt(ctx->Curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
	curl_easy_setopt(ctx->Curl,CURLOPT_SSL_VERIFYPEER,1);
	curl_easy_setopt(ctx->Curl,CURLOPT_SSL_VERIFYHOST,2);
	if (strlen(cert) > 0 && strlen(key) > 0) {
		curl_easy_setopt(ctx->Curl,CURLOPT_SSLCERT,cert);
		curl_easy_setopt(ctx->Curl,CURLOPT_SSLCERTTYPE,"PEM");
		curl_easy_setopt(ctx->Curl,CURLOPT_SSLKEY,key);
		curl_easy_setopt(ctx->Curl,CURLOPT_SSLKEYTYPE,"PEM");
		curl_easy_setopt(ctx->Curl,CURLOPT_SSLKEYPASSWD,pass);
	}
	if (cafile == NULL || strlen(cafile) <= 0) {
		Error("set CAFile option");
	}
	curl_easy_setopt(ctx->Curl,CURLOPT_CAINFO,cafile);
}
#endif

static	CURL*
InitCURL(
	const char *user,
	const char *pass)
{
	CURL *Curl;
	char userpass[1024];

	curl_global_init(CURL_GLOBAL_ALL);
	Curl = curl_easy_init();
	if (!Curl) {
		Warning("curl_easy_init failure");
		exit(0);
	}
	if (getenv("GLCLIENT_CURL_DEBUG") != NULL) {
		curl_easy_setopt(Curl,CURLOPT_VERBOSE,1);
	}

	memset(userpass,0,sizeof(userpass));
	snprintf(userpass,sizeof(userpass)-1,"%s:%s",user,pass);
	curl_easy_setopt(Curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(Curl, CURLOPT_USERPWD, userpass);
	curl_easy_setopt(Curl, CURLOPT_NOPROXY, "*");

	return Curl;
}

void
FinalCURL(
	GLProtocol *ctx)
{
#ifdef USE_SSL
	if (ctx->fPKCS11) {
		if (ctx->Engine != NULL) {
			ENGINE_finish(ctx->Engine);
		}
		ENGINE_cleanup();
	}
#endif
	if (ctx->Curl != NULL) {
		curl_easy_cleanup(ctx->Curl);
	}
	curl_global_cleanup();
}

extern	GLProtocol*
InitProtocol(
	const char *authuri,
	const char *user,
	const char *pass
)
{
	GLProtocol *ctx;

	ctx = g_new0(GLProtocol,1);

	ctx->AuthURI = g_strdup(authuri);
	ctx->RPCID   = 0;
	ctx->fGinbee = FALSE;
#ifdef USE_SSL
	ctx->fSSL    = FALSE;
	ctx->fPKCS11 = FALSE;
#endif

	ctx->Curl = InitCURL(user,pass);
	if (getenv("GLCLIENT_DO_JSONRPC_LOGGING") != NULL) {
		Logging = TRUE;
		LogDir = MakeTempSubDir("jsonrpc_log");
	}
	return ctx;
}

extern	void
FinalProtocol(
	GLProtocol *ctx)
{
	FinalCURL(ctx);
}
