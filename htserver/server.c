/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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
#include	"DDparser.h"
#include	"trid.h"
#include	"front.h"
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
dbgmsg(">FinishSession");
	sprintf(msg,"[%s] session end",scr->user);
	MessageLog(msg);
dbgmsg("<FinishSession");
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
		WindowData	*win,
		NETFILE		*fp)
	{
		if		(  win->PutType  !=  SCREEN_NULL  ) {
			switch	(win->PutType) {
			  case	SCREEN_CURRENT_WINDOW:
			  case	SCREEN_NEW_WINDOW:
			  case	SCREEN_CHANGE_WINDOW:
				SendStringDelim(fp,"Window: ");
				SendStringDelim(fp,wname);
				SendStringDelim(fp,"\n");
				break;
			  default:
				break;
			}
			win->PutType = SCREEN_NULL;
		}
	}
dbgmsg(">SendWindowName");
	g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,(void *)fp);
	SendStringDelim(fp,"\n");
dbgmsg("<SendWindowName");
}

static	void
WriteClient(
	NETFILE		*fp,
	ScreenData	*scr)
{
	char	buff[SIZE_BUFF+1];
	char	*vname
	,		*wname;
	char	*str;
	WindowData	*win;
	ValueStruct	*value;
	char	*p;
    LargeByteString	*lbs;
	size_t	size;

dbgmsg(">WriteClient");
	SendWindowName(fp,scr);
    lbs = NewLBS();
	do {
		if		(  !RecvStringDelim(fp,SIZE_BUFF,buff)  )	break;
		if		(  ( p = strchr(buff,' ') )  !=  NULL  ) {
			*p = 0;
		}
		if		(  *buff  !=  0  ) {
			DecodeName(&wname,&vname,buff);
            LBS_ReserveSize(lbs, 0, FALSE);
			if		(  ( win = g_hash_table_lookup(scr->Windows,wname) )  !=  NULL  ) {
				value = GetItemLongName(win->rec->value,vname);
                if (value == NULL) {
                    fprintf(stderr, "no ValueStruct: %s.%s\n", wname, vname);
                }
                else if (ValueType(value) == GL_TYPE_ARRAY ||
                         ValueType(value) == GL_TYPE_RECORD) {
                    fprintf(stderr, "can't send array or record: %s.%s\n",
                            wname, vname);
                }
                else {
                    size = NativeSizeValue(NULL, value);
                    LBS_ReserveSize(lbs, size, FALSE);
                    NativePackValue(NULL, LBS_Body(lbs), value);
                    if		(	(  p  !=  NULL            )
							&&	(  !stricmp(p+1,"clear")  ) ) {
                        InitializeValue(value);
                    }
                }
            }
            SendLBS(fp, lbs);
		}
	}	while	(  *buff  !=  0  );
    FreeLBS(lbs);
dbgmsg("<WriteClient");
}

static	void
RecvScreenData(
	NETFILE		*fp,
	ScreenData	*scr)
{
    char buff[SIZE_BUFF + 1];
	char *vname, *wname;
	char *p;
	WindowData	*win;
	ValueStruct	*value;
    LargeByteString *lbs;

    lbs = NewLBS();
    while (RecvStringDelim(fp, SIZE_BUFF, buff) && *buff != '\0') {
        DecodeName(&wname, &vname, buff);
        LBS_EmitStart(lbs);
        RecvLBS(fp, lbs);
        if ((win = g_hash_table_lookup(scr->Windows, wname))  !=  NULL) {
            value = GetItemLongName(win->rec->value, vname);
            ValueIsUpdate(value);
            switch (ValueType(value)) {
              case GL_TYPE_ARRAY:
                {
                    char *p = LBS_Body(lbs), *pend = p + LBS_Size(lbs);
                    ValueStruct *v;
                    int i;

                    while (p < pend) {
                        i = atoi(p);
                        v = GetArrayItem(value, i);
                        SetValueBool(v, TRUE);
                        while (isdigit(*p))
                            p++;
                        if (*p == ',')
                            p++;
                    }
                }
                break;
              case GL_TYPE_BYTE:
              case GL_TYPE_BINARY:
                SetValueBinary(value, LBS_Body(lbs), LBS_Size(lbs));
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
    FreeLBS(lbs);
}

static int
RecvMessage(int sock, int *fd, void *data, size_t datalen)
{
    struct msghdr msg;
    struct iovec vec[2];
    char buf[1];
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
	int		sock)
{
	int		fd;
	NETFILE	*fp;
	char	buff[SIZE_BUFF+1];
	char	trid[SIZE_SESID+1];
	char	*p;
	HTC_Node	htc;
	fd_set		ready;
	struct	timeval	timeout;
    int len;

dbgmsg(">SesServer");
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
			htc.count += 1;
			EncodeTRID(trid,htc.ses,htc.count);
			SendStringDelim(fp,trid);
			SendStringDelim(fp,"\n");
			RecvStringDelim(fp,SIZE_BUFF,buff);
dbgprintf("buff = [%s]\n",buff);
			if		(  *buff  ==  0  )	{
				break;
			} else {
				if		(  ( p = strchr(buff,':') )  !=  NULL  ) {
					*p = 0;
					strcpy(scr->event,buff);
					strcpy(scr->widget,p+1);
				} else {
					strcpy(scr->widget,"");
					strcpy(scr->event,buff);
				}
printf("event  = [%s]\n",scr->event);
printf("widget = [%s]\n",scr->widget);
				RecvScreenData(fp,scr);
				ApplicationsCall(APL_SESSION_GET,scr);
				while	(  scr->status  ==  APL_SESSION_LINK  ) {
					ApplicationsCall(scr->status,scr);
				}
			}
			if		(  scr->status  !=  APL_SESSION_NULL  ) {
				WriteClient(fp,scr);
			}
			CloseNet(fp);
		}
	}	while	(  FD_ISSET(sock,&ready)  );
	close(sock);
dbgmsg("<SesServer");
}

static	void
ChildProcess(
	NETFILE	*fp,
	int		sock,
	int		sesid,
	char	*user,
	char	*cmd)
{
	Bool	fOk;
	ScreenData	*scr;
	char	trid[SIZE_SESID+1];

dbgmsg(">ChildProcess");
	
	scr = InitSession();
	strcpy(scr->cmd,cmd);
	strcpy(scr->user,user);
#ifdef	DEBUG
	printf("user = [%s]\n",user);
	printf("cmd  = [%s]\n",cmd);
#endif
	scr->Windows = NULL;
	ApplicationsCall(APL_SESSION_LINK,scr);
	if		(  scr->status  ==  APL_SESSION_NULL  ) {
		SendStringDelim(fp,"900 invalid program\n");
		printf("reject client(program name error)\n");
		fOk = FALSE;
	} else {
		fOk = TRUE;
	}
dbgmsg("*");
	if		(  fOk  ) {
		EncodeTRID(trid,sesid,0);
dbgprintf("Child<< [%s]\n",trid);
		SendStringDelim(fp,trid);
		SendStringDelim(fp,"\n");
		WriteClient(fp,scr);
		strcpy(scr->term,TermName(0));
		CloseNet(fp);
		SesServer(scr,sock);
		FinishSession(scr);
	} else {
		CloseNet(fp);
	}
dbgmsg("<ChildProcess");
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

static	void
NewSession(
	NETFILE	*fp,
	char	*arg)
{
	HTC_Node	*htc;
	int		pid;
	int		socks[2];
	char	*user
	,		*cmd;
	char	*p;

dbgmsg(">NewSession");
	if		(  socketpair(PF_UNIX, SOCK_STREAM, 0, socks)  !=  0  ) {
		fprintf(stderr,"make unix domain socket fail.\n");
		exit(1);
	}
	if		(  *arg  ==  0  ) {
		user = "";
		cmd = "";
	} else 
	if		(  ( p = strchr(arg,'\t') )  !=  NULL  ) {
		*p = 0;
		cmd = arg;
		p ++;
		user = p;
	} else {
		cmd = arg;
		user = "";
	}
	PutLog(user,cmd,cSession);
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
dbgmsg("<NewSession");
}

static int
SendMessage(int sock, int fd, void *data, size_t datalen)
{
    struct msghdr msg;
    struct iovec vec[1];
    char buf[1];

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

dbgmsg(">ExecuteServer");
	signal(SIGCHLD,SIG_IGN);
	signal(SIGPIPE,SIG_IGN);

	port = ParPortName(PortNumber);
	_fd = InitServerPort(port,Back);
	while	(TRUE)	{
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			printf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}
		fp = SocketToNet(fd);
		RecvStringDelim(fp,SIZE_BUFF,buff);
dbgprintf(">> [%s]\n",buff);
		if		(  strncmp(buff,"Start:",6)  ==  0  ) {
			NewSession(fp,buff+7);
		} else
		if		(  strncmp(buff,"Session:",8)  ==  0  ) {
			DecodeTRID(&ses,&count,buff+9);
			if		(  (  htc = g_int_hash_table_lookup(SesHash,ses) )  !=  NULL  ) {
				htc->count = count;
				if		(  SendMessage(htc->sock,fd,htc,sizeof(HTC_Node))  <  0  ) {
					EncodeTRID(buff,0,0);
					SendStringDelim(fp,buff);
					SendStringDelim(fp,"\n");
dbgprintf("<< [%s]\n",buff);
					xfree(htc);
					g_hash_table_remove(SesHash,(void *)ses);
				}
			}
		}
		CloseNet(fp);
	}
	DestroyPort(port);
dbgmsg("<ExecuteServer");
}

static	void
InitData(void)
{
dbgmsg(">InitData");
	DD_ParserInit();
	SesHash = NewIntHash();
	cSession = abs(rand());	/*	set some random number	*/
dbgmsg("<InitData");
}

extern	void
InitSystem(
	int		argc,
	char	**argv)
{
dbgmsg(">InitSystem");
	InitNET();
	InitData();
	ApplicationsInit(argc,argv);
dbgmsg("<InitSystem");
}
