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
	Head = NULL;
	Tail = NULL;
	cTerm = 0;
}

static  Bool
SendRecord(
	NETFILE	*fp,
	RecordStruct *rec)
{
	Bool	ret;
	
	if (rec != NULL) {
		dbgprintf("Send rec = [%s] size[%d]\n",
			rec->name, NativeSizeValue(NULL,rec->value));
		LBS_ReserveSize(buff,NativeSizeValue(NULL,rec->value),FALSE);
		NativePackValue(NULL,LBS_Body(buff),rec->value);
		ret = TRUE;
	} else {
		RewindLBS(buff);
		ret = FALSE;
	}
	SendLBS(fp,buff);
	
	return ret;
}

static  Bool
RecvRecord(
	NETFILE	*fp,
	RecordStruct *rec)
{
	Bool		ret;

	RecvLBS(fp,buff);
	if (rec != NULL) {
		dbgprintf("Recv rec = [%s] size[%d]\n",rec->name, LBS_Size(buff));
		NativeUnPackValue(NULL,LBS_Body(buff),rec->value);
#ifdef	DEBUG
		DumpValueStruct(rec->value);
#endif
		ret = TRUE;
	} else {
		ret = FALSE;
	}
	return ret;
}

static	Bool
GetWFCTerm(
	NETFILE	*fp,
	ProcessNode	*node)
{
	MessageHeader	hdr;
	int				i;
	PacketClass		c;
	ValueStruct		*e;

ENTER_FUNC;
	while (1) {
		c = RecvPacketClass(fp);ON_IO_ERROR(fp,badio);
		switch (c) {
		case APS_EVENTDATA:
			dbgmsg("EVENTDATA");
			hdr.command = RecvChar(fp);
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.uuid),hdr.uuid);
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.tenant),hdr.tenant);
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.host),hdr.host);
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.tempdir),hdr.tempdir);
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.user),hdr.user);
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.window),hdr.window);	
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.widget),hdr.widget);
				ON_IO_ERROR(fp,badio);
			RecvnString(fp,sizeof(hdr.event),hdr.event);
				ON_IO_ERROR(fp,badio);
			hdr.dbstatus = RecvChar(fp);
				ON_IO_ERROR(fp,badio);

			e = node->mcprec->value;
			SetValueString(GetItemLongName(e,"dc.term"),hdr.uuid,NULL);
			SetValueString(GetItemLongName(e,"dc.tenant"),hdr.tenant,NULL);
			SetValueString(GetItemLongName(e,"dc.host"),hdr.host,NULL);
			SetValueString(GetItemLongName(e,"dc.tempdir"),hdr.tempdir,NULL);
			SetValueString(GetItemLongName(e,"dc.user"),hdr.user,NULL);
			SetValueString(GetItemLongName(e,"dc.window"),hdr.window,NULL);
			SetValueString(GetItemLongName(e,"dc.widget"),hdr.widget,NULL);
			SetValueString(GetItemLongName(e,"dc.event"),hdr.event,NULL);
			SetValueChar(GetItemLongName(e,"dc.dbstatus"),hdr.dbstatus);

			node->command = hdr.command;
			node->dbstatus = hdr.dbstatus;
			strcpy(node->uuid,hdr.uuid);
			strcpy(node->user,hdr.user);
			strcpy(node->window,hdr.window);
			strcpy(node->widget,hdr.widget);
			strcpy(node->event,hdr.event);
			break;
		case APS_WINDOW_STACK:
			node->w.sp = RecvInt(fp);			ON_IO_ERROR(fp,badio);
			for (i=0;i<node->w.sp ;i++) {
				node->w.s[i].puttype = RecvChar(fp);	ON_IO_ERROR(fp,badio);
				RecvnString(fp,SIZE_NAME,node->w.s[i].window);
					ON_IO_ERROR(fp,badio);
			}
			break;
		case APS_LINKDATA:
			dbgmsg("LINKDATA");
			RecvRecord(fp,node->linkrec);		ON_IO_ERROR(fp,badio);
			break;
		case APS_SPADATA:
			dbgmsg("SPADATA");
			RecvRecord(fp,node->sparec);		ON_IO_ERROR(fp,badio);
			break;
		case APS_SCRDATA:
			dbgmsg("SCRDATA");
			for	( i = 0 ; i < node->cWindow ; i ++ ) {
				RecvRecord(fp,node->scrrec[i]);		ON_IO_ERROR(fp,badio);
			}
			break;
		case APS_END:
			dbgmsg("END");
			return TRUE;
		default:
			Warning("Invalid PacketClass in GetWFCTerm(%02X)",c);
			SendPacketClass(fp,APS_NOT);		ON_IO_ERROR(fp,badio);
badio:
			return FALSE;
		}
	}
LEAVE_FUNC;
	Warning("does not reach");
	return TRUE;
}

static	Bool
GetWFCAPI(
	NETFILE	*fp,
	ProcessNode	*node)
{
ENTER_FUNC;
	ValueStruct		*e;
	WindowBind		*bind;
	int 			i;
	MessageHeader	hdr;

	dbgmsg("API");

	RecvnString(fp,sizeof(hdr.window),hdr.window); 		ON_IO_ERROR(fp,badio);
	dbgprintf("window = [%s]\n",hdr.window);
	bind = g_hash_table_lookup(node->bhash, hdr.window);
	if (bind == NULL || !bind->fAPI) {
		SendChar(fp,APS_NULL);		 					ON_IO_ERROR(fp,badio);
	} else {
		SendChar(fp,APS_OK);		 					ON_IO_ERROR(fp,badio);
		RecvnString(fp, sizeof(hdr.uuid),hdr.uuid); 	
			ON_IO_ERROR(fp,badio);
		RecvnString(fp, sizeof(hdr.tenant),hdr.tenant); 	
			ON_IO_ERROR(fp,badio);
		RecvnString(fp, sizeof(hdr.host),hdr.host); 	
			ON_IO_ERROR(fp,badio);
		RecvnString(fp, sizeof(hdr.tempdir),hdr.tempdir);
			ON_IO_ERROR(fp,badio);
		RecvnString(fp, sizeof(hdr.user), hdr.user); 	
			ON_IO_ERROR(fp,badio);
		e = node->mcprec->value;
		SetValueString(GetItemLongName(e,"dc.term"),hdr.uuid,NULL);
		SetValueString(GetItemLongName(e,"dc.tenant"),hdr.tenant,NULL);
		SetValueString(GetItemLongName(e,"dc.host"),hdr.host,NULL);
		SetValueString(GetItemLongName(e,"dc.tempdir"),hdr.tempdir,NULL);
		SetValueString(GetItemLongName(e,"dc.user"),hdr.user,NULL);
		SetValueString(GetItemLongName(e,"dc.window"),hdr.window,NULL);

		strcpy(node->uuid,hdr.uuid);
		strcpy(node->user,hdr.user);
		strcpy(node->window,hdr.window);
		node->w.sp = 0;

        RecvLBS(fp,buff);								ON_IO_ERROR(fp,badio);

		e = NULL;
		for(i = 0; i < node->cWindow; i++) {
			if (node->scrrec[i] != NULL &&
				!strcmp(node->scrrec[i]->name, hdr.window)) {
				e = node->scrrec[i]->value;
				break;
			}
		}
		if (e == NULL) {
			Error("record [%s] not found", hdr.window);
		}
		NativeUnPackValue(NULL, LBS_Body(buff), e);
	}
LEAVE_FUNC;
	return TRUE;
badio:
LEAVE_FUNC;
	return FALSE;
}

extern	Bool
GetWFC(
	NETFILE	*fp,
	ProcessNode	*node)
{
	PacketClass	c;
	Bool		ret;

ENTER_FUNC;
	ret = FALSE;

	c = RecvPacketClass(fp);
	switch(c){
	case APS_REQ:
		node->messageType = MESSAGE_TYPE_TERM;
		ret = GetWFCTerm(fp,node);
		break;
	case APS_API:
		node->messageType = MESSAGE_TYPE_API;
		ret = GetWFCAPI(fp,node);
		break;
	case APS_CHECK:
		node->messageType = MESSAGE_TYPE_CHECK;
		ret = TRUE;
		break;
	default:
		Warning("Invalid PacketClass in GetWFC(%02X)",c);
		break;
	}
LEAVE_FUNC;
	return ret;
}

static	void
PutWFCTerm(
	NETFILE	*fp,
	ProcessNode	*node)
{
	int			i;
	PacketClass	c;
	ValueStruct	*e;

ENTER_FUNC;
	e = node->mcprec->value;
	SendPacketClass(fp,APS_CTRLDATA);				ON_IO_ERROR(fp,badio);
	SendString(fp,ValueStringPointer(GetItemLongName(e,"dc.window")));
		ON_IO_ERROR(fp,badio);
	SendString(fp,ValueStringPointer(GetItemLongName(e,"dc.widget")));
		ON_IO_ERROR(fp,badio);
	SendString(fp,ValueStringPointer(GetItemLongName(e,"dc.event")));
		ON_IO_ERROR(fp,badio);
	SendChar(fp,node->dbstatus);					ON_IO_ERROR(fp,badio);
	SendChar(fp,node->puttype);						ON_IO_ERROR(fp,badio);

	while (1) {
		c = RecvPacketClass(fp);					ON_IO_ERROR(fp,badio);
		switch(c) {
		case APS_WINDOW_STACK:
			dbgmsg("WINDOW_STACK");
			SendInt(fp,node->w.sp);					ON_IO_ERROR(fp,badio);
			for	(i=0;i<node->w.sp;i++) {
				SendChar(fp,node->w.s[i].puttype);
				SendString(fp,node->w.s[i].window);
			}
			break;
		case APS_LINKDATA:
			dbgmsg("LINKDATA");
			SendRecord(fp, node->linkrec);			ON_IO_ERROR(fp,badio);
			break;
		case APS_SPADATA:
			dbgmsg("SPADATA");
			SendRecord(fp, node->sparec);			ON_IO_ERROR(fp,badio);
			break;
		case APS_SCRDATA:
			dbgmsg("SCRDATA");
			for	(i=0;i<ThisLD->cWindow;i++) {
				SendRecord(fp,node->scrrec[i]);		ON_IO_ERROR(fp,badio);
			}
			break;
		case APS_END:
			dbgmsg("END");
			return;
		default:
			dbgmsg("default");
			SendPacketClass(fp,APS_NOT);			ON_IO_ERROR(fp,badio);
			return;
badio:
			Warning("badio");
			return;
		}
	}
LEAVE_FUNC;
}

static	void
PutWFCAPI(
	NETFILE	*fp,
	ProcessNode	*node)
{
	int i;
ENTER_FUNC;
	for(i = 0; i < node->cWindow; i++) {
		if (node->scrrec[i] != NULL &&
			!strcmp(node->scrrec[i]->name, node->window)) {
			SendRecord(fp, node->scrrec[i]); ON_IO_ERROR(fp,badio);
			break;
		}
	}
badio:
LEAVE_FUNC;
}

extern	void
PutWFC(
	NETFILE	*fp,
	ProcessNode	*node)
{
ENTER_FUNC;
	if (node->messageType == MESSAGE_TYPE_API) {
		PutWFCAPI(fp, node);
	} else {
		PutWFCTerm(fp, node);
	}
LEAVE_FUNC;
}
