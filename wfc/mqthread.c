/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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

#include	"types.h"
#include	"libmondai.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"queue.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"mqthread.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"directory.h"
#include	"blob.h"
#include	"driver.h"
#include	"debug.h"

static	void
ClearAPS_Node(
	APS_Node	*aps)
{
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
dbgmsg(">ActivateAPS_Node");
	aps->count = 0;
	aps->fp = fp;
	if		(  fp  !=  NULL  ) {
		SendInt(fp,aps->id);
	}
dbgmsg("<ActivateAPS_Node");
}

static	byte
CheckAPS(
	APS_Node	*aps,
	char		*term)
{
	byte	flag;
	NETFILE	*fp;

	fp = aps->fp;
	if		(  fp  !=  NULL  ) {
		SendPacketClass(fp,APS_REQ);	ON_IO_ERROR(fp,badio);
		SendString(fp,term);			ON_IO_ERROR(fp,badio);
		flag = RecvChar(fp);			ON_IO_ERROR(fp,badio);
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
dbgmsg(">WaitConnect");
	pthread_mutex_lock(&ld->lock);
	if		(  ld->aps[ix].fp  ==  NULL  ) {
		pthread_cond_wait(&ld->conn, &ld->lock);
	}
	pthread_mutex_unlock(&ld->lock);
dbgmsg("<WaitConnect");
}

static	Bool
PutAPS(
	APS_Node	*aps,
	SessionData	*data,
	byte		flag)
{
	MessageHeader	*hdr;
	int		i;
	Bool	fOK;
	NETFILE		*fp;

dbgmsg(">PutAPS");
#ifdef	DEBUG
	printf("term = [%s]\n",data->hdr->term);
	printf("user = [%s]\n",data->hdr->user);
#endif

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
	}
	if		(  ( flag & APS_MCPDATA )  !=  0  ) {
		dbgmsg("MCPDATA");
		SendPacketClass(fp,APS_MCPDATA);	ON_IO_ERROR(fp,badio);
		SendLBS(fp,data->mcpdata);			ON_IO_ERROR(fp,badio);
	}
	if		(  ( flag & APS_SCRDATA )  !=  0  ) {
		dbgmsg("SCRDATA");
		SendPacketClass(fp,APS_SCRDATA);	ON_IO_ERROR(fp,badio);
		for	( i = 0 ; i < data->ld->info->cWindow ; i ++ ) {
			SendLBS(fp,data->scrdata[i]);		ON_IO_ERROR(fp,badio);
		}
	}
	if		(  ( flag & APS_LINKDATA )  !=  0  ) {
		dbgmsg("LINKDATA");
		SendPacketClass(fp,APS_LINKDATA);	ON_IO_ERROR(fp,badio);
		SendLBS(fp,data->linkdata);			ON_IO_ERROR(fp,badio);
	}
	if		(  ( flag & APS_SPADATA )  !=  0  ) {
		dbgmsg("SPADATA");
		SendPacketClass(fp,APS_SPADATA);	ON_IO_ERROR(fp,badio);
		SendLBS(fp,data->spadata);			ON_IO_ERROR(fp,badio);
	}
	SendPacketClass(fp,APS_END);		ON_IO_ERROR(fp,badio);
	aps->count ++;
	fOK = TRUE;
  badio:
dbgmsg("<PutAPS");
	return(fOK);
}

static	byte
GetAPS_Control(
	APS_Node	*aps,
	MessageHeader	*hdr)
{
	PacketClass	c;
	NETFILE		*fp;
	byte		flag;

dbgmsg(">GetAPS_Control");
	fp = aps->fp;
	switch	( c = RecvPacketClass(fp) ) { 
	  case	APS_CTRLDATA:
		flag = RecvChar(fp);				ON_IO_ERROR(fp,badio);
		RecvString(fp,hdr->user);			ON_IO_ERROR(fp,badio);
		RecvString(fp,hdr->window);			ON_IO_ERROR(fp,badio);
		RecvString(fp,hdr->widget);			ON_IO_ERROR(fp,badio);
		hdr->puttype = (char)RecvChar(fp);	ON_IO_ERROR(fp,badio);
		break;
	  case	APS_BLOB:
		break;
	  default:
	  badio:
		MessageLog("aps die ?");
		flag = 0;
		break;
	}
dbgmsg("<GetAPS_Control");
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

dbgmsg(">GetAPS_Value");
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
				RecvString(fpLD,data->w.control[data->w.n].window);
				ON_IO_ERROR(fpLD,badio);
				data->w.n ++;
			}
			break;
		  case	APS_MCPDATA:
			dbgmsg("MCPDATA");
			RecvLBS(fpLD,data->mcpdata);	ON_IO_ERROR(fpLD,badio);
			break;
		  case	APS_LINKDATA:
			dbgmsg("LINKDATA");
			RecvLBS(fpLD,data->linkdata);	ON_IO_ERROR(fpLD,badio);
			break;
		  case	APS_SPADATA:
			dbgmsg("SPADATA");
			RecvLBS(fpLD,data->spadata);	ON_IO_ERROR(fpLD,badio);
			break;
		  case	APS_SCRDATA:
			dbgmsg("SCRDATA");
			for	( i = 0 ; i < data->cWindow ; i ++ ) {
				RecvLBS(fpLD,data->scrdata[i]);	ON_IO_ERROR(fpLD,badio);
			}
			break;
		  default:
		  badio:
			printf("class = [%X]\n",(int)c);
			dbgmsg("protocol error");
			break;
		}
	}
dbgmsg("<GetAPS_Value");
}

extern	void
MessageEnqueue(
	MQ_Node *mq,
	SessionData *data)
{
dbgmsg(">MessageEnqueue");
	EnQueue(mq->que,data);
dbgmsg("<MessageEnqueue");
}

static	void
ChangeLD(
	SessionData	*data)
{
	int		i;

	LBS_Clear(data->spadata);

	InitializeValue(data->ld->info->sparec->value);
	LBS_ReserveSize(data->spadata,NativeSizeValue(NULL,data->ld->info->sparec->value),FALSE);
	NativePackValue(NULL,data->spadata->body,data->ld->info->sparec->value);

	for	( i = 0 ; i < data->cWindow ; i ++ ) {
		FreeLBS(data->scrdata[i]);
	}
	xfree(data->scrdata);
	data->cWindow = data->ld->info->cWindow;
	data->scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
												* data->cWindow);
	for	( i = 0 ; i < data->cWindow ; i ++ ) {
		data->scrdata[i] = NewLBS();
		InitializeValue(data->ld->info->window[i]->rec->value);
		LBS_ReserveSize(data->scrdata[i],
						NativeSizeValue(NULL,data->ld->info->window[i]->rec->value),FALSE);
		NativePackValue(NULL,data->scrdata[i]->body,data->ld->info->window[i]->rec->value);
	}
	data->apsid = -1;
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
	LD_Node		*ld;

ENTER_FUNC;
#ifndef	APS_STICK
	data = DeQueue(que); 
#else
	OpenQueue(que);
	WaitQueue(que);
#ifdef	DEBUG
	{
		RewindQueue(que);
		while	(  ( data = GetElement(que) )  !=  NULL  ) {
			ld = data->ld;
			printf("apsid = %d\nid    = %d\n",data->apsid,ld->aps[ix].id);
		}
	}
#endif
	RewindQueue(que);
	while	(	( data = GetElement(que) )  !=  NULL  ) {
		ld = data->ld;
		if		(  data->apsid  ==  -1  )	break;
		if		(  data->apsid  ==  ld->aps[ix].id  )	break;
		if		(	(  ThisEnv->mlevel  ==  MULTI_NO  )
				||	(  ThisEnv->mlevel  ==  MULTI_ID  ) )	break;
	}
	if		(  data  !=  NULL  ) {
		if		(  data->apsid  ==  -1  ) {
			dbgmsg("virgin data");
		}
		if		(  data->apsid  ==  ld->aps[ix].id  ) {
			dbgmsg("same aps selected");
		}
		data = WithdrawQueue(que);
	}
	if		(  data  ==  NULL  ){
		dbgmsg("null");
		ReleaseQueue(que);
	}
	CloseQueue(que);
#endif
LEAVE_FUNC;
	return	(data);
}
static	void
MessageThread(
	APS_Start	*aps)
{
	SessionData	*data;
	MessageHeader	hdr;
	LD_Node		*ld
	,			*newld;
	NETFILE		*fp;
	char		puttype;
	int			ix;
	MQ_Node		*mq;
	byte		flag;
	char		msg[SIZE_BUFF];

dbgmsg(">MessageThread");
	mq = aps->mq; 
	ix = aps->no;
	dbgprintf("start %s(%d)\n",mq->name,ix);
	do {
		fp = NULL;
		do {
			if		(  ( data = SelectData(mq->que,ix) )  !=  NULL  ) {
				dbgprintf("act %s\n",mq->name);
				ld = data->ld;
				if		(  ( flag = CheckAPS(&ld->aps[ix],data->name) )  !=  0  ) {
					memcpy(&hdr,data->hdr,sizeof(MessageHeader));
					puttype = hdr.puttype;
					if		(	(  PutAPS(&ld->aps[ix],data,flag)  )
							&&	(  ( flag = GetAPS_Control(&ld->aps[ix],&hdr) )
									   !=  0  ) ) {
						fp = ld->aps[ix].fp;
					}
				}
				if		(  fp  ==  NULL  ) {
					ClearAPS_Node(&ld->aps[ix]);
					data->apsid = -1;
					data->retry ++;
					if		(	(  MaxRetry  >  0  )
							&&	(  data->retry  >  MaxRetry  ) ) {
						data->fAbort = TRUE;
						TermEnqueue(data->term,data);
					} else {
						EnQueue(mq->que,data);
					}
					WaitConnect(ld,ix);
					sched_yield();
				}
			} else {
				flag = 0;
				ld = NULL;
				sched_yield();
			}
		}	while	(  fp  ==  NULL  );
		memcpy(data->hdr,&hdr,sizeof(MessageHeader));
		if		(  ( newld = g_hash_table_lookup(WindowHash,hdr.window) )  !=  NULL  ) {
			GetAPS_Value(fp,data,APS_WINCTRL,flag);
			GetAPS_Value(fp,data,APS_MCPDATA,flag);
			GetAPS_Value(fp,data,APS_LINKDATA,flag);
			if		(  newld  ==  ld  ) {
				GetAPS_Value(fp,data,APS_SPADATA,flag);
				GetAPS_Value(fp,data,APS_SCRDATA,flag);
				SendPacketClass(fp,APS_END);
				if		(  puttype  ==  TO_CHAR(SCREEN_NULL)  ) {
					puttype = TO_CHAR(SCREEN_CURRENT_WINDOW);
				}
				switch	(TO_INT(data->hdr->puttype)) {
				  case	SCREEN_CHANGE_WINDOW:
				  case	SCREEN_JOIN_WINDOW:
				  case	SCREEN_FORK_WINDOW:
					data->hdr->status = TO_CHAR(APL_SESSION_LINK);
					CoreEnqueue(data);
					break;
				  case	SCREEN_CURRENT_WINDOW:
					data->hdr->puttype = puttype;
					TermEnqueue(data->term,data);
					break;
				  case	SCREEN_CLOSE_WINDOW:
				  default:
					TermEnqueue(data->term,data);
					break;
				}
			} else {
				SendPacketClass(fp,APS_END);
				data->ld = newld;
				ChangeLD(data);
				data->hdr->rc = TO_CHAR(0);
				data->hdr->status = TO_CHAR(APL_SESSION_LINK);
				CoreEnqueue(data);
			}
		} else {
			sprintf(msg,"window not found [%s] [%s:%s]\n",
					hdr.window,hdr.term,hdr.user);
			MessageLog(msg);
			ClearAPS_Node(&ld->aps[ix]);
		}
	}	while	(TRUE);
dbgmsg("<MessageThread");
}

static	void
StartMessageThread(
	char	*name,
	MQ_Node	*mq,
	void	*dummy)
{
	APS_Start	*aps;
	int		i;
	LD_Struct	*ld;

dbgmsg(">StartMessageThread");
#ifdef	TRACE
	printf("start thread for %s\n",name);
#endif
	switch	(ThisEnv->mlevel) { 
	  case	MULTI_NO:
	  case	MULTI_ID:
		aps = New(APS_Start);
		aps->mq = mq;
		aps->no = 0;
		pthread_create(&mq->thr,NULL,(void *(*)(void *))MessageThread,aps);
		break;
	  case	MULTI_LD:
	  case	MULTI_APS:
		if		(  ( ld =  g_hash_table_lookup(ThisEnv->LD_Table,name) )  !=  NULL  ) {
			for	( i = 0 ; i < ld->nports ; i ++ ) {
				aps = New(APS_Start);
				aps->mq = mq;
				aps->no = i;
				pthread_create(&mq->thr,NULL,(void *(*)(void *))MessageThread,aps);
			}
		}
		break;
	}
dbgmsg("<StartMessageThread");
}

static	GHashTable	*mqs;
extern	void
SetupMessageQueue(void)
{
	int		i;
	MQ_Node	*mq;

dbgmsg(">SetupMessageQueue");
	MQ_Hash = NewNameHash();
	mqs = NewNameHash();
	switch	(ThisEnv->mlevel) {
	  case	MULTI_NO:
		mq = New(MQ_Node);
		mq->name = "";
		mq->que = NewQueue();
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			if		(  g_hash_table_lookup(MQ_Hash, ThisEnv->ld[i]->name)  ==  NULL  ) {
				(void)g_hash_table_insert(MQ_Hash,ThisEnv->ld[i]->name,mq);
			}
			if		(  g_hash_table_lookup(mqs,mq->name)  ==  NULL  ) {
				g_hash_table_insert(mqs,mq->name,mq);
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
			if		(  g_hash_table_lookup(mqs,mq->name)  ==  NULL  ) {
				g_hash_table_insert(mqs,mq->name,mq);
			}
		}
		break;
	  case	MULTI_ID:
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			if		(  ( mq = g_hash_table_lookup(mqs,ThisEnv->ld[i]->group) )
					   ==  NULL  ) {
				mq = New(MQ_Node);
				mq->name = ThisEnv->ld[i]->group;
				mq->que = NewQueue();
				g_hash_table_insert(mqs,mq->name,mq);
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
	g_hash_table_foreach(mqs,(GHFunc)StartMessageThread,NULL);
dbgmsg("<SetupMessageQueue");
}

static	LD_Node	**LDs;
static	int		ApsId;

extern	void
ReadyAPS(void)
{
	int		i
	,		j;
	int		fh;
	LD_Struct	*info;
	NETFILE	*fp;
	LD_Node	*ld;

dbgmsg(">ReadyAPS");
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		info = ThisEnv->ld[i];
		ld = New(LD_Node);
		LDs[i] = ld;
		ld->nports = info->nports;
		ld->aps = (APS_Node *)xmalloc(sizeof(APS_Node) * ld->nports);
		if		(  ThisEnv->mlevel  !=  MULTI_APS  ) {
			ld->nports = 1;
		}
		for	( j = 0 ; j < ld->nports ; j ++ ) {
			ld->aps[j].fp = NULL;
			ld->aps[j].id = ApsId ++;
			ClearAPS_Node(&ld->aps[j]);
			if		(  info->ports[j]  !=  NULL  ) {
				fh = ConnectSocket(info->ports[j],SOCK_STREAM);
				if		(  fh  >  0  ) {
					fp = SocketToNet(fh);
				} else {
					fp = NULL;
				}
			} else {
				fp = NULL;
			}
			ActivateAPS_Node(&ld->aps[j],fp);
		}

		ld->info = info;
		pthread_mutex_init(&ld->lock,NULL);
		pthread_cond_init(&ld->conn,NULL);

		if		(  g_hash_table_lookup(APS_Hash,info->name)  ==  NULL  ) {
			g_hash_table_insert(APS_Hash,info->name,ld);
		}
		for	( j = 0 ; j < info->cWindow ; j ++ ) {
			if		(  g_hash_table_lookup(WindowHash,info->window[j]->name)  ==  NULL  ) {
				g_hash_table_insert(WindowHash,info->window[j]->name,ld);
			}
		}
	}
dbgmsg("<ReadyAPS");
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

dbgmsg(">ConnectAps");
	if		(  ( fhAps = accept(_fhAps,0,0) )  <  0  )	{
		Error("INET Domain Accept");
	}
	fp = SocketToNet(fhAps);
	RecvString(fp,buff);
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
			ClearAPS_Node(&ld->aps[i]);
			ActivateAPS_Node(&ld->aps[i],fp);
			pthread_cond_signal(&ld->conn);
		} else {
			for	( i = 0 ; i < ld->nports ; i ++ ) {
				SendPacketClass(ld->aps[i].fp,APS_PING);
				ON_IO_ERROR(ld->aps[i].fp,deadaps);
				if		(  RecvPacketClass(ld->aps[i].fp) !=  APS_PONG  ) {
				  deadaps:
					ClearAPS_Node(&ld->aps[i]);
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
		printf("invalid aps = [%s]\n",buff);
	}
dbgmsg("<ConnectAps");
}

extern	void
InitMessageQueue(void)
{
	WindowHash = NewNameHash();
	APS_Hash = NewNameHash();
	LDs = (LD_Node **)xmalloc(sizeof(LD_Node *) * ThisEnv->cLD);
	ApsId = 0;
}
