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

#include	"value.h"
#include	"comm.h"
#include	"tcp.h"
#include	"queue.h"
#include	"directory.h"
#include	"wfc.h"
#include	"wfcio.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"driver.h"
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
dbgmsg("<MakeSessionData");
	return	(data);
}

static	void
FinishSession(
	SessionData	*data)
{
	char	name[SIZE_NAME+1];
	int		i;

dbgmsg(">FinishSession");
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
	printf("session end [%s]\n",name);
dbgmsg("<FinishSession");
}

static	SessionData	*
InitSession(
	FILE	*fp)
{
	SessionData	*data;
	char	buff[SIZE_NAME];
	APS_Node	*aps;
	int			i;

dbgmsg(">InitSession");
	data = MakeSessionData();
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		data->fKeep = TRUE;
	} else {
		data->fKeep = FALSE;
	}
	RecvString(fp,data->hdr->term);
	RecvString(fp,data->hdr->user);
#ifdef	DEBUG
	printf("term = [%s]\n",data->hdr->term);
	printf("user = [%s]\n",data->hdr->user);
#endif
	RecvString(fp,buff);	/*	LD name	*/
	if		(  ( aps = g_hash_table_lookup(APS_Hash,buff) )
			   ==  NULL  ) {
		printf("not found LD name = [%s]\n",buff);
		SendPacketClass(fp,APS_NOT);
		FinishSession(data);
		data = NULL;
	} else {
		data->aps = aps;
		data->mcpdata = NewLBS();
		InitializeValue(ThisEnv->mcprec);
		LBS_RequireSize(data->mcpdata,SizeValue(ThisEnv->mcprec,0,0),FALSE);
		PackValue(data->mcpdata->body,ThisEnv->mcprec);
		data->spadata = NewLBS();
		InitializeValue(aps->ld->sparec);
		LBS_RequireSize(data->spadata,SizeValue(aps->ld->sparec,0,0),FALSE);
		PackValue(data->spadata->body,aps->ld->sparec);
		data->linkdata = NewLBS();
		InitializeValue(ThisEnv->linkrec);
		LBS_RequireSize(data->linkdata,SizeValue(ThisEnv->linkrec,0,0),FALSE);
		PackValue(data->linkdata->body,ThisEnv->linkrec);
		data->cWindow = aps->ld->cWindow;
		data->scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *)
													* data->cWindow);
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			data->scrdata[i] = NewLBS();
			InitializeValue(data->aps->ld->window[i]->value);
			LBS_RequireSize(data->scrdata[i],
							SizeValue(aps->ld->window[i]->value,0,0),FALSE);
			PackValue(data->scrdata[i]->body,aps->ld->window[i]->value);
		}
		data->name = StrDup(data->hdr->term);
		g_hash_table_insert(TermHash,data->name,data);
		data->hdr->status = TO_CHAR(APL_SESSION_LINK);
		data->hdr->puttype = TO_CHAR(0);
		data->w.n = 0;
	}
dbgmsg("<InitSession");
	return	(data);
}

static	APS_Node	*
ReadTerminal(
	FILE		*fp,
	SessionData	*data)
{
	APS_Node	*aps;
	int			ix;

dbgmsg(">ReadTerminal");
  top: 
	switch	(RecvPacketClass(fp)) {
	  case	WFC_DATA:
		dbgmsg("DATA");
		RecvString(fp,data->hdr->window);
		RecvString(fp,data->hdr->widget);
		RecvString(fp,data->hdr->event);
		if		(  ( aps = g_hash_table_lookup(WindowHash,data->hdr->window) )
				   !=  NULL  ) {
			data->aps = aps;
			ix = (int)g_hash_table_lookup(aps->ld->whash,data->hdr->window);
			SendPacketClass(fp,APS_OK);
			RecvLBS(fp,data->scrdata[ix-1]);
			data->hdr->rc = TO_CHAR(0);
			data->hdr->status = TO_CHAR(APL_SESSION_GET);
			data->hdr->puttype = TO_CHAR(0);
		} else {
			SendPacketClass(fp,APS_NOT);
		}
		break;
	  case	WFC_PING:
		dbgmsg("PING");
		SendPacketClass(fp,WFC_PONG);
		goto	top;
		break;
	  default:
		dbgmsg("default");
		SendPacketClass(fp,WFC_NOT);
		aps = NULL;
		break;
	}
dbgmsg("<ReadTerminal");
	return(aps);
}

static	Bool
WriteTerminal(
	FILE		*fp,
	SessionData	*data)
{
	MessageHeader	*hdr;
	int			i;
	Bool		rc;
	int			ix;

dbgmsg(">WriteTerminal");
	SendPacketClass(fp,WFC_PING);
	if		(  RecvPacketClass(fp)  ==  WFC_PONG  ) {
		SendPacketClass(fp,WFC_DATA);
		hdr = data->hdr;
		SendString(fp,hdr->window);
		SendString(fp,hdr->widget);
		SendChar(fp,hdr->puttype);
		SendInt(fp,data->w.n);
		for	( i = 0 ; i < data->w.n ; i ++ ) {
			SendString(fp,data->w.close[i].window);
		}
		data->w.n = 0;
		ix = (int)g_hash_table_lookup(data->aps->ld->whash,data->hdr->window);
		SendLBS(fp,data->scrdata[ix-1]);
		fflush(fp);
		if		(  data->fKeep  ) {
			rc = TRUE;
		} else {
			rc = FALSE;
		}
	} else {
		rc = FALSE;
	}
dbgmsg("<WriteTerminal");
	return	(rc); 
}

static	void
TermThread(
	void	*para)
{
	int		fhTerm = (int)para;
	APS_Node	*aps;
	TermNode	*term;
	SessionData	*data;

	term = New(TermNode);
	term->que = NewQueue();
	term->fp = fdopen(fhTerm,"w+");
	
	if		(  ( data = InitSession(term->fp) )  !=  NULL  ) {
		do {
			data->term = term;
			CoreEnqueue(data);
			dbgmsg("process !!");
			data = DeQueue(term->que);
			if		(  WriteTerminal(term->fp,data)  ) {
				aps = ReadTerminal(term->fp,data);
			} else {
				aps = NULL;
			}
		}	while	(  aps  !=  NULL  );
		FinishSession(data);
	}
	shutdown(fhTerm, 2);
	fclose(term->fp);
	FreeQueue(term->que);
	xfree(term);
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
	TermHash = NewNameHash();
}
