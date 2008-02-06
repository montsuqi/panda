/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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
#define	TRACE
#define	DEBUG
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<sys/uio.h>
#include	<unistd.h>
#include	<errno.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"
#include	"enum.h"
#include	"socket.h"
#include	"net.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"cgi.h"
#include	"comm.h"
#include	"comms.h"
#include	"authstub.h"
#include	"queue.h"
#include	"applications.h"
#include	"driver.h"
#include	"htserver.h"
#include	"dirs.h"
#include	"RecParser.h"
#include	"front.h"
#include	"term.h"
#include	"debug.h"

static	int			cServer;

typedef	struct {
	int			ses;
	ScreenData	*scr;
	Queue		*que;
}	SessionParameter;

typedef	struct	{
	int		sock;
	char	*term;
	int		count;
	int		pid;
}	HTC_Node;

static	void
PutLog(
	char	*user,
	char	*cmd)
{
	char	buff[SIZE_LONGNAME+1];

	sprintf(buff,"[%s] [%s] session start(%d)",user,cmd,cServer);
	MessageLog(buff);
}

static	void
FinishSession(
	ScreenData	*scr)
{
	char	msg[SIZE_LONGNAME+1];
ENTER_FUNC;
	sprintf(msg,"[%s] session end",scr->user);
	MessageLog(msg);
	cServer --;
LEAVE_FUNC;
	exit(0);
}

static	void
DecodeName(
	char	**rname,
	char	**vname,
	char	*p)
{
	while	(  isspace(*p)  )	p ++;
	*rname = p;
	while	(	(  *p  !=  0     )
			&&	(  *p  !=  '.'   )
			&&	(  *p  !=  '/'   ) )	p ++;
	if		(  *p  ==  0  ) {
		*vname = *rname;
		*rname = p;
	} else {
		*p = 0;
		p ++;
		while	(	(  *p  !=  0    )
				&&	(	(  isspace(*p)  )
					||	(  *p  ==  '.'  )
					||	(  *p  ==  '/'  ) ) )	p ++;
		*vname = p;
		if		(  *p  !=  0  ) {
			while	(	(  *p  !=  0     )
					&&	(  !isspace(*p)  ) )	p ++;
		}
		*p = 0;
	}
}

static	void
SendWindow(
	char		*wname,
	WindowData	*win,
	NETFILE		*fp)
{
	if		(  win->PutType  !=  SCREEN_NULL  ) {
		dbgprintf("window       = [%s]",win->name);
		dbgprintf("win->PutType = %02X",win->PutType);
		switch	(win->PutType) {
		  case	SCREEN_CURRENT_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			SendPacketClass(fp,GL_WindowName);
			SendString(fp,win->name);
			break;
		  case	SCREEN_END_SESSION:
			SendPacketClass(fp,GL_RedirectName);
			SendString(fp,win->name);
		  default:
			break;
		}
#if	0
		win->PutType = SCREEN_NULL;
#endif
	}
}

static	void
SendWindowName(
	NETFILE		*fp,
	ScreenData	*scr)
{
ENTER_FUNC;
	g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,fp);
	SendPacketClass(fp,GL_END);
	if		(  !CheckNetFile(fp)  )		FinishSession(scr);
LEAVE_FUNC;
}

static	void
WriteClient(
	NETFILE		*fp,
	ScreenData	*scr,
	Bool		fWindow)
{
    LargeByteString	*lbs;
	PacketClass	klass;
	char	buff[SIZE_LARGE_BUFF]
		,	window[SIZE_LONGNAME+1];
	char	*vname
	,		*wname;
	RecordStruct	*rec;
	ValueStruct	*value;
	Bool	fClear;
	size_t	size;
	struct	stat	sb;
	FILE	*fpf;

ENTER_FUNC;
	if		(  fWindow  ) {
		SendWindowName(fp,scr);
	}
    lbs = NewLBS();
	while	(  ( klass = RecvPacketClass(fp) )  ==  GL_GetData  ) {
		ON_IO_ERROR(fp,badio);
		RecvString(fp,buff);
		ON_IO_ERROR(fp,badio);
		fClear = RecvBool(fp);		ON_IO_ERROR(fp,badio);
		if		(  *buff  !=  0  ) {
			DecodeName(&wname,&vname,buff);
			LBS_EmitStart(lbs);
			PureWindowName(wname,window);
#ifdef	DEBUG
			dbgprintf("window = [%s]",window);
			dbgprintf("wname  = [%s]",wname);
			dbgprintf("vname  = [%s]",vname);
#endif
			if		(  ( rec = g_hash_table_lookup(scr->Records,window) )  !=  NULL  ) {
				if		(  *vname  ==  0  ) {
					value = rec->value;
				} else {
					value = GetItemLongName(rec->value,vname);
				}
				if (value == NULL) {
					fprintf(stderr, "no ValueStruct: %s.%s\n", window, vname);
				} else {
					ValueIsUpdate(value);
#ifdef	DEBUG
					printf("htserver -> mon (%s.%s)\n",window,vname);
					DumpValueStruct(value);
					printf("--\n");
#endif
					size = NativeSaveSize(value,TRUE);
					LBS_ReserveSize(lbs,size,FALSE);
					NativeSaveValue(LBS_Body(lbs),value,TRUE);
					if		(  ValueType(value)  ==  GL_TYPE_OBJECT  ) {
						if		(  ( fpf = Fopen(BlobCacheFileName(value),"r") )
								   !=  NULL  ) {
							dbgmsg("blob body");
							SendLBS(fp,lbs);	ON_IO_ERROR(fp,badio);
							fstat(fileno(fpf),&sb);
							LBS_ReserveSize(lbs,sb.st_size,FALSE);
							fread(LBS_Body(lbs),LBS_Size(lbs),1,fpf);
							fclose(fpf);
						}
					}
					if		(  fClear  ) {
						InitializeValue(value);
					}
				}
			} else {
#if	1
				LBS_ReserveSize(lbs,0,FALSE);
#else
				fprintf(stderr, "window [%s] not found\n", window);
				SetErrorNetFile(fp);
				goto	badio;
#endif
			}
			SendLBS(fp, lbs);	ON_IO_ERROR(fp,badio);
		}
	}
	dbgprintf("klass = %X",klass);
  badio:
    FreeLBS(lbs);
	if		(  !CheckNetFile(fp)  )		FinishSession(scr);
LEAVE_FUNC;
}

static	void
RecvScreenData(
	NETFILE		*fp,
	ScreenData	*scr)
{
    char buff[SIZE_LARGE_BUFF];
	char *vname, *wname;
	RecordStruct	*rec;
	ValueStruct	*value;
    LargeByteString *lbs;
	int			i
		,		ival;
	ValueStruct	*v;
	char		*p
		,		*pend;
	PacketClass	klass;
	FILE		*fpf;

ENTER_FUNC;
	ThisScreen = scr;
    lbs = NewLBS();
	while	(  ( klass = RecvPacketClass(fp) )  ==  GL_ScreenData  ) {
		ON_IO_ERROR(fp,badio);
		memclear(buff,SIZE_LARGE_BUFF);
		RecvString(fp,buff);	ON_IO_ERROR(fp,badio);
        DecodeName(&wname, &vname, buff);
        LBS_EmitStart(lbs);
        RecvLBS(fp, lbs);		ON_IO_ERROR(fp,badio);
		if		(  *wname  !=  0  ) {
			dbgprintf("[%s.%s]=[%s]",wname,vname,LBS_Body(lbs));
		}
		dbgprintf("wname = [%s]",wname);
		if		(	(  *wname                    !=  0  )
				&&	(  strlicmp(wname,"http:/")  !=  0  ) ) {
			SetWindowName(wname);
			if		(  *scr->window  ==  0  ) {
				strcpy(scr->window,wname);
			}
		}
		dbgprintf("scr->window = [%s]",scr->window);
		if		(  ( rec = SetWindowRecord(wname) )  !=  NULL  ) {
			if		(  ( value = GetItemLongName(rec->value, vname) )  ==  NULL  ) {
				fprintf(stderr, "no ValueStruct: %s.%s\n", wname, vname);
			} else {				
				ValueIsUpdate(value);
				switch (ValueType(value)) {
				  case GL_TYPE_ARRAY:
					for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
						v = GetArrayItem(value, i);
						SetValueBool(v, FALSE);
					}
					p = LBS_Body(lbs);
					pend = p + LBS_Size(lbs);
					while	(  p  <  pend  ) {
						ival = atoi(p);
						if		(  ( v = GetArrayItem(value, ival) )  !=  NULL  ) {
							SetValueBool(v, TRUE);
						}
						while	(	(  isdigit(*p)  )
								||	(  *p  ==  '-'  ) ) {
							p++;
						}
						if	(  *p  ==  ','  )	p++;
					}
					break;
				  case GL_TYPE_BYTE:
				  case GL_TYPE_BINARY:
					SetValueBinary(value, LBS_Body(lbs), LBS_Size(lbs));
					break;
				  case	GL_TYPE_OBJECT:
					if		(	(  LBS_Body(lbs)  >  0  )
							&&	(  ( fpf = Fopen(BlobCacheFileName(value),"w") ) 
								   !=  NULL  ) ) {
						fwrite(LBS_Body(lbs),LBS_Size(lbs),1,fpf);
						fclose(fpf);	
					}
					break;
				  default:
					LBS_EmitEnd(lbs);
					SetValueString(value, LBS_Body(lbs), "utf8");
					break;
				}
#ifdef	DEBUG
				printf("--\n");
				DumpValueStruct(value);
				printf("--\n");
#endif
			}
        }
    }
  badio:
    FreeLBS(lbs);
	if		(  !CheckNetFile(fp)  )		FinishSession(scr);
LEAVE_FUNC;
}

static	Bool
ExecSingle(
	NETFILE	*fp,
	ScreenData	*scr)
{
	PacketClass	klass;
	Bool	fCont;

ENTER_FUNC;
	fCont = FALSE;
	SendPacketClass(fp,GL_Session);		ON_IO_ERROR(fp,badio);
	SendString(fp,scr->term);			ON_IO_ERROR(fp,badio);
  restart:
	klass = RecvPacketClass(fp);
	ON_IO_ERROR(fp,badio);
	switch	(klass)	{
	  case	GL_Event:
		RecvnString(fp,SIZE_NAME,scr->window);		ON_IO_ERROR(fp,badio);
		RecvnString(fp,SIZE_NAME,scr->widget);		ON_IO_ERROR(fp,badio);
		RecvnString(fp,SIZE_EVENT,scr->event);		ON_IO_ERROR(fp,badio);
		dbgprintf("window = [%s]\n",scr->window);
		dbgprintf("event  = [%s]\n",scr->event);
		dbgprintf("user   = [%s]\n",scr->user);
		dbgprintf("cmd    = [%s]\n",scr->cmd);
		RecvScreenData(fp,scr);						ON_IO_ERROR(fp,badio);
		if		(  *scr->event  !=  0  ) {
			ApplicationsCall(APL_SESSION_GET,scr);
		} else {
			scr->status = APL_SESSION_RESEND;
		}
		break;
	  case	GL_GetData:
		WriteClient(fp,scr,FALSE);
		ON_IO_ERROR(fp,badio);
		goto	restart;
		break;
	  case	GL_ScreenData:
		/*	fatal error	*/
		scr->status = APL_SESSION_RESEND;
		break;
	  case	GL_END:
		ApplicationsCall(APL_SESSION_END,scr);
		scr->status = APL_SESSION_NULL;
		break;
	  default:
		Warning("invalid class = %X\n",klass);
		ApplicationsCall(APL_SESSION_END,scr);
		scr->status = APL_SESSION_NULL;
		break;
	}
	while	(  scr->status  ==  APL_SESSION_LINK  ) {
		ApplicationsCall(scr->status,scr);
	}
	dbgprintf("scr->status = %d",scr->status);
	switch	(scr->status) {
	  case	APL_SESSION_NULL:
		fCont = FALSE;
		break;
	  case	APL_SESSION_RESEND:
		WriteClient(fp,scr,TRUE);
		ON_IO_ERROR(fp,badio);
		fCont = TRUE;
		break;
	  case	APL_SESSION_END:
		WriteClient(fp,scr,TRUE);
		ON_IO_ERROR(fp,badio);
		fCont = FALSE;
		break;
	  default:
		WriteClient(fp,scr,TRUE);
		ON_IO_ERROR(fp,badio);
		fCont = TRUE;
		break;
	}
  badio:
LEAVE_FUNC;
	return	(fCont);
}

static	ScreenData	*
NewSession(
	NETFILE	*fp)
{
	char	user[SIZE_USER+1]
		,	cmd[SIZE_LONGNAME+1];
	ScreenData	*scr;

ENTER_FUNC;
	scr = NULL;
	RecvnString(fp,SIZE_LONGNAME,cmd);	/*	command		*/	ON_IO_ERROR(fp,badio);
	RecvnString(fp,SIZE_USER,user);		/*	user		*/	ON_IO_ERROR(fp,badio);
	dbgprintf("user = [%s]",user);
	dbgprintf("cmd  = [%s]",cmd);
	PutLog(user,cmd);
	cServer ++;
	if		(  ( scr = InitSession() )  !=  NULL  ) {
		strcpy(scr->cmd,cmd);
		strcpy(scr->user,user);
		strcpy(scr->term,TermName(0));
		RecvScreenData(fp,scr);								ON_IO_ERROR(fp,badio);
	}
  badio:
LEAVE_FUNC;
	return	(scr);
}

static	void
ChildProcess(
	int		_fd)
{
	int		fd;
	NETFILE	*fp;
	PacketClass	klass;
	ScreenData	*scr;
	char	buff[SIZE_LARGE_BUFF];

ENTER_FUNC;
	while	(TRUE) {
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			printf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}
		fp = SocketToNet(fd);
		klass = RecvPacketClass(fp);
		scr = NULL;
		switch	(klass) {
		  case	GL_Connect:
			if		(  ( scr = NewSession(fp) )  !=  NULL  ) {
				ApplicationsCall(APL_SESSION_LINK,scr);
				SendPacketClass(fp,GL_Session);		ON_IO_ERROR(fp,badio);
				SendString(fp,scr->term);			ON_IO_ERROR(fp,badio);
				WriteClient(fp,scr,TRUE);			ON_IO_ERROR(fp,badio);	
				SaveScreenData(scr,TRUE);
			}
			break;
		  case	GL_Session:
			RecvString(fp,buff);			ON_IO_ERROR(fp,badio);
			dbgprintf("sesid = [%s]",buff);
			if		(  ( scr = LoadScreenData(buff) )  !=  NULL  ) {
				if		(  ExecSingle(fp,scr)  ) {
					dbgmsg("save");
					SaveScreenData(scr,TRUE);
				} else {
					dbgmsg("parge");
					PargeScreenData(scr);
				}
			}
			break;
		  default:
			break;
		}
	  badio:	
		if		(  scr  !=  NULL  ) {
			FreeScreenData(scr);
		}
		CloseNet(fp);
	}
LEAVE_FUNC;
}

static	void
CountDown(void)
{
	pid_t	pid;

	while	(  ( pid = waitpid(-1,NULL,WNOHANG) )  > 0  ) {
		cServer --;
		dbgprintf("server count = %d",cServer);
	}
}

extern	void
ExecuteServer(void)
{
	int		_fd;
	Port	*port;
	pid_t	pid;

ENTER_FUNC;
	signal(SIGPIPE,SIG_IGN);

	port = ParPortName(PortNumber);
	_fd = InitServerPort(port,Back);
	while	(TRUE) {
		if		(  cServer  <  nFork  ) {
			if		(  ( pid = fork() )  >  0  ) {
				cServer ++;
			} else
			if		(  pid  ==  0  ) {
				ChildProcess(_fd);
			}
		} else {
			CountDown();
			sleep(1);
		}
	}
	DestroyPort(port);
LEAVE_FUNC;
}

static	void
InitData(void)
{
ENTER_FUNC;
	RecParserInit();
	BlobCacheCleanUp();
	cServer = 1;
LEAVE_FUNC;
}

extern	void
InitSystem(
	int		argc,
	char	**argv)
{
ENTER_FUNC;
	InitNET();
	InitData();
	ApplicationsInit(argc,argv);
LEAVE_FUNC;
}
