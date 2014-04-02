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

/*
#define	NETWORK_ORDER
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<ctype.h>
#include	<json.h>
#include	<curl/curl.h>
#include	<errno.h>

#include	"glclient.h"
#include	"gettext.h"
#include	"const.h"
#include	"notify.h"
#include	"printservice.h"
#include	"dialogs.h"
#include	"widgetOPS.h"
#include	"action.h"
#include	"message.h"
#include	"debug.h"

LargeByteString *readbuf;
LargeByteString *writebuf;

size_t write_data(
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

size_t read_text_data(
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

size_t read_binary_data(
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

static	json_object*
MakeJSONRPCRequest(
	const char *method,
	json_object *params)
{
	json_object *obj;

	obj = json_object_new_object();
	json_object_object_add(obj,"jsonrpc",json_object_new_string("2.0"));
	json_object_object_add(obj,"id",json_object_new_int(RPCID(Session)));
	json_object_object_add(obj,"method",json_object_new_string(method));
	json_object_object_add(obj,"params",params);
	RPCID(Session) += 1;
	return obj;
}

static	void
CheckJSONRPCResponse(
	json_object *obj)
{
	json_object *obj2,*obj3;
	int code,id;
	char *message;

	obj2 = json_object_object_get(obj,"jsonrpc");
	if (obj2 == NULL || is_error(obj2)) {
		Error(_("invalid jsonrpc"));
	}
	if (strcmp("2.0",json_object_get_string(obj2))) {
		Error(_("invalid jsonrpc version"));
	}

	obj2 = json_object_object_get(obj,"id");
	if (obj2 == NULL || is_error(obj2)) {
		Error(_("invalid jsonrpc id"));
	}
	id = json_object_get_int(obj2);
	if (id != RPCID(Session) - 1) {
		Error(_("invalid jsonrpc id"));
	}

	obj2 = json_object_object_get(obj,"error");
	if (obj2 != NULL && !is_error(obj2)) {
		code = 0;
		message = NULL;
		obj3 = json_object_object_get(obj2,"code");
		if (obj3 != NULL || !is_error(obj3)) {
			code = json_object_get_int(obj3);
		}
		obj3 = json_object_object_get(obj2,"message");
		if (obj3 != NULL || !is_error(obj3)) {
			message = (char*)json_object_get_string(obj3);
		}
		Error(_("jsonrpc error code:%d message:%s"),code,message);
	}

	obj2 = json_object_object_get(obj,"result");
	if (obj2 == NULL || is_error(obj2)) {
		Error(_("no result object"));
	}
}

#define TYPE_AUTH 0
#define TYPE_APP  1

static	void
JSONRPC(
	int type,
	json_object *obj)
{
	CURL *curl;
	struct curl_slist *headers = NULL;
	char userpass[2048],*ctype,clength[256],*url,*jsonstr;
	long http_code;
	gboolean fSSL;
	size_t jsonsize;

	if (type == TYPE_AUTH) {
		url = AUTHURI(Session);
	} else {
		url = RPCURI(Session);
	}
	if (readbuf == NULL) {
		readbuf = NewLBS();
	}
	jsonstr = (char*)json_object_to_json_string(obj);
	jsonsize = strlen(jsonstr);

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

	curl = curl_easy_init();
	if (!curl) {
		Error(_("could not init curl"));
	}

	fSSL = !strncmp("https",url,5);

	headers = curl_slist_append(headers, "Content-Type: application/json");
	snprintf(clength,sizeof(clength),"Content-Length: %ld",jsonsize);
	headers = curl_slist_append(headers, clength);
	snprintf(clength,sizeof(clength),"Expect:");
	headers = curl_slist_append(headers, clength);

	curl_easy_setopt(curl, CURLOPT_URL,url);
	curl_easy_setopt(curl, CURLOPT_POST,1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)writebuf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(curl, CURLOPT_READDATA,(void*)readbuf);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_text_data);
#if 0
	curl_easy_setopt(curl, CURLOPT_VERBOSE,1);
#endif
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	snprintf(userpass,sizeof(userpass),"%s:%s",User,Pass);
	userpass[sizeof(userpass)-1] = 0;
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	if (fSSL) {
		curl_easy_setopt(curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2);
	}
	if (curl_easy_perform(curl) != CURLE_OK) {
		Error(_("curl_easy_perform failure"));
	}
	if (curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&http_code)==CURLE_OK) {
		if (http_code != 200) {
			Error(_("http status code[%d]"),http_code);
		}
	} else {
		Error(_("curl_easy_getinfo"));
	}
	if (curl_easy_getinfo(curl,CURLINFO_CONTENT_TYPE,&ctype) == CURLE_OK) {
		if (strstr(ctype,"json") == NULL) {
			Error(_("invalid content type:%s"),ctype);
		}
	} else {
		Error(_("curl_easy_getinfo failure"));
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
}

void
RPC_GetServerInfo()
{
	json_object *obj,*params,*child,*result;

	params = json_object_new_object();
	obj = MakeJSONRPCRequest("get_server_info",params);

	JSONRPC(TYPE_AUTH,obj);
	
	// json parse
	LBS_EmitEnd(writebuf);
	obj = json_tokener_parse(LBS_Body(writebuf));
	if (is_error(obj)) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(obj);

	result = json_object_object_get(obj,"result");
	child = json_object_object_get(result,"protocol_version");
	if (child == NULL || is_error(child)) {
		Error(_("no protocol_version object"));
	}
	PROTOVER(Session) = g_strdup(json_object_get_string(child));

	child = json_object_object_get(result,"application_version");
	if (child == NULL || is_error(child)) {
		Error(_("no application_version object"));
	}
	APPVER(Session) = g_strdup(json_object_get_string(child));

	child = json_object_object_get(result,"server_type");
	if (child == NULL || is_error(child)) {
		Error(_("no server_type object"));
	}
	SERVERTYPE(Session) = g_strdup(json_object_get_string(child));
	json_object_put(obj);
}

void
RPC_StartSession()
{
	json_object *obj,*params,*child,*result,*meta;
	gchar *rpcuri,*resturi;
	GRegex *re;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(params,"meta",child);
	obj = MakeJSONRPCRequest("start_session",params);

	JSONRPC(TYPE_AUTH,obj);
	
	// json parse
	LBS_EmitEnd(writebuf);
	obj = json_tokener_parse(LBS_Body(writebuf));
	if (is_error(obj)) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(obj);

	result = json_object_object_get(obj,"result");
	meta = json_object_object_get(result,"meta");
	if (meta == NULL || is_error(meta)) {
		Error(_("no meta object"));
	}
	child = json_object_object_get(meta,"session_id");
	if (child == NULL || is_error(child)) {
		Error(_("no session_id object"));
	}
	SESSIONID(Session) = g_strdup(json_object_get_string(child));

	child = json_object_object_get(result,"app_rpc_endpoint_uri");
	if (child == NULL || is_error(child)) {
		Error(_("no jsonrpc_uri object"));
	}
	rpcuri = (char*)json_object_get_string(child);

	child = json_object_object_get(result,"app_rest_api_uri_root");
	if (child == NULL || is_error(child)) {
		Error(_("no rest_uri object"));
	}
	resturi = (char*)json_object_get_string(child);

	if (!strcmp(SERVERTYPE(Session),"ginbee")) {
		RPCURI(Session) = g_strdup(rpcuri);
		RESTURI(Session) = g_strdup(resturi);
	} else {
		RPCURI(Session) = g_strdup(AUTHURI(Session));
		re = g_regex_new("/rpc/",G_REGEX_CASELESS,0,NULL);
		RESTURI(Session) = g_regex_replace(re,AUTHURI(Session),-1,0,"/rest/",0,NULL);
		g_regex_unref(re);
	}
	json_object_put(obj);
}

void
RPC_EndSession()
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",
		json_object_new_string(SESSIONID(Session)));
	json_object_object_add(params,"meta",child);

	obj = MakeJSONRPCRequest("end_session",params);
	
	JSONRPC(TYPE_APP,obj);

	// json parse
	LBS_EmitEnd(writebuf);
	obj = json_tokener_parse(LBS_Body(writebuf));
	if (is_error(obj)) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(obj);
	json_object_put(obj);
}

void
RPC_GetWindow()
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",
		json_object_new_string(SESSIONID(Session)));
	json_object_object_add(params,"meta",child);

	obj = MakeJSONRPCRequest("get_window",params);
	JSONRPC(TYPE_APP,obj);

	// json parse
	LBS_EmitEnd(writebuf);
	if (SCREENDATA(Session) != NULL) {
		json_object_put(SCREENDATA(Session));
	}
	SCREENDATA(Session) = json_tokener_parse(LBS_Body(writebuf));
	if (is_error(SCREENDATA(Session))) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(SCREENDATA(Session));
}

json_object *
RPC_GetScreenDefine(
	const char*wname)
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",
		json_object_new_string(SESSIONID(Session)));
	json_object_object_add(params,"meta",child);
	json_object_object_add(params,"window",
		json_object_new_string(wname));

	obj = MakeJSONRPCRequest("get_screen_define",params);
	JSONRPC(TYPE_APP,obj);

	// json parse
	LBS_EmitEnd(writebuf);
	obj = json_tokener_parse(LBS_Body(writebuf));
	if (is_error(obj)) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(obj);
	return obj;
}

void
RPC_SendEvent(
	json_object *params)
{
	json_object *obj,*meta;

	meta = json_object_new_object();
	json_object_object_add(meta,"client_version",
		json_object_new_string(VERSION));
	json_object_object_add(meta,"session_id",
		json_object_new_string(SESSIONID(Session)));
	json_object_object_add(params,"meta",meta);

	obj = MakeJSONRPCRequest("send_event",params);
	JSONRPC(TYPE_APP,obj);

	// json parse
	LBS_EmitEnd(writebuf);
	if (SCREENDATA(Session) != NULL) {
		json_object_put(SCREENDATA(Session));
	}
	SCREENDATA(Session) = json_tokener_parse(LBS_Body(writebuf));
	if (SCREENDATA(Session) == NULL||is_error(SCREENDATA(Session))) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(SCREENDATA(Session));
}

void
RPC_GetMessage(
	char **dialog,
	char **popup,
	char **abort)
{
	json_object *obj,*params,*child,*result;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",
		json_object_new_string(SESSIONID(Session)));
	json_object_object_add(params,"meta",child);

	obj = MakeJSONRPCRequest("get_message",params);
	JSONRPC(TYPE_APP,obj);

	// json parse
	LBS_EmitEnd(writebuf);
	obj = json_tokener_parse(LBS_Body(writebuf));
	if (obj == NULL || is_error(obj)) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(obj);
	result = json_object_object_get(obj,"result");

	child = json_object_object_get(result,"dialog");
	if (child == NULL || is_error(child) ||
        !json_object_is_type(child,json_type_string)) {
		Error(_("invalid message data:dialog"));
	}
	*dialog = g_strdup((char*)json_object_get_string(child));

	child = json_object_object_get(result,"popup");
	if (child == NULL || is_error(child) ||
        !json_object_is_type(child,json_type_string)) {
		Error(_("invalid message data:popup"));
	}
	*popup = g_strdup((char*)json_object_get_string(child));

	child = json_object_object_get(result,"abort");
	if (child == NULL || is_error(child) ||
        !json_object_is_type(child,json_type_string)) {
		Error(_("invalid message data:abort"));
	}
	*abort = g_strdup((char*)json_object_get_string(child));
	json_object_put(obj);
}

size_t
HeaderPostBLOB(
	void *ptr,
	size_t size,
	size_t nmemb,
	void *userdata)
{
	static char buf[SIZE_NAME+1];
	size_t all = size * nmemb;
	char **oid;
	char *target = "X-BLOB-ID:";
	char *p;
	int i;

	oid = (char**)userdata;
	*oid = buf;
	if (all <= strlen(target)) {
		return all;
	}
	if (strncasecmp(ptr,target,strlen(target))) {
		return all;
	}
	for(p=ptr+strlen(target);isspace(*p);p++);
	for(i=0;isdigit(*p)&&i<SIZE_NAME;p++,i++) {
		buf[i] = *p;
	}
	buf[i] = 0;

	return all;
}

#define SIZE_URL_BUF 256

char *
REST_PostBLOB(
	LargeByteString *lbs)
{
	CURL *curl;
	struct curl_slist *headers = NULL;
	char *oid,url[SIZE_URL_BUF+1],clength[256],userpass[2048];
	gboolean fSSL;
	long http_code;

	oid = NULL;

	snprintf(url,sizeof(url)-1,"%ssessions/%s/blob/",
		RESTURI(Session),SESSIONID(Session));
	url[sizeof(url)-1] = 0;

	fSSL = !strncmp("https",url,5);

	curl = curl_easy_init();
	if (!curl) {
		Error(_("could not init curl"));
	}

	headers = curl_slist_append(headers, 
		"Content-Type: application/octet-stream");
	snprintf(clength,sizeof(clength),"Content-Length: %ld",LBS_Size(lbs));
	headers = curl_slist_append(headers, clength);

	LBS_SetPos(lbs,0);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_READDATA,(void*)lbs);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_binary_data);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,HeaderPostBLOB);
	curl_easy_setopt(curl, CURLOPT_WRITEHEADER,(void*)&oid);

	snprintf(userpass,sizeof(userpass),"%s:%s",User,Pass);
	userpass[sizeof(userpass)-1] = 0;
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);

	if (fSSL) {
		curl_easy_setopt(curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2);
	}
	if (curl_easy_perform(curl) != CURLE_OK) {
		Error(_("curl_easy_perfrom failure"));
	}
	if (curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&http_code)==CURLE_OK) {
		if (http_code != 200) {
			Error(_("http status code[%d]"),http_code);
		}
	} else {
		Error(_("curl_easy_getinfo"));
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return oid;
}

LargeByteString*
REST_GetBLOB(
	const char *oid)
{
	CURL *curl;
	char userpass[2048],url[SIZE_URL_BUF+1];
	LargeByteString *lbs;
	gboolean fSSL;
	long http_code;

	lbs = NewLBS();

	snprintf(url,sizeof(url)-1,"%ssessions/%s/blob/%s",
		RESTURI(Session),SESSIONID(Session),oid);
	url[sizeof(url)-1] = 0;

	fSSL = !strncmp("https",url,5);

	curl = curl_easy_init();
	if (!curl) {
		Warning("curl_easy_init failure");
		return NULL;
	}
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)lbs);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_data);

	snprintf(userpass,sizeof(userpass),"%s:%s",User,Pass);
	userpass[sizeof(userpass)-1] = 0;
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);

	if (fSSL) {
		curl_easy_setopt(curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2);
	}
	if (curl_easy_perform(curl) != CURLE_OK) {
		Warning(_("curl_easy_perfom failure"));
		return NULL;
	}
	if (curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&http_code)==CURLE_OK) {
		if (http_code != 200) {
			Warning(_("GETBLOB http status code[%d]"),http_code);
			return NULL;
		}
	} else {
		Error(_("curl_easy_getinfo"));
	}
	curl_easy_cleanup(curl);

	return lbs;
}

void
WriteAPIOutputFile(
	char **f,
	LargeByteString *lbs,
	const char *path)
{
	FILE *fp;
	int fd;
	mode_t mode;
	size_t wrote;

	mode = umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	*f = g_strdup_printf("%s/glclient_download_XXXXXX",TempDir);
	if ((fd = mkstemp(*f)) == -1) {
		Warning("mkstemp failure %s; drop path:%s",strerror(errno),path);
		*f = NULL;
		umask(mode);
		return;
	}
	if ((fp = fdopen(fd,"w")) == NULL) {
		Warning("fdopne failure %s; drop path:%s",strerror(errno),path);
		*f = NULL;
		umask(mode);
		return;
	}
	if (fwrite(LBS_Body(lbs),LBS_Size(lbs),1,fp) != 1) {
		Warning("fwite failure %s",strerror(errno));
		*f = NULL;
	}
	fclose(fp);
	umask(mode);
}

gboolean
REST_APIDownload(
	const char*path,
	char **f,
	size_t *s)
{
	CURL *curl;
	char userpass[2048],url[SIZE_URL_BUF+1],*msg;
	LargeByteString *lbs;
	gboolean fSSL,doRetry;
	long http_code;

	*f = NULL;
	*s = 0;

	lbs = NewLBS();
	snprintf(url,sizeof(url)-1,"%s%s",
		RESTURI(Session),path);
	url[sizeof(url)-1] = 0;

	fSSL = !strncmp("https",url,5);

	curl = curl_easy_init();
	if (!curl) {
		Warning("curl_easy_init failure");
		return FALSE;
	}
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)lbs);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_data);

	snprintf(userpass,sizeof(userpass),"%s:%s",User,Pass);
	userpass[sizeof(userpass)-1] = 0;
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);

	if (fSSL) {
		curl_easy_setopt(curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2);
	}
	if (curl_easy_perform(curl) != CURLE_OK) {
		Error("curl_easy_getinfo");
	}
	if (!curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&http_code)==CURLE_OK) {
		Error("curl_easy_getinfo");
	}
	curl_easy_cleanup(curl);

	if (fMlog) {
		MessageLogPrintf("api download %ld %s",http_code,path);
	}

	if (http_code == 200) {
		if (LBS_Size(lbs)>0) {
			WriteAPIOutputFile(f,lbs,path);
			if (*f == NULL) {
				*s = 0;
				doRetry = TRUE;
			} else {
				*s = LBS_Size(lbs);
				doRetry = FALSE;
			}
		} else {
			doRetry = TRUE;
		}
	} else if (http_code == 204) {
		doRetry = FALSE;
	} else {
		msg = g_strdup_printf(_("download failure\npath:%s\n"),path);
		Notify(_("glclient download notify"),msg,"gtk-dialog-error",0);
		g_free(msg);
		doRetry = FALSE;
	}
	FreeLBS(lbs);
	return doRetry;
}

extern	void
InitProtocol(void)
{
ENTER_FUNC;
	THISWINDOW(Session) = NULL;
	WINDOWTABLE(Session) = NewNameHash();
	SCREENDATA(Session) = NULL;
LEAVE_FUNC;
}
