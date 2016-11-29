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
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <json.h>
#include <curl/curl.h>
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

#include "glclient.h"
#include "gettext.h"
#include "const.h"
#include "bd_config.h"
#include "message.h"
#include "debug.h"

static LargeByteString *readbuf;
static LargeByteString *writebuf;
static gboolean Logging = FALSE;
static gboolean Ginbee = FALSE;
static char *LogDir;

static void UnSetHTTPAuth();

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

#define SIZE_URL_BUF 256

char *
REST_PostBLOB(
	LargeByteString *lbs)
{
	struct curl_slist *headers = NULL;
	char *oid,url[SIZE_URL_BUF+1],clength[256],errbuf[CURL_ERROR_SIZE+1];
	long http_code;

	oid = malloc(SIZE_NAME+1);
	memset(oid,0,SIZE_NAME+1);
	snprintf(url,sizeof(url)-1,"%ssessions/%s/blob/",
		RESTURI(Session),SESSIONID(Session));
	url[sizeof(url)-1] = 0;

	headers = curl_slist_append(headers,"Content-Type: application/octet-stream");
	snprintf(clength,sizeof(clength),"Content-Length: %ld",LBS_Size(lbs));
	headers = curl_slist_append(headers, clength);

	LBS_SetPos(lbs,0);

	curl_easy_setopt(Curl, CURLOPT_URL, url);
	curl_easy_setopt(Curl, CURLOPT_POST, 1);
	curl_easy_setopt(Curl, CURLOPT_READDATA,(void*)lbs);
	curl_easy_setopt(Curl, CURLOPT_READFUNCTION, read_binary_data);

	curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION,HeaderPostBLOB);
	curl_easy_setopt(Curl, CURLOPT_WRITEHEADER,(void*)oid);

	memset(errbuf,0,CURL_ERROR_SIZE+1);
	curl_easy_setopt(Curl, CURLOPT_ERRORBUFFER, errbuf);

	if (curl_easy_perform(Curl) != CURLE_OK) {
		Error(_("comm error:%s"),errbuf);
	}
	if (curl_easy_getinfo(Curl,CURLINFO_RESPONSE_CODE,&http_code)==CURLE_OK) {
		switch (http_code) {
		case 200:
			break;
		case 401:
		case 403:
			Error(_("authentication error:incorrect user or password"));
			break;
		default:
			Error(_("http status code[%d]"),http_code);
			break;
		}
	} else {
		Error(_("comm error:%s"),errbuf);
	}
	curl_slist_free_all(headers);

	return oid;
}

LargeByteString*
REST_GetBLOB(
	const char *oid)
{
	char url[SIZE_URL_BUF+1],errbuf[CURL_ERROR_SIZE+1];
	LargeByteString *lbs;
	long http_code;

	if (oid == NULL || !strcmp(oid,"0")) {
		return NULL;
	}

	lbs = NewLBS();

	snprintf(url,sizeof(url)-1,"%ssessions/%s/blob/%s",RESTURI(Session),SESSIONID(Session),oid);
	url[sizeof(url)-1] = 0;

	curl_easy_setopt(Curl, CURLOPT_URL, url);
	curl_easy_setopt(Curl, CURLOPT_POST,0);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA,(void*)lbs);
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, NULL);

	memset(errbuf,0,CURL_ERROR_SIZE+1);
	curl_easy_setopt(Curl, CURLOPT_ERRORBUFFER, errbuf);

	if (curl_easy_perform(Curl) != CURLE_OK) {
		Warning(_("comm error:can not get blob:%s"),oid);
		return NULL;
	}
	if (curl_easy_getinfo(Curl,CURLINFO_RESPONSE_CODE,&http_code)==CURLE_OK) {
		switch (http_code) {
		case 200:
			break;
		case 401:
		case 403:
			Warning(_("authentication error:incorrect user or password"));
			return NULL;
		default:
			Warning(_("http status code[%d]"),http_code);
			return NULL;
		}
	} else {
		Error(_("comm error:%s"),errbuf);
	}

	return lbs;
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
	char *message,*jsonstr,*path;

	obj2 = json_object_object_get(obj,"jsonrpc");
	if (!CheckJSONObject(obj2,json_type_string)) {
		Error(_("invalid jsonrpc"));
	}
	if (strcmp("2.0",json_object_get_string(obj2))) {
		Error(_("invalid jsonrpc version"));
	}

	obj2 = json_object_object_get(obj,"id");
	if (!CheckJSONObject(obj2,json_type_int)) {
		Error(_("invalid jsonrpc id"));
	}
	id = json_object_get_int(obj2);
	if (id != RPCID(Session) - 1) {
		Error(_("invalid jsonrpc id"));
	}

	obj2 = json_object_object_get(obj,"error");
	if (CheckJSONObject(obj2,json_type_object)) {
		code = 0;
		message = NULL;
		obj3 = json_object_object_get(obj2,"code");
		if (CheckJSONObject(obj3,json_type_int)) {
			code = json_object_get_int(obj3);
		}
		obj3 = json_object_object_get(obj2,"message");
		if (CheckJSONObject(obj3,json_type_string)) {
			message = (char*)json_object_get_string(obj3);
		}
		Error(_("jsonrpc error code:%d message:%s"),code,message);
	}

	obj2 = json_object_object_get(obj,"result");
	if (obj2 == NULL || is_error(obj2)) {
		Error(_("no result object"));
	}

	if (Logging) {
		jsonstr = (char*)json_object_to_json_string(obj);
		path = g_strdup_printf("%s/res_%05d.json",LogDir,RPCID(Session));
		if (!g_file_set_contents(path,jsonstr,strlen(jsonstr),NULL)) {
			Error("could not create %s",path);
		}
		fprintf(stderr,"----\n");
		fprintf(stderr,"%s\n",jsonstr);
		g_free(path);
	}
}

#define TYPE_AUTH 0
#define TYPE_APP  1

static	json_object*
JSONRPC(
	int type,
	json_object *obj)
{
	struct curl_slist *headers = NULL;
	char *ctype,clength[256],*url,*jsonstr,errbuf[CURL_ERROR_SIZE+1],*path;
	long http_code;
	size_t jsonsize;
	json_object *ret;

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

	if (Logging) {
		path = g_strdup_printf("%s/req_%05d.json",LogDir,RPCID(Session));
		if (!g_file_set_contents(path,jsonstr,jsonsize,NULL)) {
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

	headers = curl_slist_append(headers, "Content-Type: application/json");
	snprintf(clength,sizeof(clength),"Content-Length: %ld",jsonsize);
	headers = curl_slist_append(headers, clength);
	snprintf(clength,sizeof(clength),"Expect:");
	headers = curl_slist_append(headers, clength);

	curl_easy_setopt(Curl, CURLOPT_URL, url);
	curl_easy_setopt(Curl, CURLOPT_POST,1);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA,(void*)writebuf);
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(Curl, CURLOPT_READDATA,(void*)readbuf);
	curl_easy_setopt(Curl, CURLOPT_READFUNCTION, read_text_data);
	curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, headers);

	memset(errbuf,0,CURL_ERROR_SIZE+1);
	curl_easy_setopt(Curl, CURLOPT_ERRORBUFFER, errbuf);

	if (curl_easy_perform(Curl) != CURLE_OK) {
		Error(_("comm error:%s"),errbuf);
	}
	if (curl_easy_getinfo(Curl,CURLINFO_RESPONSE_CODE,&http_code)==CURLE_OK) {
		switch (http_code) {
		case 200:
			break;
		case 401:
		case 403:
			Error(_("authentication error:incorrect user or password"));
			break;
		default:
			Error(_("http status code[%d]"),http_code);
			break;
		}
	} else {
		Error(_("comm error:%s"),errbuf);
	}
	if (curl_easy_getinfo(Curl,CURLINFO_CONTENT_TYPE,&ctype) == CURLE_OK) {
		if (strstr(ctype,"json") == NULL) {
			Error(_("invalid content type:%s"),ctype);
		}
	} else {
		Error(_("comm error:%s"),errbuf);
	}
	curl_slist_free_all(headers);

	LBS_EmitEnd(writebuf);
	ret = json_tokener_parse(LBS_Body(writebuf));
	if (is_error(ret)) {
		Error(_("invalid json"));
	}
	CheckJSONRPCResponse(ret);
	return ret;
}

void
RPC_GetServerInfo()
{
	json_object *obj,*params,*child,*result;
	char *type;

	params = json_object_new_object();
	obj = MakeJSONRPCRequest("get_server_info",params);
	obj = JSONRPC(TYPE_AUTH,obj);
	result = json_object_object_get(obj,"result");
	child = json_object_object_get(result,"protocol_version");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("no protocol_version object"));
	}
	PROTOVER(Session) = g_strdup(json_object_get_string(child));

	child = json_object_object_get(result,"application_version");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("no application_version object"));
	}
	APPVER(Session) = g_strdup(json_object_get_string(child));

	child = json_object_object_get(result,"server_type");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("no server_type object"));
	}
	type = (char*)json_object_get_string(child);
	if (!strcmp(type,"ginbee")) {
		Ginbee = TRUE;
	} else {
		Ginbee = FALSE;
	}
	json_object_put(obj);
}

void
RPC_StartSession()
{
	json_object *obj,*params,*child,*result,*meta;
	gchar *rpcuri,*resturi;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(params,"meta",child);
	obj = MakeJSONRPCRequest("start_session",params);
	obj = JSONRPC(TYPE_AUTH,obj);
	result = json_object_object_get(obj,"result");
	meta = json_object_object_get(result,"meta");
	if (!CheckJSONObject(meta,json_type_object)) {
		Error(_("no meta object"));
	}
	child = json_object_object_get(meta,"session_id");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("no session_id object"));
	}
	SESSIONID(Session) = g_strdup(json_object_get_string(child));

	child = json_object_object_get(result,"app_rpc_endpoint_uri");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("no jsonrpc_uri object"));
	}
	rpcuri = (char*)json_object_get_string(child);

	child = json_object_object_get(result,"app_rest_api_uri_root");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("no rest_uri object"));
	}
	resturi = (char*)json_object_get_string(child);

	RPCURI(Session) = g_strdup(rpcuri);
	RESTURI(Session) = g_strdup(resturi);
	if (fMlog) {
		MessageLogPrintf("RPCURI[%s]\n",RPCURI(Session));
		MessageLogPrintf("RESTURI[%s]\n",RESTURI(Session));
	}
	json_object_put(obj);
	if (Ginbee) {
		UnSetHTTPAuth();
	}
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
	obj = JSONRPC(TYPE_APP,obj);
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
	obj = JSONRPC(TYPE_APP,obj);

	if (SCREENDATA(Session) != NULL) {
		json_object_put(SCREENDATA(Session));
	}
	SCREENDATA(Session) = obj;
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
	obj = JSONRPC(TYPE_APP,obj);
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
	obj = JSONRPC(TYPE_APP,obj);
	if (SCREENDATA(Session) != NULL) {
		json_object_put(SCREENDATA(Session));
	}
	SCREENDATA(Session) = obj;
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
	obj = JSONRPC(TYPE_APP,obj);
	result = json_object_object_get(obj,"result");

	child = json_object_object_get(result,"dialog");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("invalid message data:dialog"));
	}
	*dialog = g_strdup((char*)json_object_get_string(child));

	child = json_object_object_get(result,"popup");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("invalid message data:popup"));
	}
	*popup = g_strdup((char*)json_object_get_string(child));

	child = json_object_object_get(result,"abort");
	if (!CheckJSONObject(child,json_type_string)) {
		Error(_("invalid message data:abort"));
	}
	*abort = g_strdup((char*)json_object_get_string(child));
	json_object_put(obj);
}

json_object *
RPC_ListDownloads()
{
	json_object *obj,*params,*child;

	params = json_object_new_object();
	child = json_object_new_object();
	json_object_object_add(child,"client_version",
		json_object_new_string(PACKAGE_VERSION));
	json_object_object_add(child,"session_id",
		json_object_new_string(SESSIONID(Session)));
	json_object_object_add(params,"meta",child);

	obj = MakeJSONRPCRequest("list_downloads",params);
	obj = JSONRPC(TYPE_APP,obj);

	return obj;
}

#ifdef USE_SSL
static	void
InitCURLPKCS11()
{
	int i,rc,fd,n;
	unsigned int nslots,nslot,ncerts;
	char part[3],*id,*p,*cacertfile,*certid;
	PKCS11_CTX *p11ctx;
	PKCS11_SLOT *slots, *slot;
	PKCS11_CERT *certs,*cert;
	BIO *out;
	size_t size;

	nslot = 0;
	p11ctx = PKCS11_CTX_new();

	/* load pkcs #11 module */
	rc = PKCS11_CTX_load(p11ctx,PKCS11Lib);
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

	n = gl_config_get_index();
   	gl_config_set_boolean(n,"savepin", FALSE);
   	gl_config_set_string(n,"pin", "");

	rc = PKCS11_login(slot, 0, PIN);
	if (rc != 0) {
		Error("PKCS11_login failure");
	} else {
		if (fSavePIN) {
    		gl_config_set_boolean(n,"savepin", TRUE);
    		gl_config_set_string(n,"pin", PIN);
		}
	}
	gl_config_save();

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

	/* write cacertfile */
	cacertfile = g_strdup_printf("%s/glclient_cacerts_XXXXXX",TempDir);
	if ((fd = mkstemp(cacertfile)) == -1) {
		Error("mkstemp failure");
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
	Engine = ENGINE_by_id("dynamic");
	if (!Engine) {
		Error("ENIGNE_by_id failure");
	}
	if (!ENGINE_ctrl_cmd_string(Engine, "SO_PATH", ENGINE_PKCS11_PATH, 0)||
		!ENGINE_ctrl_cmd_string(Engine, "ID", "pkcs11", 0) ||
		!ENGINE_ctrl_cmd_string(Engine, "LIST_ADD", "1", 0) ||
		!ENGINE_ctrl_cmd_string(Engine, "LOAD", NULL, 0) ||
		!ENGINE_ctrl_cmd_string(Engine, "MODULE_PATH", PKCS11Lib, 0) ||
		!ENGINE_ctrl_cmd_string(Engine, "PIN", PIN, 0) ) {
		Error("ENGINE_ctrl_cmd_string failure");
	}
	if (!ENGINE_init(Engine)) {
		Error("ENGINE_init failure");
	}

	curl_easy_setopt(Curl,CURLOPT_SSLCERT,certid);
	curl_easy_setopt(Curl,CURLOPT_SSLKEY,certid);
	curl_easy_setopt(Curl,CURLOPT_SSLENGINE,"pkcs11");
	curl_easy_setopt(Curl,CURLOPT_SSLKEYTYPE,"ENG");
	curl_easy_setopt(Curl,CURLOPT_SSLCERTTYPE,"ENG");
	curl_easy_setopt(Curl,CURLOPT_SSLENGINE_DEFAULT,1L);
	curl_easy_setopt(Curl,CURLOPT_CAINFO,cacertfile);

	g_free(cacertfile);
	free(certid);
}
#endif

static	void
SetHTTPAuth()
{
	char userpass[1024];
	
	memset(userpass,0,sizeof(userpass));
	snprintf(userpass,sizeof(userpass)-1,"%s:%s",User,Pass);
	curl_easy_setopt(Curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(Curl, CURLOPT_USERPWD, userpass);
}

static	void
UnSetHTTPAuth()
{
	curl_easy_setopt(Curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(Curl, CURLOPT_USERPWD, NULL);
}

static	void
InitCURL()
{
	curl_global_init(CURL_GLOBAL_ALL);
	Curl = curl_easy_init();
	if (!Curl) {
		Warning("curl_easy_init failure");
		exit(0);
	}
	if (getenv("GLCLIENT_CURL_DEBUG") != NULL) {
		curl_easy_setopt(Curl,CURLOPT_VERBOSE,1);
	}

#ifdef USE_SSL
	if (fSSL) {
		curl_easy_setopt(Curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
		curl_easy_setopt(Curl,CURLOPT_SSL_VERIFYPEER,1);
		curl_easy_setopt(Curl,CURLOPT_SSL_VERIFYHOST,2);
		if (fPKCS11) {
			InitCURLPKCS11();
		} else {
			if (strlen(CertFile) > 0) {
				curl_easy_setopt(Curl,CURLOPT_SSLCERT,CertFile);
				curl_easy_setopt(Curl,CURLOPT_SSLCERTTYPE,"PEM");
				curl_easy_setopt(Curl,CURLOPT_SSLKEY,CertKeyFile);
				curl_easy_setopt(Curl,CURLOPT_SSLKEYTYPE,"PEM");
				curl_easy_setopt(Curl,CURLOPT_SSLKEYPASSWD,CertPass);
			}
			if (CAFile == NULL || strlen(CAFile) <= 0) {
				Error("set CAFile option");
			}
			curl_easy_setopt(Curl,CURLOPT_CAINFO,CAFile);
		}
	}
	SetHTTPAuth();
#else
	SetHTTPAuth();
#endif
}

void FinalCURL()
{
#ifdef USE_SSL
	if (fPKCS11) {
		if (Engine != NULL) {
			ENGINE_finish(Engine);
		}
		ENGINE_cleanup();
	}
#endif
	if (Curl != NULL) {
		curl_easy_cleanup(Curl);
	}
	curl_global_cleanup();
}

static	void
MakeLogDir(void)
{
#if 1
	gchar *template;
	gchar *tmpdir;
	gchar *p;

	tmpdir = g_strconcat(g_get_home_dir(),"/.glclient/jsonrpc",NULL);
	MakeDir(tmpdir,0700);
	template = g_strconcat(tmpdir,"/XXXXXX",NULL);
	g_free(tmpdir);
	if ((p = mkdtemp(template)) == NULL) {
		Error(_("mkdtemp failure"));
	}
	LogDir = p; 
	printf("LogDir:%s\n",LogDir);
#else
	/* glib >= 2.26*/
	TempDir = g_mkdtemp(g_strdup("glclient_XXXXXX"));
#endif
}

extern	void
InitProtocol()
{
ENTER_FUNC;
	THISWINDOW(Session) = NULL;
	WINDOWTABLE(Session) = NewNameHash();
	SCREENDATA(Session) = NULL;
	InitCURL();
	if (getenv("GLCLIENT_DO_JSONRPC_LOGGING") != NULL) {
		Logging = TRUE;
		MakeLogDir();
	}
LEAVE_FUNC;
}

extern	void
FinalProtocol()
{
	FinalCURL();
}
