/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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
*/
#define	DEBUG
#define	TRACE

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>
#include	<sys/time.h>

#include	"types.h"
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
#include	"blob.h"
#include	"blobserv.h"
#include	"driver.h"
#include	"debug.h"

static	LD_Node	**LDs;
static	APS_Node	**APS_Table;

static	void
ClearAPS_Node(
	LD_Node		*ld,
	int			ix)
{
	APS_Node	*aps;

	aps = &ld->aps[ix];
	if		(  aps->fp  !=  NULL  ) {
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
	if		(  fp  !=  NULL  ) {
		SendInt(fp,aps->id);
	}
LEAVE_FUNC;
}

static	byte
CheckAPS(
	LD_Node		*ld,
	int			ix,
	char		*term)
{
	APS_Node	*aps;
	byte	flag;
	NETFILE	*fp;

	aps = &ld->aps[ix];
	fp = aps->fp;
	if		(  fp  !=  NULL  ) {
		SendPacketClass(fp,APS_REQ);		ON_IO_ERROR(fp,badio);
		SendString(fp,term);				ON_IO_ERROR(fp,badio);
		SendPacketClass(fp,APS_START);		ON_IO_ERROR(fp,badio);
		if		(  RecvPacketClass(fp)  ==  APS_OK  ) {
			SendPacketClass(fp,APS_REQ);		ON_IO_ERROR(fp,badio);
			SendString(fp,term);				ON_IO_ERROR(fp,badio);
			SendPacketClass(fp,APS_EVENT);		ON_IO_ERROR(fp,badio);
			flag = RecvChar(fp);				ON_IO_ERROR(fp,badio);
		} else {
			flag = 0;
		}
	} else {
	  badio:
		flag = 0;
	}
	return	(flag);
}

static	void
WaitConnect(
	LD_Node	*ld,
	int		ix)
{
ENTER_FUNC;
	pthread_mutex_lock(&ld->lock);
	if		(  ld->aps[ix].fp  ==  NULL  ) {
		pthread_cond_wait(&ld->conn, &ld->lock);
	}
	pthread_mutex_unlock(&ld->lock);
LEAVE_FUNC;
}

static	Bool
PutAPS(
	LD_Node		*ld,
	int			ix,
	SessionData	*data,
	byte		flag)
{
	MessageHeader	*hdr;
	APS_Node	*aps;
	int		i;
	Bool	fOK;
	NETFILE		*fp;

ENTER_FUNC;
#ifdef	DEBUG
	printf("term = [%s]\n",data->hdr->term);
	printf("user = [%s]\n",data->hdr->user);
#endif

	dbgprintf("ld = [%s]",data->ld->info->name);
	aps = &ld->aps[ix];
	fp = aps->fp;
	hdr = data->hdr;
	fOK = FALSE;
	if		(  data->apsid  !=  aps->id  ) {
		flag |= APS_LINKDATA;
		flag |= APS_SPADATA;
		data->apsid = aps->id;
	}
	if		(  ( flag & APS_EVENTDATA )  !=  0  ) {
		dbgmsg("EVENTDATA");
		SendPacketClass(fp,APS_EVENTDATA);	ON_IO_ERROR(fp,badio);
		SendChar(fp,hdr->status);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->term);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->user);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->window);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->widget);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->event);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->lang);			ON_IO_ERROR(fp,badio);
		SendChar(fp,hdr->dbstatus);			ON_IO_ERROR(fp,badio);
	}
	if		(  ( flag & APS_MCPDATA )  !=  0  ) {
		dbgmsg("MCPDATA");
		SendPacketClass(fp,APS_MCPDATA);	ON_IO_ERROR(fp,badio);
		SendLBS(fp,data->mcpdata);			ON_IO_ERROR(fp,badio);
		SendChar(fp,data->tnest);			ON_IO_ERROR(fp,badio);
	}
	if		(  ( flag & APS_SCRDATA )  !=  0  ) {
		dbgmsg("SCRDATA");
		SendPacketClass(fp,APS_SCRDATA);	ON_IO_ERROR(fp,badio);
		for	( i = 0 ; i < data->ld->info->cWindow ; i ++ ) {
			if		(  data->scrdata[i]  !=  NULL  ) {
				dbgprintf("[%s]",data->ld->info->windows[i]->name);
				SendLBS(fp,data->scrdata[i]);		ON_IO_ERROR(fp,badio);
			}
		}
	}
	if		(  ( flag & APS_LINKDATA )  !=  0  ) {
		dbgmsg("LINKDATA");
		if		(  data->linkdata  !=  NULL  ) {
			SendPacketClass(fp,APS_LINKDATA);	ON_IO_ERROR(fp,badio);
			SendLBS(fp,data->linkdata);			ON_IO_ERROR(fp,badio);
		}
	}
	if		(  ( flag & APS_SPADATA )  !=  0  ) {
		dbgmsg("SPADATA");
		if		(  ( data->spa = g_hash_table_lookup(data->spadata,data->ld->info->name) )
				   ==  NULL  ) {
			data->spa = NewLBS();
			g_hash_table_insert(data->spadata,StrDup(data->ld->info->name),data->spa);
			InitializeValue(ld->info->sparec->value);
			LBS_ReserveSize(data->spa,
							NativeSizeValue(NULL,ld->info->sparec->value),FALSE);
			NativePackValue(NULL,LBS_Body(data->spa),ld->info->sparec->value);
		}
		SendPacketClass(fp,APS_SPADATA);	ON_IO_ERROR(fp,badio);
		SendLBS(fp,data->spa);				ON_IO_ERROR(fp,badio);
	}
	SendPacketClass(fp,APS_END);		ON_IO_ERROR(fp,badio);
	aps->count ++;
	fOK = TRUE;
  badio:
LEAVE_FUNC;
	return(fOK);
}

static	byte
GetAPS_Control(
	LD_Node			*ld,
	int				ix,
	MessageHeader	*hdr)
{
	APS_Node	*aps;
	PacketClass	c;
	NETFILE		*fp;
	byte		flag;
	Bool		done;

ENTER_FUNC;
	aps = &ld->aps[ix];
	fp = aps->fp;
	done = FALSE;
	while	(  !done  )	{
		switch	( c = RecvPacketClass(fp) ) { 
		  case	APS_CTRLDATA:
			flag = RecvChar(fp);				ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,hdr->user);			ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,hdr->window);			ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,hdr->widget);			ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,hdr->event);			ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,hdr->lang);			ON_IO_ERROR(fp,badio);
			hdr->dbstatus = (char)RecvChar(fp);				ON_IO_ERROR(fp,badio);
			hdr->puttype = (byte)RecvInt(fp);				ON_IO_ERROR(fp,badio);
			dbgprintf("hdr->window  = [%s]",hdr->window);
			dbgprintf("hdr->puttype = %02X",(int)hdr->puttype);
			done = TRUE;
			break;
		  case	APS_BLOB:
			PassiveBLOB(fp,BlobState);			ON_IO_ERROR(fp,badio);
			flag = 0;
			break;
		  default:
		  badio:
			dbgprintf("%02X",c);
			MessageLog("aps die ?");
			done = TRUE;
			flag = 0;
			break;
		}
	}
LEAVE_FUNC;
	return	(flag); 
}

static	void
GetAPS_Value(
	NETFILE	*fpLD,
	SessionData	*data,
	PacketClass	c,
	byte		flag)
{
	int		i
	,		n;

ENTER_FUNC;
	if		(  ( flag & c )  !=  0  ) {
		dbgmsg("send");
		SendPacketClass(fpLD,c);		ON_IO_ERROR(fpLD,badio);
		switch	(c) {
		  case	APS_WINCTRL:
			dbgmsg("WINCTRL");
			n = RecvInt(fpLD);		ON_IO_ERROR(fpLD,badio);
			for	( i = 0 ; i < n ; i ++ ) {
				data->w.control[data->w.n].PutType = (byte)RecvInt(fpLD);
				ON_IO_ERROR(fpLD,badio);
				RecvnString(fpLD,SIZE_NAME,data->w.control[data->w.n].window);
				ON_IO_ERROR(fpLD,badio);
				data->w.n ++;
			}
			break;
		  case	APS_MCPDATA:
			dbgmsg("MCPDATA");
			RecvLBS(fpLD,data->mcpdata);		ON_IO_ERROR(fpLD,badio);
			data->tnest = (int)RecvChar(fpLD);	ON_IO_ERROR(fpLD,badio);
			break;
		  case	APS_LINKDATA:
			dbgmsg("LINKDATA");
			RecvLBS(fpLD,data->linkdata);	ON_IO_ERROR(fpLD,badio);
			break;
		  case	APS_SPADATA:
			dbgmsg("SPADATA");
			RecvLBS(fpLD,data->spa);
			ON_IO_ERROR(fpLD,badio);
			break;
		  case	APS_SCRDATA:
			dbgmsg("SCRDATA");
			dbgprintf("cWindow = %d",data->cWindow);
			for	( i = 0 ; i < data->cWindow ; i ++ ) {
				if		(  data->scrdata[i]  !=  NULL  ) {
					dbgprintf("i = %d",i);
					RecvLBS(fpLD,data->scrdata[i]);	ON_IO_ERROR(fpLD,badio);
				}
			}
			break;
		  default:
		  badio:
			Message("protocol error, class = [%X]", (int)c);
			break;
		}
	}
LEAVE_FUNC;
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
	int		no
	,		id;
	MQ_Node	*mq;
}	APS_Start;

static	SessionData	*
SelectData(
	Queue	*que,
	int		ix,
	int		id)
{
	SessionData	*data;
	LD_Node		*ld;

ENTER_FUNC;
	OpenQueue(que);
	WaitQueue(que);
#ifdef	DEBUG
#if	0
	{
		RewindQueue(que);
		while	(  ( data = GetElement(que) )  !=  NULL  ) {
			ld = data->ld;
			dbgprintf("apsid = %d\nid    = %d\n",data->apsid,ld->aps[ix].id);
		}
	}
#endif
#endif
	RewindQueue(que);
	while	(	( data = GetElement(que) )  !=  NULL  ) {
		if		(	(  ThisEnv->mlevel  ==  MULTI_NO  )
				||	(  ThisEnv->mlevel  ==  MULTI_ID  ) )	break;
		if		(  data->mtype  ==  MESSAGE_TYPE_EVENT  ) {
			ld = data->ld;
			if		(  data->apsid  ==  -1  )	break;
			if		(  data->apsid  ==  ld->aps[ix].id  )	break;
		} else {
			if		(  data->dbstatus[id]  !=  DB_STATUS_NOCONNECT  )	break;
		}
	}
	if		(  data  !=  NULL  ) {
		dbgprintf("data->apsid = %d apsid = %d",data->apsid,id);
		if		(  data->mtype  ==  MESSAGE_TYPE_EVENT  ) {
			if		(  data->apsid  ==  -1  ) {
				dbgmsg("virgin data");
			}
			if		(  data->apsid  ==  ld->aps[ix].id  ) {
				dbgmsg("same aps selected");
			}
		}
		data = WithdrawQueue(que);
	}
	if		(  data  ==  NULL  ){
		ReleaseQueue(que);
	}
	CloseQueue(que);

LEAVE_FUNC;
	return	(data);
}

static	void
ReleaseProcess(
	SessionData	*data)
{
	struct	timeval	tv;

ENTER_FUNC;
	gettimeofday(&tv,NULL);
	timersub(&tv,&data->start,&data->elapse);
	UnLock(data);
LEAVE_FUNC;
}

static	void
HandleTermMessage(
	SessionData *data,
	MQ_Node		*mq,
	int			ix)
{
	MessageHeader	hdr;
	LD_Node		*ld
	,			*newld;
	NETFILE		*fp;
	byte		puttype;
	byte		flag;
	char		buff[SIZE_LONGNAME+1];

	fp = NULL; 
	puttype = 0;
	ld = data->ld;
	if ((flag = CheckAPS(ld,ix,data->name)) != 0) {
		memcpy(&hdr,data->hdr,sizeof(MessageHeader));
		puttype = hdr.puttype;
		if ((PutAPS(ld,ix,data,flag)) && 
			((flag = GetAPS_Control(ld,ix,&hdr)) != 0)) {
			fp = ld->aps[ix].fp;
			data->dbstatus[data->apsid] = DB_STATUS_START;
		}
	}
	if (fp == NULL) {
		ClearAPS_Node(ld,ix);
		data->apsid = -1;
		data->retry ++;
		if ((MaxTransactionRetry > 0) && 
			(MaxTransactionRetry < data->retry)) {
			Warning("transaction abort %s", mq->name);
			data->fAbort = TRUE;
			ReleaseProcess(data);
		} else {
			Warning("transaction retry %s", mq->name);
			EnQueue(mq->que,data);
		}
		WaitConnect(ld,ix);
		sched_yield();
	} else {
		memcpy(data->hdr,&hdr,sizeof(MessageHeader));
		PureComponentName(hdr.window,buff);
		newld = g_hash_table_lookup(ComponentHash,buff);
		GetAPS_Value(fp,data,APS_WINCTRL,flag);
		GetAPS_Value(fp,data,APS_MCPDATA,flag);
		GetAPS_Value(fp,data,APS_LINKDATA,flag);
		GetAPS_Value(fp,data,APS_SPADATA,flag);
		GetAPS_Value(fp,data,APS_SCRDATA,flag);
		SendPacketClass(fp,APS_END);
		if		(  puttype  ==  SCREEN_NULL  ) {
			puttype = SCREEN_CURRENT_WINDOW;
		}
		dbgprintf("           puttype = %02X",puttype);
		dbgprintf("data->hdr->puttype = %02X",data->hdr->puttype);
		switch	(data->hdr->puttype) {
		  case	SCREEN_CHANGE_WINDOW:
		  case	SCREEN_JOIN_WINDOW:
		  case	SCREEN_FORK_WINDOW:
			dbgmsg("transition");
			data->hdr->status = TO_CHAR(APL_SESSION_LINK);
			if		(  newld  !=  ld  ) {
					ChangeLD(data,newld);
			}
			CoreEnqueue(data);
			break;
		  case	SCREEN_RETURN_COMPONENT:
			dbgmsg("return");
			data->hdr->status = TO_CHAR(APL_SESSION_GET);
			data->hdr->puttype = SCREEN_NULL;
			if		(  newld  !=  ld  ) {
				ChangeLD(data,newld);
			}
			CoreEnqueue(data);
			break;
		  case	SCREEN_NEW_WINDOW:
			dbgmsg("new");
			if		(  newld  ==  ld  ) {
				ReleaseProcess(data);
			} else {
				data->hdr->status = TO_CHAR(APL_SESSION_LINK);
				ChangeLD(data,newld);
				CoreEnqueue(data);
			}
			break;
		  case	SCREEN_CURRENT_WINDOW:
			dbgmsg("current");
			//	data->hdr->puttype = puttype;		??? change at 2008/12/26
			ReleaseProcess(data);
			break;
		  case	SCREEN_CLOSE_WINDOW:
			dbgmsg("close");
			ReleaseProcess(data);
			break;
		  case	SCREEN_END_SESSION:
			dbgmsg("end");
			ReleaseProcess(data);
			break;
		  case	SCREEN_CHANGE_LD:
			dbgmsg("change LD");
			ReleaseProcess(data);
			break;
		  default:
			/*	don't reach here	*/
			break;
		}
		if		(  newld  ==  NULL  ) {
			MessageLogPrintf("exititting panda [%s@%s] change to [%s]",
				hdr.user,hdr.term,hdr.window);
		}
	}
}

static	void
HandleTransactionMessage(
	SessionData *data,
	int			id)
{
	APS_Node	*aps;
	NETFILE		*fp;

	aps = APS_Table[id];
	if		(  ( fp = aps->fp )  !=  NULL  ) {
		SendPacketClass(fp,APS_REQ);		ON_IO_ERROR(fp,badio);
		SendString(fp,data->name);			ON_IO_ERROR(fp,badio);
		switch	(data->mtype) {
		  case	MESSAGE_TYPE_START:	/*	not send here	*/
			SendPacketClass(fp,APS_START);		ON_IO_ERROR(fp,badio);
			if		(  RecvPacketClass(fp)  ==  APS_OK  ) {
				data->dbstatus[id] = DB_STATUS_START;
			}
			break;
		  case	MESSAGE_TYPE_PREPARE:
			SendPacketClass(fp,APS_PREPARE);	ON_IO_ERROR(fp,badio);
			if		(  RecvPacketClass(fp)  ==  APS_OK  ) {
				data->dbstatus[id] = DB_STATUS_PREPARE;
			}
			break;
		  case	MESSAGE_TYPE_COMMIT:
			SendPacketClass(fp,APS_COMMIT);		ON_IO_ERROR(fp,badio);
			if		(  RecvPacketClass(fp)  ==  APS_OK  ) {
				data->dbstatus[id] = DB_STATUS_CONNECT;
			}
			break;
		  case	MESSAGE_TYPE_ROLLBACK:
			SendPacketClass(fp,APS_ROLLBACK);	ON_IO_ERROR(fp,badio);
			if		(  RecvPacketClass(fp)  ==  APS_OK  ) {
				data->dbstatus[id] = DB_STATUS_CONNECT;
			}
			break;
		  default:
			Error("invalid mtype");
			break;
		}
		ReleaseProcess(data);
	} else {
	  badio:;
	}
}

static	void
MessageThread(
	APS_Start	*aps)
{
	SessionData	*data;
	int			ix
		,		id;
	MQ_Node		*mq;

ENTER_FUNC;
	mq = aps->mq; 
	ix = aps->no;
	id = aps->id;
	dbgprintf("start %s(%d)\n",mq->name,ix);
	do {
		if		(  ( data = SelectData(mq->que,ix,id) )  !=  NULL  )	{
			dbgprintf("act %s\n",mq->name);
			switch(data->type) {
			  case	SESSION_TYPE_TERM:
				HandleTermMessage(data,mq,ix);
				break;
			  case	SESSION_TYPE_TRANSACTION:
				HandleTransactionMessage(data,id);
				break;
#if 0
			  case SESSION_TYPE_API:
				HandleAPIMessage();
#else
				ReleaseProcess(data);
#endif
				break;
			}
		} else {
			sched_yield();
		}
	}	while	(TRUE);
LEAVE_FUNC;
}

static	void
StartMessageThread(
	char	*name,
	MQ_Node	*mq,
	int		*id)
{
	APS_Start	*aps;
	int		i;
	LD_Struct	*ld;

ENTER_FUNC;
	dbgprintf("start thread for %s",name);
	switch	(ThisEnv->mlevel) { 
	  case	MULTI_NO:
	  case	MULTI_ID:
		aps = New(APS_Start);
		aps->mq = mq;
		aps->no = 0;
		aps->id = (*id) ++;
		dbgprintf("id = %d",*id);
		pthread_create(&mq->thr,NULL,(void *(*)(void *))MessageThread,aps);
		break;
	  case	MULTI_LD:
	  case	MULTI_APS:
		if		(  ( ld =  g_hash_table_lookup(ThisEnv->LD_Table,name) )  !=  NULL  ) {
			for	( i = 0 ; i < ld->nports ; i ++ ) {
				aps = New(APS_Start);
				aps->mq = mq;
				aps->no = i;
				dbgprintf("id = %d",*id);
				aps->id = (*id) ++;
				pthread_create(&mq->thr,NULL,(void *(*)(void *))MessageThread,aps);
			}
		}
		break;
	}
LEAVE_FUNC;
}

extern	void
SetupMessageQueue(void)
{
	int		i
		,	j
		,	id;
	MQ_Node	*mq;
	MQ_Node	**mqs;
	GHashTable	*mqh;

ENTER_FUNC;
	MQ_Hash = NewNameHash();
	mqh = NewNameHash();
	mqs = (MQ_Node **)xmalloc(sizeof(MQ_Node *) * ApsId);
	j = 0;
	switch	(ThisEnv->mlevel) {
	  case	MULTI_NO:
		mq = New(MQ_Node);
		mq->name = "";
		mq->que = NewQueue();
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			if		(  g_hash_table_lookup(MQ_Hash, ThisEnv->ld[i]->name)  ==  NULL  ) {
				(void)g_hash_table_insert(MQ_Hash,ThisEnv->ld[i]->name,mq);
			}
			if		(  g_hash_table_lookup(mqh,mq->name)  ==  NULL  ) {
				g_hash_table_insert(mqh,mq->name,mq);
				mqs[j ++] = mq;
			}
		}
		break;
	  case	MULTI_LD:
	  case	MULTI_APS:
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			mq = New(MQ_Node);
			mq->name = ThisEnv->ld[i]->name;
			mq->que = NewQueue();
			if		(  g_hash_table_lookup(MQ_Hash, ThisEnv->ld[i]->name)  ==  NULL  ) {
				(void)g_hash_table_insert(MQ_Hash,ThisEnv->ld[i]->name,mq);
			}
			if		(  g_hash_table_lookup(mqh,mq->name)  ==  NULL  ) {
				g_hash_table_insert(mqh,mq->name,mq);
				mqs[j ++] = mq;
			}
		}
		break;
	  case	MULTI_ID:
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			if		(  ( mq = g_hash_table_lookup(mqh,ThisEnv->ld[i]->group) )
					   ==  NULL  ) {
				mq = New(MQ_Node);
				mq->name = ThisEnv->ld[i]->group;
				mq->que = NewQueue();
				g_hash_table_insert(mqh,mq->name,mq);
				mqs[j ++] = mq;
			}
			if		(  g_hash_table_lookup(MQ_Hash,ThisEnv->ld[i]->name)  ==  NULL  ) {
				g_hash_table_insert(MQ_Hash,ThisEnv->ld[i]->name,mq);
			}
		}
		break;
	  case	MULTI_DB:
		Error("not supported");
		break;
	}
	id = 0;
	for	( i = 0 ; i < j ; i ++ ) {
		StartMessageThread(mqs[i]->name,mqs[i],&id);
	}
	g_hash_table_destroy(mqh);
	xfree(mqs);
LEAVE_FUNC;
}

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
	int		i
		,	j
		,	k
		,	naps;
	LD_Struct	*info;
	LD_Node	*ld;
	char	cname[SIZE_LONGNAME+1];

ENTER_FUNC;
	naps = 0;
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		naps += ThisEnv->ld[i]->nports;
	}
	APS_Table = (APS_Node **)xmalloc(sizeof(APS_Node *) * naps);
	k = 0;
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		info = ThisEnv->ld[i];
		ld = New(LD_Node);
		LDs[i] = ld;
		ld->nports = info->nports;
		if		(  ThisEnv->mlevel  !=  MULTI_APS  ) {
			ld->nports = 1;
		}
		ld->aps = (APS_Node *)xmalloc(sizeof(APS_Node) * ld->nports);
		for	( j = 0 ; j < ld->nports ; j ++ ) {
			ld->aps[j].fp = NULL;
			APS_Table[ApsId] = &ld->aps[j];
			ld->aps[j].id = ApsId ++;
			ClearAPS_Node(ld,j);
		}
		ld->info = info;
		pthread_mutex_init(&ld->lock,NULL);
		pthread_cond_init(&ld->conn,NULL);

		if		(  g_hash_table_lookup(APS_Hash,info->name)  ==  NULL  ) {
			g_hash_table_insert(APS_Hash,info->name,ld);
		}
		dbgprintf("info->cBind = %d",info->cBind);
		dbgprintf("LD [%s]",info->name);
		for	( j = 0 ; j < info->cBind ; j ++ ) {
			PureComponentName(info->binds[j]->name,cname);
			if		(	(  *cname  !=  0  )
					&&	(  g_hash_table_lookup(ComponentHash,cname)  ==  NULL  ) ) {
				dbgprintf("add component [%s]",cname);
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
	if		(  ( fhAps = accept(_fhAps,0,0) )  <  0  )	{
		Error("INET Domain Accept");
	}
	fp = SocketToNet(fhAps);
	RecvStringDelim(fp,SIZE_BUFF,buff);
#ifdef	DEBUG
	printf("connect %s\n",buff);
#endif
	if		(  ( ld = g_hash_table_lookup(APS_Hash,buff) )  !=  NULL  ) {
		if		(  ThisEnv->mlevel  !=  MULTI_APS  ) {
			ld->nports = 1;
		}
		SendPacketClass(fp,APS_OK);
		for	( i = 0 ; i < ld->nports ; i ++ ) {
			if		(  ld->aps[i].fp  ==  NULL  )	break;
		}
		if		(  i  <  ld->nports  ) {
			ClearAPS_Node(ld,i);
			ActivateAPS_Node(&ld->aps[i],fp);
			pthread_cond_signal(&ld->conn);
		} else {
			for	( i = 0 ; i < ld->nports ; i ++ ) {
				SendPacketClass(ld->aps[i].fp,APS_PING);
				ON_IO_ERROR(ld->aps[i].fp,deadaps);
				if		(  RecvPacketClass(ld->aps[i].fp) !=  APS_PONG  ) {
				  deadaps:
					ClearAPS_Node(ld,i);
					ActivateAPS_Node(&ld->aps[i],fp);
					pthread_cond_signal(&ld->conn);
					break;
				}
			}
			if		(  i  ==  ld->nports  ) {
				/*	if you need permitte more APS connection,
					you should expand ld->fp.
				*/
				Error("too many aps tasks");
			}
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
	int		i
		,	cBind;

	ComponentHash = NewNameHash();
	APS_Hash = NewNameHash();
	LDs = (LD_Node **)xmalloc(sizeof(LD_Node *) * ThisEnv->cLD);
	ApsId = 0;
	cBind = 0;
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		cBind += ThisEnv->ld[i]->cBind;
	}
	if		(  cBind  >  0  ) {
		BindTable = (char **)xmalloc(cBind * sizeof(char *));
	} else {
		Error("no binds.");
	}
}
