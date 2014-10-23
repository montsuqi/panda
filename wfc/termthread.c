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
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>
#include	<time.h>
#include	<errno.h>
#include	<uuid/uuid.h>

#include	"enum.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"lock.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"queue.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"wfc.h"
#include	"glterm.h"
#include	"termthread.h"
#include	"corethread.h"
#include	"sessionthread.h"
#include	"dirs.h"
#include	"message.h"
#include	"debug.h"

extern	void
TermEnqueue(
	TermNode	*term,
	SessionData	*data)
{
	EnQueue(term->que,data);
}

static	SessionData	*
NewSessionData(void)
{
	SessionData	*data;

ENTER_FUNC;
	data = New(SessionData);
	memclear(data,sizeof(SessionData));
	data->type = SESSION_TYPE_TERM;
	data->status = SESSION_STATUS_NORMAL;
	data->hdr = New(MessageHeader);
	memclear(data->hdr,sizeof(MessageHeader));
	data->hdr->command = APL_COMMAND_LINK;
	data->apsid = -1;
	data->spadata = NewNameHash();
	data->scrpool = NewNameHash();
	gettimeofday(&(data->create_time), NULL);
	gettimeofday(&(data->access_time), NULL);
	timerclear(&(data->process_time));
	timerclear(&(data->total_process_time));
	data->apidata = New(APIData);
	data->apidata->status = WFC_API_OK;
	data->apidata->rec = NewLBS();
	data->sysdbval = RecParseValueMem(SYSDBVAL_DEF,NULL);
	InitializeValue(data->sysdbval);
	data->count = 0;
	data->w.sp = 0;
LEAVE_FUNC;
	return	(data);
}

static	guint
FreeSpa(
	char	*name,
	LargeByteString	*spa,
	void		*dummy)
{
	if (name != NULL) {
		xfree(name);
	}
	if (spa != NULL) {
		FreeLBS(spa);
	}
	return	TRUE;
}

static	guint
FreeScr(
	char	*name,
	LargeByteString	*scr,
	void		*dummy)
{
	if (name != NULL) {
		xfree(name);
	}
	if (scr != NULL) {
		FreeLBS(scr);
	}
	return	TRUE;
}

static	void
FreeSessionData(
	SessionData	*data)
{
ENTER_FUNC;
	if (data->type != SESSION_TYPE_API) {
		MessageLogPrintf("[%s@%s] session end",data->hdr->user,data->hdr->uuid);
	}
	if (data->linkdata != NULL) {
		FreeLBS(data->linkdata);
	}
	xfree(data->hdr);
	g_hash_table_foreach_remove(data->spadata,(GHRFunc)FreeSpa,NULL);
	DestroyHashTable(data->spadata);
	g_hash_table_foreach_remove(data->scrpool,(GHRFunc)FreeScr,NULL);
	DestroyHashTable(data->scrpool);
	xfree(data->scrdata);
	FreeLBS(data->apidata->rec);
	xfree(data->apidata);
	FreeValueStruct(data->sysdbval);
	xfree(data);
LEAVE_FUNC;
}

static LargeByteString	*
NewLinkData(void)
{
	LargeByteString *linkdata;
	size_t size;

	if (ThisEnv->linkrec != NULL) {
		linkdata = NewLBS();
		size = NativeSizeValue(NULL,ThisEnv->linkrec->value);
		LBS_ReserveSize(linkdata,size,FALSE);
		NativePackValue(NULL,LBS_Body(linkdata),ThisEnv->linkrec->value);
	} else {
		linkdata = NULL;
	}
	return linkdata;
}

static	void
RegisterSession(
	SessionData	*data)
{
	SessionCtrl *ctrl;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_INSERT);
	ctrl->session = data;
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
}

static	SessionData	*
LookupSession(
	char	*term)
{
	SessionData *data;
	SessionCtrl *ctrl;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_LOOKUP);
	strcpy(ctrl->id,term);
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	data = ctrl->session;
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
	return	(data);
}

static	void
DeregisterSession(
	SessionData *data)
{
	SessionCtrl *ctrl;
ENTER_FUNC;
#if 0
	if (!getenv("WFC_KEEP_TEMPDIR")) {
		if (!rm_r(data->hdr->tempdir)) {
			Error("cannot remove session tempdir %s",data->hdr->tempdir);
		}
	}
#endif
	ctrl = NewSessionCtrl(SESSION_CONTROL_DELETE);
	ctrl->session = data;
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	FreeSessionCtrl(ctrl);
	FreeSessionData(data);
LEAVE_FUNC;
}

static	void
UpdateSession(
	SessionData *data)
{
	SessionCtrl *ctrl;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_UPDATE);
	ctrl->session = data;
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
}

static unsigned int
GetSessionNum()
{
	SessionCtrl *ctrl;
	unsigned int size;
ENTER_FUNC;
	ctrl = NewSessionCtrl(SESSION_CONTROL_GET_SESSION_NUM);
	SessionEnqueue(ctrl);
	ctrl = (SessionCtrl*)DeQueue(ctrl->waitq);
	size = ctrl->size;
	FreeSessionCtrl(ctrl);
LEAVE_FUNC;
	return size;
}

static	SessionData	*
InitAPISession(
	char *user,
	char *ldname,
	char *wname,
	char *host)
{
	SessionData		*data;
	LD_Node			*ld;
	RecordStruct	*rec;
	size_t			size;
	uuid_t			u;

ENTER_FUNC;
	data = NewSessionData();
	data->type = SESSION_TYPE_API;
	uuid_generate(u);
	uuid_unparse(u,data->hdr->uuid);
	strcpy(data->hdr->window,wname);
	strcpy(data->hdr->user,user);
	strcpy(data->host,host);
	data->fInProcess = TRUE;
	if ((ld = g_hash_table_lookup(APS_Hash, ldname)) != NULL) {
		data->ld = ld;
		data->linkdata = NULL;
		data->cWindow = ld->info->cWindow;
		data->scrdata = NULL;
		data->hdr->puttype = SCREEN_NULL;
		rec = GetWindow(wname);
		size = NativeSizeValue(NULL,rec->value);
		LBS_ReserveSize(data->apidata->rec, size,FALSE);
		NativePackValue(NULL,LBS_Body(data->apidata->rec),rec->value);
	} else {
		Warning("[%s] session fail LD [%s] not found.",data->hdr->uuid,ldname);
		data = NULL;
	}
LEAVE_FUNC;
	return	(data);
}


static	SessionData	*
ReadTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	LD_Node			*ld;
	Bool			fExit;
	int				c;
	LargeByteString	*scrdata;
	int				i;
ENTER_FUNC;
	fExit = FALSE;
	while (!fExit) {
		switch (c = RecvPacketClass(fp)) {
		case WFC_DATA:
			dbgmsg("recv DATA");
			if (data != NULL) {
				RecvnString(fp,SIZE_NAME,data->hdr->window);
					ON_IO_ERROR(fp,badio);
				RecvnString(fp,SIZE_NAME,data->hdr->widget);
					ON_IO_ERROR(fp,badio);
				RecvnString(fp,SIZE_NAME,data->hdr->event);
					ON_IO_ERROR(fp,badio);

				data->w.sp = RecvInt(fp);					
					ON_IO_ERROR(fp,badio);
				for (i=0;i<data->w.sp ;i++) {
					data->w.s[i].puttype = RecvChar(fp);
						ON_IO_ERROR(fp,badio);
					RecvnString(fp,SIZE_NAME,data->w.s[i].window);
						ON_IO_ERROR(fp,badio);
				}

				dbgprintf("window = [%s]",data->hdr->window);
				dbgprintf("widget = [%s]",data->hdr->widget);
				dbgprintf("event  = [%s]",data->hdr->event);
				ld = g_hash_table_lookup(ComponentHash,data->hdr->window);
				if (ld != NULL) {
					dbgprintf("ld = [%s]",ld->info->name);
					dbgprintf("window = [%s]",data->hdr->window);
					scrdata = GetScreenData(data,data->hdr->window);
					if (scrdata != NULL) {
						SendPacketClass(fp,WFC_OK);	ON_IO_ERROR(fp,badio);
						dbgmsg("send OK");
						if (RecvPacketClass(fp) == WFC_DATA) {
							RecvLBS(fp,scrdata);ON_IO_ERROR(fp,badio);
						}
						data->hdr->puttype = SCREEN_NULL;
					} else {
						Error("invalid window [%s]",data->hdr->window);
					}
					if (data->ld != ld) {
						ChangeLD(data,ld);
					}
				} else {
					Error("component [%s] not found.",data->hdr->window);
					fExit = TRUE;
				}
			} else {
				fExit = TRUE;
			}
			break;
		case WFC_OK:
			dbgmsg("OK");
			fExit = TRUE;
			break;
		case WFC_END:
			dbgmsg("END");
			if ((ld = g_hash_table_lookup(APS_Hash, "sessionend"))  !=  NULL) {
				data->hdr->window[0] = 0;
				data->hdr->widget[0] = 0;
				sprintf(data->hdr->event,"SESSIONEND");
				data->hdr->puttype = SCREEN_NULL;
				ChangeLD(data,ld);
				data->status = SESSION_STATUS_END;
			} else {
				data->status = SESSION_STATUS_ABORT;
			}
			fExit = TRUE;
			break;
		default:
			Warning("Invalid PacketClass in ReadTerminal [%X]", c);
			SendPacketClass(fp,WFC_NOT);ON_IO_ERROR(fp,badio);
			fExit = TRUE;
			data->status = SESSION_STATUS_ABORT;
			break;
		}
	}
badio:
LEAVE_FUNC;
	return(data);
}

static	Bool
SendTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	unsigned char	c;
	char			wname[SIZE_LONGNAME+1];
	LargeByteString	*scrdata;
	int 			i;

ENTER_FUNC;
	SendPacketClass(fp,WFC_HEADER);		ON_IO_ERROR(fp,badio);
	dbgmsg("send DATA");
	SendString(fp,data->hdr->user);		ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->window);	ON_IO_ERROR(fp,badio);
	SendString(fp,data->hdr->widget);	ON_IO_ERROR(fp,badio);
	SendChar  (fp,data->hdr->puttype);	ON_IO_ERROR(fp,badio);
	dbgprintf("window    = [%s]",data->hdr->window);
	SendInt(fp,data->w.sp);				ON_IO_ERROR(fp,badio);
	for (i=0;i<data->w.sp;i++) {
		SendChar(fp,data->w.s[i].puttype);ON_IO_ERROR(fp,badio);
		SendString(fp,data->w.s[i].window);ON_IO_ERROR(fp,badio);
	}
	while (1) {
		c = RecvPacketClass(fp);		ON_IO_ERROR(fp,badio);
		switch (c) {
		case WFC_DATA:
			dbgmsg(">DATA");
			RecvnString(fp,SIZE_LONGNAME,wname);ON_IO_ERROR(fp,badio);
			if ((scrdata = GetScreenData(data,wname)) != NULL) {
				dbgmsg("send OK");
				SendPacketClass(fp,WFC_OK);	ON_IO_ERROR(fp,badio);
				SendLBS(fp,scrdata);		ON_IO_ERROR(fp,badio);
			} else {
				dbgmsg("send NODATA");
				SendPacketClass(fp,WFC_NODATA);	ON_IO_ERROR(fp,badio);
			}
			dbgmsg("<DATA");
			break;
		case WFC_DONE:
			dbgmsg("DONE");
			return TRUE;
		case WFC_END:
			dbgmsg("END");
			return FALSE;
		default:
			Warning("[%s] session failure packet [%X]",data->hdr->uuid,c);
			dbgprintf("c = [%X]\n",c);
			return FALSE;
		}
	}
	Warning("does not reach");
LEAVE_FUNC;
	return FALSE;
badio:
	Warning("[%s] session recv failure",data->hdr->uuid);
LEAVE_FUNC;
	return FALSE;
}

static	SessionData	*
Process(
	SessionData	*data)
{
	struct	timeval	tv1;
	struct	timeval	tv2;
ENTER_FUNC;
	gettimeofday(&tv1,NULL);
	CoreEnqueue(data);
	data = DeQueue(data->term->que);
	data->count += 1;
	gettimeofday(&tv2,NULL);
	timersub(&tv2, &tv1, &(data->process_time));
	timeradd(&(data->total_process_time), &(data->process_time), &tv1);
	data->total_process_time = tv1;
LEAVE_FUNC;
	return	(data);
}

static	SessionData	*
CheckSession(
	NETFILE	*fp,
	char	*term)
{
	SessionData	*data;
ENTER_FUNC;
	if ((data = LookupSession(term)) != NULL) {
		if (!data->fInProcess) {
			SendPacketClass(fp,WFC_TRUE);			ON_IO_ERROR(fp,badio);
			data->hdr->command = APL_COMMAND_GET;
			gettimeofday(&data->access_time,NULL);
		} else {
			Warning("Error: Other threads are processing it.");
			SendPacketClass(fp,WFC_FALSE);			ON_IO_ERROR(fp,badio);
			data = NULL;
		}
	} else {
		Warning("session [%s] does not found",term);
	}
LEAVE_FUNC;
	return data;
badio:
LEAVE_FUNC;
	Warning("CheckSession failure: io error");
	return NULL;
}

static	void
KeepSession(
	SessionData	*data)
{
ENTER_FUNC;
	data->fInProcess = FALSE;
LEAVE_FUNC;
}

static	void
TermMain(
	TermNode	*term,
	SessionData	*data)
{
	if (data != NULL) {
		if (data->status != SESSION_STATUS_ABORT) {
			UpdateSession(data);
			data = Process(data);
		}
		if (data->status != SESSION_STATUS_NORMAL) {
			DeregisterSession(data);
		} else {
			if (SendTerminal(term->fp,data)) {
				KeepSession(data);
				UpdateSession(data);
			} else {
				DeregisterSession(data);
			}
		}
	}
	SendPacketClass(term->fp,WFC_DONE);
}

static	void
TermInit(
	TermNode	*term)
{
	SessionData	*data;
	LD_Node		*ld;
	int			i,sesnum;
	uuid_t		u;
ENTER_FUNC;
	sesnum = GetSessionNum();
	if (SesNum != 0 && sesnum >= SesNum) {
		Warning("Discard new session;max session number(%d) reached", SesNum);
		SendPacketClass(term->fp,WFC_NOT);
		CloseNet(term->fp);
		return;
	}

	data = NewSessionData();
	data->term = term;
	data->fInProcess = TRUE;
	uuid_generate(u);
	uuid_unparse(u,data->hdr->uuid);

	if ((ld = g_hash_table_lookup(APS_Hash,ThisEnv->InitialLD)) == NULL) {
		Error("cannot find initial ld:%s.check directory",ThisEnv->InitialLD);
	}

	RecvnString(term->fp,SIZE_NAME,data->hdr->user);ON_IO_ERROR(term->fp,badio);
	RecvnString(term->fp,SIZE_HOST,data->host);	ON_IO_ERROR(term->fp,badio);
	RecvnString(term->fp,SIZE_NAME,data->agent);ON_IO_ERROR(term->fp,badio);

	MessageLogPrintf("[%s@%s] session start(%d)",
		data->hdr->user,data->hdr->uuid,sesnum+1);
	dbgprintf("uuid   = [%s]",data->hdr->uuid);
	dbgprintf("user   = [%s]",data->hdr->user);
	dbgprintf("host   = [%s]",data->host);
	dbgprintf("agent  = [%s]",data->agent);

	SendPacketClass(term->fp,WFC_OK);	ON_IO_ERROR(term->fp,badio);
	SendString(term->fp,data->hdr->uuid);	ON_IO_ERROR(term->fp,badio);

	data->ld = ld;
	data->linkdata = NewLinkData();
	data->cWindow = ld->info->cWindow;
	data->scrdata = 
		(LargeByteString **)xmalloc(sizeof(void*)*data->cWindow);
	for	(i = 0 ; i < data->cWindow ; i ++) {
		if (data->ld->info->windows[i] != NULL) {
			dbgprintf("[%s]",data->ld->info->windows[i]->name);
			data->scrdata[i] = 
				GetScreenData(data,data->ld->info->windows[i]->name);
		} else {
			data->scrdata[i] = NULL;
		}
	}
	data->hdr->puttype = SCREEN_NULL;
	RegisterSession(data);
	TermMain(term,data);
	CloseNet(term->fp);
badio:
LEAVE_FUNC;
	return;
}

static	void
TermSession(
	TermNode	*term)
{
	SessionData	*data;
	char		buff[SIZE_TERM+1];
	
	RecvnString(term->fp,SIZE_TERM,buff);
	data = CheckSession(term->fp,buff);
	if (data != NULL) {
		data = ReadTerminal(term->fp,data);
		data->term = term;
		data->retry = 0;
		TermMain(term,data);
	}
	CloseNet(term->fp);
}

static	void
APISession(
	TermNode	*term)
{
	SessionData *data;
	APIData *api;
	char ld[SIZE_NAME+1];
	char window[SIZE_NAME+1];
	char user[SIZE_USER+1];
	char host[SIZE_HOST+1];

	data = NULL;
	RecvnString(term->fp, sizeof(ld), ld);			
		ON_IO_ERROR(term->fp,badio);
	RecvnString(term->fp, sizeof(window), window);			
		ON_IO_ERROR(term->fp,badio);
	RecvnString(term->fp, sizeof(user), user);		
		ON_IO_ERROR(term->fp,badio);
	RecvnString(term->fp, sizeof(host), host);	
		ON_IO_ERROR(term->fp,badio);

	data = InitAPISession(user,ld,window,host);
	if (data != NULL) {
		data->term = term;
		data->retry = 0;
		api = data->apidata;
		RecvLBS(term->fp, api->rec);

		data = Process(data);
		api = data->apidata;

		SendPacketClass(term->fp, api->status);
			ON_IO_ERROR(term->fp,badio2);
		if (api->status == WFC_API_OK) {
			SendLBS(term->fp, api->rec);
				ON_IO_ERROR(term->fp,badio2);
		}
		CloseNet(term->fp);
	badio2:
		FreeSessionData(data);
	} else {
		SendPacketClass(term->fp,WFC_API_NOT_FOUND);
			ON_IO_ERROR(term->fp,badio2);
		CloseNet(term->fp);
	}
	badio:
		;
}

static	void
TermThread(
	TermNode	*term)
{
	PacketClass klass;

ENTER_FUNC;
	klass = RecvPacketClass(term->fp);
	switch (klass) {
	case WFC_TERM_INIT:
		TermInit(term);
		break;
	case WFC_TERM:
		TermSession(term);
		break;
	case WFC_API:
		APISession(term);
		break;
	}
	FreeQueue(term->que);
	xfree(term);
LEAVE_FUNC;
}

extern	int
ConnectTerm(
	int		_fhTerm)
{
	int				fhTerm;
	pthread_t		thr;
	pthread_attr_t	attr;
	TermNode		*term;

ENTER_FUNC;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,256*1024);
	if ((fhTerm = accept(_fhTerm,0,0)) < 0) {
		Error("accept(2) failure:%s",strerror(errno));
	}
	term = New(TermNode);
	term->que = NewQueue();
	term->fp = SocketToNet(fhTerm);
	pthread_create(&thr,&attr,(void *(*)(void *))TermThread,(void *)term);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(fhTerm); 
}

extern	void
InitTerm(void)
{
ENTER_FUNC;
	RecParserInit();
	if (ThisEnv->linkrec != NULL) {
		InitializeValue(ThisEnv->linkrec->value);
	}
LEAVE_FUNC;
}
