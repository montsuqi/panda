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
*/
#define	DEBUG
#define	TRACE

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
	char	name[SIZE_NAME+1];
	char	msg[SIZE_LONGNAME+1];
	int		i;
	void	FreeSpa(
		char	*name,
		LargeByteString	*spa,
		void		*dummy)
	{
		if		(  name  !=  NULL  ) {
			xfree(name);
		}
		if		(  spa  !=  NULL  ) {
			FreeLBS(spa);
		}
	}

ENTER_FUNC;
	snprintf(msg,SIZE_LONGNAME,"[%s:%s] session end",data->hdr->term,data->hdr->user);
	MessageLog(msg);
	xfree(data->hdr);
	if		(  data->name  !=  NULL  ) {
		strcpy(name,data->name);
		g_hash_table_remove(TermHash,data->name);
		xfree(data->name);
		if		(  data->mcpdata  !=  NULL  ) {
			FreeLBS(data->mcpdata);
		}
		g_hash_table_foreach(data->spadata,(GHFunc)FreeSpa,NULL);
		g_hash_table_destroy(data->spadata);
		if		(  data->linkdata  !=  NULL  ) {
			FreeLBS(data->linkdata);
		}
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			if		(  data->scrdata[i]  !=  NULL  ) {
				FreeLBS(data->scrdata[i]);
			}
		}
	}
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
	snprintf(msg,SIZE_LONGNAME,"[%s:%s] session start",data->hdr->term,data->hdr->user);
	MessageLog(msg);
	dbgprintf("term = [%s]",data->hdr->term);
	dbgprintf("user = [%s]",data->hdr->user);
	RecvStringDelim(fp,SIZE_NAME,buff);	/*	LD name	*/	ON_IO_ERROR(fp,badio);
	if		(  ( ld = g_hash_table_lookup(APS_Hash,buff) )
			   !=  NULL  ) {
		data->ld = ld;
		if		(  ThisEnv->mcprec  !=  NULL  ) {
			data->mcpdata = NewLBS();
			InitializeValue(ThisEnv->mcprec->value);
			LBS_ReserveSize(data->mcpdata,NativeSizeValue(NULL,ThisEnv->mcprec->value),FALSE);
			NativePackValue(NULL,data->mcpdata->body,ThisEnv->mcprec->value);
		}
		data->spadata = NewNameHash();
		if		(  ThisEnv->linkrec  !=  NULL  ) {
			data->linkdata = NewLBS();
			InitializeValue(ThisEnv->linkrec->value);
			LBS_ReserveSize(data->linkdata,NativeSizeValue(NULL,ThisEnv->linkrec->value),FALSE);
			NativePackValue(NULL,data->linkdata->body,ThisEnv->linkrec->value);
		} else {
			data->linkdata = NULL;
		}

		data->cWindow = ld->info->cWindow;
		data->scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
													* data->cWindow);
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			if		(  data->ld->info->windows[i]  !=  NULL  ) {
				data->scrdata[i] = NewLBS();
				InitializeValue(data->ld->info->windows[i]->value);
				LBS_ReserveSize(data->scrdata[i],
								NativeSizeValue(NULL,ld->info->windows[i]->value),FALSE);
				NativePackValue(NULL,data->scrdata[i]->body,ld->info->windows[i]->value);
			} else {
				data->scrdata[i] = NULL;
			}
		}
		data->name = StrDup(data->hdr->term);
		g_hash_table_insert(TermHash,data->name,data);
		data->hdr->status = TO_CHAR(APL_SESSION_LINK);
		data->hdr->puttype = TO_CHAR(SCREEN_NULL);
		data->w.n = 0;
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
	int			ix;
	Bool		fExit;
	int			c;
	char		window[SIZE_LONGNAME+1]
		,		*p;

ENTER_FUNC;
	ld = NULL;
	fExit = FALSE;
	do {
dbgmsg("*");
		switch	(c = RecvPacketClass(fp)) {
		  case	WFC_DATA:
			dbgmsg("recv DATA");
			RecvnString(fp,SIZE_NAME,data->hdr->window);	ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,data->hdr->widget);	ON_IO_ERROR(fp,badio);
			RecvnString(fp,SIZE_NAME,data->hdr->event);		ON_IO_ERROR(fp,badio);
			dbgprintf("window = [%s]",data->hdr->window);
			dbgprintf("widget = [%s]",data->hdr->widget);
			dbgprintf("event  = [%s]",data->hdr->event);
			if		(  ( ld = g_hash_table_lookup(ComponentHash,data->hdr->window) )
					   !=  NULL  ) {
				strcpy(window,data->hdr->window);
				if		(  ( p = strchr(window,'.') )  !=  NULL  ) {
					*p = 0;
				}
				dbgprintf("compo = [%s]",window);
				ix = (int)g_hash_table_lookup(ld->info->whash,window);
				if		(  ix  >  0  ) {
					SendPacketClass(fp,WFC_OK);				ON_IO_ERROR(fp,badio);
					dbgmsg("send OK");
					if		(  data->scrdata[ix-1]  !=  NULL  ) {
						RecvLBS(fp,data->scrdata[ix-1]);	ON_IO_ERROR(fp,badio);
					}
					data->hdr->rc = TO_CHAR(0);
					data->hdr->status = TO_CHAR(APL_SESSION_GET);
					data->hdr->puttype = TO_CHAR(SCREEN_NULL);
				}
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
		,		c
		,		ix;
	Bool		rc;
	Bool		fExit;
	char		wname[SIZE_LONGNAME+1];
	char		*p;

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
			dbgmsg("*");
			switch	(c = RecvPacketClass(fp))	{
			  case	WFC_PING:
				dbgmsg("PING");
				SendPacketClass(fp,WFC_PONG);		ON_IO_ERROR(fp,badio);
				break;
			  case	WFC_DATA:
				dbgmsg(">DATA");
				RecvnString(fp,SIZE_NAME,wname);				ON_IO_ERROR(fp,badio);
				dbgprintf("wname = [%s]\n",wname);
				if		(  ( p = strchr(wname,'.') )  !=  NULL  ) {
					*p = 0;
				}
				ix = (int)g_hash_table_lookup(data->ld->info->whash,wname);
				dbgprintf("ix = %d",ix);
				if		(  ix  >  0  ) {
					if		(  data->scrdata[ix-1]  !=  NULL  ) {
						dbgmsg("send OK");
						SendPacketClass(fp,WFC_OK);				ON_IO_ERROR(fp,badio);
						SendLBS(fp,data->scrdata[ix-1]);		ON_IO_ERROR(fp,badio);
					} else {
						dbgmsg("send NODATA");
						SendPacketClass(fp,WFC_NODATA);			ON_IO_ERROR(fp,badio);
					}
				} else {
					dbgmsg("send NOT");
					SendPacketClass(fp,WFC_NOT);				ON_IO_ERROR(fp,badio);
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
			dbgmsg("*");
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

ENTER_FUNC;
	term = New(TermNode);
	term->que = NewQueue();
	term->fp = SocketToNet(fhTerm);
	
	if		(  ( data = InitSession(term->fp) )  !=  NULL  ) {
		do {
			data->term = term;
			data->retry = 0;
			data->fAbort = FALSE;
			if		(  !fLoopBack  ) {
				CoreEnqueue(data);
				dbgmsg("process !!");
			} else {
				EnQueue(term->que,data);
			}
			data = DeQueue(term->que);
			if		(  data->fAbort  )	break;
			if		(  SendTerminal(term->fp,data)  ) {
				ld = ReadTerminal(term->fp,data);
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

ENTER_FUNC;
	if		(  ( fhTerm = accept(_fhTerm,0,0) )  <  0  )	{
		Message("_fhTerm = %d INET Domain Accept",_fhTerm);
		exit(1);
	}
	pthread_create(&thr,NULL,(void *(*)(void *))TermThread,(void *)fhTerm);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(thr); 
}

extern	void
InitTerm(void)
{
ENTER_FUNC;
	TermHash = NewNameHash();
LEAVE_FUNC;
}
