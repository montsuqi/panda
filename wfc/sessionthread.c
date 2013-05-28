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
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>

#include	"libmondai.h"
#include	"RecParser.h"
#include	"LDparser.h"
#include	"queue.h"
#include	"term.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"debug.h"

static	Queue		*workq;

extern	SessionCtrl*
NewSessionCtrl(
	SessionCtrlType type)
{
	SessionCtrl *ctrl;
	
	ctrl = g_new0(SessionCtrl,1);
	ctrl->type = type;
	ctrl->waitq = NewQueue();
	ctrl->sysdbval = RecParseValueMem(SYSDBVAL_DEF,NULL);
	InitializeValue(ctrl->sysdbval);
	ctrl->sysdbvals = RecParseValueMem(SYSDBVALS_DEF,NULL);

	InitializeValue(ctrl->sysdbvals);
	return ctrl;
}

extern	void
FreeSessionCtrl(
	SessionCtrl *ctrl)
{
	if (ctrl->sysdbval != NULL) {
		FreeValueStruct(ctrl->sysdbval);
	}
	if (ctrl->sysdbvals != NULL) {
		FreeValueStruct(ctrl->sysdbvals);
	}
	FreeQueue(ctrl->waitq);
	g_free(ctrl);
}

static	void
TimevalToString(
	char *buf,
	size_t size,
	struct timeval tv)
{
	snprintf(buf,size,"%lld.%06d", (long long)tv.tv_sec, (int)tv.tv_usec);
}

static	void
_strftime(
	char *ret, 
	size_t size,
	time_t time)
{
	struct tm tmp;
	localtime_r(&time, &tmp);
	strftime(ret, size, "%a %b %d %H:%M:%S %Y", &tmp);
}

static	void
SetSysdbval(
	SessionData *session,
	ValueStruct *sysdbval)
{
	ValueStruct *v;
	char buf[128];

	v = GetRecordItem(sysdbval,"id");
	SetValueString(v,session->hdr->uuid,NULL);
	v = GetRecordItem(sysdbval,"user");
	SetValueString(v,session->hdr->user,NULL);
	v = GetRecordItem(sysdbval,"host");
	SetValueString(v,session->host,NULL);
	v = GetRecordItem(sysdbval,"agent");
	SetValueString(v,session->agent,NULL);
	v = GetRecordItem(sysdbval,"window");
	SetValueString(v,session->hdr->window,NULL);
	v = GetRecordItem(sysdbval,"widget");
	SetValueString(v,session->hdr->widget,NULL);
	v = GetRecordItem(sysdbval,"event");
	SetValueString(v,session->hdr->event,NULL);
	v = GetRecordItem(sysdbval,"in_process");
	SetValueString(v,(session->fInProcess ? "T":"F"),NULL);
	v = GetRecordItem(sysdbval,"create_time");
	_strftime(buf, sizeof(buf), session->create_time.tv_sec);
	SetValueString(v,buf,NULL);
	v = GetRecordItem(sysdbval,"access_time");
	_strftime(buf, sizeof(buf), session->access_time.tv_sec);
	SetValueString(v,buf,NULL);
	v = GetRecordItem(sysdbval,"process_time");
	TimevalToString(buf,sizeof(buf),session->process_time);
	SetValueString(v,buf,NULL);
	v = GetRecordItem(sysdbval,"total_process_time");
	TimevalToString(buf,sizeof(buf),session->total_process_time);
	SetValueString(v,buf,NULL);
	v = GetRecordItem(sysdbval,"count");
	snprintf(buf,sizeof(buf),"%d",session->count);
	SetValueString(v,buf,NULL);
}

static	void
InsertSession(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->session == NULL) {
		Warning("ctrl->session is NULL");
		return;
	}
	if (g_hash_table_lookup(hash,ctrl->session->hdr->uuid) != NULL) {
		Warning("[%s] is already exist",ctrl->session->hdr->uuid);
		return;
	}
	SetSysdbval(ctrl->session,ctrl->session->sysdbval);
	g_hash_table_insert(hash,StrDup(ctrl->session->hdr->uuid),ctrl->session);
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
UpdateSession(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->session == NULL) {
		Warning("ctrl->session is NULL");
		return;
	}
	SetSysdbval(ctrl->session,ctrl->session->sysdbval);
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
DeleteSession(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->session == NULL) {
		Warning("ctrl->session is NULL");
		return;
	}
	if (g_hash_table_lookup(hash,ctrl->session->hdr->uuid) != NULL) {
		g_hash_table_remove(hash,ctrl->session->hdr->uuid);
	}
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
LookupSession(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->id == NULL) {
		Warning("ctrl->id is NULL");
		return;
	}
	if ((ctrl->session = g_hash_table_lookup(hash,ctrl->id)) == NULL) {
		return;
	}
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
GetSessionNum(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ctrl->size = g_hash_table_size(hash);
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
GetData(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ValueStruct *v;

	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->sysdbval == NULL) {
		Warning("ctrl->sysdbval is NULL");
		return;
	}
	v = GetRecordItem(ctrl->sysdbval,"id");
	if ((ctrl->session = g_hash_table_lookup(hash,ValueToString(v,NULL))) == NULL) {
		return;
	}
	if (ctrl->sysdbval != NULL) {
		CopyValue(ctrl->sysdbval,ctrl->session->sysdbval);
	}
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
GetMessage(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ValueStruct *v,*w;

	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->sysdbval == NULL) {
		Warning("ctrl->sysdbval is NULL");
		return;
	}
	v = GetRecordItem(ctrl->sysdbval,"id");
	if ((ctrl->session = g_hash_table_lookup(hash,ValueToString(v,NULL))) == NULL) {
		return;
	}
	v = GetRecordItem(ctrl->session->sysdbval,"popup");
	w = GetRecordItem(ctrl->sysdbval,"popup");
	SetValueString(w,ValueToString(v,NULL),NULL);
	v = GetRecordItem(ctrl->session->sysdbval,"dialog");
	w = GetRecordItem(ctrl->sysdbval,"dialog");
	SetValueString(w,ValueToString(v,NULL),NULL);
	v = GetRecordItem(ctrl->session->sysdbval,"abort");
	w = GetRecordItem(ctrl->sysdbval,"abort");
	SetValueString(w,ValueToString(v,NULL),NULL);
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
ResetMessage(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ValueStruct *v;

	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->sysdbval == NULL) {
		Warning("ctrl->sysdbval is NULL");
		return;
	}
	v = GetRecordItem(ctrl->sysdbval,"id");
	if ((ctrl->session = g_hash_table_lookup(hash,ValueToString(v,NULL))) == NULL) {
		return;
	}
	v = GetRecordItem(ctrl->session->sysdbval,"popup");
	SetValueString(v,"",NULL);
	v = GetRecordItem(ctrl->session->sysdbval,"dialog");
	SetValueString(v,"",NULL);
	v = GetRecordItem(ctrl->session->sysdbval,"abort");
	SetValueString(v,"",NULL);
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
SetMessage(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ValueStruct *v,*w;

	ctrl->rc = SESSION_CONTROL_NG;
	if (ctrl->sysdbval == NULL) {
		Warning("ctrl->sysdbval is NULL");
		return;
	}
	v = GetRecordItem(ctrl->sysdbval,"id");
	if ((ctrl->session = g_hash_table_lookup(hash,ValueToString(v,NULL))) == NULL) {
		return;
	}
	v = GetRecordItem(ctrl->session->sysdbval,"popup");
	w = GetRecordItem(ctrl->sysdbval,"popup");
	SetValueString(v,ValueToString(w,NULL),NULL);
	v = GetRecordItem(ctrl->session->sysdbval,"dialog");
	w = GetRecordItem(ctrl->sysdbval,"dialog");
	SetValueString(v,ValueToString(w,NULL),NULL);
	v = GetRecordItem(ctrl->session->sysdbval,"abort");
	w = GetRecordItem(ctrl->sysdbval,"abort");
	SetValueString(v,ValueToString(w,NULL),NULL);
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
_SetMessage(
	char *id,
	SessionData *session,
	SessionCtrl *ctrl)
{
	ValueStruct *v,*w;

	if (ctrl->sysdbval == NULL) {
		Warning("ctrl->sysdbval is NULL");
		return;
	}
	if (session == NULL) {
		return;
	}
	v = GetRecordItem(session->sysdbval,"popup");
	w = GetRecordItem(ctrl->sysdbval,"popup");
	SetValueString(v,ValueToString(w,NULL),NULL);
	v = GetRecordItem(session->sysdbval,"dialog");
	w = GetRecordItem(ctrl->sysdbval,"dialog");
	SetValueString(v,ValueToString(w,NULL),NULL);
	v = GetRecordItem(session->sysdbval,"abort");
	w = GetRecordItem(ctrl->sysdbval,"abort");
	SetValueString(v,ValueToString(w,NULL),NULL);
}

static	void
SetMessageAll(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ctrl->rc = SESSION_CONTROL_NG;
	g_hash_table_foreach(hash,(GHFunc)_SetMessage,ctrl);
	ctrl->rc = SESSION_CONTROL_OK;
}

static	void
_GetData(
	char *id,
	SessionData *session,
	SessionCtrl *ctrl)
{
	ValueStruct *v,*a;
	int size;

	if (session == NULL) {
		return;
	}
	if (ctrl->sysdbvals == NULL) {
		Warning("ctrl->sysdbvals is null");
		return;
	}
	a = GetRecordItem(ctrl->sysdbvals,"values");
	size = ValueArraySize(a);
	if (ctrl->size >= size) {
		if (ctrl->size == size) {
			Warning("GetDataAll SYSDBVALS_SIZE(%d) over",size);
		}
		ctrl->size += 1;
		return;
	}
	v = GetArrayItem(a,ctrl->size);
	if (v != NULL) {
		CopyValue(v,session->sysdbval);
	}
	ctrl->size += 1;
}

static	void
GetDataAll(
	GHashTable *hash,
	SessionCtrl *ctrl)
{
	ctrl->rc = SESSION_CONTROL_NG;
	ctrl->size = 0;
	g_hash_table_foreach(hash,(GHFunc)_GetData,ctrl);
	ctrl->rc = SESSION_CONTROL_OK;
}

extern	void
SessionEnqueue(
	SessionCtrl	*ctrl)
{
	EnQueue(workq,ctrl);
}

static	void
SessionThread(
	void	*para)
{
	SessionCtrl *ctrl;
	GHashTable *hash;
ENTER_FUNC;
	workq = NewQueue();
	hash = NewNameHash();
	RecParserInit();
	while (TRUE) {
		ctrl = (SessionCtrl *)DeQueue(workq);
		switch (ctrl->type) {
		case SESSION_CONTROL_INSERT:
			InsertSession(hash,ctrl);
			break;
		case SESSION_CONTROL_UPDATE:
			UpdateSession(hash,ctrl);
			break;
		case SESSION_CONTROL_DELETE:
			DeleteSession(hash,ctrl);
			break;
		case SESSION_CONTROL_LOOKUP:
			LookupSession(hash,ctrl);
			break;
		case SESSION_CONTROL_GET_SESSION_NUM:
			GetSessionNum(hash,ctrl);
			break;
		case SESSION_CONTROL_GET_DATA:
			GetData(hash,ctrl);
			break;
		case SESSION_CONTROL_GET_MESSAGE:
			GetMessage(hash,ctrl);
			break;
		case SESSION_CONTROL_RESET_MESSAGE:
			ResetMessage(hash,ctrl);
			break;
		case SESSION_CONTROL_SET_MESSAGE:
			SetMessage(hash,ctrl);
			break;
		case SESSION_CONTROL_SET_MESSAGE_ALL:
			SetMessageAll(hash,ctrl);
			break;
		case SESSION_CONTROL_GET_DATA_ALL:
			GetDataAll(hash,ctrl);
			break;
		}
		EnQueue(ctrl->waitq,ctrl);
	}
	pthread_exit(NULL);
LEAVE_FUNC;
}

extern	void
StartSessionThread(void)
{
	static	pthread_t	ses;
ENTER_FUNC;
	pthread_create(&ses,NULL,(void *(*)(void *))SessionThread,NULL);
LEAVE_FUNC;
}

