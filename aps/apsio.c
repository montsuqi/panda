/*
PANDA -- a simple transaction monitor
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

#include	"libmondai.h"
#include	"directory.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"wfcdata.h"
#include	"handler.h"
#include	"apsio.h"
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

ENTER_FUNC;
	if		(  ( ent = g_hash_table_lookup(CacheTable,term) )  !=  NULL  ) {
		NativeUnPackValue(NULL,ent->linkdata->body,node->linkrec->value);
		NativeUnPackValue(NULL,ent->spadata->body,node->sparec->value);
		ret = TRUE;
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

static	byte
SaveCache(
	ProcessNode	*node)
{
	byte		flag;
	SessionCache	*ent;
	size_t		size;

ENTER_FUNC;
	flag = 0;
	dbgprintf("node->term = [%s]\n",node->term);
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

	size =  NativeSizeValue(NULL,node->linkrec->value);
	LBS_ReserveSize(buff,size,FALSE);
	NativePackValue(NULL,buff->body,node->linkrec->value);
	if		(	(  ent->linkdata  ==  NULL  )
			||	(  memcmp(buff->body,ent->linkdata->body,size)  ) ) {
			flag |= APS_LINKDATA;
			if		(  ent->linkdata  !=  NULL  ) {
				FreeLBS(ent->linkdata);
			}
			ent->linkdata = LBS_Duplicate(buff);
	}
	size =  NativeSizeValue(NULL,node->sparec->value);
	LBS_ReserveSize(buff,size,FALSE);
	NativePackValue(NULL,buff->body,node->sparec->value);
	if		(	(  ent->spadata  ==  NULL  )
			||	(  memcmp(buff->body,ent->spadata->body,size)  ) ) {
			flag |= APS_SPADATA;
			if		(  ent->spadata  !=  NULL  ) {
				FreeLBS(ent->spadata);
			}
			ent->spadata = LBS_Duplicate(buff);
	}
LEAVE_FUNC;
	return	(flag);
}


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
	ValueStruct	*e;

ENTER_FUNC;
	fEnd = FALSE; 
	fSuc = FALSE;
	flag = APS_EVENTDATA | APS_MCPDATA | APS_SCRDATA;
	fSuc = FALSE;
	if		(  RecvPacketClass(fp)  ==  APS_REQ  ) {
		dbgmsg("REQ");
		RecvnString(fp, sizeof(term), term);			ON_IO_ERROR(fp,badio2);
		dbgprintf("term = [%s]\n",term);
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
				hdr.status = RecvChar(fp);					ON_IO_ERROR(fp,badio);
				RecvnString(fp, SIZE_TERM+1, hdr.term);		ON_IO_ERROR(fp,badio);
				RecvnString(fp, SIZE_USER+1, hdr.user);		ON_IO_ERROR(fp,badio);
				RecvnString(fp, SIZE_NAME+1, hdr.window);	ON_IO_ERROR(fp,badio);
				RecvnString(fp, SIZE_NAME+1, hdr.widget);	ON_IO_ERROR(fp,badio);
				RecvnString(fp, SIZE_EVENT+1, hdr.event);	ON_IO_ERROR(fp,badio);
				hdr.dbstatus = RecvChar(fp);				ON_IO_ERROR(fp,badio);
#ifdef	DEBUG
				dbgprintf("status = [%c]\n",hdr.status);
				dbgprintf("term   = [%s]\n",hdr.term);
				dbgprintf("user   = [%s]\n",hdr.user);
				dbgprintf("window = [%s]\n",hdr.window);
				dbgprintf("widget = [%s]\n",hdr.widget);
				dbgprintf("event  = [%s]\n",hdr.event);
				dbgprintf("dbstatus=[%c]\n",hdr.dbstatus);
#endif
				fSuc = TRUE;
				break;
			  case	APS_MCPDATA:
				dbgmsg("MCPDATA");
				RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
				e = node->mcprec->value;
				NativeUnPackValue(NULL,LBS_Body(buff),e);

				SetValueChar(GetItemLongName(e,"private.pstatus"),hdr.status);
				SetValueString(GetItemLongName(e,"dc.term"),hdr.term,NULL);
				SetValueString(GetItemLongName(e,"dc.user"),hdr.user,NULL);
				SetValueString(GetItemLongName(e,"dc.window"),hdr.window,NULL);
				SetValueString(GetItemLongName(e,"dc.widget"),hdr.widget,NULL);
				SetValueString(GetItemLongName(e,"dc.event"),hdr.event,NULL);
				SetValueChar(GetItemLongName(e,"dc.dbstatus"),hdr.dbstatus);

				node->pstatus = hdr.status;
				node->dbstatus = hdr.dbstatus;
				strcpy(node->term,hdr.term);
				strcpy(node->user,hdr.user);
				strcpy(node->window,hdr.window);
				strcpy(node->widget,hdr.widget);
				strcpy(node->event,hdr.event);
				break;
			  case	APS_LINKDATA:
				dbgmsg("LINKDATA");
				RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
				NativeUnPackValue(NULL,LBS_Body(buff),node->linkrec->value);
				break;
			  case	APS_SPADATA:
				dbgmsg("SPADATA");
				RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
				NativeUnPackValue(NULL,LBS_Body(buff),node->sparec->value);
				break;
			  case	APS_SCRDATA:
				dbgmsg("SCRDATA");
				for	( i = 0 ; i < node->cWindow ; i ++ ) {
					if		(  node->scrrec[i]  !=  NULL  ) {
						RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
						NativeUnPackValue(NULL,LBS_Body(buff),node->scrrec[i]->value);
					}
				}
				break;
			  case	APS_END:
				dbgmsg("END");
				SetValueInteger(GetItemLongName(node->mcprec->value,"private.prc"),0);
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
	dbgprintf("mcp  = %d\n",NativeSizeValue(NULL,node->mcprec->value));
	dbgprintf("link = %d\n",NativeSizeValue(NULL,node->linkrec->value));
	dbgprintf("spa  = %d\n",NativeSizeValue(NULL,node->sparec->value));
  badio2:
LEAVE_FUNC;
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
	ValueStruct	*e;

ENTER_FUNC;
	flag = APS_MCPDATA | APS_SCRDATA;
	flag |= ( node->w.n > 0 ) ? APS_WINCTRL : 0;
	if		(  nCache  >  0  ) {
		flag |= SaveCache(node);
	} else {
		flag |= APS_LINKDATA | APS_SPADATA;
	}

	e = node->mcprec->value;
	SendPacketClass(fp,APS_CTRLDATA);		ON_IO_ERROR(fp,badio);
	SendChar(fp,flag);						ON_IO_ERROR(fp,badio);
	SendString(fp,ValueStringPointer(GetItemLongName(e,"dc.user")));
	ON_IO_ERROR(fp,badio);
	SendString(fp,ValueStringPointer(GetItemLongName(e,"dc.window")));
	ON_IO_ERROR(fp,badio);
	SendString(fp,ValueStringPointer(GetItemLongName(e,"dc.widget")));
	ON_IO_ERROR(fp,badio);
	SendChar(fp,node->dbstatus);
	ON_IO_ERROR(fp,badio);
	SendChar(fp,*ValueStringPointer(GetItemLongName(e,"private.pputtype")));
	ON_IO_ERROR(fp,badio);
	fEnd = FALSE; 
	while	(  !fEnd  ) {
		if		(  node->mcprec  !=  NULL  ) {
			dbgprintf("mcp  = %d\n",NativeSizeValue(NULL,node->mcprec->value));
		}
		if		(  node->linkrec  !=  NULL  ) {
			dbgprintf("link = %d\n",NativeSizeValue(NULL,node->linkrec->value));
		}
		if		(  node->sparec  !=  NULL  ) {
			dbgprintf("spa  = %d\n",NativeSizeValue(NULL,node->sparec->value));
		}
		for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
			if		(  node->scrrec[i]  !=  NULL  ) {
				dbgprintf("scr[%s]  = %d\n",node->scrrec[i]->name,
						  NativeSizeValue(NULL,node->scrrec[i]->value));
			}
		}
		switch	(c = RecvPacketClass(fp)) {
		  case	APS_WINCTRL:
			dbgmsg("WINCTRL");
			SendInt(fp,node->w.n);					ON_IO_ERROR(fp,badio);
			for	( i = 0 ; i < node->w.n ; i ++ ) {
				SendInt(fp,node->w.control[i].PutType);
				SendString(fp,node->w.control[i].window);
			}
			break;
		  case	APS_MCPDATA:
			dbgmsg("MCPDATA");
			if		(  node->mcprec  !=  NULL  ) {
				LBS_ReserveSize(buff,NativeSizeValue(NULL,node->mcprec->value),FALSE);
				NativePackValue(NULL,LBS_Body(buff),node->mcprec->value);
			} else {
				RewindLBS(buff);
			}
			SendLBS(fp,buff);						ON_IO_ERROR(fp,badio);
			break;
		  case	APS_LINKDATA:
			dbgmsg("LINKDATA");
			if		(  node->linkrec  !=  NULL  ) {
				LBS_ReserveSize(buff,NativeSizeValue(NULL,node->linkrec->value),FALSE);
				NativePackValue(NULL,LBS_Body(buff),node->linkrec->value);
			} else {
				RewindLBS(buff);
			}
			SendLBS(fp,buff);
			break;
		  case	APS_SPADATA:
			dbgmsg("SPADATA");
			if		(  node->sparec  !=  NULL  ) {
				LBS_ReserveSize(buff,NativeSizeValue(NULL,node->sparec->value),FALSE);
				NativePackValue(NULL,LBS_Body(buff),node->sparec->value);
			} else {
				RewindLBS(buff);
			}
			SendLBS(fp,buff);						ON_IO_ERROR(fp,badio);
			break;
		  case	APS_SCRDATA:
			dbgmsg("SCRDATA");
			for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
				if		(  node->scrrec[i]  !=  NULL  ) {
					LBS_ReserveSize(buff,
									NativeSizeValue(NULL,node->scrrec[i]->value),FALSE);
					NativePackValue(NULL,LBS_Body(buff),node->scrrec[i]->value);
					SendLBS(fp,buff);						ON_IO_ERROR(fp,badio);
				}
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
			fEnd = TRUE;
			break;
		  badio:
			dbgmsg("badio");
			fEnd = TRUE;
			break;
		}
	}
LEAVE_FUNC;
}

