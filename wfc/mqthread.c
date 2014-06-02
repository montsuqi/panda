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

#define	APS_STICK

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>
#include	<signal.h>
#include	<errno.h>

#include	"libmondai.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"queue.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"mqthread.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"directory.h"
#include	"term.h"
#include	"glterm.h"
#include	"enum.h"
#include	"message.h"
#include	"debug.h"

static	void
ClearAPS_Node(
	LD_Node		*ld,
	int			ix)
{
	APS_Node	*aps;

	aps = &ld->aps[ix];
	if (aps->fp != NULL) {
		CloseNet(aps->fp);
		aps->fp = NULL;
	}
	aps->count = 0;
}

static	void
ActivateAPS_Node(
	APS_Node	*aps,
	NETFILE		*fp)
{
ENTER_FUNC;
	aps->count = 0;
	aps->fp = fp;
LEAVE_FUNC;
}

static	Bool
CheckAPS(
	PacketClass	klass,
	LD_Node		*ld,
	int			ix,
	char		*uuid)
{
	APS_Node		*aps;

	aps = &ld->aps[ix];
	if (aps->fp != NULL ) {
		SendPacketClass(aps->fp,klass);ON_IO_ERROR(aps->fp,badio);
		return TRUE;
	}
badio:
	return FALSE;
}

static	void
WaitConnect(
	LD_Node	*ld,
	int		ix)
{
ENTER_FUNC;
	pthread_mutex_lock(&ld->lock);
	if (ld->aps[ix].fp == NULL) {
		pthread_cond_wait(&ld->conn, &ld->lock);
	}
	pthread_mutex_unlock(&ld->lock);
LEAVE_FUNC;
}

static	Bool
PutAPS(
	LD_Node			*ld,
	int				ix,
	SessionData		*data)
{
	APS_Node	*aps;
	int			i;
	NETFILE		*fp;

ENTER_FUNC;
#ifdef	DEBUG
	printf("uuid = [%s]\n",data->hdr->uuid);
	printf("user = [%s]\n",data->hdr->user);
#endif

	dbgprintf("ld = [%s]",data->ld->info->name);
	aps = &ld->aps[ix];
	data->apsid = aps->id;
	fp = aps->fp;

	dbgmsg("EVENTDATA");
	SendPacketClass(fp,APS_EVENTDATA);	ON_IO_ERROR(fp,badio);
	SendChar  (fp,data->hdr->command);	ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->uuid);		ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->tenant);	ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->host);		ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->tempdir);	ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->user);		ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->window);	ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->widget);	ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->event);	ON_IO_ERROR(fp,badio);
	SendChar  (fp,data->hdr->dbstatus);	ON_IO_ERROR(fp,badio);

	dbgmsg("WINDOW_STACK");
	SendPacketClass(fp,APS_WINDOW_STACK);	ON_IO_ERROR(fp,badio);
	SendInt(fp,data->w.sp);					ON_IO_ERROR(fp,badio);
	for (i=0;i<data->w.sp;i++) {
		SendChar(fp,data->w.s[i].puttype);	ON_IO_ERROR(fp,badio);
		SendString(fp,data->w.s[i].window);	ON_IO_ERROR(fp,badio);
	}

	dbgmsg("SCRDATA");
	SendPacketClass(fp,APS_SCRDATA);		ON_IO_ERROR(fp,badio);
	for	(i=0;i<data->ld->info->cWindow;i++) {
		if (data->scrdata[i] != NULL) {
			dbgprintf("[%s]",data->ld->info->windows[i]->name);
			SendLBS(fp,data->scrdata[i]);	ON_IO_ERROR(fp,badio);
		}
	}

	dbgmsg("LINKDATA");
	if (data->linkdata != NULL) {
		SendPacketClass(fp,APS_LINKDATA);	ON_IO_ERROR(fp,badio);
		SendLBS(fp,data->linkdata);			ON_IO_ERROR(fp,badio);
	}

	dbgmsg("SPADATA");
	data->spa = g_hash_table_lookup(data->spadata,data->ld->info->name);
	if (data->spa == NULL) {
		data->spa = NewLBS();
		g_hash_table_insert(data->spadata,
			StrDup(data->ld->info->name),data->spa);
		InitializeValue(ld->info->sparec->value);
		LBS_ReserveSize(data->spa,
			NativeSizeValue(NULL,ld->info->sparec->value),FALSE);
		NativePackValue(NULL,LBS_Body(data->spa),ld->info->sparec->value);
	}
	SendPacketClass(fp,APS_SPADATA);		ON_IO_ERROR(fp,badio);
	SendLBS(fp,data->spa);					ON_IO_ERROR(fp,badio);

	dbgmsg("END");
	SendPacketClass(fp,APS_END);			ON_IO_ERROR(fp,badio);

	aps->count ++;
LEAVE_FUNC;
	return TRUE;
badio:
LEAVE_FUNC;
	return FALSE;
}

static	Bool
GetAPS_Control(
	LD_Node			*ld,
	int				ix,
	SessionData		*data)
{
	APS_Node		*aps;
	PacketClass		c;
	NETFILE			*fp;

ENTER_FUNC;
	aps = &ld->aps[ix];
	fp = aps->fp;

	c = RecvPacketClass(fp);						ON_IO_ERROR(fp,badio);
	if (c != APS_CTRLDATA) {
		Message("aps die [%s@%s] %s:%s:%s",
			data->hdr->user,data->hdr->uuid, 
			data->hdr->window,data->hdr->widget,data->hdr->event);
		return FALSE;
	}
	RecvnString(fp,SIZE_NAME,data->hdr->user);		ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,data->hdr->window);	ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,data->hdr->widget);	ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,data->hdr->event);		ON_IO_ERROR(fp,badio);
	data->hdr->dbstatus = (char)RecvChar(fp);		ON_IO_ERROR(fp,badio);
	data->hdr->puttype = RecvChar(fp);				ON_IO_ERROR(fp,badio);

	dbgprintf("hdr->window  = [%s]",data->hdr->window);
	dbgprintf("hdr->puttype = %02X",(int)data->hdr->puttype);
LEAVE_FUNC;
	return TRUE;
badio:
LEAVE_FUNC;
	return FALSE;
}

static	Bool
GetAPS_Value(
	NETFILE			*fpLD,
	SessionData		*data)
{
	int	i;

ENTER_FUNC;
	dbgmsg("WINDOW_STACK");
	SendPacketClass(fpLD,APS_WINDOW_STACK);ON_IO_ERROR(fpLD,badio);
	data->w.sp = RecvInt(fpLD);			ON_IO_ERROR(fpLD,badio);
	for (i=0;i<data->w.sp;i++) {
		data->w.s[i].puttype = RecvChar(fpLD);
			ON_IO_ERROR(fpLD,badio);
		RecvnString(fpLD,SIZE_NAME,data->w.s[i].window);
			ON_IO_ERROR(fpLD,badio);
	}

	dbgmsg("LINKDATA");
	SendPacketClass(fpLD,APS_LINKDATA);	ON_IO_ERROR(fpLD,badio);
	RecvLBS(fpLD,data->linkdata);		ON_IO_ERROR(fpLD,badio);

	dbgmsg("SPADATA");
	SendPacketClass(fpLD,APS_SPADATA);	ON_IO_ERROR(fpLD,badio);
	RecvLBS(fpLD,data->spa);			ON_IO_ERROR(fpLD,badio);

	dbgmsg("SCRDATA");
	SendPacketClass(fpLD,APS_SCRDATA);	ON_IO_ERROR(fpLD,badio);
	for	(i=0;i<data->cWindow;i++) {
		if (data->scrdata[i] != NULL) {
			RecvLBS(fpLD,data->scrdata[i]);	ON_IO_ERROR(fpLD,badio);
		}
	}
	dbgmsg("END");
	SendPacketClass(fpLD,APS_END);		ON_IO_ERROR(fpLD,badio);
LEAVE_FUNC;
	return TRUE;
badio:
	Warning("badio");
LEAVE_FUNC;
	return FALSE;
}

extern	void
MessageEnqueue(
	MQ_Node *mq,
	SessionData *data)
{
ENTER_FUNC;
	EnQueue(mq->que,data);
LEAVE_FUNC;
}

typedef	struct {
	int		no;
	MQ_Node	*mq;
}	APS_Start;

static	SessionData	*
SelectData(
	Queue	*que,
	int		ix)
{
	SessionData	*data;
#ifdef	APS_STICK
	LD_Node		*ld;
#endif

ENTER_FUNC;
#ifndef	APS_STICK
	data = DeQueue(que); 
#else
	OpenQueue(que);
	WaitQueue(que);
	RewindQueue(que);
	while ((data = GetElement(que)) != NULL) {
		ld = data->ld;
		if (data->apsid == -1){
			break;
		}
		if (data->apsid == ld->aps[ix].id){
			break;
		}
		if ((ThisEnv->mlevel == MULTI_NO) ||
			(ThisEnv->mlevel == MULTI_ID)) {
			break;
		}
	}
	if (data != NULL) {
		if (data->apsid == -1) {
			dbgmsg("virgin data");
		}
		if (data->apsid == ld->aps[ix].id) {
			dbgmsg("same aps selected");
		}
		data = WithdrawQueue(que);
	}
	if (data == NULL){
		dbgmsg("null");
		ReleaseQueue(que);
	}
	CloseQueue(que);
#endif
LEAVE_FUNC;
	return	(data);
}

static	void
SendTermMessage(
	SessionData *data,
	MQ_Node		*mq,
	int	ix)
{
	LD_Node			*ld,*newld;
	NETFILE			*fp;

	fp = NULL; 
	ld = data->ld;
	if (CheckAPS(APS_REQ, ld,ix,data->hdr->uuid)) {
		if (PutAPS(ld,ix,data) && GetAPS_Control(ld,ix,data)) {
			fp = ld->aps[ix].fp;
		}
	}
	if (fp == NULL) {
		ClearAPS_Node(ld,ix);
		data->apsid = -1;
		data->retry ++;
		if ((MaxTransactionRetry > 0) && 
			(MaxTransactionRetry < data->retry)) {
			Warning("transaction abort %s", mq->name);
			data->status = SESSION_STATUS_ABORT;
			TermEnqueue(data->term,data);
		} else {
			Warning("transaction retry %s", mq->name);
			EnQueue(mq->que,data);
		}
		WaitConnect(ld,ix);
		sched_yield();
	} else {
		dbgprintf("before data->puttype = %02X",data->hdr->puttype);
		newld = g_hash_table_lookup(ComponentHash,data->hdr->window);
		if (!GetAPS_Value(fp,data)) {
			Warning("GetAPS_Value failure %s -> session end",data->hdr->window);
			data->status = SESSION_STATUS_ABORT;
			TermEnqueue(data->term,data);
			return;
		}
		dbgprintf("after data->puttype = %02X",data->hdr->puttype);

		if (newld == NULL) {
			Warning("ld not found for %s -> session end",data->hdr->window);
			data->status = SESSION_STATUS_ABORT;
			TermEnqueue(data->term,data);
			return;
		}

		switch (data->hdr->puttype) {
		case SCREEN_CHANGE_WINDOW:
		case SCREEN_JOIN_WINDOW:
			dbgmsg("transition");
			data->hdr->command = APL_COMMAND_LINK;
			if (newld != ld) {
				ChangeLD(data,newld);
			}
			CoreEnqueue(data);
			break;
		case SCREEN_NEW_WINDOW:
			dbgmsg("new");
			if (newld == ld) {
				TermEnqueue(data->term,data);
			} else {
				data->hdr->command = APL_COMMAND_LINK;
				ChangeLD(data,newld);
				CoreEnqueue(data);
			}
			break;
		case SCREEN_CURRENT_WINDOW:
			dbgmsg("current");
			TermEnqueue(data->term,data);
			break;
		case SCREEN_CLOSE_WINDOW:
			dbgmsg("close");
			TermEnqueue(data->term,data);
			break;
		default:
			Warning("invalid puttype [%d]",data->hdr->puttype);
			break;
		}
	}
}

static	void
SendAPIMessage(
	SessionData *data,
	MQ_Node		*mq,
	int	ix)
{
	LD_Node		*ld;
	NETFILE		*fp;
	PacketClass	c;

	fp = NULL;
	ld = data->ld;
	if (!CheckAPS(APS_API,ld,ix,data->hdr->uuid)) {
		data->apidata->status = WFC_API_NOT_FOUND;
	} else {
		fp = ld->aps[ix].fp;
		SendString(fp,data->hdr->window);		ON_IO_ERROR(fp,badio);
		c = RecvChar(fp);						ON_IO_ERROR(fp,badio);
		if (c == APS_NULL) {
			Warning("api[%s] not found",data->hdr->window);
			data->apidata->status = WFC_API_NOT_FOUND;
		} else {
			SendString(fp,data->hdr->uuid);		ON_IO_ERROR(fp,badio);
			SendString(fp,data->hdr->tenant);	ON_IO_ERROR(fp,badio);
			SendString(fp,data->hdr->host);		ON_IO_ERROR(fp,badio);
			SendString(fp,data->hdr->tempdir);	ON_IO_ERROR(fp,badio);
			SendString(fp,data->hdr->user);		ON_IO_ERROR(fp,badio);
			SendLBS(fp,data->apidata->rec);		ON_IO_ERROR(fp,badio);
			RecvLBS(fp,data->apidata->rec);		ON_IO_ERROR(fp,badio);
			data->apidata->status = WFC_API_OK;
		}
	}	
	TermEnqueue(data->term,data);
	return;
badio:
	ClearAPS_Node(ld,ix);
	Warning("transaction abort %s", mq->name);
	data->status = SESSION_STATUS_ABORT;
	data->apidata->status = WFC_API_ERROR;
	TermEnqueue(data->term,data);
	WaitConnect(ld,ix);
	sched_yield();
}

static	void
MessageThread(
	APS_Start	*aps)
{
	SessionData	*data;
	int			ix;
	MQ_Node		*mq;

ENTER_FUNC;
	mq = aps->mq; 
	ix = aps->no;
	dbgprintf("start %s(%d)\n",mq->name,ix);
	do {
		if ((data = SelectData(mq->que,ix)) != NULL) {
			dbgprintf("act %s\n",mq->name);
			switch(data->type) {
			case SESSION_TYPE_TERM:
				SendTermMessage(data,mq,ix);
				break;
			case SESSION_TYPE_API:
				SendAPIMessage(data,mq,ix);
				break;
			}
		} else {
			sched_yield();
		}
	} while(TRUE);
LEAVE_FUNC;
}

static	void
StartMessageThread(
	char	*name,
	MQ_Node	*mq,
	void	*dummy)
{
	APS_Start	*aps;
	int			i;
	LD_Struct	*ld;

ENTER_FUNC;
#ifdef	TRACE
	printf("start thread for %s\n",name);
#endif
	switch (ThisEnv->mlevel) { 
	case MULTI_NO:
	case MULTI_ID:
		aps = New(APS_Start);
		aps->mq = mq;
		aps->no = 0;
		pthread_create(&mq->thr,NULL,(void *(*)(void *))MessageThread,aps);
		break;
	case MULTI_LD:
	case MULTI_APS:
		if ((ld =  g_hash_table_lookup(ThisEnv->LD_Table,name)) != NULL) {
			for	( i = 0 ; i < ld->nports ; i ++ ) {
				aps = New(APS_Start);
				aps->mq = mq;
				aps->no = i;
				pthread_create(&mq->thr,NULL,(void *(*)(void *))MessageThread,aps);
			}
		}
		break;
	}
LEAVE_FUNC;
}

static	GHashTable	*mqs;
extern	void
SetupMessageQueue(void)
{
	int		i;
	MQ_Node	*mq;

ENTER_FUNC;
	MQ_Hash = NewNameHash();
	mqs = NewNameHash();
	switch (ThisEnv->mlevel) {
	case MULTI_NO:
		mq = New(MQ_Node);
		mq->name = "";
		mq->que = NewQueue();
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			if (g_hash_table_lookup(MQ_Hash, ThisEnv->ld[i]->name) == NULL) {
				(void)g_hash_table_insert(MQ_Hash,ThisEnv->ld[i]->name,mq);
			}
			if (g_hash_table_lookup(mqs,mq->name) == NULL) {
				g_hash_table_insert(mqs,mq->name,mq);
			}
		}
		break;
	case MULTI_LD:
	case MULTI_APS:
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			mq = New(MQ_Node);
			mq->name = ThisEnv->ld[i]->name;
			mq->que = NewQueue();
			if (g_hash_table_lookup(MQ_Hash, ThisEnv->ld[i]->name) == NULL) {
				(void)g_hash_table_insert(MQ_Hash,ThisEnv->ld[i]->name,mq);
			}
			if (g_hash_table_lookup(mqs,mq->name) == NULL) {
				g_hash_table_insert(mqs,mq->name,mq);
			}
		}
		break;
	case MULTI_ID:
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			mq = g_hash_table_lookup(mqs,ThisEnv->ld[i]->group);
			if (mq == NULL) {
				mq = New(MQ_Node);
				mq->name = ThisEnv->ld[i]->group;
				mq->que = NewQueue();
				g_hash_table_insert(mqs,mq->name,mq);
			}
			if (g_hash_table_lookup(MQ_Hash,ThisEnv->ld[i]->name) == NULL) {
				g_hash_table_insert(MQ_Hash,ThisEnv->ld[i]->name,mq);
			}
		}
		break;
	  case	MULTI_DB:
		Error("not supported");
		break;
	}
	g_hash_table_foreach(mqs,(GHFunc)StartMessageThread,NULL);
LEAVE_FUNC;
}

static	int		ApsId;

static	int
TableComp(
	char	**x,
	char	**y)
{
	return	(strcmp(*y,*x));	/*	down	*/
}

extern	void
ReadyAPS(void)
{
	int			i,j,k;
	LD_Struct	*info;
	LD_Node		*ld;
	char		*cname;
ENTER_FUNC;
	k = 0;
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		info = ThisEnv->ld[i];
		ld = New(LD_Node);
		ld->nports = info->nports;
		ld->aps = (APS_Node *)xmalloc(sizeof(APS_Node) * ld->nports);
		if (ThisEnv->mlevel != MULTI_APS) {
			ld->nports = 1;
		}
		for	( j = 0 ; j < ld->nports ; j ++ ) {
			ld->aps[j].fp = NULL;
			ld->aps[j].id = ApsId ++;
			ClearAPS_Node(ld,j);
		}

		ld->info = info;
		pthread_mutex_init(&ld->lock,NULL);
		pthread_cond_init(&ld->conn,NULL);

		if (g_hash_table_lookup(APS_Hash,info->name) == NULL) {
			g_hash_table_insert(APS_Hash,info->name,ld);
		}
		dbgprintf("info->cBind = %d",info->cBind);
		dbgprintf("LD [%s]",info->name);
		for	( j = 0 ; j < info->cBind ; j ++ ) {
			cname = info->binds[j]->name;
			dbgprintf("add component [%s]",cname);
			if (g_hash_table_lookup(ComponentHash,cname) == NULL) {
				BindTable[k] = info->binds[j]->name;
				k ++;
				g_hash_table_insert(ComponentHash,StrDup(cname),ld);
			}
		}
	}
	qsort(BindTable,k,sizeof(char *),
		(int (*)(const void *,const void *))TableComp);
#ifdef	DEBUG
	for	( i = 0 ; i < k ; i ++ ) {
		printf("[%s]\n",BindTable[i]);
	}
#endif
LEAVE_FUNC;
}

extern	void
ConnectAPS(
	int		_fhAps)
{
	int		fhAps;
	char	buff[SIZE_BUFF];
	NETFILE	*fp;
	int		i;
	LD_Node	*ld;

ENTER_FUNC;
	if ((fhAps = accept(_fhAps,0,0)) < 0) {
		Error("accept(2) failure:%s",strerror(errno));
	}
	fp = SocketToNet(fhAps);
	RecvStringDelim(fp,SIZE_BUFF,buff);
#ifdef	DEBUG
	printf("connect %s\n",buff);
#endif
	if ((ld = g_hash_table_lookup(APS_Hash,buff)) != NULL) {
		if (ThisEnv->mlevel != MULTI_APS) {
			ld->nports = 1;
		}
		SendPacketClass(fp,APS_OK);
		for	( i = 0 ; i < ld->nports ; i ++ ) {
			if (ld->aps[i].fp == NULL) {
				break;
			}
		}
		if (i < ld->nports) {
			ClearAPS_Node(ld,i);
			ActivateAPS_Node(&ld->aps[i],fp);
			pthread_cond_signal(&ld->conn);
		} else {
			Warning("aps connection overflow");
			CloseNet(fp);
		}
	} else {
		SendPacketClass(fp,APS_NOT);
		CloseNet(fp);
		printf("invalid aps = [%s]\n",buff);
	}
LEAVE_FUNC;
}

extern	void
InitMessageQueue(void)
{
	int	i,cBind;

	ComponentHash = NewNameHash();
	APS_Hash = NewNameHash();
	ApsId = 0;
	cBind = 0;
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		cBind += ThisEnv->ld[i]->cBind;
	}
	if (cBind > 0) {
		BindTable = (char **)xmalloc(cBind * sizeof(char *));
	} else {
		Error("No LD defintion. Please check Directory and dbgroup.");
	}
}
