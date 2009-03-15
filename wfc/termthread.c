/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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
#include	<time.h>
#include	<pthread.h>
#ifdef	HAVE_UUID
#include	<uuid.h>
#endif

#include	"types.h"
#include	"enum.h"

#include	"libmondai.h"
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
#include	"termthread.h"
#include	"corethread.h"
#include	"blob.h"
#include	"blobcom.h"
#include	"blobserv.h"
#include	"driver.h"
#include	"dirs.h"
#include	"message.h"
#include	"debug.h"

static	struct {
	LOCKOBJECT;
	GHashTable	*Hash;
}	Terminal;
	
static	int			cTerm;
static	Queue		*RemoveQueue;

static	SessionData	*
MakeSessionData(void)
{
	SessionData	*data;
	int		i;

ENTER_FUNC;
	data = New(SessionData);
	memclear(data,sizeof(SessionData));
	data->type = SESSION_TYPE_TERM;
	data->mtype = MESSAGE_TYPE_EVENT;
	data->hdr = New(MessageHeader);
	data->name = NULL;
	memclear(data->hdr,sizeof(MessageHeader));
	data->fAbort = FALSE;
	data->apsid = -1;
	data->spadata = NewNameHash();
	data->scrpool = NewNameHash();
	gettimeofday(&data->create_time,NULL);
	gettimeofday(&data->access_time,NULL);
	data->apidata = New(APIData);
	data->apidata->arguments = NewLBS();
	data->apidata->headers = NewLBS();
	data->apidata->body = NewLBS();
	data->dbstatus = (int *)xmalloc(sizeof(int) * ApsId);
	for	( i = 0 ; i < ApsId ; i ++ ) {
		data->dbstatus[i] = DB_STATUS_NOCONNECT;
	}
	InitLock(data);
LEAVE_FUNC;
	return	(data);
}

static	void
_UnrefSession(
	SessionData	*data)
{
	if		(	(  data->name  !=  NULL  )
			&&	(  g_hash_table_lookup(Terminal.Hash,data->name)  !=  NULL  ) ) {
		cTerm --;
		g_hash_table_remove(Terminal.Hash,data->name);
	}
	EnQueue(RemoveQueue,data);
}

static void
CheckAccessTime(
	char *name,
	SessionData *data,
	SessionData **candidate)
{
	if (*candidate == NULL) {
		*candidate = data;
	} else {
		if		(  timercmp(&(*candidate)->access_time,&data->access_time, > )  ) {
			*candidate = data;
		}	
	}
}

static	void
DiscardSession()
{
	SessionData *data;

	data = NULL;
	g_hash_table_foreach(Terminal.Hash, 
		(GHFunc)CheckAccessTime, (gpointer)(&data));
	if (data != NULL) {
		Message("[%s@%s] discard session; nCache size over", 
			data->hdr->user, data->hdr->term);
		_UnrefSession(data);
	}
}

static	Bool
RegistSession(
	SessionData	*data)
{
	Bool	rc;

	LockWrite(&Terminal);
	if		(  g_hash_table_lookup(Terminal.Hash,data->name)  ==  NULL  ) {
		if (nCache != 0 && cTerm >= nCache) {
			DiscardSession();
		}
		cTerm ++;
		g_hash_table_insert(Terminal.Hash,data->name,data);
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	UnLock(&Terminal);

	return	(rc);
}

static	SessionData	*
LookupSession(
	char	*term)
{
	SessionData	*data;

ENTER_FUNC;
	LockRead(&Terminal);
	data = g_hash_table_lookup(Terminal.Hash,term);
	UnLock(&Terminal);
LEAVE_FUNC;
	return	(data);
}

static	void
FinishSession(
	SessionData	*data)
{
	char	fname[SIZE_LONGNAME+1];

ENTER_FUNC;
	dbgprintf("unref name = [%s]\n",data->name);
	if		(  SesDir  !=  NULL  ) {
		sprintf(fname,"%s/%s.ses",SesDir,data->name);
		remove(fname);
	}
	LockWrite(&Terminal);
	_UnrefSession(data);
	UnLock(&Terminal);
LEAVE_FUNC;
}

static	guint
FreeSpa(
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
	return	(TRUE);
}

static	guint
FreeScr(
	char	*name,
	LargeByteString	*scr,
	void		*dummy)
{
	if		(  name  !=  NULL  ) {
		xfree(name);
	}
	if		(  scr  !=  NULL  ) {
		FreeLBS(scr);
	}
	return	(TRUE);
}

static	void
FreeSession(
	SessionData	*data)
{
	char	msg[SIZE_LONGNAME+1];

ENTER_FUNC;
	DestroyLock(data);
	snprintf(msg,SIZE_LONGNAME,"[%s@%s] session end",data->hdr->user,data->hdr->term);
	MessageLog(msg);
	xfree(data->name);
	xfree(data->hdr);
	if		(  data->mcpdata  !=  NULL  ) {
		FreeLBS(data->mcpdata);
	}
	if		(  data->linkdata  !=  NULL  ) {
		FreeLBS(data->linkdata);
	}
	g_hash_table_foreach_remove(data->spadata,(GHRFunc)FreeSpa,NULL);
	g_hash_table_destroy(data->spadata);
	g_hash_table_foreach_remove(data->scrpool,(GHRFunc)FreeScr,NULL);
	g_hash_table_destroy(data->scrpool);
	xfree(data->scrdata);
	FreeLBS(data->apidata->arguments);
	FreeLBS(data->apidata->headers);
	FreeLBS(data->apidata->body);
	xfree(data->apidata);
	xfree(data->dbstatus);
	xfree(data);
LEAVE_FUNC;
}

static	SessionData	*
InitAPISession(
	char *term,
	Bool fKeep,
	char *hdr_user,
	char *ldname)
{
	SessionData	*data;
	LD_Node		*ld;

ENTER_FUNC;
	data = MakeSessionData();
	data->type = SESSION_TYPE_API;
	strcpy(data->hdr->term,term);
	data->fKeep = fKeep;
	MessageLogPrintf("[%s@%s] session start(%d)",
					 data->hdr->user, data->hdr->term, cTerm+1);
	if		(  ( ld = g_hash_table_lookup(APS_Hash, ldname) )  !=  NULL  ) {
		data->ld = ld;
		if		(  ThisEnv->mcprec  !=  NULL  ) {
			data->mcpdata = NewLBS();
			LBS_ReserveSize(data->mcpdata,NativeSizeValue(NULL,ThisEnv->mcprec->value),FALSE);
			NativePackValue(NULL,LBS_Body(data->mcpdata),ThisEnv->mcprec->value);
		}
		data->linkdata = NULL;
		data->cWindow = ld->info->cWindow;
		data->scrdata = NULL;
		data->name = StrDup(data->hdr->term);
		data->hdr->puttype = SCREEN_NULL;
		data->w.n = 0;
	} else {
		Warning("[%s] session fail LD [%s] not found.",data->hdr->term,ldname);
		data = NULL;
	}
LEAVE_FUNC;
	return	(data);
}

static	SessionData	*
CreateSession(
	NETFILE	*fp,
	char	*term)
{
	SessionData	*data;

ENTER_FUNC;
	data = MakeSessionData();
	data->fKeep = ( RecvPacketClass(fp) == WFC_TRUE ) ? TRUE : FALSE;
#ifdef	HAVE_UUID
	if		(  *term  ==  0  ) {
		uuid_t	*uu;
		size_t	len;

		uuid_create(&uu);
		uuid_make(uu, UUID_MAKE_V1);
		len = SIZE_TERM+1;
		uuid_export(uu, UUID_FMT_STR, (void **)&term, &len );
		uuid_destroy(uu);
	}
#endif
	strcpy(data->hdr->term,term);
	RecvnString(fp,SIZE_NAME,data->hdr->user);		ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,data->hdr->lang);		ON_IO_ERROR(fp,badio);
	MessageLogPrintf("[%s@%s] session start(%d)",
		data->hdr->user,data->hdr->term, cTerm+1);
	data->hdr->status = TO_CHAR(APL_SESSION_LINK);
	dbgprintf("term = [%s]",data->hdr->term);
	dbgprintf("user = [%s]",data->hdr->user);
	dbgprintf("lang = [%s]",data->hdr->lang);
	if		(  CheckNetFile(fp)  ) {
		data->name = StrDup(data->hdr->term);
		data->hdr->puttype = SCREEN_NULL;
		data->w.n = 0;
		if		(  RegistSession(data)  ) {
			SendPacketClass(fp,WFC_TRUE);
			SendString(fp,data->hdr->term);					ON_IO_ERROR(fp,badio);
		} else {
			SendPacketClass(fp,WFC_FALSE);
			FinishSession(data);
			data = NULL;
		}
	} else {
	  badio:;
		FinishSession(data);
		data = NULL;
	}
LEAVE_FUNC;
	return	(data);
}

static	SessionData	*
SwitchLD(
	NETFILE	*fp,
	SessionData	*data,
	char	*ldname)
{
	LD_Node	*ld;
	int			i;

ENTER_FUNC;
	dbgprintf("LD = [%s]",ldname);
	data->hdr->status = TO_CHAR(APL_SESSION_LINK);
	*data->hdr->window = 0;
	*data->hdr->widget = 0;
	*data->hdr->event = 0;
	data->apsid = -1;
	if		(  ( ld = g_hash_table_lookup(APS_Hash,ldname) )  !=  NULL  ) {
		SendPacketClass(fp,WFC_TRUE);					ON_IO_ERROR(fp,badio);
		data->ld = ld;
		if		(  ThisEnv->mcprec  !=  NULL  ) {
			data->mcpdata = NewLBS();
			LBS_ReserveSize(data->mcpdata,NativeSizeValue(NULL,ThisEnv->mcprec->value),FALSE);
			NativePackValue(NULL,LBS_Body(data->mcpdata),ThisEnv->mcprec->value);
		}
		if		(  ThisEnv->linkrec  !=  NULL  ) {
			data->linkdata = NewLBS();
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
	} else {
		Warning("[%s] session fail LD [%s] not found.",data->hdr->term,ldname);
	  badio:
		SendPacketClass(fp,WFC_FALSE);
		FinishSession(data);
		data = NULL;
	}
LEAVE_FUNC;
	return	(data);
}

static	Bool
ReadTerminalHeader(
	NETFILE		*fp,
	SessionData	*data)
{
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	RecvnString(fp,SIZE_NAME,data->hdr->lang);		ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,data->hdr->window);	ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,data->hdr->widget);	ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,data->hdr->event);		ON_IO_ERROR(fp,badio);
	rc = TRUE;
  badio:;
	dbgprintf("lang   = [%s]",data->hdr->lang);
	dbgprintf("widget = [%s]",data->hdr->window);
	dbgprintf("widget = [%s]",data->hdr->widget);
	dbgprintf("event  = [%s]",data->hdr->event);
LEAVE_FUNC;
	return	(rc);
}

static	Bool
ReadTerminalBody(
	NETFILE		*fp,
	SessionData	*data)
{
	Bool	rc;
	char		window[SIZE_LONGNAME+1]
		,		comp[SIZE_LONGNAME+1];
	LD_Node		*ld;
	LargeByteString	*scrdata;

ENTER_FUNC;
	rc = FALSE;
	RecvnString(fp,SIZE_NAME,data->hdr->window);	ON_IO_ERROR(fp,badio);
	dbgprintf("window = [%s]",data->hdr->window);
	PureComponentName(data->hdr->window,comp);
	if		(  ( ld = g_hash_table_lookup(ComponentHash,comp) )  !=  NULL  ) {
		dbgprintf("ld = [%s]",ld->info->name);
		PureWindowName(data->hdr->window,window);
		dbgprintf("window = [%s]",window);
		if		(  ( scrdata = GetScreenData(data,window) )  !=  NULL  ) {
			SendPacketClass(fp,WFC_TRUE);				ON_IO_ERROR(fp,badio);
			dbgmsg("send OK");
			switch	(RecvPacketClass(fp))	{
			  case	WFC_DATA:
				RecvLBS(fp,scrdata);					ON_IO_ERROR(fp,badio);
				break;
			  case	WFC_NODATA:
				break;
			  default:
				Error("protocol exception [%s]",window);
				break;
			}
			data->hdr->rc = TO_CHAR(0);
			data->hdr->puttype = SCREEN_NULL;
		} else {
			SendPacketClass(fp,WFC_FALSE);				ON_IO_ERROR(fp,badio);
			Error("invalid window [%s]",window);
		}
		if		(  data->ld  !=  ld  ) {
			ChangeLD(data,ld);
		}
		rc = TRUE;
	} else {
		Error("component [%s] not found.",data->hdr->window);
	}
  badio:;
LEAVE_FUNC;
	return	(rc);
}

static	Bool
SendHeader(
	NETFILE		*fp,
	SessionData	*data)
{
	MessageHeader	*hdr;
	int			i;
	Bool		rc;

ENTER_FUNC;
	rc = FALSE;
	SendPacketClass(fp,WFC_HEADER);		ON_IO_ERROR(fp,badio);
	hdr = data->hdr;
	SendString(fp,hdr->user);			ON_IO_ERROR(fp,badio);
	SendString(fp,hdr->lang);			ON_IO_ERROR(fp,badio);
	SendString(fp,hdr->window);			ON_IO_ERROR(fp,badio);
	SendString(fp,hdr->widget);			ON_IO_ERROR(fp,badio);
	SendChar(fp,hdr->puttype);			ON_IO_ERROR(fp,badio);
	SendInt(fp,data->w.n);				ON_IO_ERROR(fp,badio);
	dbgprintf("window    = [%s]",hdr->window);
	dbgprintf("puttype   = [%02X]",hdr->puttype);
	dbgprintf("data->w.n = %d",data->w.n);
	for	( i = 0 ; i < data->w.n ; i ++ ) {
		SendInt(fp,data->w.control[i].PutType);			ON_IO_ERROR(fp,badio);
		SendString(fp,data->w.control[i].window);		ON_IO_ERROR(fp,badio);
		dbgprintf("*window    = [%s]",data->w.control[i].window);
		dbgprintf("*puttype   = [%02X]",data->w.control[i].PutType);
	}
	data->w.n = 0;
	rc = TRUE;
  badio:;
LEAVE_FUNC;
	return	(rc); 
}

#define	RECORD_SPA		1
#define	RECORD_SCRPOOL	2
#define	RECORD_SCRDATA	3

static	SessionData	*
LoadSession(
	char	*term)
{
	FILE		*fp;
	SessionData	*data;
	char	fname[SIZE_LONGNAME+1]
		,	name[SIZE_LONGNAME+1];
	size_t	size;
	LargeByteString	*lbs;
	LD_Node	*ld;
	int		flag;

ENTER_FUNC;
	if		(  SesDir  ==  NULL  ) {
		data = NULL;
	} else {
		sprintf(fname,"%s/%s.ses",SesDir,term);
		if		(  ( fp = fopen(fname,"r") )  !=  NULL  ) {
			data = MakeSessionData();
			strcpy(data->hdr->term,term);
			fread(&data->fKeep,sizeof(Bool),1,fp);
			fread(data->hdr,sizeof(MessageHeader),1,fp);
			fread(&size,sizeof(size_t),1,fp);	/*	ld		*/
			fread(name,size,1,fp);
			ld = g_hash_table_lookup(APS_Hash,name);
			fread(&data->w,sizeof(WindowControl),1,fp);	/*	w		*/
			while	(  ( flag = fgetc(fp) )  ==  RECORD_SPA  ) {
				fread(&size,sizeof(size_t),1,fp);	/*	spa name	*/
				fread(name,size,1,fp);
				lbs = NewLBS();
				fread(&size,sizeof(size_t),1,fp);	/*	spa data	*/
				LBS_ReserveSize(lbs,size,FALSE);
				fread(LBS_Body(lbs),size,1,fp);
				dbgprintf("spa name = [%s]",name);
				if		(  data->spadata  !=  NULL  ) {
					g_hash_table_insert(data->spadata,StrDup(name),lbs);
				}
			}
			ungetc(flag,fp);
			data->mcpdata = NewLBS();
			fread(&size,sizeof(size_t),1,fp);	/*	mcp data	*/
			LBS_ReserveSize(data->mcpdata,size,FALSE);
			fread(LBS_Body(data->mcpdata),size,1,fp);
			fread(&size,sizeof(size_t),1,fp);	/*	link data	*/
			if		(  size  >  0  ) {
				data->linkdata = NewLBS();
				LBS_ReserveSize(data->linkdata,size,FALSE);
				fread(LBS_Body(data->linkdata),size,1,fp);
			} else {
				data->linkdata = NULL;
			}
			while	(  ( flag = fgetc(fp) )  ==  RECORD_SCRPOOL  ) {
				fread(&size,sizeof(size_t),1,fp);	/*	screen name	*/
				fread(name,size,1,fp);
				lbs = NewLBS();
				fread(&size,sizeof(size_t),1,fp);	/*	scr data	*/
				LBS_ReserveSize(lbs,size,FALSE);
				fread(LBS_Body(lbs),size,1,fp);
				dbgprintf("scr name = [%s]",name);
				if		(  data->scrpool  !=  NULL  ) {
					g_hash_table_insert(data->scrpool,StrDup(name),lbs);
				}
			}
			ungetc(flag,fp);
			ChangeLD(data,ld);
			data->name = StrDup(data->hdr->term);
			RegistSession(data);
			fclose(fp);
		} else {
			data = NULL;
		}
	}
LEAVE_FUNC;
	return	(data);
}

static	void
_SaveSpa(
	char	*name,
	LargeByteString	*lbs,
	FILE	*fp)
{
	size_t	size;

	dbgprintf("[%s] size = %d\n",name,LBS_Size(lbs));
	fputc(RECORD_SPA,fp);
	size = strlen(name) + 1;
	fwrite(&size,sizeof(size_t),1,fp);	/*	spa name	*/
	fwrite(name,size,1,fp);
	size = LBS_Size(lbs);
	fwrite(&size,sizeof(size_t),1,fp);	/*	spa data	*/
	fwrite(LBS_Body(lbs),size,1,fp);
}

static	void
_SaveScrpool(
	char	*name,
	LargeByteString	*lbs,
	FILE	*fp)
{
	size_t	size;

	dbgprintf("[%s] size = %d",name,LBS_Size(lbs));
	fputc(RECORD_SCRPOOL,fp);
	size = strlen(name) + 1;
	fwrite(&size,sizeof(size_t),1,fp);	/*	spa name	*/
	fwrite(name,size,1,fp);
	size = LBS_Size(lbs);
	fwrite(&size,sizeof(size_t),1,fp);	/*	spa data	*/
	fwrite(LBS_Body(lbs),size,1,fp);
}

static	void
SaveSession(
	SessionData	*data)
{
	char	fname[SIZE_LONGNAME+1];
	FILE	*fp;
	size_t	size;

ENTER_FUNC;
	if		(  SesDir  !=  NULL  ) {
		sprintf(fname,"%s/%s.ses",SesDir,data->name);
		if		(  ( fp = Fopen(fname,"w") )  !=  NULL  ) {
			fwrite(&data->fKeep,sizeof(Bool),1,fp);
			fwrite(data->hdr,sizeof(MessageHeader),1,fp);
			size = strlen(data->ld->info->name) + 1;
			fwrite(&size,sizeof(size_t),1,fp);				/*	ld		*/
			fwrite(data->ld->info->name,size,1,fp);
			fwrite(&data->w,sizeof(WindowControl),1,fp);	/*	w		*/
			if		(  data->spadata  !=  NULL  ) {
				g_hash_table_foreach(data->spadata,(GHFunc)_SaveSpa,fp);
			}
			size = LBS_Size(data->mcpdata);
			fwrite(&size,sizeof(size_t),1,fp);				/*	mcp data	*/
			fwrite(LBS_Body(data->mcpdata),LBS_Size(data->mcpdata),1,fp);
			if		(  data->linkdata  !=  NULL  ) {
				size = LBS_Size(data->linkdata);
				fwrite(&size,sizeof(size_t),1,fp);				/*	link data	*/
				fwrite(LBS_Body(data->linkdata),LBS_Size(data->linkdata),1,fp);
			} else {
				size = 0;
				fwrite(&size,sizeof(size_t),1,fp);
			}
			if		(  data->scrpool  !=  NULL  ) {
				g_hash_table_foreach(data->scrpool,(GHFunc)_SaveScrpool,fp);
			}
			fclose(fp);
		}
	}
LEAVE_FUNC;
}

static	SessionData	*
CheckSession(
	NETFILE	*fp,
	char	*term)
{
	SessionData	*data;

ENTER_FUNC;
	if		(  ( data = LookupSession(term) )  !=  NULL  ) {
		dbgmsg("TRUE");
		SendPacketClass(fp,WFC_TRUE);			ON_IO_ERROR(fp,badio);
		data->hdr->status = TO_CHAR(APL_SESSION_GET);
	} else {
		if		(  ( data = LoadSession(term) )  ==  NULL  ) {
			dbgmsg("INIT");
			SendPacketClass(fp,WFC_FALSE);			ON_IO_ERROR(fp,badio);
		} else {
			dbgmsg("TRUE");
			SendPacketClass(fp,WFC_TRUE);			ON_IO_ERROR(fp,badio);
			data->hdr->status = TO_CHAR(APL_SESSION_GET);
		}
	}
  badio:;
LEAVE_FUNC;
	return	(data);
}

static	void
KeepSession(
	SessionData	*data)
{
ENTER_FUNC;
	dbgprintf("data->name = [%s]\n",data->name);
	LockWrite(&Terminal);
	SaveSession(data);
	gettimeofday(&data->access_time,NULL);
	dbgprintf("cTerm  = %d",cTerm);
	dbgprintf("nCache = %d",nCache);
	UnLock(&Terminal);
LEAVE_FUNC;
}

static	void
SendWindowData(
	NETFILE	*fp,
	SessionData	*data,
	char		*buff)
{
	char		wname[SIZE_LONGNAME+1];
	LargeByteString	*scrdata;

	PureWindowName(buff,wname);
	if		(  ( scrdata = GetScreenData(data,wname) )  !=  NULL  ) {
		SendPacketClass(fp,WFC_TRUE);			ON_IO_ERROR(fp,badio);
		SendLBS(fp,scrdata);					ON_IO_ERROR(fp,badio);
	} else {
		dbgmsg("send NODATA");
		SendPacketClass(fp,WFC_FALSE);			ON_IO_ERROR(fp,badio);
	}
  badio:;
}

static	void
APISession(
	NETFILE		*fp)
{
	SessionData *data;
	APIData *api;
	char ld[SIZE_NAME+1];
	char user[SIZE_USER+1];
	char sterm[SIZE_TERM+1];
	struct	timeval	tv
		,			res;

	data = NULL;
	RecvnString(fp, sizeof(ld), ld);			
		ON_IO_ERROR(fp,badio);
	RecvnString(fp, sizeof(user), user);		
		ON_IO_ERROR(fp,badio);
	RecvnString(fp, sizeof(sterm), sterm);	
		ON_IO_ERROR(fp,badio);

	data = InitAPISession(sterm, FALSE, user, ld);
	if (data != NULL) {
		data->retry = 0;
		api = data->apidata;
		api->method = RecvPacketClass(fp);
		RecvLBS(fp, api->arguments);	ON_IO_ERROR(fp,badio2);
		RecvLBS(fp, api->headers);	ON_IO_ERROR(fp,badio2);
		RecvLBS(fp, api->body);		ON_IO_ERROR(fp,badio2);
		RegistSession(data);

#if 0
		Message("== request",ld);
		Message("ld:%s",ld);
		Message("user:%s",user);
		Message("term:%s",sterm);
		Message("method:%c", (char)api->method);
		Message("arguments:%s", (char *)LBS_Body(api->arguments));
		Message("headers size:%d", LBS_Size(api->headers));
		Message("body:%s", (char *)LBS_Body(api->body));
#endif
#if	1
		gettimeofday(&data->start,NULL);
		data->retry = 0;
		if		(  !fLoopBack  ) {
			LockWrite(data);
			CoreEnqueue(data);
		}
		LockRead(data);
		if		(  fTimer  ) {
			gettimeofday(&tv,NULL);
			timersub(&tv,&data->start,&res);
			printf("wfc %s@%s:%s process time %6ld(ms)\n",
				   data->hdr->user,data->hdr->term,data->hdr->window,
				   (res.tv_sec * 1000L + res.tv_usec / 1000L));
		}
#endif
		api = data->apidata;

		SendPacketClass(fp, WFC_TRUE);
		SendLBS(fp, api->headers);
		SendLBS(fp, api->body);
	badio2:
		FinishSession(data);
	} else {
		SendPacketClass(fp, WFC_END);
	}
  badio:;
}

static	void
TermThread(
	NETFILE		*fp)
{
	int		c;
	SessionData	*data;
	char	buff[SIZE_TERM+1];
	Bool	fCont;
	struct	timeval	tv
		,			res;
	int		i;

ENTER_FUNC;
	fCont = TRUE;
	data = NULL;
#ifdef	DEBUG
	//LockDebug(TRUE);
#endif
	do {
		switch	(c = RecvPacketClass(fp))	{
		  case	WFC_INIT:
			dbgmsg("WFC_INIT");
			RecvnString(fp,SIZE_TERM,buff);
			data = CreateSession(fp,buff);				ON_IO_ERROR(fp,badio);
			break;
		  case	WFC_BREAK:
			dbgmsg("WFC_BREAK");
			SendPacketClass(fp,WFC_DONE);				ON_IO_ERROR(fp,badio);
			fCont = FALSE;
			break;
		  case	WFC_TERMID:
			dbgmsg("WFC_TERMID");
			RecvnString(fp,SIZE_TERM,buff);				ON_IO_ERROR(fp,badio);
			data = CheckSession(fp,buff);
			break;
		  case	WFC_SWITCH_LD:
			dbgmsg("WFC_SWITCH_LD");
			RecvnString(fp,SIZE_TERM,buff);				ON_IO_ERROR(fp,badio);
			data = SwitchLD(fp,data,buff);
			break;
		  case	WFC_HEADER:
			dbgmsg("WFC_HEADER");
			ReadTerminalHeader(fp,data);				ON_IO_ERROR(fp,badio);
			break;
		  case	WFC_BODY:
			dbgmsg("WFC_BODY");
			if		(  ReadTerminalBody(fp,data)  ) {
				SaveSession(data);
			}
			break;
		  case	WFC_EXEC:
			dbgmsg("WFC_EXEC");
			gettimeofday(&data->start,NULL);
			data->retry = 0;
			if		(  !fLoopBack  ) {
				LockWrite(data);
				data->mtype = MESSAGE_TYPE_EVENT;
				data->type = SESSION_TYPE_TERM;
				CoreEnqueue(data);
			}
			break;
		  case	WFC_REQ_HEAD:
			dbgmsg("WFC_REQ_HEAD");
			LockRead(data);
			if		(  fTimer  ) {
				gettimeofday(&tv,NULL);
				timersub(&tv,&data->start,&res);
				printf("wfc %s@%s:%s process time %6ld(ms)\n",
					   data->hdr->user,data->hdr->term,data->hdr->window,
					   (res.tv_sec * 1000L + res.tv_usec / 1000L));
			}
			if		(  !data->fAbort  ) {
				SendHeader(fp,data);
				UnLock(data);
			} else {
				SendPacketClass(fp,WFC_NODATA);			ON_IO_ERROR(fp,badio);
				FinishSession(data);
				data = NULL;
			}
			break;
		  case	WFC_REQ_DATA:
			dbgmsg("WFC_REQ_DATA");
			RecvnString(fp,SIZE_TERM,buff);				ON_IO_ERROR(fp,badio);
			LockRead(data);
			SendWindowData(fp,data,buff);
			UnLock(data);
			break;
		  case	WFC_START:
			dbgmsg("WFC_START");
			/*	start	*/
			break;
		  case	WFC_PREPARE:
			dbgmsg("WFC_PREPARE");
			for	( i = 0 ; i < ApsId ; i ++ ) {
				if		(  data->dbstatus[i]  ==  DB_STATUS_START  ) {
					LockWrite(data);
					data->mtype = MESSAGE_TYPE_PREPARE;
					data->type = SESSION_TYPE_TRANSACTION;
					CoreEnqueue(data);
				}
			}
			break;
		  case	WFC_COMMIT:
			dbgmsg("WFC_COMMIT");
			if		(  !data->fAbort  ) {
				for	( i = 0 ; i < ApsId ; i ++ ) {
					if		(	(  data->dbstatus[i]  ==  DB_STATUS_START    )
							||	(  data->dbstatus[i]  ==  DB_STATUS_PREPARE  ) ) {
						LockWrite(data);
						data->mtype = MESSAGE_TYPE_COMMIT;
						data->type = SESSION_TYPE_TRANSACTION;
						CoreEnqueue(data);
					}
				}
				LockRead(data);
				KeepSession(data);
				UnLock(data);
			} else {
				FinishSession(data);
				data = NULL;
			}
			break;
		  case	WFC_ABORT:
			dbgmsg("WFC_ABORT");
			/*	abort	*/
			UnLock(data);
			data = NULL;
			break;
		  case	WFC_PING:
			dbgmsg("PING");
			SendPacketClass(fp,WFC_PONG);				ON_IO_ERROR(fp,badio);
			break;
		  case	WFC_BLOB:
			dbgmsg("WFC_BLOB");
			PassiveBLOB(fp,BlobState);					ON_IO_ERROR(fp,badio);
			break;
		  case	WFC_END:
			dbgmsg("WFC_END");
			FinishSession(data);
			data = NULL;
			break;
		  case	WFC_API:
			APISession(fp);
			data = NULL;
			break;
		  default:
			dbgmsg("default");
			dbgprintf("c = [%X]\n",c);
			SendPacketClass(fp,WFC_FALSE);				ON_IO_ERROR(fp,badio);
			data->fAbort = TRUE;
			break;
		}
		if		(  data  ==  NULL  ) {
			fCont = FALSE;
		}
	}	while	(  fCont  );
  badio:;
	CloseNet(fp);
LEAVE_FUNC;
}

extern	int
ConnectTerm(
	int		_fhTerm)
{
	int		fhTerm;
	pthread_t	thr;
	pthread_attr_t	attr;
	NETFILE	*fp;

ENTER_FUNC;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,256*1024);
	if		(  ( fhTerm = accept(_fhTerm,0,0) )  <  0  )	{
		Message("_fhTerm = %d INET Domain Accept",_fhTerm);
		exit(1);
	}
	fp = SocketToNet(fhTerm);
	pthread_create(&thr,&attr,(void *(*)(void *))TermThread,(void *)fp);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(fhTerm); 
}

static	void
RemoveThread(void)
{
	SessionData	*data;

	while	(TRUE)	{
		data = (SessionData *)DeQueue(RemoveQueue);
		FreeSession(data);
	}
}

extern	void
InitTerm(void)
{
	pthread_t	thr;

ENTER_FUNC;
	cTerm = 0;
	RemoveQueue = NewQueue();
	InitLock(&Terminal);
	LockWrite(&Terminal);
	Terminal.Hash = NewNameHash();
	UnLock(&Terminal);

	pthread_create(&thr,NULL,(void *(*)(void *))RemoveThread,NULL);
	pthread_detach(thr);

	if		(  ThisEnv->mcprec  !=  NULL  ) {
		InitializeValue(ThisEnv->mcprec->value);
	}
	if		(  ThisEnv->linkrec  !=  NULL  ) {
		InitializeValue(ThisEnv->linkrec->value);
	}
LEAVE_FUNC;
}
