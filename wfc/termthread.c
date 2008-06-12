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
	gettimeofday(&(data->tv),NULL);
LEAVE_FUNC;
	return	(data);
}

static	void
RegistSession(
	SessionData	*data)
{
	if		(  g_hash_table_lookup(Terminal.Hash,data->name)  ==  NULL  ) {
		g_hash_table_insert(Terminal.Hash,data->name,data);
	}
}

static	void
UnRegistSession(
	SessionData	*data)
{
	if		(  g_hash_table_lookup(Terminal.Hash,data->name)  !=  NULL  ) {
		g_hash_table_remove(Terminal.Hash,data->name);
	}
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

	data->fInProcess = FALSE;
	UnRegistSession(data);
	FreeSession(data);

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

extern	void
FreeSession(
	SessionData	*data)
{
	char	msg[SIZE_LONGNAME+1];

ENTER_FUNC;
	snprintf(msg,SIZE_LONGNAME,"[%s@%s] session end",data->hdr->user,data->hdr->term);
	MessageLog(msg);
	g_hash_table_foreach_remove(data->scrpool,(GHRFunc)FreeScr,NULL);
	g_hash_table_destroy(data->scrpool);
	xfree(data->hdr);
	if		(  data->mcpdata  !=  NULL  ) {
		FreeLBS(data->mcpdata);
	}
	if		(  data->linkdata  !=  NULL  ) {
		FreeLBS(data->linkdata);
	}
	g_hash_table_foreach_remove(data->spadata,(GHRFunc)FreeSpa,NULL);
	g_hash_table_destroy(data->spadata);
	xfree(data->scrdata);
	xfree(data->name);
	xfree(data);
LEAVE_FUNC;
}

static  TerminalHeader *
ReadTerminalHeader(
	NETFILE *fp)
{
	TerminalHeader 	*termhdr;
	char			buff[SIZE_LONGNAME+1];
	Bool			fKeep;
ENTER_FUNC;
	termhdr = New(TerminalHeader);
	RecvnString(fp,SIZE_TERM,termhdr->term);		ON_IO_ERROR(fp,badio);
	termhdr->status = RecvPacketClass(fp);			ON_IO_ERROR(fp,badio);
	fKeep = RecvPacketClass(fp);					ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_NAME,termhdr->user);		ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_LONGNAME,buff);	/*	LD name	*/	ON_IO_ERROR(fp,badio);
	if		(  fKeep == WFC_TRUE  ) {
		termhdr->fKeep = TRUE;
	} else {
		termhdr->fKeep = FALSE;
	}
	if		(  (termhdr->ld = g_hash_table_lookup(APS_Hash,buff)) ==  NULL  ) {
		Warning("[%s] session fail LD [%s] not found.", termhdr->term, buff);
	badio:
		xfree(termhdr);
		termhdr = NULL;
	}
LEAVE_FUNC;
	return termhdr;	
}

static	SessionData	*
InitSession(
	TerminalHeader 	*termhdr)
{
	SessionData	*data;
	int			i;

ENTER_FUNC;
	data = MakeSessionData();
	strcpy(data->hdr->term, termhdr->term);
	strcpy(data->hdr->user, termhdr->user);
	data->ld = termhdr->ld;
	data->fKeep = termhdr->fKeep;
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

	data->cWindow = data->ld->info->cWindow;
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
	Message("[%s@%s] session start(%d)", termhdr->user, termhdr->term, g_hash_table_size(Terminal.Hash));
LEAVE_FUNC;
	return	(data);
}

static	SessionData	*
ReadTerminal(
	SessionData	*data)
{
	LD_Node	*ld;
	Bool		fExit;
	int			c;
	LargeByteString	*scrdata;
	char		window[SIZE_LONGNAME+1]
		,		comp[SIZE_LONGNAME+1];
	NETFILE		*fp;

ENTER_FUNC;
	fp = data->term->fp;
	fExit = FALSE;
	while	(  !fExit  ) {
		switch	(c = RecvPacketClass(fp)) {
		  case	WFC_DATA:
			dbgmsg("recv DATA");
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
			SendPacketClass(fp,WFC_NOT);		ON_IO_ERROR(fp,badio);
			fExit = TRUE;
			data->fAbort = TRUE;
			break;
		}
	}
  badio:
	if (fExit != TRUE) {
		data->fAbort = TRUE;	
		Warning("badio %s ", data->name);
	}
LEAVE_FUNC;
	return(data);
}

static	Bool
SendTerminal(
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
	NETFILE		*fp;

ENTER_FUNC;
	fp = data->term->fp;
	rc = FALSE;
	fExit = FALSE;
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
				fExit = TRUE;
				rc = FALSE;
				break;
			}
		}	while	(  !fExit  );
	} else {
	  badio:
		Warning("[%s] session recv failure",data->hdr->term);
		if (fExit != TRUE) {
			data->fAbort = TRUE;
		}
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
			data->fInProcess = FALSE;
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

static	void
_LookupOldSession(
	char	*name,
	SessionData	*data,
	SessionData	**olddata_p)
{
	SessionData	*olddata;

	if ( ! data->fInProcess ){
		olddata = *olddata_p;
		if ( olddata != NULL){
			if(timercmp(&(olddata->tv), &(data->tv), >)){
				olddata = data;	
			}	
		} else {
			olddata = data;	
		}
		*olddata_p = olddata;
	}
}

static	void
FreeOldSession(void)
{
	SessionData	*olddata = NULL;
	g_hash_table_foreach(Terminal.Hash,(GHFunc)_LookupOldSession, &olddata);
	UnRegistSession(olddata);
	FreeSession(olddata);
}

static	void
KeepSession(
	SessionData	*data)
{
ENTER_FUNC;
	dbgprintf("data->name = [%s]\n",data->name);
	SaveSession(data);
	gettimeofday(&(data->tv),NULL);
	LockWrite(&Terminal);
	data->fInProcess = FALSE;
	if ( (nCache !=0) && (g_hash_table_size(Terminal.Hash) > nCache) ){
		FreeOldSession();
	}
	UnLock(&Terminal);
LEAVE_FUNC;
}

static	SessionData	*
LookupSession(
	TerminalHeader	*termhdr)
{
	SessionData	*data;

ENTER_FUNC;
	data = NULL;
	if		( termhdr != NULL  ) {
		LockWrite(&Terminal);
		data = g_hash_table_lookup(Terminal.Hash, termhdr->term);
		data = data ? data : LoadSession(termhdr->term);
		if		(  data != NULL  ) {
			if 		(  ! data->fInProcess  ) {
				data->hdr->status = TO_CHAR(APL_SESSION_GET);
				data->fInProcess = TRUE;
			} else {
				Warning("Error: Other threads are processing it.");
				data = NULL;
			}
		} else {
			if ( termhdr->status == WFC_START ) {
				data = InitSession(termhdr);
				data->hdr->status = TO_CHAR(APL_SESSION_LINK);
				data->fInProcess = TRUE;
			}
		}
		UnLock(&Terminal);
	}
LEAVE_FUNC;
	return	(data);
}

static	void
TermThread(
	TermNode	*term)
{
	SessionData		*data;
	TerminalHeader *termhdr;

ENTER_FUNC;
	termhdr = ReadTerminalHeader(term->fp);
	data = LookupSession(termhdr);
	if		( data != NULL ) {
		SendPacketClass(term->fp,WFC_OK);		ON_IO_ERROR(term->fp,badio);
		data->term = term;
		data->retry = 0;
		data = ReadTerminal(data);

		if		(  !data->fAbort  ) {
			data = Process(data);
		}

		if		(  !data->fAbort  ) {
			SendTerminal(data);
		}

		if 		(  !data->fAbort  ) {
			KeepSession(data);
		} else {
			FinishSession(data);
		}
	} else {
		SendPacketClass(term->fp,WFC_NOT);		ON_IO_ERROR(term->fp,badio);
	}
	SendPacketClass(term->fp,WFC_DONE);			ON_IO_ERROR(term->fp,badio);
badio:
	CloseNet(term->fp);
	FreeQueue(term->que);
	xfree(term);
	if	(  termhdr != NULL ) xfree(termhdr);
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

extern	void
InitTerm(void)
{
ENTER_FUNC;
	InitLock(&Terminal);
	LockWrite(&Terminal);
	Terminal.Hash = NewNameHash();
	UnLock(&Terminal);

	if		(  ThisEnv->mcprec  !=  NULL  ) {
		InitializeValue(ThisEnv->mcprec->value);
	}
	if		(  ThisEnv->linkrec  !=  NULL  ) {
		InitializeValue(ThisEnv->linkrec->value);
	}
LEAVE_FUNC;
}
