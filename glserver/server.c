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
#ifdef	USE_SSL
#include	<openssl/crypto.h>
#include	<openssl/x509.h>
#include	<openssl/pem.h>
#include	<openssl/ssl.h>
#include	<openssl/err.h>
#endif

#include	"types.h"
#include	"enum.h"
#include	"misc.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"authstub.h"
#include	"applications.h"
#include	"driver.h"
#include	"glserver.h"
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

dbgmsg(">CheckCache");
	SendPacketClass(fpComm,GL_QueryScreen);	ON_IO_ERROR(fpComm,badio);
	SendString(fpComm,name);				ON_IO_ERROR(fpComm,badio);
	SendLong(fpComm,(long)stsize);			ON_IO_ERROR(fpComm,badio);
	SendLong(fpComm,(long)stmtime);			ON_IO_ERROR(fpComm,badio);
	SendLong(fpComm,(long)stctime);			ON_IO_ERROR(fpComm,badio);
	switch	(  klass = RecvPacketClass(fpComm)  ) {
	  case	GL_GetScreen:
		dbgmsg("GetScreen");
		ret = TRUE;
		break;
	  case	GL_NOT:
		dbgmsg("Not");
		ret = FALSE;
		break;
	  default:
		printf("kass = [%d]\n",klass);
	  badio:
		dbgmsg("error");
		ret = FALSE;
		break;
	}
dbgmsg("<CheckCache");
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

dbgmsg(">SendFile");
	stat(fname,&stbuf);
	if		(  CheckCache(fpComm,
						  wname,
						  stbuf.st_size, stbuf.st_mtime, stbuf.st_ctime)  ) {
		rc = FALSE;
		RecvString(fpComm,wname);	/*	dummy	*/
		if		(  ( fp = fopen(fname,"r") )  !=  NULL  ) {
			SendPacketClass(fpComm,GL_ScreenDefine);	ON_IO_ERROR(fpComm,badio);
			SendLong(fpComm,(long)stbuf.st_size);		ON_IO_ERROR(fpComm,badio);
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
dbgmsg("<SendFile");
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

dbgmsg(">CheckScreen");
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
				fDone = SendFile(fpComm,fname,wname);
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
dbgmsg("<CheckScreen");
}

static	void
CheckScreens(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
dbgmsg(">CheckScreens");
	if		(  setjmp(envCheckScreen)  ==  0  ) {
		g_hash_table_foreach(scr->Windows,(GHFunc)CheckScreen,fpComm);
	}
	SendPacketClass(fpComm,GL_END);
dbgmsg("<CheckScreens");
}

static	jmp_buf	envSendWindow;
static	void
SendWindow(
	char		*wname,
	WindowData	*win,
	NETFILE		*fpComm)
{
	Bool	rc;
dbgmsg(">SendWindow");

	rc = FALSE; 
	if		(  win->PutType  !=  SCREEN_NULL  ) {
		SendPacketClass(fpComm,GL_WindowName);	ON_IO_ERROR(fpComm,badio);
		SendString(fpComm,wname);				ON_IO_ERROR(fpComm,badio);
		SendInt(fpComm,win->PutType);			ON_IO_ERROR(fpComm,badio);
		switch	(win->PutType) {
		  case	SCREEN_CURRENT_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			if		(  win->rec->value  !=  NULL  ) {
				SendPacketClass(fpComm,GL_ScreenData);	ON_IO_ERROR(fpComm,badio);
				SendValue(fpComm,win->rec->value);		ON_IO_ERROR(fpComm,badio);
			} else {
				SendPacketClass(fpComm,GL_NOT);			ON_IO_ERROR(fpComm,badio);
			}
			break;
		  default:
			SendPacketClass(fpComm,GL_NOT);				ON_IO_ERROR(fpComm,badio);
			break;
		}
		win->PutType = SCREEN_NULL;
	}
	rc = TRUE;
  badio:
	if		(  !rc  )	longjmp(envSendWindow,1);
dbgmsg("<SendWindow");
}

static	Bool
SendScreenAll(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	Bool	rc;
dbgmsg(">SendScreenAll");
	rc = FALSE;
	if		(  setjmp(envSendWindow)  ==  0  ) {
		g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,fpComm);
	} else {
		goto	badio;
	}
	if		(	(  *scr->window  !=  0  )
			&&	(  *scr->widget  !=  0  ) ) {
		SendPacketClass(fpComm,GL_FocusName);	ON_IO_ERROR(fpComm,badio);
		SendString(fpComm,scr->window);			ON_IO_ERROR(fpComm,badio);
		SendString(fpComm,scr->widget);			ON_IO_ERROR(fpComm,badio);
	}
	SendPacketClass(fpComm,GL_END);			ON_IO_ERROR(fpComm,badio);
	rc = TRUE;
  badio:
dbgmsg("<SendScreenAll");
	return	(rc); 
}

static	Bool
SendScreenData(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	Bool	rc;

dbgmsg(">SendScreenData");
	if		(  RecvPacketClass(fpComm)  ==  GL_GetData  ) {
		if		(  RecvInt(fpComm)  ==  0  ) {	/*	get all data	*/
			dbgmsg("get all data");
			rc = SendScreenAll(fpComm,scr);
		} else {
			rc = TRUE;
			dbgmsg("get partial");
		}
	} else {
		rc = FALSE;
	}
dbgmsg("<SendScreenData");
	return	(rc);
}

static	Bool
RecvScreenData(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	int		c;
	char		wname[SIZE_NAME+1]
	,			name[SIZE_NAME+1];
	char		str[SIZE_BUFF];
	int			ival;
	Bool		bval;
	double		fval;
	Fixed		*xval;
	WindowData	*win;
	ValueStruct	*value;
	PacketDataType	type;
	Bool		rc;

dbgmsg(">RecvScreenData");
	rc = FALSE;
	while	(  RecvPacketClass(fpComm)  ==  GL_WindowName  ) {
		ON_IO_ERROR(fpComm,badio);
		RecvString(fpComm,wname);	ON_IO_ERROR(fpComm,badio);
		if		(  ( win = g_hash_table_lookup(scr->Windows,wname) )  !=  NULL  ) {
			while	(  ( c = RecvPacketClass(fpComm) )  ==  GL_ScreenData  ) {
				ON_IO_ERROR(fpComm,badio);
				RecvString(fpComm,name);		ON_IO_ERROR(fpComm,badio);
				if		(  ( value = GetItemLongName(win->rec->value,name+strlen(wname)+1) )
						   !=  NULL  ) {
					value->fUpdate = TRUE;
					type = RecvDataType(fpComm);	ON_IO_ERROR(fpComm,badio);
					switch	(type)	{
					  case	GL_TYPE_CHAR:
					  case	GL_TYPE_VARCHAR:
					  case	GL_TYPE_DBCODE:
					  case	GL_TYPE_TEXT:
						RecvString(fpComm,str);		ON_IO_ERROR(fpComm,badio);
						SetValueString(value,str);
						break;
					  case	GL_TYPE_NUMBER:
						xval = RecvFixed(fpComm);	ON_IO_ERROR(fpComm,badio);
						SetValueFixed(value,xval);
						xfree(xval->sval);
						xfree(xval);
						break;
					  case	GL_TYPE_INT:
						ival = RecvInt(fpComm);		ON_IO_ERROR(fpComm,badio);
						SetValueInteger(value,ival);
						break;
					  case	GL_TYPE_FLOAT:
						fval = RecvFloat(fpComm);	ON_IO_ERROR(fpComm,badio);
						SetValueFloat(value,fval);
						break;
					  case	GL_TYPE_BOOL:
						bval = RecvBool(fpComm);	ON_IO_ERROR(fpComm,badio);
						SetValueBool(value,bval);
						break;
					  default:
						printf("type = [%d]\n",type);
						break;
					}
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
dbgmsg("<RecvScreenData");
	return	(rc);
}

static	Bool
SendScreen(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;

dbgmsg(">SendScreen");
	CheckScreens(fpComm,scr);
	ret = SendScreenData(fpComm,scr);
	if		(  !ret  ) {
		printf("SendScreenData invalid\n");
	}
dbgmsg("<SendScreen");
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
		ret = AuthUser(user,pass,other);
	}
	return	(ret);
}
#else
#define	ThisAuth(user,pass,other)	AuthUser((user),(pass),(other))
#endif

static	Bool
MainLoop(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;

	PacketClass	klass;
	char	pass[SIZE_PASS+1];
	char	ver[SIZE_NAME+1];
	char	msg[SIZE_BUFF];

dbgmsg(">MainLoop");
	klass = RecvPacketClass(fpComm); ON_IO_ERROR(fpComm,badio);
	dbgprintf("class = %d",(int)klass);
	if		(  klass  !=  GL_Null  ) {
		switch	(klass) {
		  case	GL_Connect:
			RecvString(fpComm,ver);			ON_IO_ERROR(fpComm,badio);
			RecvString(fpComm,scr->user);	ON_IO_ERROR(fpComm,badio);
			RecvString(fpComm,pass);		ON_IO_ERROR(fpComm,badio);
			RecvString(fpComm,scr->cmd);	ON_IO_ERROR(fpComm,badio);
			sprintf(msg,"[%s@%s] session start",scr->user,scr->term);
			MessageLog(msg);
			if		(  fIgnoreVersion  ) {
				strcpy(ver,VERSION);
			}
			if		(  strcmp(ver,VERSION)  ) {
				SendPacketClass(fpComm,GL_E_VERSION);	ON_IO_ERROR(fpComm,badio);
				g_warning("reject client(invalid version)");
			} else
			if		(  ThisAuth(scr->user,pass,scr->other)  ) {
				scr->Windows = NULL;
				ApplicationsCall(APL_SESSION_LINK,scr);
				if		(  scr->status  ==  APL_SESSION_NULL  ) {
					SendPacketClass(fpComm,GL_E_APPL);	ON_IO_ERROR(fpComm,badio);
				} else {
					SendPacketClass(fpComm,GL_OK);		ON_IO_ERROR(fpComm,badio);
					CheckScreens(fpComm,scr);
				}
			} else {
				g_warning("reject client(authentication error)");
				SendPacketClass(fpComm,GL_E_AUTH);		ON_IO_ERROR(fpComm,badio);
			}
			break;
		  case	GL_Name:
			RecvString(fpComm,scr->term);		ON_IO_ERROR(fpComm,badio);
			break;
		  case	GL_Event:
			RecvString(fpComm,scr->window);		ON_IO_ERROR(fpComm,badio);
			RecvString(fpComm,scr->widget);		ON_IO_ERROR(fpComm,badio);
			RecvString(fpComm,scr->event);		ON_IO_ERROR(fpComm,badio);
			dbgprintf("event = [%s]\n",scr->event);
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
			printf("class = %X\n",klass);
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

dbgmsg("<MainLoop");
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
#ifdef	USE_SSL
	SSL_CTX	*ctx;
#endif
dbgmsg(">ExecureServer");
	signal(SIGCHLD,SIG_IGN);

	_fd = InitServerPort(PortNumber,Back);
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
dbgmsg("<ExecureServer");
}

static	void
InitData(void)
{
dbgmsg(">InitData");
	DD_ParserInit();
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
