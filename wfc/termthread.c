/*
 * PANDA -- a simple transaction monitor
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
#include	<unistd.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>
#include	<time.h>
#include	<errno.h>
#include	<json.h>
#include	<uuid/uuid.h>

#include	"enum.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"lock.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"queue.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"glterm.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"sessionthread.h"
#include	"dirs.h"
#include	"message.h"
#include	"debug.h"

extern	void
TermEnqueue(
	TermNode	*term,
	SessionData	*data)
{
	EnQueue(term->que,data);
}

static	SessionData	*
NewSessionData(void)
{
	SessionData	*data;

ENTER_FUNC;
	data = New(SessionData);
	memclear(data,sizeof(SessionData));
	data->type = SESSION_TYPE_TERM;
	data->status = SESSION_STATUS_NORMAL;
	data->hdr = New(MessageHeader);
	memclear(data->hdr,sizeof(MessageHeader));
	data->hdr->command = APL_COMMAND_LINK;
	data->apsid = -1;
	data->spadata = NewNameHash();
	data->scrpool = NewNameHash();
	gettimeofday(&(data->create_time), NULL);
	gettimeofday(&(data->access_time), NULL);
	timerclear(&(data->process_time));
	timerclear(&(data->total_process_time));
	data->apidata = New(APIData);
	data->apidata->status = WFC_API_OK;
	data->apidata->rec = NewLBS();
	data->sysdbval = RecParseValueMem(SYSDBVAL_DEF,NULL);
	InitializeValue(data->sysdbval);
	data->count = 0;
	data->w.sp = 0;
LEAVE_FUNC;
	return	(data);
}

static	guint
FreeSpa(
	char	*name,
	LargeByteString	*spa,
	void		*dummy)
{
	if (name != NULL) {
		xfree(name);
	}
	if (spa != NULL) {
		FreeLBS(spa);
	}
	return	TRUE;
}

static	guint
FreeScr(
	char	*name,
	LargeByteString	*scr,
	void		*dummy)
{
	if (name != NULL) {
		xfree(name);
	}
	if (scr != NULL) {
		FreeLBS(scr);
	}
	return	TRUE;
}

static	void
FreeSessionData(
	SessionData	*data)
{
ENTER_FUNC;
	if (data->type != SESSION_TYPE_API) {
		MessageLogPrintf("[%s@%s] session end",data->hdr->user,data->hdr->uuid);
	}
	if (data->linkdata != NULL) {
		FreeLBS(data->linkdata);
	}
	xfree(data->hdr);
	g_hash_table_foreach_remove(data->spadata,(GHRFunc)FreeSpa,NULL);
	DestroyHashTable(data->spadata);
	g_hash_table_foreach_remove(data->scrpool,(GHRFunc)FreeScr,NULL);
	DestroyHashTable(data->scrpool);
	xfree(data->scrdata);
	FreeLBS(data->apidata->rec);
	xfree(data->apidata);
	FreeValueStruct(data->sysdbval);
	xfree(data);
LEAVE_FUNC;
}

static LargeByteString	*
NewLinkData(void)
{
	LargeByteString *linkdata;
	size_t size;

	if (ThisEnv->linkrec != NULL) {
		linkdata = NewLBS();
		size = NativeSizeValue(NULL,ThisEnv->linkrec->value);
		LBS_ReserveSize(linkdata,size,FALSE);
		NativePackValue(NULL,LBS_Body(linkdata),ThisEnv->linkrec->value);
	} else {
		linkdata = NULL;
	}
	return linkdata;
}

static	void
RegisterSession(
	SessionData	*data)
{
	SessionCtrl *ctrl;
ENTER_FUNC;
	snprintf(data->hdr->tempdir,SIZE_PATH,"%s/%s",TempDirRoot,data->hdr->uuid);
	if (!MakeDir(data->hdr->tempdir,0700)) {
		Error("cannot make session tempdir %s",data->hdr->tempdir);
	}

	ctrl = NewSessionCtrl(SESSION_CONTROL_INSERT);
	ctrl->session = data;
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
}

static	SessionData	*
LookupSession(
	const char	*term)
{
	SessionData *data;
	SessionCtrl *ctrl;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_LOOKUP);
	strcpy(ctrl->id,term);
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	data = ctrl->session;
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
	return	(data);
}

static	void
DeregisterSession(
	SessionData *data)
{
	SessionCtrl *ctrl;
ENTER_FUNC;
#if 0
	if (!getenv("WFC_KEEP_TEMPDIR")) {
		if (!rm_r(data->hdr->tempdir)) {
			Error("cannot remove session tempdir %s",data->hdr->tempdir);
		}
	}
#endif
	ctrl = NewSessionCtrl(SESSION_CONTROL_DELETE);
	ctrl->session = data;
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	FreeSessionCtrl(ctrl);
	FreeSessionData(data);
LEAVE_FUNC;
}

static	void
UpdateSession(
	SessionData *data)
{
	SessionCtrl *ctrl;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_UPDATE);
	ctrl->session = data;
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
}

static unsigned int
GetSessionNum()
{
	SessionCtrl *ctrl;
	unsigned int size;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_GET_SESSION_NUM);
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	size = ctrl->size;
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
	return size;
}

static	SessionData	*
InitAPISession(
	const char *user,
	const char *ldname,
	const char *wname,
	const char *host)
{
	SessionData		*data;
	LD_Node			*ld;
	RecordStruct	*rec;
	size_t			size;
	uuid_t			u;

ENTER_FUNC;
	data = NewSessionData();
	data->type = SESSION_TYPE_API;
	uuid_generate(u);
	uuid_unparse(u,data->hdr->uuid);
	strcpy(data->hdr->window,wname);
	strcpy(data->hdr->user,user);
	strcpy(data->host,host);
	if ((ld = g_hash_table_lookup(APS_Hash, ldname)) != NULL) {
		data->ld = ld;
		data->linkdata = NULL;
		data->cWindow = ld->info->cWindow;
		data->scrdata = NULL;
		data->hdr->puttype = SCREEN_NULL;
		rec = GetWindow((char*)wname);
		size = NativeSizeValue(NULL,rec->value);
		LBS_ReserveSize(data->apidata->rec, size,FALSE);
		NativePackValue(NULL,LBS_Body(data->apidata->rec),rec->value);
	} else {
		Warning("[%s] session fail LD [%s] not found.",data->hdr->uuid,ldname);
		data = NULL;
	}
LEAVE_FUNC;
	return	(data);
}


static	SessionData	*
Process(
	SessionData	*data)
{
	struct	timeval	tv1;
	struct	timeval	tv2;
ENTER_FUNC;
	data->fInProcess = TRUE;
	gettimeofday(&tv1,NULL);
	CoreEnqueue(data);
	data = DeQueue(data->term->que);
	data->count += 1;
	gettimeofday(&tv2,NULL);
	timersub(&tv2, &tv1, &(data->process_time));
	timeradd(&(data->total_process_time), &(data->process_time), &tv1);
	data->total_process_time = tv1;
	data->fInProcess = FALSE;
LEAVE_FUNC;
	return data;
}

static	Bool
CheckJSONObject(
	json_object *obj,
	enum json_type type)
{
	if (obj == NULL) {
		return FALSE;
	}
	if (is_error(obj)) {
		return FALSE;
	}
	if (!json_object_is_type(obj,type)) {
		return FALSE;
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

static	void
JSONRPC_Error(
	TermNode *term,
	json_object *obj,
	int no,
	const char *msg)
{
	json_object *error,*res;

	res = MakeJSONResponseTemplate(obj);
	error = json_object_new_object();
	json_object_object_add(error,"code",json_object_new_int(no));
	json_object_object_add(error,"message",json_object_new_string(msg));
	json_object_object_add(res,"error",error);

	SendString(term->fp,(char*)json_object_to_json_string(res));
	if (CheckNetFile(term->fp)) {
		CloseNet(term->fp);
	}
	json_object_put(res);
}

static	gboolean
CheckClientVersion(
	char *version)
{
	if (!strncmp("1.4.8",version,strlen("1.4.8"))) {
		return TRUE;
	}
	return FALSE;
}

static	void
RPC_StartSession(
	TermNode *term,
	json_object *obj)
{
	json_object *params,*meta,*child,*result,*res;
	SessionData *data;
	uuid_t u;
	int sesnum;
ENTER_FUNC;
	params = json_object_object_get(obj,"params");
	if (!CheckJSONObject(params,json_type_object)) {
		Warning("request have not params");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	meta = json_object_object_get(params,"meta");
	if (!CheckJSONObject(meta,json_type_object)) {
		Warning("request have not meta");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	child = json_object_object_get(meta,"client_version");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not client_version");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	if (!CheckClientVersion((char*)json_object_get_string(child))) {
		Warning("invalid client version:%s",json_object_get_string(child));
		JSONRPC_Error(term,obj,-20001,"Invalid Client Version");
		return;
	}

	sesnum = GetSessionNum();
	if (SesNum != 0 && sesnum >= SesNum) {
		Warning("Discard new session(%s);max session number(%d)",term,SesNum);
		CloseNet(term->fp);
		return;
	}

	data = NewSessionData();
	data->term = term;
	uuid_generate(u);
	uuid_unparse(u,data->hdr->uuid);

	child = json_object_object_get(meta,"user");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not user");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	memset(data->hdr->user,0,SIZE_USER+1);
	strncpy(data->hdr->user,(char*)json_object_get_string(child),SIZE_USER);

	child = json_object_object_get(meta,"host");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not host");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	memset(data->host,0,SIZE_HOST+1);
	strncpy(data->host,json_object_get_string(child),SIZE_HOST);

	memset(data->agent,0,SIZE_NAME+1);

	MessageLogPrintf("[%s:%s] session start(%d)",data->hdr->user,data->hdr->uuid,sesnum+1);
	dbgprintf("uuid   = [%s]",data->hdr->uuid);
	dbgprintf("user   = [%s]",data->hdr->user);
	dbgprintf("host   = [%s]",data->host);
	dbgprintf("agent  = [%s]",data->agent);

	data->hdr->puttype = SCREEN_INIT;
	RegisterSession(data);

	res = MakeJSONResponseTemplate(obj);
	result = json_object_new_object();
	meta = json_object_new_object();
	json_object_object_add(meta,"session_id",json_object_new_string(data->hdr->uuid));
	json_object_object_add(result,"meta",meta);
	json_object_object_add(result,"app_rpc_endpoint_uri",json_object_new_string(""));
	json_object_object_add(result,"app_rest_api_uri_root",json_object_new_string(""));
	json_object_object_add(res,"result",result);

	SendString(term->fp,(char*)json_object_to_json_string(res));
	if (CheckNetFile(term->fp)) {
		CloseNet(term->fp);
	} else {
		DeregisterSession(data);
	}
	json_object_put(res);
LEAVE_FUNC;
}

static	void
RPC_EndSession(
	TermNode *term,
	json_object *obj)
{
	json_object *params,*meta,*child,*result,*res;
	SessionData *data;
	LD_Node		*ld;
	const char *session_id;
ENTER_FUNC;
	params = json_object_object_get(obj,"params");
	if (!CheckJSONObject(params,json_type_object)) {
		Warning("request have not params");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	meta = json_object_object_get(params,"meta");
	if (!CheckJSONObject(meta,json_type_object)) {
		Warning("request have not meta");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	child = json_object_object_get(meta,"session_id");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not session_id");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	session_id = json_object_get_string(child);
	data = LookupSession(session_id);
	if (data == NULL) {
		Warning("session [%s] does not found",session_id);
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}

	data->term = term;

	if ((ld = g_hash_table_lookup(APS_Hash, "session"))  !=  NULL) {
		strncpy(data->hdr->window,"session_end",
			sizeof(data->hdr->window));
		data->hdr->widget[0] = 0;
		sprintf(data->hdr->event,"SESSIONEND");
		data->hdr->puttype = SCREEN_NULL;
		ChangeLD(data,ld);
		data->status = SESSION_STATUS_END;
		data = Process(data);
	}


	DeregisterSession(data);

	res = MakeJSONResponseTemplate(obj);
	result = json_object_new_object();
	json_object_object_add(res,"result",result);

	SendString(term->fp,(char*)json_object_to_json_string(res));
	json_object_put(res);

	CloseNet(term->fp);
LEAVE_FUNC;
}

static	json_object*
MakeEventResponse(
	json_object *obj,
	SessionData *data)
{
	json_object *result,*res,*window_data,*windows,*w,*child;
	RecordStruct *rec;
	LargeByteString *scrdata;
	char *buf;
	const char *puttype;
	int i;

	res = MakeJSONResponseTemplate(obj);
	result = json_object_new_object();
	json_object_object_add(res,"result",result);

	window_data = json_object_new_object();
	json_object_object_add(window_data,"focused_window",
		json_object_new_string(data->hdr->window));
	json_object_object_add(window_data,"focused_widget",
		json_object_new_string(data->hdr->widget));

	windows = json_object_new_array();
	for(i=0;i<data->w.sp;i++) {
		w = json_object_new_object();

		switch(data->w.s[i].puttype) {
		case SCREEN_CURRENT_WINDOW:
			puttype = "current";
			break;
		case SCREEN_NEW_WINDOW:
			puttype = "new";
			break;
		case SCREEN_CLOSE_WINDOW:
			puttype = "close";
			break;
		default:
			Warning("invalid puttype %s:%d",data->w.s[i].window,data->w.s[i].puttype);
			puttype = "new";
			break;
		}
		json_object_object_add(w,"put_type",
			json_object_new_string(puttype));
		json_object_object_add(w,"window",
			json_object_new_string(data->w.s[i].window));

		scrdata = GetScreenData(data,data->w.s[i].window);
		if (scrdata == NULL) {
			Warning("scrdata %s null",data->w.s[i].window);
			json_object_object_add(w,"screen_data",
				json_object_new_object());
		} else {
			rec = GetWindow(data->w.s[i].window);
			NativeUnPackValue(NULL,LBS_Body(scrdata),rec->value);
			buf = xmalloc(JSON_SizeValue(NULL,rec->value));
			JSON_PackValue(NULL,buf,rec->value);
			child = json_tokener_parse(buf);
			xfree(buf);
			if (child == NULL || is_error(child)) {
				json_object_object_add(w,"screen_data",
					json_object_new_object());
			} else {
				json_object_object_add(w,"screen_data",
					child);
			}
		}
		json_object_array_add(windows,w);
	}
	json_object_object_add(window_data,"windows",windows);
	json_object_object_add(result,"window_data",window_data);
	return res;
}

static	void
RPC_GetWindow(
	TermNode *term,
	json_object *obj)
{
	json_object *params,*meta,*child,*res;
	SessionData *data;
	LD_Node		*ld;
	const char *session_id;
	int i;
ENTER_FUNC;
	params = json_object_object_get(obj,"params");
	if (!CheckJSONObject(params,json_type_object)) {
		Warning("request have not params");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	meta = json_object_object_get(params,"meta");
	if (!CheckJSONObject(meta,json_type_object)) {
		Warning("request have not meta");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	child = json_object_object_get(meta,"session_id");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not session_id");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	session_id = json_object_get_string(child);
	data = LookupSession(session_id);
	if (data == NULL) {
		Warning("session [%s] does not found",session_id);
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}

	if (data->fInProcess) {
		Warning("Error: %s is proccesing in other thread",session_id);
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}

	if (data->hdr->puttype != SCREEN_INIT) {
		Warning("Error: %s recall get_window",session_id);
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}

	if ((ld = g_hash_table_lookup(APS_Hash,ThisEnv->InitialLD)) == NULL) {
		Error("cannot find initial ld:%s.check directory",ThisEnv->InitialLD);
	}

	data->term = term;
	data->ld = ld;
	data->linkdata = NewLinkData();
	data->cWindow = ld->info->cWindow;
	data->scrdata = (LargeByteString **)xmalloc(sizeof(void*)*data->cWindow);
	for	(i = 0 ; i < data->cWindow ; i ++) {
		if (data->ld->info->windows[i] != NULL) {
			dbgprintf("[%s]",data->ld->info->windows[i]->name);
			data->scrdata[i] = 
				GetScreenData(data,data->ld->info->windows[i]->name);
		} else {
			data->scrdata[i] = NULL;
		}
	}
	data->hdr->puttype = SCREEN_NULL;
	UpdateSession(data);
	data = Process(data);

	if (data->status != SESSION_STATUS_NORMAL) {
		Warning("Error: %s Session Abort",data->hdr->uuid);
		JSONRPC_Error(term,obj,-20002,"Session Abort");
		DeregisterSession(data);
		return;
	} else {
		UpdateSession(data);
	}

	res = MakeEventResponse(obj,data);
	SendString(term->fp,(char*)json_object_to_json_string(res));
	if (CheckNetFile(term->fp)) {
		CloseNet(term->fp);
	} else {
		DeregisterSession(data);
	}
	json_object_put(res);
LEAVE_FUNC;
}

static	void
RPC_SendEvent(
	TermNode *term,
	json_object *obj)
{
	json_object *params,*meta,*child,*res,*event_data;
	SessionData *data;
	LD_Node		*ld;
	RecordStruct *rec;
	LargeByteString *scrdata;
	const char *session_id,*window,*widget,*event;
	size_t	size;
	int i;
ENTER_FUNC;
	params = json_object_object_get(obj,"params");
	if (!CheckJSONObject(params,json_type_object)) {
		Warning("request have not params");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	meta = json_object_object_get(params,"meta");
	if (!CheckJSONObject(meta,json_type_object)) {
		Warning("request have not meta");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	child = json_object_object_get(meta,"session_id");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not session_id");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	session_id = json_object_get_string(child);
	data = LookupSession(session_id);
	if (data == NULL) {
		Warning("session [%s] does not found",session_id);
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}

	if (data->fInProcess) {
		Warning("Error: %s is proccesing in other thread",session_id);
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}

	data->term = term;

	// readterminal
	event_data = json_object_object_get(params,"event_data");
	if (!CheckJSONObject(event_data,json_type_object)) {
		Warning("request have not event_data");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	child = json_object_object_get(event_data,"window");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not event_data->window");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	window = json_object_get_string(child);

	child = json_object_object_get(event_data,"widget");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not event_data->widget");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	widget = json_object_get_string(child);

	child = json_object_object_get(event_data,"event");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not event_data->event");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	event = json_object_get_string(child);

	child = json_object_object_get(event_data,"event");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not event_data->event");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	event = json_object_get_string(child);

	child = json_object_object_get(event_data,"screen_data");
	if (!CheckJSONObject(child,json_type_object)) {
		Warning("request have not event_data->screen_data");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}

	memset(data->hdr->window,0,SIZE_NAME+1);
	memset(data->hdr->widget,0,SIZE_NAME+1);
	memset(data->hdr->event,0,SIZE_NAME+1);

	strncpy(data->hdr->window,window,SIZE_NAME);
	strncpy(data->hdr->widget,widget,SIZE_NAME);
	strncpy(data->hdr->event,event,SIZE_NAME);

	ld = g_hash_table_lookup(ComponentHash,data->hdr->window);
	if (ld == NULL) {
		Warning("invalid window name:%s",data->hdr->window);
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	dbgprintf("ld = [%s]",ld->info->name);
	dbgprintf("window = [%s]",data->hdr->window);

	rec = GetWindow(data->hdr->window);
	scrdata = GetScreenData(data,data->hdr->window);
	JSON_UnPackValue(NULL,(char*)json_object_to_json_string(child),rec->value);
	size = NativeSizeValue(NULL,rec->value);
	LBS_ReserveSize(scrdata,size,FALSE);
	NativePackValue(NULL,LBS_Body(scrdata),rec->value);

	data->hdr->puttype = SCREEN_NULL;
	data->hdr->command = APL_COMMAND_GET;
	data->term = term;
	data->retry = 0;

	UpdateSession(data);
	data = Process(data);

	if (data->status != SESSION_STATUS_NORMAL) {
		Warning("Error: %s Session Abort",data->hdr->uuid);
		JSONRPC_Error(term,obj,-20002,"Session Abort");
		DeregisterSession(data);
		return;
	} else {
		UpdateSession(data);
	}

	res = MakeEventResponse(obj,data);

	for (i=0;i<data->w.sp;i++) {
		if (data->w.s[i].puttype == SCREEN_CLOSE_WINDOW) {
			data->w.s[i].puttype = SCREEN_NULL;
		}
	}

	SendString(term->fp,(char*)json_object_to_json_string(res));
	if (CheckNetFile(term->fp)) {
		CloseNet(term->fp);
	} else {
		DeregisterSession(data);
	}
	json_object_put(res);
LEAVE_FUNC;
}

static	void
RPC_PandaAPI(
	TermNode *term,
	json_object *obj)
{
	json_object *params,*meta,*child,*res;
	SessionData *data;
	RecordStruct *rec;
	APIData *api;
	const char *user,*host,*ld,*window;
	char *buf;
	size_t size;
ENTER_FUNC;
	params = json_object_object_get(obj,"params");
	if (!CheckJSONObject(params,json_type_object)) {
		Warning("request have not params");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	meta = json_object_object_get(params,"meta");
	if (!CheckJSONObject(meta,json_type_object)) {
		Warning("request have not meta");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	child = json_object_object_get(meta,"user");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not user");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	user = json_object_get_string(child);

	child = json_object_object_get(meta,"host");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not host");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	host = json_object_get_string(child);

	child = json_object_object_get(meta,"ld");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not ld");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	ld = json_object_get_string(child);

	child = json_object_object_get(meta,"window");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("request have not window");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		return;
	}
	window = json_object_get_string(child);

	data = InitAPISession(user,ld,window,host);
	if (data == NULL) {
		Warning("api %s not found",ld);
		JSONRPC_Error(term,obj,-20003,"Invalid Window");
		return;
	}
	data->term = term;
	data->retry = 0;
	api = data->apidata;

	rec = GetWindow((char*)window);
	JSON_UnPackValue(NULL,(char*)json_object_to_json_string(params),rec->value);
	size = NativeSizeValue(NULL,rec->value);
	LBS_ReserveSize(api->rec,size,FALSE);
	NativePackValue(NULL,LBS_Body(api->rec),rec->value);

	data = Process(data);
	api = data->apidata;

	res = MakeJSONResponseTemplate(obj);
	NativeUnPackValue(NULL,LBS_Body(api->rec),rec->value);
	buf = xmalloc(JSON_SizeValue(NULL,rec->value));
	JSON_PackValue(NULL,buf,rec->value);
	child = json_tokener_parse(buf);
	xfree(buf);

	json_object_object_add(res,"result",child);

	SendString(term->fp,(char*)json_object_to_json_string(res));
	if (CheckNetFile(term->fp)) {
		CloseNet(term->fp);
	}
	FreeSessionData(data);
	json_object_put(res);
LEAVE_FUNC;
}

static	void
JSONRPCHandler(
	TermNode *term)
{
	char *reqjson,*method;
	json_object *obj,*child;
ENTER_FUNC;
	reqjson =  RecvStringNew(term->fp); ON_IO_ERROR(term->fp,badio);
	obj = json_tokener_parse(reqjson);
	xfree(reqjson);
	if (!CheckJSONObject(obj,json_type_object)) {
		Warning("jsonrpc json parse error");
		JSONRPC_Error(term,NULL,-32700,"Parse Error");
		return;
	}
	child = json_object_object_get(obj,"jsonrpc");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("jsonrpc invalid reqeust");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		json_object_put(obj);
		return;
	}
	if (strcmp((char*)json_object_get_string(child),"2.0")) {
		Warning("jsonrpc invalid reqeust");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		json_object_put(obj);
		return;
	}
	child = json_object_object_get(obj,"id");
	if (!CheckJSONObject(child,json_type_int)) {
		Warning("jsonrpc invalid reqeust");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		json_object_put(obj);
		return;
	}
	child = json_object_object_get(obj,"params");
	if (!CheckJSONObject(child,json_type_object)) {
		Warning("jsonrpc invalid reqeust");
		JSONRPC_Error(term,obj,-32600,"Invalid Request");
		json_object_put(obj);
		return;
	}
	child = json_object_object_get(obj,"method");
	if (!CheckJSONObject(child,json_type_string)) {
		Warning("jsonrpc method not found");
		JSONRPC_Error(term,obj,-32601,"Method not found");
		json_object_put(obj);
		return;
	}
	method = (char*)json_object_get_string(child);
	if (!strcmp(method,"start_session")) {
		RPC_StartSession(term,obj);
	} else if (!strcmp(method,"end_session")) {
		RPC_EndSession(term,obj);
	} else if (!strcmp(method,"get_window")) {
		RPC_GetWindow(term,obj);
	} else if (!strcmp(method,"send_event")) {
		RPC_SendEvent(term,obj);
	} else if (!strcmp(method,"panda_api")) {
		RPC_PandaAPI(term,obj);
	} else {
		Warning("jsonrpc method(%s) not found",method);
		JSONRPC_Error(term,obj,-32601,"Method not found");
	}
	json_object_put(obj);
badio:
LEAVE_FUNC;
}

static	void
TermThread(
	TermNode	*term)
{
ENTER_FUNC;
	JSONRPCHandler(term);
	FreeQueue(term->que);
	xfree(term);
LEAVE_FUNC;
}

extern	int
ConnectTerm(
	int		_fhTerm)
{
	int				fhTerm;
	pthread_t		thr;
	pthread_attr_t	attr;
	TermNode		*term;

ENTER_FUNC;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,256*1024);
	if ((fhTerm = accept(_fhTerm,0,0)) < 0) {
		Error("accept(2) failure:%s",strerror(errno));
	}
	term = New(TermNode);
	term->que = NewQueue();
	term->fp = SocketToNet(fhTerm);
	pthread_create(&thr,&attr,(void *(*)(void *))TermThread,(void *)term);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(fhTerm); 
}

extern	void
InitTerm(void)
{
ENTER_FUNC;
	RecParserInit();
	if (ThisEnv->linkrec != NULL) {
		InitializeValue(ThisEnv->linkrec->value);
	}
LEAVE_FUNC;
}
