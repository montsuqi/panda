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
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>

#include	"types.h"
#include	"const.h"
#include	"misc.h"

#include	"libmondai.h"
#include	"directory.h"
#include	"net.h"
#include	"comm.h"
#include	"wfc.h"
#include	"handler.h"
#include	"apsio.h"
#include	"tcp.h"
#include	"debug.h"

typedef	struct _SessionCache	{
	char	*term;
	struct	_SessionCache	*prev
	,						*next;
	LargeByteString	*spadata;
	LargeByteString	*linkdata;
}	SessionCache;

static	LargeByteString	*buff;
static	GHashTable		*CacheTable;
static	SessionCache	*Head
		,				*Tail;
static	int				cTerm;

extern	void
InitAPSIO(
	NETFILE	*fpWFC)
{
	buff = NewLBS();
	CacheTable = NewNameHash();
	ApsId = RecvInt(fpWFC);
	Head = NULL;
	Tail = NULL;
	cTerm = 0;
	if		(  nCache  ==  0  ) {
		nCache = ThisLD->nCache;
	}
#ifdef	DEBUG
	printf("my id = %d\n",ApsId);
#endif
}

static	Bool
CheckCache(
	ProcessNode	*node,
	char		*term)
{
	Bool	ret;
	SessionCache	*ent;

dbgmsg(">CheckCache");
	if		(  ( ent = g_hash_table_lookup(CacheTable,term) )  !=  NULL  ) {
		NativeUnPackValue(NULL,ent->linkdata->body,node->linkrec);
		NativeUnPackValue(NULL,ent->spadata->body,node->linkrec);
		ret = TRUE;
	} else {
		ret = FALSE;
	}
dbgmsg("<CheckCache");
	return	(ret);
}

static	byte
SaveCache(
	ProcessNode	*node)
{
	byte		flag;
	SessionCache	*ent;
	size_t		size;

dbgmsg(">SaveCache");
	flag = 0;
	printf("node->term = [%s]\n",node->term);
	if		(  ( ent = g_hash_table_lookup(CacheTable,node->term) )  ==  NULL  ) {
		dbgmsg("empty");
		/*	cache purge logic here	*/
		if		(  cTerm  ==  nCache  ) {
			if		(  Tail  !=  NULL  ) {
				if		(  Tail->prev  !=  NULL  ) {
					Tail->prev->next = NULL;
				}
				ent = Tail->prev;
				xfree(Tail->linkdata);
				xfree(Tail->spadata);
				xfree(Tail->term);
				xfree(Tail);
				if		(  Head  ==  Tail  ) {
					Head = NULL;
					Tail = NULL;
				} else {
					Tail = ent;
				}
			}
		} else {
			cTerm ++;
		}
		ent = New(SessionCache);
		ent->term = StrDup(node->term);
		g_hash_table_insert(CacheTable,ent->term,ent);
		ent->linkdata = NULL;
		ent->spadata = NULL;
		ent->next = Head;
		if		(  Tail  ==  NULL  ) {
			Tail = ent;
		}
	} else {
		if		(  ent  !=  Head  ) {
			if		(  ent->next  !=  NULL  ) {
				ent->next->prev = ent->prev;
			}
			if		(  ent->prev  !=  NULL  ) {
				ent->prev->next = ent->next;
			}
			if		(  ent  ==  Tail  ) {
				Tail = ent->prev;
			}
			ent->next = Head;
		}
	}
	if		(  Head  !=  NULL  ) {
		Head->prev = ent;
	}
	ent->prev = NULL;
	Head = ent;
#ifdef	DEBUG
	{
		SessionCache	*p;
		printf("*** term dump Head -> Tail ***\n");
		for	( p = Head ; p != NULL ; p = p->next ) {
			printf("[%s]\n",p->term);
		}
		printf("*****************\n");
		printf("*** term dump Tail -> Head  ***\n");
		for	( p = Tail ; p != NULL ; p = p->prev ) {
			printf("[%s]\n",p->term);
		}
		printf("*****************\n");
	}
#endif

	size =  NativeSizeValue(NULL,node->linkrec);
	LBS_ReserveSize(buff,size,FALSE);
	NativePackValue(NULL,buff->body,node->linkrec);
	if		(	(  ent->linkdata  ==  NULL  )
			||	(  memcmp(buff->body,ent->linkdata->body,size)  ) ) {
			flag |= APS_LINKDATA;
			if		(  ent->linkdata  !=  NULL  ) {
				FreeLBS(ent->linkdata);
			}
			ent->linkdata = LBS_Duplicate(buff);
	}
	size =  NativeSizeValue(NULL,node->sparec);
	LBS_ReserveSize(buff,size,FALSE);
	NativePackValue(NULL,buff->body,node->sparec);
	if		(	(  ent->spadata  ==  NULL  )
			||	(  memcmp(buff->body,ent->spadata->body,size)  ) ) {
			flag |= APS_SPADATA;
			if		(  ent->spadata  !=  NULL  ) {
				FreeLBS(ent->spadata);
			}
			ent->spadata = LBS_Duplicate(buff);
	}
dbgmsg("<SaveCache");
	return	(flag);
}


/*
 *	for APS
 */
extern	Bool
GetWFC(
	NETFILE	*fp,
	ProcessNode	*node)
{
	int			i;
	PacketClass	c;
	Bool		fSuc;
	Bool		fEnd;
	MessageHeader	hdr;
	char		term[SIZE_TERM+1];
	byte		flag;

dbgmsg(">GetWFC");
	fEnd = FALSE; 
	fSuc = FALSE;
	flag = APS_EVENTDATA | APS_MCPDATA | APS_SCRDATA;
	fSuc = FALSE;
	if		(  RecvPacketClass(fp)  ==  APS_REQ  ) {
		dbgmsg("REQ");
		RecvString(fp,term);				ON_IO_ERROR(fp,badio2);
		if		(  nCache  >  0  ) {
			if		(  !CheckCache(node,term)  ) {
				flag |= APS_SPADATA;
				flag |= APS_LINKDATA;
			}
		} else {
			flag |= APS_SPADATA | APS_LINKDATA;
		}
		SendChar(fp,flag);					ON_IO_ERROR(fp,badio2);
		while	(  !fEnd  ) {
			switch	(c = RecvPacketClass(fp)) {
			  case	APS_EVENTDATA:
				dbgmsg("EVENTDATA");
				hdr.status = RecvChar(fp);			ON_IO_ERROR(fp,badio);
				RecvString(fp,hdr.term);			ON_IO_ERROR(fp,badio);
				RecvString(fp,hdr.user);			ON_IO_ERROR(fp,badio);
				RecvString(fp,hdr.window);			ON_IO_ERROR(fp,badio);
				RecvString(fp,hdr.widget);			ON_IO_ERROR(fp,badio);
				RecvString(fp,hdr.event);			ON_IO_ERROR(fp,badio);
#ifdef	DEBUG
				printf("status = [%c]\n",hdr.status);
				printf("term   = [%s]\n",hdr.term);
				printf("user   = [%s]\n",hdr.user);
				printf("window = [%s]\n",hdr.window);
				printf("widget = [%s]\n",hdr.widget);
				printf("event  = [%s]\n",hdr.event);
#endif
				fSuc = TRUE;
				break;
			  case	APS_MCPDATA:
				dbgmsg("MCPDATA");
				RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
				NativeUnPackValue(NULL,buff->body,node->mcprec);
				node->pstatus = ValueString(GetItemLongName(node->mcprec,"private.pstatus"));
				strcpy(ValueString(GetItemLongName(node->mcprec,"dc.term")),hdr.term);
				strcpy(ValueString(GetItemLongName(node->mcprec,"dc.user")),hdr.user);
				node->window = ValueString(GetItemLongName(node->mcprec,"dc.window"));
				node->widget = ValueString(GetItemLongName(node->mcprec,"dc.widget"));
				node->event = ValueString(GetItemLongName(node->mcprec,"dc.event"));
				*node->pstatus = hdr.status;
				strcpy(node->term,hdr.term);
				strcpy(node->user,hdr.user);
				strcpy(node->window,hdr.window);
				strcpy(node->widget,hdr.widget);
				strcpy(node->event,hdr.event);
				break;
			  case	APS_LINKDATA:
				dbgmsg("LINKDATA");
				RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
				NativeUnPackValue(NULL,buff->body,node->linkrec);
				break;
			  case	APS_SPADATA:
				dbgmsg("SPADATA");
				RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
				NativeUnPackValue(NULL,buff->body,node->sparec);
				break;
			  case	APS_SCRDATA:
				dbgmsg("SCRDATA");
				for	( i = 0 ; i < node->cWindow ; i ++ ) {
					RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
					NativeUnPackValue(NULL,buff->body,node->scrrec[i]);
				}
				break;
			  case	APS_END:
				dbgmsg("END");
				*ValueString(GetItemLongName(node->mcprec,"private.prc")) = TO_CHAR(0);
				fEnd = TRUE;
				break;
			  case	APS_STOP:
				dbgmsg("STOP");
				fSuc = FALSE;
				fEnd = TRUE;
				break;
			  case	APS_PING:
				dbgmsg("PING");
				SendPacketClass(fp,APS_PONG);		ON_IO_ERROR(fp,badio);
				break;
			  default:
				dbgmsg("default");
				SendPacketClass(fp,APS_NOT);		ON_IO_ERROR(fp,badio);
			  badio:
				fEnd = TRUE;
				break;
			}
		}
	}
  badio2:
dbgmsg("<GetWFC");
	return	(fSuc); 
}

extern	void
PutWFC(
	NETFILE	*fp,
	ProcessNode	*node)
{
	int				i;
	PacketClass		c;
	Bool			fEnd;
	byte		flag;

dbgmsg(">PutWFC");
	flag = APS_MCPDATA | APS_SCRDATA;
	flag |= ( node->w.n > 0 ) ? APS_CLSWIN : 0;
	if		(  nCache  >  0  ) {
		flag |= SaveCache(node);
	} else {
		flag |= APS_LINKDATA | APS_SPADATA;
	}

	SendPacketClass(fp,APS_CTRLDATA);		ON_IO_ERROR(fp,badio);
	SendChar(fp,flag);						ON_IO_ERROR(fp,badio);
	SendString(fp,ValueString(GetItemLongName(node->mcprec,"dc.term")));
	ON_IO_ERROR(fp,badio);
	SendString(fp,ValueString(GetItemLongName(node->mcprec,"dc.window")));
	ON_IO_ERROR(fp,badio);
	SendString(fp,ValueString(GetItemLongName(node->mcprec,"dc.widget")));
	ON_IO_ERROR(fp,badio);
	SendChar(fp,*ValueString(GetItemLongName(node->mcprec,"private.pputtype")));
	ON_IO_ERROR(fp,badio);
	fEnd = FALSE; 
	while	(  !fEnd  ) {
		switch	(c = RecvPacketClass(fp)) {
		  case	APS_CLSWIN:
			dbgmsg("CLSWIN");
			SendInt(fp,node->w.n);					ON_IO_ERROR(fp,badio);
			for	( i = 0 ; i < node->w.n ; i ++ ) {
				SendString(fp,node->w.close[i].window);
			}
			break;
		  case	APS_MCPDATA:
			dbgmsg("MCPDATA");
			LBS_ReserveSize(buff,NativeSizeValue(NULL,node->mcprec),FALSE);
			NativePackValue(NULL,buff->body,node->mcprec);
			SendLBS(fp,buff);						ON_IO_ERROR(fp,badio);
			break;
		  case	APS_LINKDATA:
			dbgmsg("LINKDATA");
			LBS_ReserveSize(buff,NativeSizeValue(NULL,node->linkrec),FALSE);
			NativePackValue(NULL,buff->body,node->linkrec);
			SendLBS(fp,buff);
			break;
		  case	APS_SPADATA:
			dbgmsg("SPADATA");
			LBS_ReserveSize(buff,NativeSizeValue(NULL,node->sparec),FALSE);
			NativePackValue(NULL,buff->body,node->sparec);
			SendLBS(fp,buff);						ON_IO_ERROR(fp,badio);
			break;
		  case	APS_SCRDATA:
			dbgmsg("SCRDATA");
			for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
				LBS_ReserveSize(buff,
								NativeSizeValue(NULL,node->scrrec[i]),FALSE);
				NativePackValue(NULL,buff->body,node->scrrec[i]);
				SendLBS(fp,buff);						ON_IO_ERROR(fp,badio);
			}
			break;
		  case	APS_END:
			dbgmsg("END");
			fEnd = TRUE;
			break;
		  case	APS_PING:
			dbgmsg("PING");
			SendPacketClass(fp,APS_PONG);			ON_IO_ERROR(fp,badio);
			break;
		  default:
			dbgmsg("default");
			SendPacketClass(fp,APS_NOT);			ON_IO_ERROR(fp,badio);
		  badio:
			fEnd = TRUE;
			break;
		}
	}
dbgmsg("<PutWFC");
}

