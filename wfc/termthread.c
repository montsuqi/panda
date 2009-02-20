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
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>

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
static	SessionData	*Head;
static	SessionData	*Tail;
static	Queue		*RemoveQueue;

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
	memclear(data,sizeof(SessionData));
	data->hdr = New(MessageHeader);
	data->name = NULL;
	memclear(data->hdr,sizeof(MessageHeader));
	data->fAbort = FALSE;
	data->apsid = -1;
	data->spadata = NewNameHash();
	data->scrpool = NewNameHash();
	data->next = NULL;
	data->prev = NULL;
	data->apidata = New(APIData);
	data->apidata->arguments = NewLBS();
	data->apidata->headers = NewLBS();
	data->apidata->body = NewLBS();
LEAVE_FUNC;
	return	(data);
}

static	void
RegistSession(
	SessionData	*data)
{
	LockWrite(&Terminal);
	if		(  g_hash_table_lookup(Terminal.Hash,data->name)  ==  NULL  ) {
		cTerm ++;
		g_hash_table_insert(Terminal.Hash,data->name,data);
	}
	UnLock(&Terminal);
}

static	SessionData	*
LookupSession(
	char	*term,
	Bool	*fInProcess)
{
	SessionData	*data;

ENTER_FUNC;
	LockRead(&Terminal);
	if		(  ( data = g_hash_table_lookup(Terminal.Hash,term) )  !=  NULL  ) {
		*fInProcess = data->fInProcess;
		data->fInProcess = TRUE;
	} else {
		*fInProcess = FALSE;
	}
	UnLock(&Terminal);
LEAVE_FUNC;
	return	(data);
}

static	void
_UnrefSession(
	SessionData	*data)
{
	data->fInProcess = FALSE;
	if		(  data->next  !=  NULL  ) {
		data->next->prev = data->prev;
		if		(  data  ==  Head  ) {
			Head = data->next;
		}
	}
	if		(  data->prev  !=  NULL  ) {
		data->prev->next = data->next;
		if		(  data  ==  Tail  ) {
			Tail = data->prev;
		}
	}
	if		(	(  data->name  !=  NULL  )
			&&	(  g_hash_table_lookup(Terminal.Hash,data->name)  !=  NULL  ) ) {
		cTerm --;
		g_hash_table_remove(Terminal.Hash,data->name);
	}
	EnQueue(RemoveQueue,data);
}

static	void
FinishSession(
	SessionData	*data)
{
	char	fname[SIZE_LONGNAME+1];

	dbgprintf("unref name = [%s]\n",data->name);
	if		(  SesDir  !=  NULL  ) {
		sprintf(fname,"%s/%s.ses",SesDir,data->name);
		remove(fname);
	}
	LockWrite(&Terminal);
	_UnrefSession(data);
	UnLock(&Terminal);
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
	xfree(data);
LEAVE_FUNC;
}

static	SessionData	*
_InitSession(
	PacketClass type,
	char *term,
	Bool fKeep,
	char *hdr_user,
	char *ldname)
{
	SessionData	*data;
	LD_Node		*ld;
	int			i;

ENTER_FUNC;
	data = MakeSessionData();
	data->type = type;
	strcpy(data->hdr->term,term);
	data->fKeep = fKeep;
	data->fInProcess = TRUE;
	if		(  ( ld = g_hash_table_lookup(APS_Hash, ldname) )  !=  NULL  ) {
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
InitSession(
	NETFILE	*fp,
	char	*term)
{
	SessionData	*data;
	char	buff[SIZE_LONGNAME+1];
	char	msg[SIZE_LONGNAME+1];
	LD_Node	*ld;
	int			i;
	Bool	fKeep;

ENTER_FUNC;
	data = MakeSessionData();
	if		(  RecvPacketClass(fp)  ==  WFC_TRUE  ) {
		fKeep = TRUE;
	} else {
		fKeep = FALSE;
	}
	strcpy(data->hdr->term,term);
	RecvnString(fp,SIZE_NAME,data->hdr->user);		ON_IO_ERROR(fp,badio);
	data->fKeep = fKeep;
	data->fInProcess = TRUE;
	snprintf(msg,SIZE_LONGNAME,"[%s@%s] session start(%d)",data->hdr->user,data->hdr->term,
		cTerm+1);
	MessageLog(msg);
	dbgprintf("term = [%s]",data->hdr->term);
	dbgprintf("user = [%s]",data->hdr->user);
	RecvnString(fp,SIZE_LONGNAME,buff);	/*	LD name	*/	ON_IO_ERROR(fp,badio);
	if		(  ( ld = g_hash_table_lookup(APS_Hash,buff) )  !=  NULL  ) {
		SendPacketClass(fp,WFC_OK);
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
		data->name = StrDup(data->hdr->term);
		data->hdr->puttype = SCREEN_NULL;
		data->w.n = 0;
		RegistSession(data);
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

static	SessionData	*
ReadTerminal(
	NETFILE		*fp,
	SessionData	*data)
{
	LD_Node	*ld;
	Bool		fExit;
	int			c;
	LargeByteString	*scrdata;
	char		window[SIZE_LONGNAME+1]
		,		comp[SIZE_LONGNAME+1];

ENTER_FUNC;
	fExit = FALSE;
	while	(  !fExit  ) {
		switch	(c = RecvPacketClass(fp)) {
		  case	WFC_DATA:
			dbgmsg("recv DATA");
			if		(  data  !=  NULL  ) {
				RecvnString(fp,SIZE_NAME,data->hdr->window);	ON_IO_ERROR(fp,badio);
				RecvnString(fp,SIZE_NAME,data->hdr->widget);	ON_IO_ERROR(fp,badio);
				RecvnString(fp,SIZE_NAME,data->hdr->event);		ON_IO_ERROR(fp,badio);
				dbgprintf("window = [%s]",data->hdr->window);
				dbgprintf("widget = [%s]",data->hdr->widget);
				dbgprintf("event  = [%s]",data->hdr->event);
				PureComponentName(data->hdr->window,comp);
				if		(  ( ld = g_hash_table_lookup(ComponentHash,comp) )
						   !=  NULL  ) {
					dbgprintf("ld = [%s]",ld->info->name);
					PureWindowName(data->hdr->window,window);
					dbgprintf("window = [%s]",window);
					if		(  ( scrdata = GetScreenData(data,window) )  !=  NULL  ) {
						SendPacketClass(fp,WFC_OK);				ON_IO_ERROR(fp,badio);
						dbgmsg("send OK");
						if		(  RecvPacketClass(fp)  ==  WFC_DATA  ) {
							RecvLBS(fp,scrdata);					ON_IO_ERROR(fp,badio);
						}
						data->hdr->rc = TO_CHAR(0);
						data->hdr->puttype = SCREEN_NULL;
					} else {
						Error("invalid window [%s]",window);
					}
					if		(  data->ld  !=  ld  ) {
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
		  case	WFC_END:
			dbgmsg("END");
			data->fAbort = TRUE;
			fExit = TRUE;
			break;
		  default:
			dbgmsg("default");
			dbgprintf("c = [%X]\n",c);
			SendPacketClass(fp,WFC_NOT);
			ON_IO_ERROR(fp,badio);
			fExit = TRUE;
			data->fAbort = TRUE;
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
	MessageHeader	*hdr;
	int			i
		,		c;
	Bool		rc;
	Bool		fExit;
	char		wname[SIZE_LONGNAME+1]
		,		buff[SIZE_LONGNAME+1];
	LargeByteString	*scrdata;

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
		dbgprintf("window    = [%s]",hdr->window);
		dbgprintf("data->w.n = %d",data->w.n);
		for	( i = 0 ; i < data->w.n ; i ++ ) {
			SendInt(fp,data->w.control[i].PutType);			ON_IO_ERROR(fp,badio);
			SendString(fp,data->w.control[i].window);		ON_IO_ERROR(fp,badio);
		}
		data->w.n = 0;
		fExit = FALSE;
		do {
			rc = TRUE;
			switch	(c = RecvPacketClass(fp))	{
			  case	WFC_PING:
				dbgmsg("PING");
				SendPacketClass(fp,WFC_PONG);		ON_IO_ERROR(fp,badio);
				break;
			  case	WFC_DATA:
				dbgmsg(">DATA");
				RecvnString(fp,SIZE_NAME,buff);				ON_IO_ERROR(fp,badio);
				PureWindowName(buff,wname);
				if		(  ( scrdata = GetScreenData(data,wname) )  !=  NULL  ) {
					dbgmsg("send OK");
					SendPacketClass(fp,WFC_OK);			ON_IO_ERROR(fp,badio);
					SendLBS(fp,scrdata);				ON_IO_ERROR(fp,badio);
				} else {
					dbgmsg("send NODATA");
					SendPacketClass(fp,WFC_NODATA);			ON_IO_ERROR(fp,badio);
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
			  case	WFC_END:
				dbgmsg("END");
				fExit = TRUE;
				rc = FALSE;
				break;
			  default:
				Warning("[%s] session failure packet [%X]",data->hdr->term, c);
				dbgprintf("c = [%X]\n",c);
				fExit = TRUE;
				rc = FALSE;
				break;
			}
		}	while	(  !fExit  );
	} else {
	  badio:
		Warning("[%s] session recv failure",data->hdr->term);
	}
LEAVE_FUNC;
	return	(rc); 
}

static	SessionData	*
Process(
	SessionData	*data)
{
	struct	timeval	tv;
	long	ever
		,	now;

ENTER_FUNC;
	gettimeofday(&tv,NULL);
	ever = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
	if		(  !fLoopBack  ) {
		CoreEnqueue(data);
	} else {
		EnQueue(data->term->que,data);
	}
	data = DeQueue(data->term->que);
	if		(  fTimer  ) {
		gettimeofday(&tv,NULL);
		now = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
		printf("wfc %s@%s:%s process time %6ld(ms)\n",
			   data->hdr->user,data->hdr->term,data->hdr->window,(now - ever));
	}
LEAVE_FUNC;
	return	(data);
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
#if	0
			fread(&data->fInProcess,sizeof(Bool),1,fp);
#else
			data->fInProcess = TRUE;
#endif
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
#if	0
			fwrite(&data->fInProcess,sizeof(Bool),1,fp);
#endif
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
	Bool		fError
		,		fInProcess;

ENTER_FUNC;
	fError = TRUE;
	if		(  ( data = LookupSession(term,&fInProcess) )  !=  NULL  ) {
		if		(  !fInProcess  ) {
			dbgmsg("TRUE");
			SendPacketClass(fp,WFC_TRUE);			ON_IO_ERROR(fp,badio);
			data->hdr->status = TO_CHAR(APL_SESSION_GET);
			data = ReadTerminal(fp,data);
		} else {
			dbgmsg("FALSE");
			Warning("Error: Other threads are processing it.");
			SendPacketClass(fp,WFC_FALSE);			ON_IO_ERROR(fp,badio);
			data = NULL;
		}
	} else {
		if		(  ( data = LoadSession(term) )  ==  NULL  ) {
			dbgmsg("INIT");
			if		(  ( data = InitSession(fp,term) )  ==  NULL  )	{
				Warning("Error: session initialize failure");
			} else {
				data->hdr->status = TO_CHAR(APL_SESSION_LINK);
			}
		} else {
			dbgmsg("TRUE");
			SendPacketClass(fp,WFC_TRUE);			ON_IO_ERROR(fp,badio);
			data->hdr->status = TO_CHAR(APL_SESSION_GET);
		}
		data = ReadTerminal(fp,data);
	}
	fError = FALSE;
  badio:
	if		(  fError  ) {
		data = NULL;
	}
	if		(  data  !=  NULL  ) {
		SaveSession(data);
	}
LEAVE_FUNC;
	return	(data);
}

static	void
KeepSession(
	SessionData	*data)
{
	SessionData	*exp;

ENTER_FUNC;
	dbgprintf("data->name = [%s]\n",data->name);
	LockWrite(&Terminal);
	SaveSession(data);
	data->fInProcess = FALSE;
	if		(  data  !=  Head  ) {
		if		(  data->next  !=  NULL  ) {
			data->next->prev = data->prev;
		}
		if		(  data->prev  !=  NULL  ) {
			data->prev->next = data->next;
		}
		if		(  Tail  ==  NULL  ) {
			Tail = data;
		} else {
			if		(  data  ==  Tail  ) {
				Tail = data->prev;
			}
		}
		if		(  Head  !=  NULL  ) {
			Head->prev = data;
		}
		data->next = Head;
		data->prev = NULL;
		Head = data;
	}
	dbgprintf("cTerm  = %d",cTerm);
	dbgprintf("nCache = %d",nCache);
	if		(  nCache  ==  0  ) {
		exp = NULL;
	} else
	if		(  cTerm  >  nCache  ) {
		if		(  ( exp = Tail )  !=  NULL  ) {
			if		(  Tail->prev  !=  NULL  ) {
				Tail->prev->next = NULL;
			}
			if		(  Head  ==  Tail  ) {
				Head = NULL;
				Tail = NULL;
			} else {
				Tail = Tail->prev;
			}
			exp->next = NULL;
			exp->prev = NULL;
		}
	} else {
		exp = NULL;
	}
	if		(  exp  !=  NULL  ) {
		_UnrefSession(exp);
	}
	UnLock(&Terminal);
#ifdef	DEBUG
	{
		SessionData	*p;
		printf("*** term dump Head -> Tail ***\n");
		for	( p = Head ; p != NULL ; p = p->next ) {
			printf("[%s]\n",p->name);
		}
		printf("*****************\n");
		printf("*** term dump Tail -> Head  ***\n");
		for	( p = Tail ; p != NULL ; p = p->prev ) {
			printf("[%s]\n",p->name);
		}
		printf("*****************\n");
	}
#endif
LEAVE_FUNC;
}

static	void
ExecTerm(
	TermNode	*term)
{
	SessionData	*data;
	char	buff[SIZE_TERM+1];
	
	RecvnString(term->fp,SIZE_TERM,buff);
	if		(  ( data = CheckSession(term->fp,buff) )  !=  NULL  ) {
		data->term = term;
		data->retry = 0;
		if		(  !data->fAbort  ) {
			data = Process(data);
		}
		if		(	(  data->fAbort  )
				||	(  !SendTerminal(term->fp,data)  ) ) {
			FinishSession(data);
		} else {
			KeepSession(data);
		}
	}
	SendPacketClass(term->fp,WFC_DONE);
	CloseNet(term->fp);
}

static	void
CallAPI(
	TermNode	*term)
{
	SessionData *data;
	APIData *api;
	char ld[SIZE_NAME+1];
	char user[SIZE_USER+1];
	char sterm[SIZE_TERM+1];

	data = NULL;
	RecvnString(term->fp, sizeof(ld), ld);			
		ON_IO_ERROR(term->fp,badio);
	RecvnString(term->fp, sizeof(user), user);		
		ON_IO_ERROR(term->fp,badio);
	RecvnString(term->fp, sizeof(sterm), sterm);	
		ON_IO_ERROR(term->fp,badio);

	data = _InitSession(APS_API, sterm, FALSE, user, ld);
	if (data != NULL) {
		api = data->apidata;
		api->method = RecvPacketClass(term->fp);
		RecvLBS(term->fp, api->arguments);	ON_IO_ERROR(term->fp,badio2);
		RecvLBS(term->fp, api->headers);	ON_IO_ERROR(term->fp,badio2);
		RecvLBS(term->fp, api->body);		ON_IO_ERROR(term->fp,badio2);
		RegistSession(data);

		Message("== request",ld);
		Message("ld:%s",ld);
		Message("user:%s",user);
		Message("term:%s",sterm);
		Message("method:%c", (char)api->method);
		Message("arguments:%s", (char *)LBS_Body(api->arguments));
		Message("headers size:%d", LBS_Size(api->headers));
		Message("body:%s", (char *)LBS_Body(api->body));

#if 0
		data = Process(data);
#endif
		api = data->apidata;

		SendPacketClass(term->fp, WFC_OK);
		SendLBS(term->fp, api->headers);
		SendLBS(term->fp, api->body);
		CloseNet(term->fp);
	badio2:
		FinishSession(data);
	} else {
		SendPacketClass(term->fp, WFC_END);
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
	case WFC_TERM:
		ExecTerm(term);
		break;
	case WFC_API:
		CallAPI(term);
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
	int		fhTerm;
	pthread_t	thr;
	pthread_attr_t	attr;
	TermNode	*term;

ENTER_FUNC;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,256*1024);
	if		(  ( fhTerm = accept(_fhTerm,0,0) )  <  0  )	{
		Message("_fhTerm = %d INET Domain Accept",_fhTerm);
		exit(1);
	}
	term = New(TermNode);
	term->que = NewQueue();
	term->fp = SocketToNet(fhTerm);
	pthread_create(&thr,&attr,(void *(*)(void *))TermThread,(void *)term);
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
	Head = NULL;
	Tail = NULL;
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
