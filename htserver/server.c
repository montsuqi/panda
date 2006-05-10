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

#include	"types.h"
#include	"enum.h"
#include	"socket.h"
#include	"net.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"comm.h"
#include	"comms.h"
#include	"authstub.h"
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

typedef	struct	{
	int		sock;
	int		ses;
	int		count;
	int		pid;
}	HTC_Node;

static	void
FinishSession(
	ScreenData	*scr)
{
	char	msg[128];
ENTER_FUNC;
	sprintf(msg,"[%s] session end",scr->user);
	MessageLog(msg);
LEAVE_FUNC;
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
			  default:
				break;
			}
			win->PutType = SCREEN_NULL;
		}
	}
ENTER_FUNC;
	g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,NULL);
	SendPacketClass(fp,GL_END);
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
		RecvString(fp,buff);
		dbgprintf("name = [%s]",buff);
		fClear = RecvBool(fp);
		if		(  *buff  !=  0  ) {
			DecodeName(&wname,&vname,buff);
			LBS_EmitStart(lbs);
			PureWindowName(wname,window);
			dbgprintf("window = [%s]",window);
			dbgprintf("wname  = [%s]",wname);
			dbgprintf("vname  = [%s]",vname);
			if		(  ( rec = g_hash_table_lookup(scr->Records,window) )  !=  NULL  ) {
				if		(  *vname  ==  0  ) {
					value = rec->value;
				} else {
					value = GetItemLongName(rec->value,vname);
				}
				if (value == NULL) {
					fprintf(stderr, "no ValueStruct: %s.%s\n", window, vname);
				} else {
					size = NativeSaveSize(value,TRUE);
					LBS_ReserveSize(lbs,size,FALSE);
					NativeSaveValue(LBS_Body(lbs),value,TRUE);
					if		(  ValueType(value)  ==  GL_TYPE_OBJECT  ) {
						if		(  ( fpf = Fopen(BlobCacheFileName(value),"r") )
								   !=  NULL  ) {
							dbgmsg("blob body");
							SendLBS(fp,lbs);
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
			SendLBS(fp, lbs);
		}
	}
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
		RecvString(fp,buff);
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
    FreeLBS(lbs);
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
	char	buff[SIZE_BUFF];
	char	trid[SIZE_SESID+1];
	HTC_Node	htc;
	fd_set		ready;
	struct	timeval	timeout;
	int		sts;
	PacketClass	klass;

ENTER_FUNC;
	do {
		FD_ZERO(&ready);
		FD_SET(sock,&ready);
		timeout.tv_sec = Expire;
		timeout.tv_usec = 0;
		select(sock+1,&ready,NULL,NULL,&timeout);
		if		(  FD_ISSET(sock,&ready)  ) {
            if (RecvMessage(sock, &fd, &htc, sizeof(HTC_Node)) == -1) {
                Error("recvmsg(2) failed");
                exit(1);
            }
			dbgmsg("session");
            fp = SocketToNet(fd);
			EncodeTRID(trid,htc.ses,0);
			SendPacketClass(fp,GL_Session);
			SendString(fp,trid);
			if		(  ( klass = RecvPacketClass(fp) )  ==  GL_Event  ) {
				RecvString(fp,buff);	/*	window	*/
				strncpy(scr->window,buff,SIZE_NAME);
				RecvString(fp,buff);	/*	widget	*/
				strncpy(scr->widget,buff,SIZE_NAME);
				RecvString(fp,buff);	/*	widget	*/
				strncpy(scr->event,buff,SIZE_EVENT);
				sts = APL_SESSION_GET;
				RecvScreenData(fp,scr);
				dbgprintf("user = [%s]\n",scr->user);
				dbgprintf("cmd  = [%s]\n",scr->cmd);
				ApplicationsCall(sts,scr);
				while	(  scr->status  ==  APL_SESSION_LINK  ) {
					ApplicationsCall(scr->status,scr);
				}
				if		(  scr->status  !=  APL_SESSION_NULL  ) {
					WriteClient(fp,scr);
				}
			}
			CloseNet(fp);
		}
	}	while	(  FD_ISSET(sock,&ready)  );
	close(sock);
LEAVE_FUNC;
}

static	void
ChildProcess(
	NETFILE	*fp,
	int		sock,
	int		sesid,
	char	*user,
	char	*cmd)
{
	ScreenData	*scr;
	char	trid[SIZE_SESID+1];

ENTER_FUNC;
	scr = InitSession();
	strcpy(scr->cmd,cmd);
	strcpy(scr->user,user);
#ifdef	DEBUG
	printf("user = [%s]\n",user);
	printf("cmd  = [%s]\n",cmd);
#endif
	scr->Windows = NULL;
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
		FinishSession(scr);
	}
LEAVE_FUNC;
}

static	void
PutLog(
	char	*user,
	char	*cmd,
	int		sesno)
{
	char	buff[128];

	sprintf(buff,"%08d [%s] [%s] session start",sesno,user,cmd);
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

ENTER_FUNC;
	if		(  socketpair(PF_UNIX, SOCK_STREAM, 0, socks)  !=  0  ) {
		fprintf(stderr,"make unix domain socket fail.\n");
		exit(1);
	}
	RecvString(fp,buff);	/*	command		*/
	strncpy(cmd,buff,SIZE_LONGNAME);
	if		(  ( klass = RecvPacketClass(fp) )  ==  GL_Name  ) {
		RecvString(fp,buff);	/*	user	*/
		strncpy(user,buff,SIZE_USER);
		PutLog(user,cmd,cSession);
		rc = TRUE;
		if		(  ( pid = fork() )  ==  0  ) {
			close(socks[0]);
			ChildProcess(fp,socks[1],cSession,user,cmd);
			exit(0);
		} else {
			close(socks[1]);
			htc = New(HTC_Node);
			htc->sock = socks[0];
			htc->pid = pid;
			htc->ses = cSession;
			htc->count = 0;
			g_int_hash_table_insert(SesHash,htc->ses,htc);
			cSession += (rand()>>16)+1;		/*	some random number	*/
		}
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

ENTER_FUNC;
	signal(SIGCHLD,SIG_IGN);
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
		klass = RecvPacketClass(fp);
		switch	(klass) {
		  case	GL_Connect:
			if		(  !NewSession(fp)  )	fExit = TRUE;
			break;
		  case	GL_Session:
			RecvString(fp,buff);
			dbgprintf("recv trid [%s]\n",buff);
			DecodeTRID(&ses,&count,buff);
			if		(  (  htc = g_int_hash_table_lookup(SesHash,ses) )  !=  NULL  ) {
				htc->count = count;
				if		(  SendMessage(htc->sock,fd,htc,sizeof(HTC_Node))  <  0  ) {
					EncodeTRID(buff,0,0);
					SendPacketClass(fp,GL_Session);
					SendString(fp,buff);
					dbgprintf("send trid [%s]\n",buff);
					xfree(htc);
					g_hash_table_remove(SesHash,(void *)ses);
				}
			}
			break;
		  default:
			fExit = TRUE;
			break;
		}
		CloseNet(fp);
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
