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

#define	DEBUG
#define	TRACE
/*
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
#include	"misc.h"

#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"tcp.h"
#include	"queue.h"
#include	"directory.h"
#include	"wfc.h"
#include	"wfcio.h"
#include	"termthread.h"
#include	"corethread.h"
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
	} else {
		strcpy(name,"");
	}
	FreeLBS(data->mcpdata);
	FreeLBS(data->spadata);
	FreeLBS(data->linkdata);
	for	( i = 0 ; i < data->cWindow ; i ++ ) {
		FreeLBS(data->scrdata[i]);
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
#ifdef	DEBUG
	printf("term = [%s]\n",data->hdr->term);
	printf("user = [%s]\n",data->hdr->user);
#endif
	RecvString(fp,buff);	/*	LD name	*/		ON_IO_ERROR(fp,badio);
	if		(  ( ld = g_hash_table_lookup(APS_Hash,buff) )
			   !=  NULL  ) {
		data->ld = ld;
		data->mcpdata = NewLBS();
		InitializeValue(ThisEnv->mcprec);
		LBS_ReserveSize(data->mcpdata,NativeSizeValue(NULL,ThisEnv->mcprec),FALSE);
		NativePackValue(NULL,data->mcpdata->body,ThisEnv->mcprec);
		data->spadata = NewLBS();
		InitializeValue(ld->info->sparec);
		LBS_ReserveSize(data->spadata,NativeSizeValue(NULL,ld->info->sparec),FALSE);
		NativePackValue(NULL,data->spadata->body,ld->info->sparec);
		data->linkdata = NewLBS();
		InitializeValue(ThisEnv->linkrec);
		LBS_ReserveSize(data->linkdata,NativeSizeValue(NULL,ThisEnv->linkrec),FALSE);
		NativePackValue(NULL,data->linkdata->body,ThisEnv->linkrec);
		data->cWindow = ld->info->cWindow;
		data->scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
													* data->cWindow);
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			data->scrdata[i] = NewLBS();
			InitializeValue(data->ld->info->window[i]->value);
			LBS_ReserveSize(data->scrdata[i],
							NativeSizeValue(NULL,ld->info->window[i]->value),FALSE);
			NativePackValue(NULL,data->scrdata[i]->body,ld->info->window[i]->value);
		}
		data->name = StrDup(data->hdr->term);
		g_hash_table_insert(TermHash,data->name,data);
		data->hdr->status = TO_CHAR(APL_SESSION_LINK);
		data->hdr->puttype = TO_CHAR(0);
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

static	LD_Node	*
ReadTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	LD_Node	*ld;
	int			ix;

dbgmsg(">ReadTerminal");
  top: 
	ld = NULL; 
	switch	(RecvPacketClass(fp)) {
	  case	WFC_DATA:
		dbgmsg("recv DATA");
		RecvString(fp,data->hdr->window);	ON_IO_ERROR(fp,badio);
		RecvString(fp,data->hdr->widget);	ON_IO_ERROR(fp,badio);
		RecvString(fp,data->hdr->event);	ON_IO_ERROR(fp,badio);
#ifdef	DEBUG
		printf("window = [%s]\n",data->hdr->window);
		printf("widget = [%s]\n",data->hdr->widget);
		printf("event  = [%s]\n",data->hdr->event);
#endif
		if		(  ( ld = g_hash_table_lookup(WindowHash,data->hdr->window) )
				   !=  NULL  ) {
			data->ld = ld;
			ix = (int)g_hash_table_lookup(ld->info->whash,data->hdr->window);
			if		(  ix  >  0  ) {
				SendPacketClass(fp,APS_OK);			ON_IO_ERROR(fp,badio);
				dbgmsg("send OK");
				RecvLBS(fp,data->scrdata[ix-1]);	ON_IO_ERROR(fp,badio);
				data->hdr->rc = TO_CHAR(0);
				data->hdr->status = TO_CHAR(APL_SESSION_GET);
				data->hdr->puttype = TO_CHAR(0);
			}
		}
		break;
	  case	WFC_PING:
		dbgmsg("recv PING");
		SendPacketClass(fp,WFC_PONG);		ON_IO_ERROR(fp,badio);
		dbgmsg("send PONG");
		goto	top;
		break;
	  default:
		ON_IO_ERROR(fp,badio);
		dbgmsg("recv default");
		break;
	}
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
	int			ix;

dbgmsg(">WriteTerminal");
	rc = FALSE;
	SendPacketClass(fp,WFC_PING);		ON_IO_ERROR(fp,badio);
	dbgmsg("send PING");
	if		(  RecvPacketClass(fp)  ==  WFC_PONG  ) {
		dbgmsg("recv PONG");
		ON_IO_ERROR(fp,badio);
		SendPacketClass(fp,WFC_DATA);		ON_IO_ERROR(fp,badio);
		dbgmsg("send DATA");
		hdr = data->hdr;
		SendString(fp,hdr->window);			ON_IO_ERROR(fp,badio);
		SendString(fp,hdr->widget);			ON_IO_ERROR(fp,badio);
		SendChar(fp,hdr->puttype);			ON_IO_ERROR(fp,badio);
		SendInt(fp,data->w.n);				ON_IO_ERROR(fp,badio);
		for	( i = 0 ; i < data->w.n ; i ++ ) {
printf("send [%s]\n",data->w.close[i].window);
			SendString(fp,data->w.close[i].window);		ON_IO_ERROR(fp,badio);
		}
printf("data->w.n = %d\n",data->w.n);
		data->w.n = 0;
		ix = (int)g_hash_table_lookup(data->ld->info->whash,data->hdr->window);
		if		(  ix  >  0  ) {
			SendLBS(fp,data->scrdata[ix-1]);			ON_IO_ERROR(fp,badio);
dbgmsg("SendLBS");
			rc = TRUE;
		}
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
			CoreEnqueue(data);
			dbgmsg("process !!");
			data = DeQueue(term->que);
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
		printf("_fhTerm = %d\n",_fhTerm);
		Error("INET Domain Accept");
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
