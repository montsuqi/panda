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

dbgmsg(">CheckCache");
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
	GL_SendPacketClass(fpComm,GL_END,fFetureNetwork);
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
	int		puttype;
dbgmsg(">SendWindow");

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
								 fFetureExpand,fFetureNetwork);
					ON_IO_ERROR(fpComm,badio);
				} else {
					GL_SendValue(fpComm,win->rec->value,"euc-jp",
								 fFetureExpand,fFetureNetwork);
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
		GL_SendPacketClass(fpComm,GL_FocusName,fFetureNetwork);
		ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->window,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->widget,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	}
	GL_SendPacketClass(fpComm,GL_END,fFetureNetwork);		ON_IO_ERROR(fpComm,badio);
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
	if		(  GL_RecvPacketClass(fpComm,fFetureNetwork)  ==  GL_GetData  ) {
		if		(  GL_RecvInt(fpComm,fFetureNetwork)  ==  0  ) {	/*	get all data	*/
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
					ValueIsUpdate(value);
					type = GL_RecvDataType(fpComm,fFetureNetwork);
					ON_IO_ERROR(fpComm,badio);
					switch	(type)	{
					  case	GL_TYPE_CHAR:
					  case	GL_TYPE_VARCHAR:
					  case	GL_TYPE_DBCODE:
					  case	GL_TYPE_TEXT:
						GL_RecvString(fpComm,str,fFetureNetwork);ON_IO_ERROR(fpComm,badio);
						if		(  fFetureI18N  ) {
							SetValueString(value,str,NULL);	ON_IO_ERROR(fpComm,badio);
						} else {
							SetValueString(value,str,"euc-jp");ON_IO_ERROR(fpComm,badio);
						}
						break;
					  case	GL_TYPE_NUMBER:
						xval = GL_RecvFixed(fpComm,fFetureNetwork);
						ON_IO_ERROR(fpComm,badio);
						SetValueFixed(value,xval);
						xfree(xval->sval);
						xfree(xval);
						break;
					  case	GL_TYPE_INT:
						ival = GL_RecvInt(fpComm,fFetureNetwork);
						ON_IO_ERROR(fpComm,badio);
						SetValueInteger(value,ival);
						break;
					  case	GL_TYPE_FLOAT:
						fval = GL_RecvFloat(fpComm,fFetureNetwork);
						ON_IO_ERROR(fpComm,badio);
						SetValueFloat(value,fval);
						break;
					  case	GL_TYPE_BOOL:
						bval = GL_RecvBool(fpComm,fFetureNetwork);
						ON_IO_ERROR(fpComm,badio);
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
	if		(	(  strlcmp(ver,"1.2")    ==  0  )
			||	(  strlcmp(ver,"1.1.2")  ==  0  ) ) {
		TermFeture |= FETURE_CORE;
	} else
	if		(  !strlicmp(ver,"symbolic")  ) {
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
				p = n;
			}
		}
	}
	printf("core    = %s\n",fFetureCore ? "YES" : "NO");
	printf("i18n    = %s\n",fFetureI18N ? "YES" : "NO");
	printf("expand  = %s\n",fFetureExpand ? "YES" : "NO");
	printf("blob    = %s\n",fFetureBlob ? "YES" : "NO");
	printf("network = %s\n",fFetureNetwork ? "YES" : "NO");
}

static	void
Connect(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	char	pass[SIZE_PASS+1];
	char	ver[SIZE_BUFF];
	char	msg[SIZE_BUFF];

	GL_RecvString(fpComm,ver,fFetureNetwork);		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm,scr->user,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm,pass,fFetureNetwork);		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm,scr->cmd,fFetureNetwork);	ON_IO_ERROR(fpComm,badio);
	sprintf(msg,"[%s@%s] session start",scr->user,scr->term);
	MessageLog(msg);

	CheckFeture(ver);

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
MainLoop(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;

	PacketClass	klass;

dbgmsg(">MainLoop");
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
	InitGL_Comm();
	ApplicationsInit(argc,argv);
dbgmsg("<InitSystem");
}
