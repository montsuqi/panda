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
#include	"queue.h"
#include	"directory.h"
#include	"wfc.h"
#include	"wfcio.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"blob.h"
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

dbgmsg(">MakeSessionData");
	data = New(SessionData);
	data->hdr = New(MessageHeader);
	data->name = NULL;
	memclear(data->hdr,sizeof(MessageHeader));
	data->apsid = -1;
dbgmsg("<MakeSessionData");
	return	(data);
}

static	void
FinishSession(
	SessionData	*data)
{
	char	name[SIZE_NAME+1];
	char	msg[SIZE_BUFF];
	int		i;

dbgmsg(">FinishSession");
	sprintf(msg,"[%s:%s] session end",data->hdr->term,data->hdr->user);
	MessageLog(msg);
	xfree(data->hdr);
	if		(  data->name  !=  NULL  ) {
		strcpy(name,data->name);
		g_hash_table_remove(TermHash,data->name);
		xfree(data->name);
		FreeLBS(data->mcpdata);
		FreeLBS(data->spadata);
		FreeLBS(data->linkdata);
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			FreeLBS(data->scrdata[i]);
		}
	}
	xfree(data);
dbgmsg("<FinishSession");
}

static	SessionData	*
InitSession(
	NETFILE	*fp)
{
	SessionData	*data;
	char	buff[SIZE_NAME];
	char	msg[SIZE_NAME];
	LD_Node	*ld;
	int			i;

dbgmsg(">InitSession");
	data = MakeSessionData();
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		data->fKeep = TRUE;
	} else {
		data->fKeep = FALSE;
	}
	ON_IO_ERROR(fp,badio);
	RecvString(fp,data->hdr->term);		ON_IO_ERROR(fp,badio);
	RecvString(fp,data->hdr->user);		ON_IO_ERROR(fp,badio);
	sprintf(msg,"[%s:%s] session start",data->hdr->term,data->hdr->user);
	MessageLog(msg);
	dbgprintf("term = [%s]",data->hdr->term);
	dbgprintf("user = [%s]",data->hdr->user);
	RecvString(fp,buff);	/*	LD name	*/		ON_IO_ERROR(fp,badio);
	if		(  ( ld = g_hash_table_lookup(APS_Hash,buff) )
			   !=  NULL  ) {
		data->ld = ld;
		data->mcpdata = NewLBS();
		InitializeValue(ThisEnv->mcprec->value);
		LBS_ReserveSize(data->mcpdata,NativeSizeValue(NULL,ThisEnv->mcprec->value),FALSE);
		NativePackValue(NULL,data->mcpdata->body,ThisEnv->mcprec->value);
		data->spadata = NewLBS();
		InitializeValue(ld->info->sparec->value);
		LBS_ReserveSize(data->spadata,NativeSizeValue(NULL,ld->info->sparec->value),FALSE);
		NativePackValue(NULL,data->spadata->body,ld->info->sparec->value);
		data->linkdata = NewLBS();
		InitializeValue(ThisEnv->linkrec->value);
		LBS_ReserveSize(data->linkdata,NativeSizeValue(NULL,ThisEnv->linkrec->value),FALSE);
		NativePackValue(NULL,data->linkdata->body,ThisEnv->linkrec->value);
		data->cWindow = ld->info->cWindow;
		data->scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
													* data->cWindow);
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			data->scrdata[i] = NewLBS();
			InitializeValue(data->ld->info->window[i]->rec->value);
			LBS_ReserveSize(data->scrdata[i],
							NativeSizeValue(NULL,ld->info->window[i]->rec->value),FALSE);
			NativePackValue(NULL,data->scrdata[i]->body,ld->info->window[i]->rec->value);
		}
		data->name = StrDup(data->hdr->term);
		g_hash_table_insert(TermHash,data->name,data);
		data->hdr->status = TO_CHAR(APL_SESSION_LINK);
		data->hdr->puttype = TO_CHAR(SCREEN_NULL);
		data->w.n = 0;
	} else {
		sprintf(msg,"[%s] session fail LD [%s] not found.",data->hdr->term,buff);
		MessageLog(msg);
	  badio:
		SendPacketClass(fp,APS_NOT);
		FinishSession(data);
		data = NULL;
	}
dbgmsg("<InitSession");
	return	(data);
}

static	void
ProcessBLOB(
	NETFILE		*fp,
	SessionData	*data)
{
#if	0
	MonObjectType	obj;
	int				mode;
	size_t			size;

	switch	(RecvPacketClass(fp)) {
	  case	BLOB_CREATE:
		mode = RecvInt(fp);
		if		(  NewBLOB(&obj,mode)  ) {
			SendPacketClass(fp,WFC_OK);
			SendObject(fp,&obj);
		} else {
			SendPacketClass(fp,WFC_NG);
		}
		break;
	  case	BLOB_OPEN:
		mode = RecvInt(fp);
		RecvObject(fp,&obj);
		if		(  OpenBLOB(&obj,mode)  ) {
			SendPacketClass(fp,WFC_OK);
		} else {
			SendPacketClass(fp,WFC_NG);
		}
		break;
	  case	BLOB_WRITE:
		RecvObject(fp,&obj);
		if		(  ( size = RecvLength(fp) )  >  0  ) {
			buff = xmalloc(size);
			Recv(fp,buff,size);
			size = WriteBLOB(&obj,buff,size);
			xfree(buff);
		}
		SendLength(fp,size);
		break;
	  case	BLOB_READ:
		RecvObject(fp,&obj);
		if		(  ( size = RecvLength(fp) )  >  0  ) {
			buff = xmalloc(size);
			size = ReadBLOB(&obj,buff,size);
			Send(fp,buff,size);
			xfree(buff);
		}
		SendLength(fp,size);
		break;
	  case	BLOB_CLOSE:
		RecvObject(fp,&obj);
		if		(  CloseBLOB(&obj)  ) {
			SendPacketClass(fp,WFC_OK);
		} else {
			SendPacketClass(fp,WFC_NG);
		}
		break;
	  default:
		break;
	}
#endif
}


static	LD_Node	*
ReadTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	LD_Node	*ld;
	WindowBind	*bind;
	int			ix;
	Bool		fExit;

dbgmsg(">ReadTerminal");
	ld = NULL;
	fExit = FALSE;
	do {
		switch	(RecvPacketClass(fp)) {
		  case	WFC_DATA:
			dbgmsg("recv DATA");
			RecvString(fp,data->hdr->window);	ON_IO_ERROR(fp,badio);
			RecvString(fp,data->hdr->widget);	ON_IO_ERROR(fp,badio);
			RecvString(fp,data->hdr->event);	ON_IO_ERROR(fp,badio);
			dbgprintf("window = [%s]",data->hdr->window);
			dbgprintf("widget = [%s]",data->hdr->widget);
			dbgprintf("event  = [%s]",data->hdr->event);
			if		(  ( ld = g_hash_table_lookup(WindowHash,data->hdr->window) )
					   !=  NULL  ) {
				data->ld = ld;
				bind = (WindowBind *)g_hash_table_lookup(ld->info->whash,data->hdr->window);
				if		(  bind  !=  NULL  ) {
					if		(  bind->ix  >=  0  ) {
						SendPacketClass(fp,APS_OK);				ON_IO_ERROR(fp,badio);
						dbgmsg("send OK");
						RecvLBS(fp,data->scrdata[bind->ix]);	ON_IO_ERROR(fp,badio);
						data->hdr->rc = TO_CHAR(0);
						data->hdr->status = TO_CHAR(APL_SESSION_GET);
						data->hdr->puttype = TO_CHAR(SCREEN_NULL);
					}
				}
			}
			break;
		  case	WFC_BLOB:
			dbgmsg("recv LARGE");
			ProcessBLOB(fp,data);
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
			ON_IO_ERROR(fp,badio);
			dbgmsg("recv default");
			break;
		}
	}	while	(  !fExit  );
  badio:
	if		(  ld  ==  NULL  ) {
		SendPacketClass(fp,WFC_NOT);
	}
dbgmsg("<ReadTerminal");
	return(ld);
}

static	Bool
WriteTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	MessageHeader	*hdr;
	int			i;
	Bool		rc;
	WindowBind	*bind;
	int			ix;
	Bool		fExit;
	char		wname[SIZE_LONGNAME+1];

dbgmsg(">WriteTerminal");
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
#if	1
		SendChar(fp,hdr->puttype);			ON_IO_ERROR(fp,badio);
#endif
		SendInt(fp,data->w.n);				ON_IO_ERROR(fp,badio);
		for	( i = 0 ; i < data->w.n ; i ++ ) {
			SendInt(fp,data->w.control[i].PutType);			ON_IO_ERROR(fp,badio);
			SendString(fp,data->w.control[i].window);		ON_IO_ERROR(fp,badio);
		}
		data->w.n = 0;
		fExit = FALSE;
		do {
			switch	(RecvPacketClass(fp))	{
			  case	WFC_PING:
				dbgmsg("PING");
				SendPacketClass(fp,WFC_PONG);		ON_IO_ERROR(fp,badio);
				break;
			  case	WFC_DATA:
				dbgmsg("DATA");
				RecvString(fp,wname);				ON_IO_ERROR(fp,badio);
				bind = (WindowBind *)g_hash_table_lookup(data->ld->info->whash,wname);
				if		(	(  bind      !=  NULL  )
						&&	(  bind->ix  >=  0     ) )	{
					SendPacketClass(fp,WFC_OK);					ON_IO_ERROR(fp,badio);
					SendLBS(fp,data->scrdata[bind->ix]);		ON_IO_ERROR(fp,badio);
				} else {
					SendPacketClass(fp,WFC_NOT);				ON_IO_ERROR(fp,badio);
				}
				break;
			  case	WFC_BLOB:
				dbgmsg("recv LARGE");
				ProcessBLOB(fp,data);
				break;
			  case	WFC_OK:
				dbgmsg("OK");
				fExit = TRUE;
				break;
			  default:
				break;
			}
		}	while	(  !fExit  );
		rc = TRUE;
	} else {
	  badio:
		dbgmsg("recv FALSE");
	}
dbgmsg("<WriteTerminal");
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

dbgmsg(">TermThread");
	term = New(TermNode);
	term->que = NewQueue();
	term->fp = SocketToNet(fhTerm);
	
	if		(  ( data = InitSession(term->fp) )  !=  NULL  ) {
		do {
			data->term = term;
			data->retry = 0;
			data->fAbort = FALSE;
			CoreEnqueue(data);
			dbgmsg("process !!");
			data = DeQueue(term->que);
			if		(  data->fAbort  )	break;
			if		(  WriteTerminal(term->fp,data)  ) {
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
dbgmsg("<TermThread");
	pthread_exit(NULL);
}

extern	pthread_t
ConnectTerm(
	int		_fhTerm)
{
	int		fhTerm;
	pthread_t	thr;

dbgmsg(">ConnectTerm");
	if		(  ( fhTerm = accept(_fhTerm,0,0) )  <  0  )	{
		MessagePrintf("_fhTerm = %d INET Domain Accept",_fhTerm);
		exit(1);
	}
	pthread_create(&thr,NULL,(void *(*)(void *))TermThread,(void *)fhTerm);
dbgmsg("<ConnectTerm");
	return	(thr); 
}

extern	void
InitTerm(void)
{
dbgmsg(">InitTerm");
	TermHash = NewNameHash();
dbgmsg("<InitTerm");
}
