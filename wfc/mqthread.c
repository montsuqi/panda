/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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
#include	"enum.h"
#include	"misc.h"

#include	"value.h"
#include	"comm.h"
#include	"tcp.h"
#include	"queue.h"
#include	"wfc.h"
#include	"wfcio.h"
#include	"mqthread.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"directory.h"
#include	"driver.h"
#include	"debug.h"

static	APS_Node	**APSs;

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
#if	1
	FreeLBS(data->spadata);
	data->spadata = NewLBS();
#else
	LBS_Clear(data->spadata);
#endif
	InitializeValue(data->aps->ld->sparec);
	LBS_RequireSize(data->spadata,SizeValue(data->aps->ld->sparec,0,0),FALSE);
	PackValue(data->spadata->body,data->aps->ld->sparec);

	for	( i = 0 ; i < data->cWindow ; i ++ ) {
		FreeLBS(data->scrdata[i]);
	}
	xfree(data->scrdata);
	data->cWindow = data->aps->ld->cWindow;
	data->scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
												* data->cWindow);
	for	( i = 0 ; i < data->cWindow ; i ++ ) {
		data->scrdata[i] = NewLBS();
		InitializeValue(data->aps->ld->window[i]->value);
		LBS_RequireSize(data->scrdata[i],
						SizeValue(data->aps->ld->window[i]->value,0,0),FALSE);
		PackValue(data->scrdata[i]->body,data->aps->ld->window[i]->value);
	}
}

static	void
MessageThread(
	MQ_Node	*mq)
{
	SessionData	*data;
	MessageHeader	hdr;
	APS_Node	*aps
	,			*newaps;
	FILE		*fpAPS;
	char		puttype;
	int			ix;

dbgmsg(">MessageThread");
	do {
		do {
			data = DeQueue(mq->que);
#ifdef	DEBUG
			printf("act %s\n",mq->name);
#endif
			aps = data->aps;
			memcpy(&hdr,data->hdr,sizeof(MessageHeader));
			ix = PutAPS(aps,data);
			puttype = hdr.puttype;
			if		(  GetAPS_Control(aps->fp[ix],&hdr)  ) {
				fpAPS = aps->fp[ix];
			} else {
				shutdown(fileno(aps->fp[ix]), 2);
				fclose(aps->fp[ix]);
				aps->fp[ix] = NULL;
				EnQueue(mq->que,data);
				data = NULL;
				fpAPS = NULL;
				sleep(1);
			}
		}	while	(  data  ==  NULL  );
#ifdef	DEBUG
		printf("Window = [%s][%c]\n",ctrl.window,puttype);
#endif
		memcpy(data->hdr,&hdr,sizeof(MessageHeader));
		if		(  ( newaps = g_hash_table_lookup(WindowHash,hdr.window) )
				   !=  NULL  ) {
			GetAPS_Value(fpAPS,data,APS_CLSWIN);
			GetAPS_Value(fpAPS,data,APS_MCPDATA);
			GetAPS_Value(fpAPS,data,APS_LINKDATA);
			if		(  newaps  ==  aps  ) {
				GetAPS_Value(fpAPS,data,APS_SPADATA);
				GetAPS_Value(fpAPS,data,APS_SCRDATA);
				GetAPS_Value(fpAPS,data,APS_END);
				switch	(TO_INT(data->hdr->puttype)) {
				  case	SCREEN_CHANGE_WINDOW:
				  case	SCREEN_JOIN_WINDOW:
					data->hdr->status = TO_CHAR(APL_SESSION_LINK);
					CoreEnqueue(data);
					break;
				  case	SCREEN_NULL:
				  case	SCREEN_CURRENT_WINDOW:
					if		(  puttype  ==  TO_CHAR(SCREEN_NULL)  ) {
						puttype = TO_CHAR(SCREEN_CURRENT_WINDOW);
					}
					data->hdr->puttype = puttype;
					TermEnqueue(data->term,data);
					break;
				  default:
					TermEnqueue(data->term,data);
					break;
				}
			} else {
				GetAPS_Value(fpAPS,data,APS_END);
				data->aps = newaps;
				ChangeLD(data);
				data->hdr->rc = TO_CHAR(0);
				data->hdr->status = TO_CHAR(APL_SESSION_LINK);
				CoreEnqueue(data);
			}
		} else {
			printf("[%s]\n",hdr.window);
			Error("window not found");
		}
	}	while	(TRUE);
dbgmsg("<MessageThread");
}

extern	void
ReadyAPS(void)
{
	int		i
	,		j;
	int		fh;
	LD_Struct	*ld;
	FILE	*fp;
	APS_Node	*aps;

dbgmsg(">ReadyAPS");
	WindowHash = NewNameHash();
	APS_Hash = NewNameHash();

	APSs = (APS_Node **)xmalloc(sizeof(APS_Node *) * ThisEnv->cLD);
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		ld = ThisEnv->ld[i];
		aps = New(APS_Node);
		APSs[i] = aps;
		aps->nports = ld->nports;
		aps->fp = (FILE **)xmalloc(sizeof(FILE *) * aps->nports);
		for	( j = 0 ; j < aps->nports ; j ++ ) {
			fh = ConnectSocket(ld->ports[j]->port,
							   SOCK_STREAM,
							   ld->ports[j]->host);
			if		(  fh  >  0  ) {
				if		(  (  fp = fdopen(fh,"w+") )  ==  NULL  ) {
					close(fh);
					exit(1);
				}
			} else {
				fp = NULL;
			}
			aps->fp[j] = fp;
		}

		aps->ld = ld;
		pthread_mutex_init(&aps->lock,NULL);
		pthread_cond_init(&aps->conn,NULL);
		if		(  g_hash_table_lookup(APS_Hash,ld->name)  ==  NULL  ) {
			g_hash_table_insert(APS_Hash,ld->name,aps);
		}
		for	( j = 0 ; j < ld->cWindow ; j ++ ) {
			if		(  g_hash_table_lookup(WindowHash,ld->window[j]->name)  ==  NULL  ) {
				g_hash_table_insert(WindowHash,ld->window[j]->name,aps);
			}
		}
	}
dbgmsg("<ReadyAPS");
}

static	void
StartMessageThread(
	char	*name,
	MQ_Node	*mq,
	void	*dummy)
{
#ifdef	TRACE
	printf("start thread for %s\n",name);
#endif
	pthread_create(&mq->thr,NULL,(void *(*)(void *))MessageThread,mq);
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

extern	void
ConnectAPS(
	int		_fhAps)
{
	int		fhAps;
	char	buff[SIZE_BUFF];
	FILE	*fp;
	int		i;
	APS_Node	*aps;

dbgmsg(">ConnectAps");
	if		(  ( fhAps = accept(_fhAps,0,0) )  <  0  )	{
		Error("INET Domain Accept");
	}
	fp = fdopen(fhAps,"w+");
	RecvString(fp,buff);
#ifdef	DEBUG
	printf("connect %s\n",buff);
#endif
	if		(  ( aps = g_hash_table_lookup(APS_Hash,buff) )  !=  NULL  ) {
		SendPacketClass(fp,APS_OK);
		for	( i = 0 ; i < aps->nports ; i ++ ) {
			if		(  aps->fp[i]  ==  NULL  )	break;
		}
		if		(  i  <  aps->nports  ) {
			aps->fp[i] = fp;
			pthread_cond_signal(&aps->conn);
		} else {
			for	( i = 0 ; i < aps->nports ; i ++ ) {
				SendPacketClass(aps->fp[i],APS_PING);
				if		(  RecvPacketClass(aps->fp[i]) !=  APS_PONG  ) {
					shutdown(fileno(aps->fp[i]), 2);
					fclose(aps->fp[i]);
					aps->fp[i] = fp;
					break;
				}
			}
			if		(  i  ==  aps->nports  ) {
				/*	if you need permitte more APS connection,
					you should expand aps->fp.
				*/
				Error("too many aps tasks");
			}
		}
	} else {
		SendPacketClass(fp,APS_NOT);
		printf("invalid aps = [%s]\n",buff);
	}
	fflush(fp);
dbgmsg("<ConnectAps");
}

