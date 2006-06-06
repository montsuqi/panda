/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
#include	"comm.h"
#include	"comms.h"
#include	"authstub.h"
#include	"queue.h"
#include	"applications.h"
#include	"driver.h"
#include	"htserver.h"
#include	"dirs.h"
#include	"RecParser.h"
#include	"trid.h"
#include	"front.h"
#include	"term.h"
#include	"debug.h"

static	GHashTable	*SesHash;
static	int			cSession;
static	int			cServer;

typedef	struct {
	int			ses;
	ScreenData	*scr;
	Queue		*que;
}	SessionParameter;

typedef	struct	{
	int		sock;
	int		ses;
	int		count;
	int		pid;
	pthread_t	thr;
	Queue		*que;
}	HTC_Node;

static	void
_FinishSession(
	ScreenData	*scr)
{
	char	msg[128];
ENTER_FUNC;
	sprintf(msg,"[%s] session end",scr->user);
	MessageLog(msg);
	cServer --;
LEAVE_FUNC;
}

static	void
FinishSession(
	ScreenData	*scr)
{
ENTER_FUNC;
	_FinishSession(scr);
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
			&&	(  *p  !=  '.'   ) )	p ++;
	*p = 0;
	p ++;
	while	(  isspace(*p)  )	p ++;
	*vname = p;
	if		(  *p  !=  0  ) {
		while	(	(  *p  !=  0     )
				&&	(  !isspace(*p)  ) )	p ++;
	}
	*p = 0;
}

static	void
SendWindowName(
	NETFILE		*fp,
	ScreenData	*scr)
{
	void
	SendWindow(
		char		*wname,
		WindowData	*win)
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
			win->PutType = SCREEN_NULL;
		}
	}
ENTER_FUNC;
	g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,NULL);
	SendPacketClass(fp,GL_END);
	if		(  !CheckNetFile(fp)  )		FinishSession(scr);
LEAVE_FUNC;
}

static	void
WriteClient(
	NETFILE		*fp,
	ScreenData	*scr)
{
    LargeByteString	*lbs;
	PacketClass	klass;
	char	buff[SIZE_BUFF+1]
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
	SendWindowName(fp,scr);
    lbs = NewLBS();
	while	(  ( klass = RecvPacketClass(fp) )  ==  GL_GetData  ) {
		ON_IO_ERROR(fp,badio);
		RecvString(fp,buff);
		ON_IO_ERROR(fp,badio);
#ifdef	DEBUG
		dbgprintf("name = [%s]",buff);
#endif
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
#ifdef	DEBUG
					printf("htserver -> mon\n");
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
				fprintf(stderr, "window [%s] not found\n", window);
			}
			SendLBS(fp, lbs);	ON_IO_ERROR(fp,badio);
		}
	}
  badio:
    FreeLBS(lbs);
LEAVE_FUNC;
}

static	void
RecvScreenData(
	NETFILE		*fp,
	ScreenData	*scr)
{
    char buff[SIZE_BUFF + 1];
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
    lbs = NewLBS();
	while	(  ( klass = RecvPacketClass(fp) )  ==  GL_ScreenData  ) {
		ON_IO_ERROR(fp,badio);
		RecvString(fp,buff);	ON_IO_ERROR(fp,badio);
		dbgprintf("name = [%s]",buff);
        DecodeName(&wname, &vname, buff);
        LBS_EmitStart(lbs);
        RecvLBS(fp, lbs);
        if		(  ( rec = g_hash_table_lookup(scr->Records, wname))  !=  NULL) {
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
	ScreenData	*scr,
	HTC_Node	*htc)
{
	char	trid[SIZE_SESID+1];
	PacketClass	klass;
	char	buff[SIZE_BUFF];
	Bool	fCont;

ENTER_FUNC;
	fCont = TRUE;
	EncodeTRID(trid,htc->ses,0);
	SendPacketClass(fp,GL_Session);		ON_IO_ERROR(fp,badio);
	SendString(fp,trid);				ON_IO_ERROR(fp,badio);
	switch	(klass = RecvPacketClass(fp)) {
	  case	GL_Event:
		RecvString(fp,buff);	/*	window	*/
		ON_IO_ERROR(fp,badio);
		strncpy(scr->window,buff,SIZE_NAME);
		RecvString(fp,buff);	/*	widget	*/
		ON_IO_ERROR(fp,badio);
		strncpy(scr->widget,buff,SIZE_NAME);
		RecvString(fp,buff);	/*	event	*/
		ON_IO_ERROR(fp,badio);
		strncpy(scr->event,buff,SIZE_EVENT);
		dbgprintf("window = [%s]\n",scr->window);
		dbgprintf("event  = [%s]\n",scr->event);
		dbgprintf("user   = [%s]\n",scr->user);
		dbgprintf("cmd    = [%s]\n",scr->cmd);
		RecvScreenData(fp,scr);				ON_IO_ERROR(fp,badio);
		ApplicationsCall(APL_SESSION_GET,scr);
		break;
	  case	GL_ScreenData:
		/*	fatal error	*/
		scr->status = APL_SESSION_RESEND;
		break;
	  case	GL_END:
		scr->status = APL_SESSION_NULL;
		break;
	  default:
		Warning("invalid class = %X\n",klass);
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
		fCont = TRUE;
		break;
	  case	APL_SESSION_END:
		WriteClient(fp,scr);
		fCont = FALSE;
		break;
	  default:
		WriteClient(fp,scr);
		break;
	}
  badio:
LEAVE_FUNC;
	return	(fCont);
}

static	void
SesThread(
	SessionParameter	*ses)
{
	ScreenData	*scr = ses->scr;
	NETFILE	*fp;
	Bool	fCont;
	HTC_Node	*htc;

ENTER_FUNC;
	if		(  (  htc = g_int_hash_table_lookup(SesHash,ses->ses) )  !=  NULL  ) {
		htc->ses = ses->ses;
		do {
			if		(  ( fp = (NETFILE *)DeQueueTime(ses->que,Expire*1000) )  !=  NULL  ) {
				dbgmsg("*");
				fCont = ExecSingle(fp,scr,htc);
				dbgmsg("*");
				CloseNet(fp);
			} else {
				dbgmsg("*");
				fCont = FALSE;
			}
		}	while	(  fCont  );
		FreeQueue(ses->que);
		_FinishSession(scr);
		g_hash_table_remove(SesHash,(void *)ses->ses);
		xfree(htc);
		xfree(ses);
	}
LEAVE_FUNC;
}

static int
RecvMessage(int sock, int *fd, void *data, size_t datalen)
{
    struct msghdr msg;
    struct iovec vec[2];
    int len;

    struct {
        struct cmsghdr hdr;
        int fd;
    } cmsg;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    vec[0].iov_base = data;
    vec[0].iov_len = datalen;
    msg.msg_iov = vec;
    msg.msg_iovlen = 1;

    msg.msg_control = (caddr_t)&cmsg;
    msg.msg_controllen = sizeof(struct cmsghdr) + sizeof(int);
    msg.msg_flags = 0;
    cmsg.hdr.cmsg_len = sizeof(struct cmsghdr) + sizeof(int);
    cmsg.hdr.cmsg_level = SOL_SOCKET;
    cmsg.hdr.cmsg_type = SCM_RIGHTS;
    cmsg.fd = -1;

    len = recvmsg(sock, &msg, 0);
    *fd = cmsg.fd;
    return len;
}

static	void
SesServer(
	ScreenData	*scr,
	int			sock)
{
	int		fd;
	NETFILE	*fp;
	fd_set		ready;
	struct	timeval	timeout;
	Bool	fCont;
	HTC_Node	htc;

ENTER_FUNC;
	do {
		FD_ZERO(&ready);
		FD_SET(sock,&ready);
		timeout.tv_sec = Expire;
		timeout.tv_usec = 0;
		select(sock+1,&ready,NULL,NULL,&timeout);
		if		(  FD_ISSET(sock,&ready)  ) {
			if		(  RecvMessage(sock, &fd, &htc, sizeof(HTC_Node))  <=  0  )	{
                Error("recvmsg(2) failed");
                exit(1);
            }
			dbgmsg("session");
            fp = SocketToNet(fd);
			fCont = ExecSingle(fp,scr,&htc);
			CloseNet(fp);
		} else {
			fCont = FALSE;
		}
	}	while	(  fCont  );
	close(sock);
LEAVE_FUNC;
}

static	void
ChildProcess(
	NETFILE				*fp,
	int					sock,
	SessionParameter	*ses)
{
	int		sesid = ses->ses;
	ScreenData	*scr = ses->scr;
	char	trid[SIZE_SESID+1];

ENTER_FUNC;
	strcpy(scr->term,TermName(0));
	ApplicationsCall(APL_SESSION_LINK,scr);
	if		(  scr->status  ==  APL_SESSION_NULL  ) {
		SendPacketClass(fp,GL_E_APPL);
		printf("reject client(program name error)\n");
		CloseNet(fp);
	} else {
		EncodeTRID(trid,sesid,0);
		SendPacketClass(fp,GL_Session);
		SendString(fp,trid);
		WriteClient(fp,scr);
		CloseNet(fp);
		SesServer(scr,sock);
	}
	FinishSession(scr);
LEAVE_FUNC;
}

static	void
PutLog(
	char	*user,
	char	*cmd,
	int		sesno)
{
	char	buff[128];

	sprintf(buff,"%08d [%s] [%s] session start(%d)",sesno,user,cmd,cServer);
	MessageLog(buff);
}

static	Bool
NewSession(
	NETFILE	*fp)
{
	char	buff[SIZE_BUFF];
	HTC_Node	*htc;
	int		pid;
	int		socks[2];
	char	user[SIZE_USER+1]
		,	cmd[SIZE_LONGNAME+1];
	PacketClass	klass;
	Bool	rc;
	ScreenData	*scr;
	SessionParameter	*ses;
	pthread_t	thr;
	char	trid[SIZE_SESID+1];
	pthread_attr_t	attr;
	size_t	ssize;

ENTER_FUNC;
	RecvString(fp,buff);	/*	command		*/
	strncpy(cmd,buff,SIZE_LONGNAME);
	if		(  ( klass = RecvPacketClass(fp) )  ==  GL_Name  ) {
		RecvString(fp,buff);	/*	user	*/
		strncpy(user,buff,SIZE_USER);
		PutLog(user,cmd,cSession);
		cServer ++;
		rc = TRUE;
		scr = InitSession();
		strcpy(scr->cmd,cmd);
		strcpy(scr->user,user);
#ifdef	DEBUG
		printf("user = [%s]\n",user);
		printf("cmd  = [%s]\n",cmd);
#endif
		scr->Windows = NULL;
		ses = New(SessionParameter);
		ses->ses = cSession;
		ses->scr = scr;
		htc = New(HTC_Node);
		htc->count = 0;
		htc->ses = cSession;

		if		(  fThread  ) {
			ApplicationsCall(APL_SESSION_LINK,scr);
			if		(  scr->status  ==  APL_SESSION_NULL  ) {
				SendPacketClass(fp,GL_E_APPL);
			} else {
				strcpy(scr->term,TermName(0));
				EncodeTRID(trid,ses->ses,0);
				SendPacketClass(fp,GL_Session);
				SendString(fp,trid);
				WriteClient(fp,scr);
				ses->que = NewQueue();
				pthread_attr_init(&attr);
				pthread_attr_setstacksize(&attr,1024*1024);
				pthread_create(&thr,&attr,(void *(*)(void *))SesThread,ses);
				pthread_detach(thr);
				htc->thr = thr;
				htc->que = ses->que;
				htc->sock = 0;
				htc->pid = pid;
			}
		} else {
			if		(  socketpair(PF_UNIX, SOCK_STREAM, 0, socks)  !=  0  ) {
				fprintf(stderr,"make unix domain socket fail.\n");
				exit(1);
			}
			if		(  ( pid = fork() )  ==  0  ) {
				close(socks[0]);
				ChildProcess(fp,socks[1],ses);
			} else {
				close(socks[1]);
				htc->sock = socks[0];
				htc->pid = pid;
				htc->thr = (pthread_t)0;
				htc->que = NULL;
			}
		}
		g_int_hash_table_insert(SesHash,htc->ses,htc);
		cSession += (rand()>>16)+1;		/*	some random number	*/
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc);
}

static int
SendMessage(int sock, int fd, void *data, size_t datalen)
{
	struct msghdr msg;
	struct iovec vec[1];

	struct {
		struct cmsghdr hdr;
		int fd;
	}	cmsg;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	vec[0].iov_base = data;
	vec[0].iov_len = datalen;
	msg.msg_iov = vec;
	msg.msg_iovlen = 1;

	msg.msg_control = (caddr_t)&cmsg;
	msg.msg_controllen = sizeof(struct cmsghdr) + sizeof(int);
	msg.msg_flags = 0;
	cmsg.hdr.cmsg_len = sizeof(struct cmsghdr) + sizeof(int);
	cmsg.hdr.cmsg_level = SOL_SOCKET;
	cmsg.hdr.cmsg_type = SCM_RIGHTS;
	cmsg.fd = fd;

	return sendmsg(sock, &msg, 0);
}

static	void
CountDown(void)
{
	int		status;
ENTER_FUNC;
	wait(&status);
	cServer --;
	printf("server count = %d\n",cServer);
LEAVE_FUNC;
}

extern	void
ExecuteServer(void)
{
	int		fd
	,		_fd;
	NETFILE	*fp;
	char	buff[SIZE_BUFF+1];
	int			ses
	,			count;
	HTC_Node	*htc;
	Port	*port;
	PacketClass	klass;
	Bool	fExit;
	int		rc;

ENTER_FUNC;
	signal(SIGCHLD,(void *)CountDown);
	signal(SIGPIPE,SIG_IGN);

	port = ParPortName(PortNumber);
	_fd = InitServerPort(port,Back);
	fExit = FALSE;
	while	(!fExit)	{
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			printf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}
		fp = SocketToNet(fd);
		klass = RecvPacketClass(fp);	ON_IO_ERROR(fp,badio);
		switch	(klass) {
		  case	GL_Connect:
			dbgmsg("GL_Connect");
			NewSession(fp);
			break;
		  case	GL_Session:
			dbgmsg("GL_Session");
			RecvString(fp,buff);			ON_IO_ERROR(fp,badio);
			dbgprintf("recv trid [%s]\n",buff);
			DecodeTRID(&ses,&count,buff);
			if		(  (  htc = g_int_hash_table_lookup(SesHash,ses) )  !=  NULL  ) {
				htc->count = count;
				if		(  fThread  ) {
					if		(  EnQueue(htc->que,fp)  ) {
						fp = NULL;
						rc = 1;
					} else {
						rc = -1;
					}
				} else {
					rc = SendMessage(htc->sock,fd,htc,sizeof(HTC_Node));
				}
				if		(  rc  <  0  ) {
					if		(  htc->que  !=  NULL  ) {
						FreeQueue(htc->que);
					}
					g_hash_table_remove(SesHash,(void *)ses);
					xfree(htc);
					SendPacketClass(fp,GL_E_Session);	ON_IO_ERROR(fp,badio);
				}
			}
			break;
		  default:
			dbgprintf("klass = %02X",klass);
			dbgmsg("*");
			break;
		}
	  badio:
		if		(  fp  !=  NULL  ) {
			CloseNet(fp);
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
	SesHash = NewIntHash();
	cServer = 1;
#ifndef	DEBUG
	cSession = abs(rand());	/*	set some random number	*/
#endif
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
