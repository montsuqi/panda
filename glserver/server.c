/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<setjmp.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"authstub.h"
#include	"applications.h"
#include	"driver.h"
#include	"glserver.h"
#include	"glcomm.h"
#include	"front.h"
#include	"dirs.h"
#include	"DDparser.h"
#include	"message.h"
#include	"debug.h"

static	void
FinishSession(
	ScreenData	*scr)
{
	char	msg[SIZE_BUFF];

	sprintf(msg,"[%s@%s] session end",scr->user,scr->term);
	MessageLog(msg);
}

static	Bool
CheckCache(
	NETFILE	*fpComm,
	char	*name,
	off_t	stsize,
	time_t	stmtime,
	time_t	stctime)
{
	Bool	ret;
	int		klass;

ENTER_FUNC;
	GL_SendPacketClass(fpComm,GL_QueryScreen,fFetureNetwork);
	ON_IO_ERROR(fpComm,badio);
	GL_SendString(fpComm,name,fFetureNetwork);			ON_IO_ERROR(fpComm,badio);
	GL_SendLong(fpComm,(long)stsize,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	GL_SendLong(fpComm,(long)stmtime,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	GL_SendLong(fpComm,(long)stctime,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	switch	(  klass = GL_RecvPacketClass(fpComm,fFetureNetwork)  ) {
	  case	GL_GetScreen:
		dbgmsg("GetScreen");
		ret = TRUE;
		break;
	  case	GL_NOT:
		dbgmsg("Not");
		ret = FALSE;
		break;
	  default:
		printf("klass = [%d]\n",klass);
	  badio:
		dbgmsg("error");
		ret = FALSE;
		break;
	}
LEAVE_FUNC;
	return	(ret);
}
	
static	Bool
SendFile(
	NETFILE	*fpComm,
	char	*fname,
	char	*wname)
{
	char	buff[SIZE_BUFF];
	struct	stat	stbuf;
	size_t	size
	,		left;
	FILE	*fp;
	Bool	rc;

ENTER_FUNC;
	stat(fname,&stbuf);
	if		(  CheckCache(fpComm,
						  wname,
						  stbuf.st_size, stbuf.st_mtime, stbuf.st_ctime)  ) {
		rc = FALSE;
		GL_RecvString(fpComm,wname,fFetureNetwork);	/*	dummy	*/
		if		(  ( fp = fopen(fname,"r") )  !=  NULL  ) {
			GL_SendPacketClass(fpComm,GL_ScreenDefine,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			GL_SendLong(fpComm,(long)stbuf.st_size,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			left = stbuf.st_size;
			do {
				if		(  left  >  SIZE_BUFF  ) {
					size = SIZE_BUFF;
				} else {
					size = left;
				}
				size = fread(buff,1,size,fp);
				if		(  size  >  0  ) {
					Send(fpComm,buff,size);			ON_IO_ERROR(fpComm,badio);
					left -= size;
				}
			}	while	(  left  >  0  );
			rc = TRUE;
		  badio:
			fclose(fp);
		}
	} else {
		rc = TRUE;
	}
LEAVE_FUNC;
	return	(rc);
}

static	jmp_buf	envCheckScreen;
static	void
CheckScreen(
	char		*wname,
	WindowData	*win,
	NETFILE		*fpComm)
{
	char	fname[SIZE_BUFF]
	,		dir[SIZE_BUFF];
	char	msg[SIZE_BUFF];
	struct	stat	stbuf;
	char	*p
	,		*q;
	Bool	fDone
	,		fExit;

ENTER_FUNC;
	if		(  win->fNew  ) {
		strcpy(dir,ScreenDir);
		p = dir;
		fDone = FALSE;
		do {
			if		(  ( q = strchr(p,':') )  !=  NULL  ) {
				*q = 0;
				fExit = FALSE;
			} else {
				fExit = TRUE;
			}
			sprintf(fname,"%s/%s.glade",p,wname);
			if		(  stat(fname,&stbuf)  ==  0  ) {
				if		(  ( fDone = SendFile(fpComm,fname,wname) )  )	break;
			}
			p = q + 1;
		}	while	(  !fExit  );
		if		(  !fDone  ) {
			sprintf(msg,"[%s] screen file not exitsts.",wname);
			MessageLog(msg);
			longjmp(envCheckScreen,1);
		}
		win->fNew = FALSE;
	}
LEAVE_FUNC;
}

static	void
CheckScreens(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
ENTER_FUNC;
	if		(  setjmp(envCheckScreen)  ==  0  ) {
		g_hash_table_foreach(scr->Windows,(GHFunc)CheckScreen,fpComm);
	}
	GL_SendPacketClass(fpComm,GL_END,fFetureNetwork);
LEAVE_FUNC;
}

static	jmp_buf	envSendWindow;
static	void
SendWindow(
	char		*wname,
	WindowData	*win,
	NETFILE		*fpComm)
{
	Bool	rc;
ENTER_FUNC;

	rc = FALSE; 
	if		(  win->PutType  !=  SCREEN_NULL  ) {
		GL_SendPacketClass(fpComm,GL_WindowName,fFetureNetwork);
		ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,wname,fFetureNetwork);		ON_IO_ERROR(fpComm,badio);
		dbgprintf("wname = [%s]\n",wname);
		GL_SendInt(fpComm,win->PutType,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
		switch	(win->PutType) {
		  case	SCREEN_CURRENT_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			if		(  win->rec->value  !=  NULL  ) {
				GL_SendPacketClass(fpComm,GL_ScreenData,fFetureNetwork);
				ON_IO_ERROR(fpComm,badio);
				if		(  fFetureI18N  ) {
					GL_SendValue(fpComm,win->rec->value,NULL,
								 fFetureBlob,fFetureExpand,fFetureNetwork);
					ON_IO_ERROR(fpComm,badio);
				} else {
					GL_SendValue(fpComm,win->rec->value,"euc-jp",
								 fFetureBlob,fFetureExpand,fFetureNetwork);
					ON_IO_ERROR(fpComm,badio);
				}
			} else {
				GL_SendPacketClass(fpComm,GL_NOT,fFetureNetwork);
				ON_IO_ERROR(fpComm,badio);
			}
			break;
		  default:
			GL_SendPacketClass(fpComm,GL_NOT,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			break;
		}
		win->PutType = SCREEN_NULL;
	}
	rc = TRUE;
  badio:
	if		(  !rc  )	longjmp(envSendWindow,1);
LEAVE_FUNC;
}

static	Bool
SendScreenAll(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	Bool	rc;
ENTER_FUNC;
	rc = FALSE;
	if		(  setjmp(envSendWindow)  ==  0  ) {
		g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,fpComm);
	} else {
		goto	badio;
	}
	if		(	(  *scr->window  !=  0  )
			&&	(  *scr->widget  !=  0  ) ) {
		GL_SendPacketClass(fpComm,GL_FocusName,fFetureNetwork);
		ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->window,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->widget,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	}
	GL_SendPacketClass(fpComm,GL_END,fFetureNetwork);		ON_IO_ERROR(fpComm,badio);
	rc = TRUE;
  badio:
LEAVE_FUNC;
	return	(rc); 
}

static	Bool
SendScreenData(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	Bool	rc;

ENTER_FUNC;
	if		(  GL_RecvPacketClass(fpComm,fFetureNetwork)  ==  GL_GetData  ) {
		if		(  GL_RecvInt(fpComm,fFetureNetwork)  ==  0  ) {	/*	get all data	*/
			dbgmsg("get all data");
			rc = SendScreenAll(fpComm,scr);
		} else {
			rc = TRUE;
			dbgmsg("get partial");
		}
		dbgmsg("*");
	} else {
		dbgmsg("*");
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc);
}

static	Bool
SendScreen(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;

ENTER_FUNC;
	CheckScreens(fpComm,scr);
	ret = SendScreenData(fpComm,scr);
	if		(  !ret  ) {
		printf("SendScreenData invalid\n");
	}
LEAVE_FUNC;
	return	(ret);
}

#ifdef	USE_SSL
static	Bool
ThisAuth(
	char	*user,
	char	*pass,
	char	*other)
{
	Bool	ret;

	if		(	(  fSsl     )
			&&	(  fVerify  ) ) {
		ret = TRUE;
	} else {
		ret = AuthUser(&Auth,user,pass,other);
	}
	return	(ret);
}
#else
#define	ThisAuth(user,pass,other)	AuthUser(&Auth,(user),(pass),(other))
#endif

static	void
CheckFeture(
	char	*ver)
{
	char	*p
		,	*q
		,	*n;

	TermFeture = FETURE_NULL;
	if		(  strlcmp(ver,"1.2")    ==  0  ) {
		TermFeture |= FETURE_CORE;
	} else
	  if	(	(  strlcmp(ver,"1.1.1")  ==  0  )
			||	(  strlcmp(ver,"1.1.2")  ==  0  ) ) {
		TermFeture |= FETURE_CORE;
		TermFeture |= FETURE_OLD;
	} else {
		TermFeture = FETURE_CORE;
		if		(  ( p = strchr(ver,':') )  !=  NULL  ) {
			p ++;
			while	(  *p  !=  0  ) {
				if		(  ( q = strchr(p,':') )  !=  NULL  ) {
					*q = 0;
					n = q + 1;
				} else {
					n = p + strlen(p);
				}
				if		(  !strlicmp(p,"blob")  ) {
					TermFeture |= FETURE_BLOB;
				}
				if		(  !strlicmp(p,"expand")  ) {
					TermFeture |= FETURE_EXPAND;
				}
				if		(  !strlicmp(p,"i18n")  ) {
					TermFeture |= FETURE_I18N;
				}
				if		(  !strlicmp(p,"no")  ) {
					TermFeture |= FETURE_NETWORK;
				}
				if		(  !strlicmp(p,"ps")  ) {
					TermFeture |= FETURE_PS;
				}
				if		(  !strlicmp(p,"pdf")  ) {
					TermFeture |= FETURE_PDF;
				}
				p = n;
			}
		}
	}
#ifdef	DEBUG
	printf("core      = %s\n",fFetureCore ? "YES" : "NO");
	printf("i18n      = %s\n",fFetureI18N ? "YES" : "NO");
	printf("blob      = %s\n",fFetureBlob ? "YES" : "NO");
	printf("expand    = %s\n",fFetureExpand ? "YES" : "NO");
	printf("network   = %s\n",fFetureNetwork ? "YES" : "NO");
	printf("ps        = %s\n",fFeturePS ? "YES" : "NO");
	printf("old       = %s\n",fFetureOld ? "YES" : "NO");
	printf("pdf       = %s\n",fFeturePDF ? "YES" : "NO");
#endif
}

static	void
Connect(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	char	pass[SIZE_PASS+1];
	char	ver[SIZE_BUFF];
	char	msg[SIZE_BUFF];

	GL_RecvString(fpComm,ver,FALSE);				ON_IO_ERROR(fpComm,badio);
	CheckFeture(ver);
	GL_RecvString(fpComm,scr->user,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm,pass,fFetureNetwork);		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm,scr->cmd,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	sprintf(msg,"[%s@%s] session start",scr->user,scr->term);
	MessageLog(msg);

	if		(  TermFeture  ==  FETURE_NULL  ) {
		GL_SendPacketClass(fpComm,GL_E_VERSION,fFetureNetwork);
		ON_IO_ERROR(fpComm,badio);
		g_warning("reject client(invalid version)");
	} else
	if		(  ThisAuth(scr->user,pass,scr->other)  ) {
		scr->Windows = NULL;
		ApplicationsCall(APL_SESSION_LINK,scr);
		if		(  scr->status  ==  APL_SESSION_NULL  ) {
			GL_SendPacketClass(fpComm,GL_E_APPL,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
		} else {
			GL_SendPacketClass(fpComm,GL_OK,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			CheckScreens(fpComm,scr);
		}
	} else {
		g_warning("reject client(authentication error)");
		GL_SendPacketClass(fpComm,GL_E_AUTH,fFetureNetwork);
		ON_IO_ERROR(fpComm,badio);
	}
  badio:;
}

static	Bool
RecvScreenData(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	int		c;
	char		wname[SIZE_NAME+1]
	,			name[SIZE_NAME+1];
	WindowData	*win;
	ValueStruct	*value;
	Bool		rc;
	char		*coding;

ENTER_FUNC;
	rc = FALSE;
	if		(  fFetureI18N  ) {
		coding = NULL;
	} else {
		coding = "euc-jp";
	}
	while	(  GL_RecvPacketClass(fpComm,fFetureNetwork)  ==  GL_WindowName  ) {
		ON_IO_ERROR(fpComm,badio);
		GL_RecvString(fpComm,wname,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
		if		(  ( win = g_hash_table_lookup(scr->Windows,wname) )  !=  NULL  ) {
			while	(  ( c = GL_RecvPacketClass(fpComm,fFetureNetwork) )
					   ==  GL_ScreenData  ) {
				ON_IO_ERROR(fpComm,badio);
				GL_RecvString(fpComm,name,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
				if		(  ( value = GetItemLongName(win->rec->value,name+strlen(wname)+1) )
						   !=  NULL  ) {
					GL_RecvValue(fpComm,value,coding,fFetureBlob,fFetureExpand,fFetureNetwork);
				} else {
					MessagePrintf("invalid item name [%s]\n",name);
					exit(1);
				}
			}
		} else {
			MessagePrintf("invalud wind name [%s]\n",wname);
			exit(1);
		}
	}
	rc = TRUE;
  badio:
LEAVE_FUNC;
	return	(rc);
}

static	Bool
MainLoop(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;

	PacketClass	klass;

ENTER_FUNC;
	klass = GL_RecvPacketClass(fpComm,fFetureNetwork); ON_IO_ERROR(fpComm,badio);
	dbgprintf("class = %d",(int)klass);
	if		(  klass  !=  GL_Null  ) {
		switch	(klass) {
		  case	GL_Connect:
			Connect(fpComm,scr);				ON_IO_ERROR(fpComm,badio);
			break;
		  case	GL_Name:
			GL_RecvString(fpComm,scr->term,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			break;
		  case	GL_Event:
			GL_RecvString(fpComm,scr->window,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			GL_RecvString(fpComm,scr->widget,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			GL_RecvString(fpComm,scr->event,fFetureNetwork);
			ON_IO_ERROR(fpComm,badio);
			dbgprintf("window = [%s]\n",scr->window);
			dbgprintf("event  = [%s]\n",scr->event);
			RecvScreenData(fpComm,scr);			ON_IO_ERROR(fpComm,badio);
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
			printf("invalid class = %X\n",klass);
			scr->status = APL_SESSION_NULL;
			break;
		}
		while	(  scr->status  ==  APL_SESSION_LINK  ) {
			ApplicationsCall(scr->status,scr);
		}
		switch	(scr->status) {
		  case	APL_SESSION_NULL:
			ret = FALSE;
			break;
		  case	APL_SESSION_RESEND:
			ret = TRUE;
			break;
		  default:
			ret = SendScreen(fpComm,scr);
			break;
		}
	} else {
	  badio:
		ret = FALSE;
		dbgmsg("Connection lost");
	}

LEAVE_FUNC;
	return	(ret);
}

extern	void
ExecuteServer(void)
{
	int		pid;
	int		fd
	,		_fd;
	NETFILE	*fpComm;
	ScreenData	*scr;
	Port	*port;
#ifdef	USE_SSL
	SSL_CTX	*ctx;
#endif
ENTER_FUNC;
	signal(SIGCHLD,SIG_IGN);

	port = ParPortName(PortNumber);
	_fd = InitServerPort(port,Back);
#ifdef	USE_SSL
	ctx = NULL;
	if		(  fSsl  ) {
		if		(  ( ctx = MakeCTX(KeyFile,CertFile,CA_File,CA_Path,fVerify) )
				   ==  NULL  ) {
			fprintf(stderr,"CTX make error\n");
			exit(1);
		}
	}
#endif
	while	(TRUE)	{
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			printf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}

		if		(  ( pid = fork() )  >  0  )	{	/*	parent	*/
			close(fd);
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
#ifdef	USE_SSL
			if		(  fSsl  ) {
				fpComm = MakeSSL_Net(ctx,fd);
				SSL_accept(NETFILE_SSL(fpComm));
			} else {
				fpComm = SocketToNet(fd);
			}
#else
			fpComm = SocketToNet(fd);
#endif
			close(_fd);
			scr = InitSession();
			strcpy(scr->term,TermName(fd));
			while	(  MainLoop(fpComm,scr)  );
			FinishSession(scr);
			CloseNet(fpComm);
			exit(0);
		}
	}
	DestroyPort(port);
LEAVE_FUNC;
}

static	void
InitData(void)
{
ENTER_FUNC;
	DD_ParserInit();
	BlobCacheCleanUp();
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
	InitGL_Comm();
	ApplicationsInit(argc,argv);
LEAVE_FUNC;
}
