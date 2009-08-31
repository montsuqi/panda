/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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
#include	<iconv.h>

#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"auth.h"
#include	"authstub.h"
#include	"applications.h"
#include	"driver.h"
#include	"glserver.h"
#include	"glcomm.h"
#include	"front.h"
#include	"term.h"
#include	"dirs.h"
#include	"RecParser.h"
#include	"http.h"
#include	"sysdataio.h"
#include	"message.h"
#include	"debug.h"

static	void
FinishSession(
	ScreenData	*scr)
{
	if (scr->fConnect) {
		ApplicationsCall(APL_SESSION_END,scr);
	}
	Message("[%s@%s] session end",scr->user,TermToHost(scr->term));
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
	GL_SendPacketClass(fpComm,GL_QueryScreen,fFeatureNetwork);
	ON_IO_ERROR(fpComm,badio);
	GL_SendString(fpComm,name,fFeatureNetwork);			ON_IO_ERROR(fpComm,badio);
	GL_SendInt(fpComm,(long)stsize,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
	GL_SendInt(fpComm,(long)stmtime,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
	GL_SendInt(fpComm,(long)stctime,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
	switch	(  klass = GL_RecvPacketClass(fpComm,fFeatureNetwork)  ) {
	  case	GL_GetScreen:
		dbgmsg("GetScreen");
		ret = TRUE;
		break;
	  case	GL_NOT:
		dbgmsg("Not");
		ret = FALSE;
		break;
	  default:
		dbgprintf("klass = [%d]\n",klass);
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
	struct	stat	stbuf;
	char			*buff;
	char			*coding;
	size_t			size;
	FILE			*fp;
	Bool			rc;
	ValueStruct		*value;

ENTER_FUNC;
	if (fFeatureI18N) {
		coding = NULL;
	} else {
		coding = "euc-jisx0213";
	}
	stat(fname,&stbuf);
	if		(  CheckCache(fpComm,
						  wname,
						  stbuf.st_size, stbuf.st_mtime, stbuf.st_ctime)  ) {
		rc = FALSE;
		fp = NULL;
		GL_RecvString(fpComm, SIZE_NAME, wname,fFeatureNetwork);	/*	dummy	*/ 
		ON_IO_ERROR(fpComm,badio);
		if		(  ( fp = fopen(fname,"r") )  !=  NULL  ) {
			GL_SendPacketClass(fpComm,GL_ScreenDefine,fFeatureNetwork);
			ON_IO_ERROR(fpComm,badio);

			size = stbuf.st_size;
			buff = xmalloc(size + 1);
			if (fread(buff, 1, size, fp) == size) {
				buff[size] = 0;
				value = NewValue(GL_TYPE_CHAR);
				SetValueString(value, buff, "euc-jisx0213");
				GL_SendString(fpComm, ValueToString(value,coding), fFeatureNetwork);
				ON_IO_ERROR(fpComm,badio);
				FreeValueStruct(value);
			}
			xfree(buff);
			rc = TRUE;
		  badio:
			if		(  fp  !=  NULL  ) {
				fclose(fp);
			}
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
			Warning("[%s] screen file not exitsts.",wname);
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
	GL_SendPacketClass(fpComm,GL_END,fFeatureNetwork);
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
	char	*coding;
ENTER_FUNC;
	rc = FALSE; 
	if (fFeatureI18N) {
		coding = NULL;
	} else {
		coding = "euc-jisx0213";
	}
	if		(  win->PutType  !=  SCREEN_NULL  ) {
		GL_SendPacketClass(fpComm,GL_WindowName,fFeatureNetwork);
		ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,win->name,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
		dbgprintf("wname = [%s]\n",win->name);
		GL_SendInt(fpComm,win->PutType,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
		switch	(win->PutType) {
		  case	SCREEN_CURRENT_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			if		(  win->rec->value  !=  NULL  ) {
				AccessBLOB(BLOB_ACCESS_EXPORT,win->rec->value);
				GL_SendPacketClass(fpComm,GL_ScreenData,fFeatureNetwork);
				ON_IO_ERROR(fpComm,badio);
				GL_SendValue(fpComm,win->rec->value,coding,fFeatureBlob,fFeatureExpand,fFeatureNetwork);
				ON_IO_ERROR(fpComm,badio);
			} else {
				GL_SendPacketClass(fpComm,GL_NOT,fFeatureNetwork);
				ON_IO_ERROR(fpComm,badio);
			}
			break;
		  default:
			GL_SendPacketClass(fpComm,GL_NOT,fFeatureNetwork);
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
		GL_SendPacketClass(fpComm,GL_FocusName,fFeatureNetwork);
		ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->window,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->widget,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
	}
	GL_SendPacketClass(fpComm,GL_END,fFeatureNetwork);		ON_IO_ERROR(fpComm,badio);
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
	if		(  GL_RecvPacketClass(fpComm,fFeatureNetwork)  ==  GL_GetData  ) {
		if		(  GL_RecvInt(fpComm,fFeatureNetwork)  ==  0  ) {	/*	get all data	*/
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
		if		( scr->window  !=  NULL  )	{
			Warning("SendScreen [%s] failed.", scr->window);
		}
		Warning("SendScreenData invalid");
	}
LEAVE_FUNC;
	return	(ret);
}

#ifdef	USE_SSL
static	Bool
ThisAuth(
    NETFILE *fpComm,
	char	*user,
	char	*pass,
	char	*other)
{
	Bool	ret;

	if (fSsl && !strcasecmp(Auth.protocol,"ssl")){
        char *subject;
        char tmp[SIZE_USER+1];
        if (!fpComm->peer_cert) return FALSE;
        subject = GetSubjectFromCertificate(fpComm->peer_cert);
        AuthLoadX509(Auth.file);
        ret = AuthX509(subject, tmp);
        xfree(subject);
        if (ret == TRUE) strcpy(user, tmp);
	}
    else {
		ret = AuthUser(&Auth, user, pass, other, NULL);
	}

	return	(ret);
}
#else
#define	ThisAuth(fp,user,pass,other)	AuthUser(&Auth,(user),(pass),(other),NULL)
#endif

static	void
CheckFeature(
	char	*ver)
{
	char	*p
		,	*q
		,	*n;

	TermFeature = FEATURE_NULL;
	TermExpandType = EXPAND_PNG;
	if		(  strlcmp(ver,"1.2")    ==  0  ) {
		TermFeature |= FEATURE_CORE;
	} else
	  if	(	(  strlcmp(ver,"1.1.1")  ==  0  )
			||	(  strlcmp(ver,"1.1.2")  ==  0  ) ) {
		TermFeature |= FEATURE_CORE;
		TermFeature |= FEATURE_OLD;
	} else {
		TermFeature = FEATURE_CORE;
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
					TermFeature |= FEATURE_BLOB;
				}
				if		(  !strlicmp(p,"expand")  ) {
					TermFeature |= FEATURE_EXPAND;
				}
				if		(  !strlicmp(p,"i18n")  ) {
					TermFeature |= FEATURE_I18N;
				}
				if		(  !strlicmp(p,"no")  ) {
					TermFeature |= FEATURE_NETWORK;
				}
				if		(  !strlicmp(p,"negotiation")  ) {
					TermFeature |= FEATURE_NEGO;
				}
				if		(  !strlicmp(p,"ps")  ) {
					TermExpandType = EXPAND_PS;
				}
				if		(  !strlicmp(p,"pdf")  ) {
					TermExpandType = EXPAND_PDF;
				}
				p = n;
			}
		}
	}
#ifdef	DEBUG
	printf("core		= %s\n",fFeatureCore ? "YES" : "NO");
	printf("i18n      	= %s\n",fFeatureI18N ? "YES" : "NO");
	printf("blob      	= %s\n",fFeatureBlob ? "YES" : "NO");
	printf("expand    	= %s\n",fFeatureExpand ? "YES" : "NO");
	printf("network   	= %s\n",fFeatureNetwork ? "YES" : "NO");
	printf("negotiation	= %s\n",fFeatureNego ? "YES" : "NO");
	printf("old			= %s\n",fFeatureOld ? "YES" : "NO");
#endif
}

static	Bool
Connect(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	char	pass[SIZE_PASS+1];
	char	ver[SIZE_BUFF];
    Bool    auth_ok = FALSE;
	Bool	rc;
ENTER_FUNC;
	rc = FALSE;
	GL_RecvString(fpComm, sizeof(ver), ver, FALSE);	ON_IO_ERROR(fpComm,badio);
	dbgprintf("ver  = [%s]",ver);
	CheckFeature(ver);
	GL_RecvString(fpComm, sizeof(scr->user), scr->user,fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
	dbgprintf("user = [%s]",scr->user);
	GL_RecvString(fpComm, sizeof(pass), pass, fFeatureNetwork);		ON_IO_ERROR(fpComm,badio);
	dbgprintf("pass = [%s]",pass);
	GL_RecvString(fpComm, sizeof(scr->cmd), scr->cmd, fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
	dbgprintf("cmd  = [%s]",scr->cmd);
	Message("[%s@%s] session start",scr->user, TermToHost(scr->term));

	if		(  TermFeature  ==  FEATURE_NULL  ) {
		GL_SendPacketClass(fpComm,GL_E_VERSION,fFeatureNetwork);
		ON_IO_ERROR(fpComm,badio);
		Warning("[%s@%s] reject client(invalid version)",scr->user,TermToHost(scr->term));
	}
    else {
	    auth_ok = ThisAuth(fpComm, scr->user, pass, scr->other);
    }

	if (auth_ok){
		Message("[%s@%s] client authenticated", scr->user,TermToHost(scr->term));
		ApplicationsCall(APL_SESSION_LINK,scr);
		if		(  scr->status  ==  APL_SESSION_NULL  ) {
			GL_SendPacketClass(fpComm,GL_E_APPL,fFeatureNetwork);
			ON_IO_ERROR(fpComm,badio);
		} else {
			if (fFeatureNego) {
				sprintf(ver,"%s.%02d", PACKAGE_VERSION, 1);
				GL_SendPacketClass(fpComm,GL_ServerVersion,fFeatureNetwork);
				GL_SendString(fpComm, ver,fFeatureNetwork);
				ON_IO_ERROR(fpComm,badio);
			} else {
				GL_SendPacketClass(fpComm,GL_OK,fFeatureNetwork);
			}
			ON_IO_ERROR(fpComm,badio);
			CheckScreens(fpComm,scr);
			rc = SendScreen(fpComm,scr);
		}
	} else {
		GL_SendPacketClass(fpComm,GL_E_AUTH,fFeatureNetwork);
		ON_IO_ERROR(fpComm,badio);
		Warning("[%s@%s] reject client(authentication error)",scr->user,TermToHost(scr->term));
	}
  badio:
LEAVE_FUNC;
	return (rc);
}

static	Bool
RecvScreenData(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	int		c;
	char		wname[SIZE_NAME+1]
		,		name[SIZE_NAME+1]
		,		buff[SIZE_BUFF];
	WindowData	*win;
	ValueStruct	*value;
	Bool		rc;
	char		*coding;

ENTER_FUNC;
	rc = FALSE;
	if		(  fFeatureI18N  ) {
		coding = NULL;
	} else {
		coding = "euc-jisx0213";
	}
	while	(  GL_RecvPacketClass(fpComm,fFeatureNetwork)  ==  GL_WindowName  ) {
		ON_IO_ERROR(fpComm,badio);
		GL_RecvString(fpComm, sizeof(buff), buff, fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
		PureWindowName(buff,wname);
		if		(  ( win = g_hash_table_lookup(scr->Windows,wname) )  !=  NULL  ) {
			while	(  ( c = GL_RecvPacketClass(fpComm,fFeatureNetwork) )
					   ==  GL_ScreenData  ) {
				ON_IO_ERROR(fpComm,badio);
				GL_RecvString(fpComm, sizeof(name), name, fFeatureNetwork);	ON_IO_ERROR(fpComm,badio);
				if		(  ( value = GetItemLongName(win->rec->value,name+strlen(wname)+1) )
						   !=  NULL  ) {
					GL_RecvValue(fpComm,value,coding,fFeatureBlob,fFeatureExpand,fFeatureNetwork);
				} else {
					Warning("invalid item name [%s]\n",name);
					goto badio;
				}
			}
			AccessBLOB(BLOB_ACCESS_IMPORT, win->rec->value);
		} else {
			Message("invalid window name [%s]\n",wname);
			goto badio;
		}
	}
	rc = TRUE;
  badio:
LEAVE_FUNC;
	return	(rc);
}

static  Bool
Glevent(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;
	
	ret = FALSE;
ENTER_FUNC;
	GL_RecvString(fpComm, sizeof(scr->window), scr->window, fFeatureNetwork);
	ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm, sizeof(scr->widget), scr->widget, fFeatureNetwork);
	ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm, sizeof(scr->event), scr->event, fFeatureNetwork);
	ON_IO_ERROR(fpComm,badio);
	dbgprintf("window = [%s]\n",scr->window);
	dbgprintf("event  = [%s]\n",scr->event);
	RecvScreenData(fpComm,scr);			ON_IO_ERROR(fpComm,badio);
	ApplicationsCall(APL_SESSION_GET,scr);
	if ( scr->status == APL_SESSION_GET ){
		ret = SendScreen(fpComm,scr);
	}
badio:
LEAVE_FUNC;
	return ret;
}

static  Bool
Pong(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	Bool		ret;
	Bool		fAbort;
	char		*message;
	char		*coding;
	ValueStruct	*value;
	
ENTER_FUNC;
	ret = TRUE;
	if (fFeatureI18N) {
		coding = NULL;
	} else {
		coding = "euc-jisx0213";
	}

	GL_SendPacketClass(fpComm,GL_Pong,fFeatureNetwork);
	ON_IO_ERROR(fpComm,badio);

	GetSysDBMessage(scr->term, &fAbort, &message);
	if (fAbort) {
		GL_SendPacketClass(fpComm,GL_STOP,fFeatureNetwork);
			ON_IO_ERROR(fpComm,badio);
		if (strlen(message) <= 0) {
			sprintf(message,"shutdown from the server");
		}
		value = NewValue(GL_TYPE_CHAR);
		SetValueString(value,message,NULL);
		GL_SendString(fpComm, ValueToString(value,coding), fFeatureNetwork);
			ON_IO_ERROR(fpComm,badio);
		FreeValueStruct(value);
		ret = FALSE;
	} else {
		if (strlen(message) > 0) {
			GL_SendPacketClass(fpComm,GL_CONTINUE,fFeatureNetwork);
				ON_IO_ERROR(fpComm,badio);
			value = NewValue(GL_TYPE_CHAR);
			SetValueString(value,message,NULL);
			GL_SendString(fpComm, ValueToString(value,coding), fFeatureNetwork);
				ON_IO_ERROR(fpComm,badio);
			FreeValueStruct(value);
		} else {
			GL_SendPacketClass(fpComm,GL_END,fFeatureNetwork);
		}
	}
badio:
LEAVE_FUNC;
	return ret;
}

static	Bool
MainLoop(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;
	PacketClass	klass;

ENTER_FUNC;
	klass = GL_RecvPacketClass(fpComm,fFeatureNetwork); ON_IO_ERROR(fpComm,badio);
	alarm(0);
	dbgprintf("class = %X\n",(int)klass);
	if		(  klass  !=  GL_Null  ) {
		switch	(klass) {
		case GL_Connect:
			if (!Connect(fpComm,scr)){
				scr->status = APL_SESSION_NULL;
			}
			ON_IO_ERROR(fpComm,badio);			
			break;
		case GL_Event:
			if (  scr->status  !=  APL_SESSION_NULL  ) {
				if	(  !Glevent(fpComm, scr)	){
					scr->status = APL_SESSION_NULL;
				}
			}
			ON_IO_ERROR(fpComm,badio);
			break;
		case GL_Ping:
			if	(  !Pong(fpComm, scr)	){
				scr->status = APL_SESSION_NULL;
			}
			ON_IO_ERROR(fpComm,badio);
			break;
		case GL_END:
			scr->status = APL_SESSION_NULL;
			break;
		case HTTP_GET:
		case HTTP_POST:
		case HTTP_HEAD:
			if (fAPI) {
				alarm(TIMEOUT_SEC);
				HTTP_Method(klass, fpComm);
			} else {
				Warning("invalid class = %X\n",klass);
			}
			// this should not come
			exit(-1);
			break;
		default:
			Warning("invalid class = %X\n",klass);
			scr->status = APL_SESSION_NULL;
			break;
		}
		switch	(scr->status) {
		  case	APL_SESSION_NULL:
			ret = FALSE;
			break;
		  default:
			ret = TRUE;
			break;
		}
	} else {
	  badio:
		ret = FALSE;
		Message("[%s@%s] abnormal client termination",scr->user,TermToHost(scr->term));
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
	char *ssl_warning;
#endif
ENTER_FUNC;
	signal(SIGCHLD,SIG_IGN);

	port = ParPortName(PortNumber);
	_fd = InitServerPort(port,Back);
#ifdef	USE_SSL
	ctx = NULL;
	if		(  fSsl  ) {
		if		(  ( ctx = MakeSSL_CTX(KeyFile,CertFile,CA_File,CA_Path,Ciphers) )
				   ==  NULL  ) {
			Warning(GetSSLErrorMessage());
			Error("CTX make error");
		}
	    if (strcasecmp(Auth.protocol, "ssl") != 0){
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
        }
		ssl_warning = GetSSLWarningMessage();
		if 	(strlen(ssl_warning) > 0) Warning(ssl_warning);
	}
#endif
	while	(TRUE)	{
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			dbgprintf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}

		if		(  ( pid = fork() )  >  0  )	{	/*	parent	*/
			close(fd);
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
#ifdef	USE_SSL
			if		(  fSsl  ) {
				fpComm = MakeSSL_Net(ctx, fd);
				if (StartSSLServerSession(fpComm) != TRUE){
			        CloseNet(fpComm);
					Warning(GetSSLErrorMessage());
                    exit(0);
                }
			} else {
				fpComm = SocketToNet(fd);
			}
#else
			fpComm = SocketToNet(fd);
#endif
			close(_fd);
			scr = InitSession();
			strcpy(scr->term,TermName(fd));
			alarm(TIMEOUT_SEC);
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
	RecParserInit();
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
