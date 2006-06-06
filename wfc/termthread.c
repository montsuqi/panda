/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"
#include	"enum.h"

#include	"libmondai.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"queue.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"wfc.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"blob.h"
#include	"blobcom.h"
#include	"blobserv.h"
#include	"driver.h"
#include	"message.h"
#include	"debug.h"

static	GHashTable	*TermHash;
static	pthread_mutex_t	TermLock;
static	pthread_cond_t	TermCond;

extern	void
TermEnqueue(
	TermNode	*term,
	SessionData	*data)
{
	EnQueue(term->que,data);
}

static	SessionData	*
MakeSessionData(void)
{
	SessionData	*data;

ENTER_FUNC;
	data = New(SessionData);
	memclear(data,sizeof(SessionData));
	data->hdr = New(MessageHeader);
	data->name = NULL;
	memclear(data->hdr,sizeof(MessageHeader));
	data->apsid = -1;
LEAVE_FUNC;
	return	(data);
}

static	void
FinishSession(
	SessionData	*data)
{
	char	msg[SIZE_LONGNAME+1];
	int		i;
	guint	FreeSpa(
		char	*name,
		LargeByteString	*spa,
		void		*dummy)
	{
		if		(  name  !=  NULL  ) {
			//xfree(name);
		}
		if		(  spa  !=  NULL  ) {
			FreeLBS(spa);
		}
		return	(TRUE);
	}
	guint	FreeScr(
		char	*name,
		LargeByteString	*scr,
		void		*dummy)
	{
		if		(  scr  !=  NULL  ) {
			FreeLBS(scr);
		}
		return	(TRUE);
	}

ENTER_FUNC;
	snprintf(msg,SIZE_LONGNAME,"[%s@%s] session end",data->hdr->user,data->hdr->term);
	MessageLog(msg);
	if		(  data->name  !=  NULL  ) {
		pthread_mutex_lock(&TermLock);
		g_hash_table_remove(TermHash,data->name);
		pthread_mutex_unlock(&TermLock);
		pthread_cond_signal(&TermCond);
		xfree(data->name);
	}
	xfree(data->hdr);
	g_hash_table_foreach_remove(data->spadata,(GHRFunc)FreeSpa,NULL);
	g_hash_table_destroy(data->spadata);
	if		(  data->mcpdata  !=  NULL  ) {
		FreeLBS(data->mcpdata);
	}
	if		(  data->linkdata  !=  NULL  ) {
		FreeLBS(data->linkdata);
	}
	for	( i = 0 ; i < data->cWindow ; i ++ ) {
		if		(  data->scrdata[i]  !=  NULL  ) {
			FreeLBS(data->scrdata[i]);
		}
	}
	g_hash_table_destroy(data->scrpool);
	xfree(data->scrdata);
	xfree(data);
LEAVE_FUNC;
}

static	SessionData	*
InitSession(
	NETFILE	*fp)
{
	SessionData	*data;
	char	buff[SIZE_LONGNAME+1];
	char	msg[SIZE_LONGNAME+1];
	LD_Node	*ld;
	int			i;

ENTER_FUNC;
	data = MakeSessionData();
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		data->fKeep = TRUE;
	} else {
		data->fKeep = FALSE;
	}
	ON_IO_ERROR(fp,badio);
	RecvStringDelim(fp,SIZE_NAME,data->hdr->term);		ON_IO_ERROR(fp,badio);
	RecvStringDelim(fp,SIZE_NAME,data->hdr->user);		ON_IO_ERROR(fp,badio);
	snprintf(msg,SIZE_LONGNAME,"[%s@%s] session start",data->hdr->user,data->hdr->term);
	MessageLog(msg);
	dbgprintf("term = [%s]",data->hdr->term);
	dbgprintf("user = [%s]",data->hdr->user);
	RecvStringDelim(fp,SIZE_NAME,buff);	/*	LD name	*/	ON_IO_ERROR(fp,badio);
	if		(  ( ld = g_hash_table_lookup(APS_Hash,buff) )  !=  NULL  ) {
		pthread_mutex_lock(&TermLock);
		data->ld = ld;
		if		(  ThisEnv->mcprec  !=  NULL  ) {
			data->mcpdata = NewLBS();
			//InitializeValue(ThisEnv->mcprec->value);
			LBS_ReserveSize(data->mcpdata,NativeSizeValue(NULL,ThisEnv->mcprec->value),FALSE);
			NativePackValue(NULL,LBS_Body(data->mcpdata),ThisEnv->mcprec->value);
		}
		data->spadata = NewNameHash();
		data->scrpool = NewNameHash();
		if		(  ThisEnv->linkrec  !=  NULL  ) {
			data->linkdata = NewLBS();
			//InitializeValue(ThisEnv->linkrec->value);
			LBS_ReserveSize(data->linkdata,NativeSizeValue(NULL,ThisEnv->linkrec->value),FALSE);
			NativePackValue(NULL,LBS_Body(data->linkdata),ThisEnv->linkrec->value);
		} else {
			data->linkdata = NULL;
		}

		data->cWindow = ld->info->cWindow;
		data->scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
													* data->cWindow);
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			if		(  data->ld->info->windows[i]  !=  NULL  ) {
				dbgprintf("[%s]",data->ld->info->windows[i]->name);
				data->scrdata[i] = GetScreenData(data,data->ld->info->windows[i]->name);
			} else {
				data->scrdata[i] = NULL;
			}
		}
		data->name = StrDup(data->hdr->term);
		g_hash_table_insert(TermHash,data->name,data);
		data->hdr->status = TO_CHAR(APL_SESSION_LINK);
		data->hdr->puttype = SCREEN_NULL;
		data->w.n = 0;
		pthread_mutex_unlock(&TermLock);
		pthread_cond_signal(&TermCond);
	} else {
		Warning("[%s] session fail LD [%s] not found.",data->hdr->term,buff);
	  badio:
		SendPacketClass(fp,WFC_NOT);
		FinishSession(data);
		data = NULL;
	}
LEAVE_FUNC;
	return	(data);
}

static	LD_Node	*
ReadTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	LD_Node	*ld;
	Bool		fExit;
	int			c;
	LargeByteString	*scrdata;
	char		window[SIZE_LONGNAME+1]
		,		comp[SIZE_LONGNAME+1];

ENTER_FUNC;
	ld = NULL;
	fExit = FALSE;
	do {
		switch	(c = RecvPacketClass(fp)) {
		  case	WFC_DATA:
			dbgmsg("recv DATA");
			RecvnString(fp,SIZE_NAME,data->hdr->window);	ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,data->hdr->widget);	ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,data->hdr->event);		ON_IO_ERROR(fp,badio);
			dbgprintf("window = [%s]",data->hdr->window);
			dbgprintf("widget = [%s]",data->hdr->widget);
			dbgprintf("event  = [%s]",data->hdr->event);
			PureComponentName(data->hdr->window,comp);
			if		(  ( ld = g_hash_table_lookup(ComponentHash,comp) )
					   !=  NULL  ) {
				dbgprintf("ld = [%s]",ld->info->name);
				PureWindowName(data->hdr->window,window);
				dbgprintf("window = [%s]",window);
				if		(  ( scrdata = GetScreenData(data,window) )  !=  NULL  ) {
					SendPacketClass(fp,WFC_OK);				ON_IO_ERROR(fp,badio);
					dbgmsg("send OK");
					RecvLBS(fp,scrdata);					ON_IO_ERROR(fp,badio);
					data->hdr->rc = TO_CHAR(0);
					data->hdr->status = TO_CHAR(APL_SESSION_GET);
					data->hdr->puttype = SCREEN_NULL;
				} else {
					Error("invalid window [%s]",window);
				}
				if		(  data->ld  !=  ld  ) {
					ChangeLD(data,ld);
				}
			} else {
				Error("component [%s] not found.",data->hdr->window);
			}
			break;
		  case	WFC_BLOB:
			dbgmsg("recv BLOB");
			PassiveBLOB(fp,BlobState);			ON_IO_ERROR(fp,badio);
			break;
		  case	WFC_PING:
			dbgmsg("recv PING");
			SendPacketClass(fp,WFC_PONG);		ON_IO_ERROR(fp,badio);
			dbgmsg("send PONG");
			break;
		  case	WFC_OK:
			dbgmsg("OK");
			fExit = TRUE;
			break;
		  default:
			dbgmsg("default");
			dbgprintf("c = [%X]\n",c);
			ON_IO_ERROR(fp,badio);
			fExit = TRUE;
			dbgmsg("recv default");
			break;
		}
	}	while	(  !fExit  );
  badio:
	if		(  ld  ==  NULL  ) {
		SendPacketClass(fp,WFC_NOT);
	}
LEAVE_FUNC;
	return(ld);
}

static	Bool
SendTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	MessageHeader	*hdr;
	int			i
		,		c;
	Bool		rc;
	Bool		fExit;
	char		wname[SIZE_LONGNAME+1]
		,		buff[SIZE_LONGNAME+1];
	LargeByteString	*scrdata;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_PING);		ON_IO_ERROR(fp,badio);
	dbgmsg("send PING");
	if		(  RecvPacketClass(fp)  ==  WFC_PONG  ) {
		dbgmsg("recv PONG");
		ON_IO_ERROR(fp,badio);
		SendPacketClass(fp,WFC_HEADER);		ON_IO_ERROR(fp,badio);
		dbgmsg("send DATA");
		hdr = data->hdr;
		SendString(fp,hdr->user);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->window);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->widget);			ON_IO_ERROR(fp,badio);
		SendChar(fp,hdr->puttype);			ON_IO_ERROR(fp,badio);
		SendInt(fp,data->w.n);				ON_IO_ERROR(fp,badio);
		dbgprintf("data->w.n = %d\n",data->w.n);
		for	( i = 0 ; i < data->w.n ; i ++ ) {
			SendInt(fp,data->w.control[i].PutType);			ON_IO_ERROR(fp,badio);
			SendString(fp,data->w.control[i].window);		ON_IO_ERROR(fp,badio);
		}
		data->w.n = 0;
		fExit = FALSE;
		do {
			switch	(c = RecvPacketClass(fp))	{
			  case	WFC_PING:
				dbgmsg("PING");
				SendPacketClass(fp,WFC_PONG);		ON_IO_ERROR(fp,badio);
				break;
			  case	WFC_DATA:
				dbgmsg(">DATA");
				RecvnString(fp,SIZE_NAME,buff);				ON_IO_ERROR(fp,badio);
				PureWindowName(buff,wname);
				if		(  ( scrdata = GetScreenData(data,wname) )  !=  NULL  ) {
					dbgmsg("send OK");
					SendPacketClass(fp,WFC_OK);			ON_IO_ERROR(fp,badio);
					SendLBS(fp,scrdata);				ON_IO_ERROR(fp,badio);
				} else {
					dbgmsg("send NODATA");
					SendPacketClass(fp,WFC_NODATA);			ON_IO_ERROR(fp,badio);
				}
				dbgmsg("<DATA");
				break;
			  case	WFC_BLOB:
				dbgmsg("send BLOB");
				PassiveBLOB(fp,BlobState);			ON_IO_ERROR(fp,badio);
				break;
			  case	WFC_DONE:
				dbgmsg("DONE");
				fExit = TRUE;
				break;
			  default:
				dbgmsg("default");
				dbgprintf("c = [%X]\n",c);
				fExit = TRUE;
				break;
			}
		}	while	(  !fExit  );
		rc = TRUE;
	} else {
	  badio:
		dbgmsg("recv FALSE");
	}
LEAVE_FUNC;
	return	(rc); 
}

static	void
TermThread(
	void	*para)
{
	int		fhTerm = (int)para;
	LD_Node	*ld;
	TermNode	*term;
	SessionData	*data;
	struct	timeval	tv;
	long	ever
		,	now;

ENTER_FUNC;
	term = New(TermNode);
	term->que = NewQueue();
	term->fp = SocketToNet(fhTerm);
	
	if		(  ( data = InitSession(term->fp) )  !=  NULL  ) {
		do {
			data->term = term;
			data->retry = 0;
			data->fAbort = FALSE;
			gettimeofday(&tv,NULL);
			ever = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
			if		(  !fLoopBack  ) {
				CoreEnqueue(data);
				dbgmsg("process !!");
			} else {
				EnQueue(term->que,data);
			}
			data = DeQueue(term->que);
			if		(  fTimer  ) {
				gettimeofday(&tv,NULL);
				now = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
				printf("wfc %s@%s:%s process time %6ld(ms)\n",
					   data->hdr->user,data->hdr->term,data->hdr->window,(now - ever));
			}
			if		(  data->fAbort  )	break;
			if		(  SendTerminal(term->fp,data)  ) {
				if		(  data->ld  !=  NULL  ) {
					ld = ReadTerminal(term->fp,data);
				} else {
					ld = NULL;
				}
			} else {
				ld = NULL;
			}
		}	while	(  ld  !=  NULL  );
		FinishSession(data);
	}
	CloseNet(term->fp);
	FreeQueue(term->que);
	xfree(term);
LEAVE_FUNC;
	pthread_exit(NULL);
}

extern	pthread_t
ConnectTerm(
	int		_fhTerm)
{
	int		fhTerm;
	pthread_t	thr;
	pthread_attr_t	attr;

ENTER_FUNC;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,1024*1024);
	if		(  ( fhTerm = accept(_fhTerm,0,0) )  <  0  )	{
		Message("_fhTerm = %d INET Domain Accept",_fhTerm);
		exit(1);
	}
	pthread_create(&thr,&attr,(void *(*)(void *))TermThread,(void *)fhTerm);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(thr); 
}

extern	void
InitTerm(void)
{
ENTER_FUNC;
	pthread_cond_init(&TermCond,NULL);
	pthread_mutex_init(&TermLock,NULL);
	TermHash = NewNameHash();
	if		(  ThisEnv->mcprec  !=  NULL  ) {
		InitializeValue(ThisEnv->mcprec->value);
	}
	if		(  ThisEnv->linkrec  !=  NULL  ) {
		InitializeValue(ThisEnv->linkrec->value);
	}
LEAVE_FUNC;
}
