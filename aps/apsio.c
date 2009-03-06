/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
		if		(  node->linkrec  !=  NULL  ) {
			NativeUnPackValue(NULL,ent->linkdata->body,node->linkrec->value);
		}
		if		(  node->sparec  !=  NULL  ) {
			NativeUnPackValue(NULL,ent->spadata->body,node->sparec->value);
		}
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

	if		(  node->linkrec  !=  NULL  ) {
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
	}
	if		(  node->sparec  !=  NULL  ) {
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
	}
LEAVE_FUNC;
	return	(flag);
}

static	Bool
GetWFCTerm(
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
			node->tnest = (int)RecvChar(fp);	ON_IO_ERROR(fp,badio);
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
			if		(  node->linkrec  !=  NULL  ) {
				NativeUnPackValue(NULL,LBS_Body(buff),node->linkrec->value);
			}
			break;
		  case	APS_SPADATA:
			dbgmsg("SPADATA");
			RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
			if		(  node->sparec  !=  NULL  ) {
				NativeUnPackValue(NULL,LBS_Body(buff),node->sparec->value);
			}
			break;
		  case	APS_SCRDATA:
			dbgmsg(">SCRDATA");
			for	( i = 0 ; i < node->cWindow ; i ++ ) {
				if		(  node->scrrec[i]  !=  NULL  ) {
					RecvLBS(fp,buff);					ON_IO_ERROR(fp,badio);
					dbgprintf("rec = [%s]\n",node->scrrec[i]->name);
					NativeUnPackValue(NULL,LBS_Body(buff),node->scrrec[i]->value);
#ifdef	DEBUG
					DumpValueStruct(node->scrrec[i]->value);
#endif
				}
			}
			dbgmsg("<SCRDATA");
			break;
		  case	APS_END:
			dbgmsg("END");
			SetValueInteger(GetItemLongName(node->mcprec->value,"private.prc"),0);
			fEnd = TRUE;
			break;
		  case	APS_STOP:
			dbgmsg("STOP");
			node->pstatus = APL_SYSTEM_END;
			fSuc = TRUE;
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
#ifdef	DEBUG
	dbgprintf("mcp  = %d\n",NativeSizeValue(NULL,node->mcprec->value));
	if		(  node->linkrec  !=  NULL  ) {
		dbgprintf("link = %d\n",NativeSizeValue(NULL,node->linkrec->value));
	}
	if		(  node->sparec  !=  NULL  ) {
		dbgprintf("spa  = %d\n",NativeSizeValue(NULL,node->sparec->value));
	}
#endif
  badio2:
LEAVE_FUNC;
	return	(fSuc); 
}

static	void
PackAPIBody(
	ValueStruct *e,
	LargeByteString *headers,
	LargeByteString *body)
{
	char *head;
	char *p;
	char *key;
	char *value;

ENTER_FUNC;
	head = (char *)LBS_Body(headers);
	if ( head == NULL) {
		return;
	}
	while(1) {
		p = strstr(head, ":");
		if (p == NULL) {
			return;
		}
		key = strndup(head, p - head);
		head = p + 1;
		while(head[0] == ' '){ head++; }
		p = strstr(head, "\r\n");
		if (p != NULL) {
			value = strndup(head, p - head);
			head = p + 2;
		} else {
			value = strdup(head);
		}

		if (!strcmp(key, "Content-Type")) {
			SetValueString(
				GetItemLongName(e, "request.content_type"), value, NULL);
		}
		if (!strcmp(key, "Content-Length")) {
			SetValueString(GetItemLongName(e, 
				"request.content_length"), value, NULL);
		}
		xfree(key);
		xfree(value);
	}
	SetValueString(GetItemLongName(e,"api.request.body"), 
		(char *)LBS_Body(body), NULL);
LEAVE_FUNC;
}

static	void
PackAPIArguments(
	ValueStruct *e,
	LargeByteString *arguments)
{
	char *head;
	char *tail;
	char *key;
	char *value;
	char buff[SIZE_BUFF+1];

ENTER_FUNC;
	head = (char *)LBS_Body(arguments);

	while(1) {
		if (head == NULL || head == '\0') {
			return;
		}
		tail = strstr(head, "=");
		if (tail == NULL) {
			return;
		}
		key = strndup(head, tail - head);
		snprintf(buff, sizeof(buff), "request.arguments.%s", key);
		xfree(key);
		head = tail + 1;
		
		tail = strstr(head, "&");
		if (tail != NULL) {
			value = strndup(head, tail - head);
			SetValueString(GetItemLongName(e, buff), value, NULL);
			xfree(value);
			head = tail + 1;
		} else {
			SetValueString(GetItemLongName(e, buff), head, NULL);
			head = NULL;
		}
	}
LEAVE_FUNC;
}

static	void
PackAPIRecord(
	ProcessNode *node, 
	char *method,
	LargeByteString *arguments, 
	LargeByteString *headers,
	LargeByteString *body)
{
	int i;
	ValueStruct	*e;

ENTER_FUNC;
	e = NULL;
	for(i = 0; i < node->cWindow; i++) {
		if (node->scrrec[i] != NULL &&
			!strcmp(node->scrrec[i]->name, "api")) {
			e = node->scrrec[i]->value;
			break;
		}
	}
	if (e == NULL) {
		Error("not found scrrec for API");
	}
	SetValueString(GetItemLongName(e,"request.method"), method,NULL);

	PackAPIArguments(e, arguments);
	PackAPIBody(e, headers, body);
LEAVE_FUNC;
}

static	Bool
GetWFCAPI(
	NETFILE	*fp,
	ProcessNode	*node)
{
ENTER_FUNC;
	PacketClass		c;
	Bool			fSuc;
	char			method[SIZE_NAME+1];
	LargeByteString *arguments;
	LargeByteString *headers;
	LargeByteString *body;
	MessageHeader	hdr;
	ValueStruct		*e;

	dbgmsg("REQ");

	fSuc = FALSE;

	RecvnString(fp, sizeof(hdr.term), hdr.term);	ON_IO_ERROR(fp,badio);
	dbgprintf("term = [%s]\n",hdr.term);

	if (g_hash_table_lookup(node->bhash, "api") == NULL) {
		SendChar(fp, 0x0);							ON_IO_ERROR(fp,badio);
	} else {
		SendChar(fp, 0xFE);							ON_IO_ERROR(fp,badio);
		
		RecvnString(fp, sizeof(hdr.user), hdr.user);ON_IO_ERROR(fp,badio);
		c = RecvPacketClass(fp);					ON_IO_ERROR(fp,badio);
		if (c != APS_MCPDATA) {
			Warning("Invalid PacketClass(%02X)",c);
			return FALSE;
		}
		RecvLBS(fp, buff);							ON_IO_ERROR(fp,badio);
		e = node->mcprec->value;
		NativeUnPackValue(NULL, LBS_Body(buff), e);
		node->tnest = (int)RecvChar(fp);			ON_IO_ERROR(fp,badio);

		SetValueString(GetItemLongName(e,"dc.term"),hdr.term,NULL);
		SetValueString(GetItemLongName(e,"dc.user"),hdr.user,NULL);
		SetValueString(GetItemLongName(e,"dc.window"),"api",NULL);
		SetValueString(GetItemLongName(e,"api.response_type"),"XML2",NULL);
		SetValueChar(GetItemLongName(e,"private.pstatus"),APL_SESSION_GET);

		strcpy(node->term,hdr.term);
		strcpy(node->user,hdr.user);
		strcpy(node->window,"api");

		arguments = NewLBS();
		headers = NewLBS();
		body = NewLBS();

		RecvnString(fp, sizeof(method), method);	ON_IO_ERROR(fp,badio);
        RecvLBS(fp,arguments);     					ON_IO_ERROR(fp,badio);
        RecvLBS(fp,headers);     					ON_IO_ERROR(fp,badio);
        RecvLBS(fp,body);	     					ON_IO_ERROR(fp,badio);
		PackAPIRecord(node, method, arguments, headers, body);
	
		FreeLBS(arguments);
		FreeLBS(headers);
		FreeLBS(body);
	}
	fSuc = TRUE;
LEAVE_FUNC;
badio:
	return fSuc;
}

extern	Bool
GetWFC(
	NETFILE	*fp,
	ProcessNode	*node)
{
	PacketClass	c;
	Bool		fSuc;

ENTER_FUNC;
	fSuc = FALSE;

	c = RecvPacketClass(fp);
	switch(c){
	case APS_REQ:
		fSuc = GetWFCTerm(fp, node);
		break;
	case APS_API:
		fSuc = GetWFCAPI(fp, node);
		break;
	}
LEAVE_FUNC;
	return	(fSuc); 
}

static	void
PutWFCTerm(
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
	SendString(fp,ValueStringPointer(GetItemLongName(e,"dc.event")));
	ON_IO_ERROR(fp,badio);
	SendChar(fp,node->dbstatus);			ON_IO_ERROR(fp,badio);
	dbgprintf("private.pputtype = %02X",
			  ValueInteger(GetItemLongName(e,"private.pputtype")));
	SendInt(fp,ValueInteger(GetItemLongName(e,"private.pputtype")));
	ON_IO_ERROR(fp,badio);
	fEnd = FALSE; 
	while	(  !fEnd  ) {
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
				dbgprintf("mcp  = %d\n",NativeSizeValue(NULL,node->mcprec->value));
				LBS_ReserveSize(buff,NativeSizeValue(NULL,node->mcprec->value),FALSE);
				NativePackValue(NULL,LBS_Body(buff),node->mcprec->value);
			} else {
				RewindLBS(buff);
			}
			SendLBS(fp,buff);						ON_IO_ERROR(fp,badio);
			SendChar(fp,(char)node->tnest);			ON_IO_ERROR(fp,badio);
			break;
		  case	APS_LINKDATA:
			dbgmsg("LINKDATA");
			if		(  node->linkrec  !=  NULL  ) {
				dbgprintf("link = %d\n",NativeSizeValue(NULL,node->linkrec->value));
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
				dbgprintf("spa  = %d\n",NativeSizeValue(NULL,node->sparec->value));
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
					dbgprintf("scr[%s]  = %d\n",node->scrrec[i]->name,
							  NativeSizeValue(NULL,node->scrrec[i]->value));
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

static	void
PutWFCAPI(
	NETFILE	*fp,
	ProcessNode	*node)
{
	CONVOPT *opt = NULL;
	ConvFuncs *func;
	ValueStruct *value;
	ValueStruct *mcp;
	LargeByteString *headers;
	LargeByteString *body;
	char str[SIZE_BUFF+1];
	char *convfunc;
	char *p;
	int size;
	int i;
	
ENTER_FUNC;
	mcp = node->mcprec->value;
	convfunc = ValueToString(
					GetItemLongName(mcp, "api.response_type"), NULL);
	opt = NewConvOpt();
	ConvSetSize(opt,ThisLD->textsize,ThisLD->arraysize);
	opt->recname = strdup("api");
	opt->codeset = strdup("EUC-JP");

	body = NewLBS();
	headers = NewLBS();
	value = NULL;

	for(i = 0; i < node->cWindow; i++) {
		if (node->scrrec[i] != NULL &&
			!strcmp(node->scrrec[i]->name, "api")) {
			value = node->scrrec[i]->value;
			break;
		}
	}
	if (value != NULL &&
		(value = GetItemLongName(value, "response")) != NULL) {
		if ((func = GetConvFunc(convfunc)) != NULL) {
			size = func->SizeValue(opt, value);
			LBS_ReserveSize(body, size, FALSE);
			p = LBS_Body(body);
			func->PackValue(opt, p, value);
			p[size] = '\0';

			// FIXME; ConvFunc should return mime type. update libmondai
			snprintf(str, sizeof(str),
				"Content-Type: application/xml\r\n");
			LBS_ReserveSize(headers, strlen(str), FALSE);
			p = LBS_Body(headers);
			memcpy(p, str, strlen(str));
		} 
	}
	SendLBS(fp,headers);	ON_IO_ERROR(fp,badio);
	SendLBS(fp,body);		ON_IO_ERROR(fp,badio);

badio:
	DestroyConvOpt(opt);
	FreeLBS(headers);
	FreeLBS(body);
LEAVE_FUNC;
}

extern	void
PutWFC(
	NETFILE	*fp,
	ProcessNode	*node)
{
ENTER_FUNC;
	if (!strcmp(node->window, "api")) {
		PutWFCAPI(fp, node);
	} else {
		PutWFCTerm(fp, node);
	}
LEAVE_FUNC;
}
