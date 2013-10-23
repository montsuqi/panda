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
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>
#include	<iconv.h>

#define		FRONT_H_MAIN

#include	"enum.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"auth.h"
#include	"authstub.h"
#include	"sessioncall.h"
#include	"driver.h"
#include	"glserver.h"
#include	"glcomm.h"
#include	"blobcache.h"
#include	"term.h"
#include	"dirs.h"
#include	"RecParser.h"
#include	"http.h"
#include	"sysdataio.h"
#include	"message.h"
#include	"debug.h"

static	ScreenData *
InitSession()
{
	ScreenData *scr;
	
	scr = NewScreenData();
	scr->status = SCREEN_DATA_NULL;
	return scr;
}

static	void
FinishSession(
	ScreenData	*scr)
{
	if (scr->status == SCREEN_DATA_END) {
		SessionExit(scr);
		Message("[%s@%s] session end",scr->user,scr->host);
	}
	FreeScreenData(scr);
}

static	Bool
CheckCache(
	NETFILE	*fpComm,
	char	*name)
{
	Bool	ret;
	int		klass;

ENTER_FUNC;
	GL_SendPacketClass(fpComm,GL_QueryScreen);
		ON_IO_ERROR(fpComm,badio);
	GL_SendString(fpComm,name);			
		ON_IO_ERROR(fpComm,badio);
	GL_SendInt(fpComm,0L);	
		ON_IO_ERROR(fpComm,badio);
	GL_SendInt(fpComm,0L);	
		ON_IO_ERROR(fpComm,badio);
	GL_SendInt(fpComm,0L);	
		ON_IO_ERROR(fpComm,badio);
	switch (klass = GL_RecvPacketClass(fpComm)) {
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
		ret = FALSE;
		break;
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
	char			*buff;
	char			*gladecoding;
	size_t			size;
	Bool			rc;
	ValueStruct		*value;
	static GRegex	*regex = NULL;

ENTER_FUNC;
	if (regex == NULL) {
		regex = g_regex_new("^<\\?xml.+encoding=.UTF-8.",0,0,NULL);
	}
	if (CheckCache(fpComm, wname)) {
		rc = FALSE;
		GL_RecvString(fpComm, SIZE_NAME, wname);/*dummy*/ 
			ON_IO_ERROR(fpComm,badio);
		if(g_file_get_contents(fname,&buff, &size, NULL)) {
			if (g_regex_match_full(regex,buff,size,0,0,NULL,NULL)) {
				gladecoding = NULL;
			} else {
				gladecoding = "euc-jisx0213";
				Warning("encoding EUC-JP is deprecated : %s",fname);
			}

			GL_SendPacketClass(fpComm,GL_ScreenDefine);
				ON_IO_ERROR(fpComm,badio);

			value = NewValue(GL_TYPE_CHAR);
			SetValueStringWithLength(value,buff,size,gladecoding);
			GL_SendString(fpComm, ValueToString(value,NULL));
            FreeValueStruct(value);
            g_free(buff);
			rc = TRUE;
		}
	} else {
		rc = TRUE;
	}
badio:
LEAVE_FUNC;
	return rc;
}

static	void
CheckScreen(
	char		*wname,
	WindowData	*win,
	NETFILE		*fpComm)
{
	gchar 			*fname;
	static gchar	**dirs = NULL;
	int 			i;
	Bool			hasWindow;
ENTER_FUNC;
	hasWindow = FALSE;
	if (dirs == NULL) {
		dirs = g_strsplit_set(ScreenDir,":",-1);
	}
	for (i=0; dirs[i]!=NULL; i++) {
		fname = g_strdup_printf("%s/%s.glade",dirs[i],wname);
		if (g_file_test(fname,G_FILE_TEST_IS_REGULAR)) {
			SendFile(fpComm,fname,wname);
			hasWindow = TRUE;
			g_free(fname);
			break;
		}
		g_free(fname);
	}
	if (!hasWindow) {
		Error("[%s] screen file not exitsts.",wname);
	}
LEAVE_FUNC;
}

static	void
CheckScreens(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
ENTER_FUNC;
	g_hash_table_foreach(scr->Windows,(GHFunc)CheckScreen,fpComm);
	GL_SendPacketClass(fpComm,GL_END);
LEAVE_FUNC;
}

static	void
SendWindow(
	char		*wname,
	WindowData	*win,
	NETFILE		*fpComm)
{
	Bool	rc;
ENTER_FUNC;
	rc = FALSE; 
	if (win->puttype != SCREEN_NULL) {
		GL_SendPacketClass(fpComm,GL_WindowName);
			ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,wname);
			ON_IO_ERROR(fpComm,badio);
		GL_SendInt(fpComm,win->puttype);
			ON_IO_ERROR(fpComm,badio);
		dbgprintf("wname = [%s]\n",wname);
		switch	(win->puttype) {
		case SCREEN_CURRENT_WINDOW:
		case SCREEN_NEW_WINDOW:
		case SCREEN_CHANGE_WINDOW:
			if (win->rec->value != NULL) {
				AccessBLOB(BLOB_ACCESS_EXPORT,win->rec->value);
				GL_SendPacketClass(fpComm,GL_ScreenData);
					ON_IO_ERROR(fpComm,badio);
				GL_SendValue(fpComm,win->rec->value,NULL);
					ON_IO_ERROR(fpComm,badio);
			} else {
				GL_SendPacketClass(fpComm,GL_NOT);
					ON_IO_ERROR(fpComm,badio);
			}
			break;
		default:
			GL_SendPacketClass(fpComm,GL_NOT);
				ON_IO_ERROR(fpComm,badio);
			break;
		}
		win->puttype = SCREEN_NULL;
	}
	rc = TRUE;
  badio:
	if (!rc) {
		Error("SendWindow failure:%s", wname);
	}
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
	g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,fpComm);
	if ((*scr->window != 0) &&
		(*scr->widget != 0)) {
		GL_SendPacketClass(fpComm,GL_FocusName);
			ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->window);
			ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm,scr->widget);
			ON_IO_ERROR(fpComm,badio);
	}
	GL_SendPacketClass(fpComm,GL_END);
		ON_IO_ERROR(fpComm,badio);
	rc = TRUE;
badio:
LEAVE_FUNC;
	return	rc;
}

static	Bool
SendScreenData(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	Bool	rc;
ENTER_FUNC;
	if (GL_RecvPacketClass(fpComm) == GL_GetData) {
		if (GL_RecvInt(fpComm) == 0) {	
			dbgmsg("get all data");
			rc = SendScreenAll(fpComm,scr);
		} else {
			rc = TRUE;
			dbgmsg("get partial");
		}
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
	return	rc;
}

static	void
SendScreen(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
ENTER_FUNC;
	dbgprintf("scr->status[%d]\n",scr->status);
	CheckScreens(fpComm,scr);
	if (!SendScreenData(fpComm,scr)) {
		Warning("SendScreenData failed");
		scr->status = SCREEN_DATA_END;
	}
	dbgprintf("scr->window[%s]\n",scr->window);
LEAVE_FUNC;
	return;
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

	ret = FALSE;
	if (fSsl && fVerifyPeer){
        char *cn;

        if (!fpComm->peer_cert) return FALSE;
		cn = GetCommonNameFromCertificate(fpComm->peer_cert);
        strcpy(user, cn);
        xfree(cn);
		ret = TRUE;
	}
    else {
		ret = AuthUser(&Auth, user, pass, other, NULL);
	}

	return	(ret);
}
#else
#define ThisAuth(fp,user,pass,other) AuthUser(&Auth,(user),(pass),(other),NULL)
#endif

static	Bool
CheckFeature(
	ScreenData	*scr,
	char		*ver)
{
	char *p,*q,*n;
	Bool fV48,fNetwork;

	strcpy(scr->agent,"unknown");
	fNetwork = FALSE;
	fV48 = FALSE;
	if		(  ( p = strchr(ver,':') )  !=  NULL  ) {
		p ++;
		while	(  *p  !=  0  ) {
			if		(  ( q = strchr(p,':') )  !=  NULL  ) {
				*q = 0;
				n = q + 1;
			} else {
				n = p + strlen(p);
			}
			if		(  !strlicmp(p,"no")  ) {
				fNetwork = TRUE;
			}
			if		(  !strlicmp(p,"negotiation")  ) {
				/* do nothing */
			}
			if		(  !strlicmp(p,"v48")  ) {
				fV48 = TRUE;
			}
			if		(  !strlicmp(p,"agent")  ) {
				if ((q = strchr(p,'=')) != NULL) {
					q++;
					strncpy(scr->agent,q,sizeof(scr->agent));
					scr->agent[sizeof(scr->agent)-1] = 0;
				}
			}
			p = n;
		}
	}
	if (!fV48) {
		return FALSE;
	}
	SetfNetwork(fNetwork);
	return TRUE;
}

static	void
OnSIGALRM(
	int		ec)
{
	Warning("session timeout");
}

static	void
Connect(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	char	pass[SIZE_PASS+1];
	char	ver[SIZE_BUFF];
	struct sigaction sa;
ENTER_FUNC;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = (void*)OnSIGALRM;
	sigemptyset (&sa.sa_mask);
	if (sigaction(SIGALRM, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	GL_RecvString(fpComm, sizeof(ver), ver);	
		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm, sizeof(scr->user), scr->user);	
		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm, sizeof(pass), pass);		
		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm, sizeof(scr->cmd), scr->cmd);	
		ON_IO_ERROR(fpComm,badio);

	dbgprintf("ver  = [%s]",ver);
	dbgprintf("user = [%s]",scr->user);
	dbgprintf("pass = [%s]",pass);
	dbgprintf("cmd  = [%s]",scr->cmd);

	if (!CheckFeature(scr,ver)) {
		GL_SendPacketClass(fpComm,GL_E_VERSION);
		ON_IO_ERROR(fpComm,badio);
		Warning("[%s@%s] reject client(invalid version)",scr->user,scr->host);
		return;
	}

	if (!ThisAuth(fpComm, scr->user, pass, "glserver")) {
		Warning("[%s@%s] reject client(authentication error)",
			scr->user,scr->host);
		GL_SendPacketClass(fpComm,GL_E_AUTH);
			ON_IO_ERROR(fpComm,badio);
		scr->status = SCREEN_DATA_NULL;
		return;
	}

	Message("[%s@%s] \"%s\" session start",scr->user, scr->host, scr->agent);
	Message("[%s@%s] client authenticated", scr->user,scr->host);

	SessionLink(scr);
	if (scr->status == SCREEN_DATA_NULL) {
		GL_SendPacketClass(fpComm,GL_E_APPL);
			ON_IO_ERROR(fpComm,badio);
		return;
	}

	ConnectSysData();
	sprintf(ver,"%s.%02d", PACKAGE_VERSION, 0);
	GL_SendPacketClass(fpComm,GL_ServerVersion);
		ON_IO_ERROR(fpComm,badio);
	GL_SendString(fpComm, ver);
		ON_IO_ERROR(fpComm,badio);

	/*dummy check screens*/
	CheckScreens(fpComm,scr);
	SendScreen(fpComm,scr);
LEAVE_FUNC;
	return;
  badio:
	Warning("Connect badio");
	scr->status = SCREEN_DATA_NULL;
	return;
}

static	Bool
RecvScreenData(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	int			i;
	char		wname[SIZE_LONGNAME+1];
	char		name[SIZE_LONGNAME+1];
	char		buff[SIZE_BUFF];
	WindowData	*win;
	ValueStruct	*value;
	Bool		rc;

ENTER_FUNC;
	rc = FALSE;
	while (GL_RecvPacketClass(fpComm) == GL_WindowName) {
			ON_IO_ERROR(fpComm,badio);
		GL_RecvString(fpComm, sizeof(buff),buff);
			ON_IO_ERROR(fpComm,badio);

		for(i=0;i<SIZE_LONGNAME;i++) {
			if (buff[i] == '.') {
				wname[i] = 0;
			} else {
				wname[i] = buff[i];
			}
		}
		wname[SIZE_LONGNAME] = 0;

		if ((win = g_hash_table_lookup(scr->Windows,wname)) != NULL) {
			while(GL_RecvPacketClass(fpComm) == GL_ScreenData){
				ON_IO_ERROR(fpComm,badio);
				GL_RecvString(fpComm, sizeof(name), name);	
					ON_IO_ERROR(fpComm,badio);
				value = GetItemLongName(win->rec->value,name+strlen(wname)+1);
				if (value != NULL) {
					GL_RecvValue(fpComm,value,NULL);
						ON_IO_ERROR(fpComm,badio);
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
	return	rc;
}

static  void
Event(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
ENTER_FUNC;
	GL_RecvString(fpComm, sizeof(scr->window), scr->window);
		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm, sizeof(scr->widget), scr->widget);
		ON_IO_ERROR(fpComm,badio);
	GL_RecvString(fpComm, sizeof(scr->event), scr->event);
		ON_IO_ERROR(fpComm,badio);
	dbgprintf("window = [%s]\n",scr->window);
	dbgprintf("widget = [%s]\n",scr->widget);
	dbgprintf("event  = [%s]\n",scr->event);
	RecvScreenData(fpComm,scr);
		ON_IO_ERROR(fpComm,badio);
	alarm(0);
	SessionMain(scr);
	if (scr->status == SCREEN_DATA_CONNECT) {
		SendScreen(fpComm,scr);
	}
badio:
LEAVE_FUNC;
	return;
}

static  Bool
Pong(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	char		*abort;
	char		*dialog;
	char		*popup;
	Bool		ret;
	
ENTER_FUNC;
	ret = TRUE;

	GetSessionMessage(scr->term,&popup,&dialog,&abort);

	if (strlen(abort) > 0) {
		GL_SendPacketClass(fpComm,GL_Pong_Abort);
			ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm, abort);
			ON_IO_ERROR(fpComm,badio);
		ret = FALSE;
	} else if (strlen(dialog) > 0) {
		GL_SendPacketClass(fpComm,GL_Pong_Dialog);
			ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm, dialog);
			ON_IO_ERROR(fpComm,badio);
	} else if (strlen(popup) > 0) {
		GL_SendPacketClass(fpComm,GL_Pong_Popup);
			ON_IO_ERROR(fpComm,badio);
		GL_SendString(fpComm, popup);
			ON_IO_ERROR(fpComm,badio);
	} else {
		GL_SendPacketClass(fpComm,GL_Pong);
			ON_IO_ERROR(fpComm,badio);
	}
	Flush(fpComm);

	xfree(abort);
	xfree(dialog);
	xfree(popup);

	ResetSessionMessage(scr->term);
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
	ret = FALSE;
	klass = GL_RecvPacketClass(fpComm);
		ON_IO_ERROR(fpComm,badio);
	alarm(0);
	dbgprintf("class = %X\n",(int)klass);

	switch	(klass) {
	case GL_Connect:
		alarm(GL_TIMEOUT_SEC);
		Connect(fpComm,scr);
		ON_IO_ERROR(fpComm,badio);
		alarm(0);
		break;
	case GL_Event:
		if (scr->status != SCREEN_DATA_CONNECT) {
			break;
		}
		alarm(GL_TIMEOUT_SEC);
		Event(fpComm, scr);
		ON_IO_ERROR(fpComm,badio);
		alarm(0);
		break;
	case GL_Ping:
		if (scr->status != SCREEN_DATA_CONNECT) {
			break;
		}
		alarm(GL_TIMEOUT_SEC);
		Pong(fpComm,scr);
		ON_IO_ERROR(fpComm,badio);
		alarm(0);
		break;
	case GL_END:
		if (scr->status == SCREEN_DATA_CONNECT) {
			scr->status = SCREEN_DATA_END;
		}
		break;
	case HTTP_GET:
	case HTTP_POST:
		if (fAPI) {
			alarm(API_TIMEOUT_SEC);
			HTTP_Method(klass, fpComm);
		} else {
			Warning("invalid class = %X\n",klass);
		}
		// does not reach here
		exit(-1);
		break;
	default:
		Warning("invalid class = %X\n",klass);
		scr->status = SCREEN_DATA_NULL;
		break;
	}

	if (scr->status != SCREEN_DATA_CONNECT) {
		ret = FALSE;
	} else {
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
badio:
	Message("[%s@%s] abnormal client termination",scr->user,scr->host);
LEAVE_FUNC;
	return FALSE;
}

extern	void
ExecuteServer(void)
{
	int		pid;
	int		fd;
	int		soc_len;
	int		soc[MAX_SOCKET];

	NETFILE	*fpComm;
	ScreenData	*scr;
	Port	*port;
#ifdef	USE_SSL
	SSL_CTX	*ctx;
	char *ssl_warning;
#endif
ENTER_FUNC;
	port = ParPortName(PortNumber);
	soc_len = InitServerMultiPort(port,Back,soc);
#ifdef	USE_SSL
	ctx = NULL;
	if (fSsl) {
		ctx = MakeSSL_CTX(KeyFile,CertFile,CA_File,CA_Path,Ciphers);
		if (ctx == NULL) {
			Warning(GetSSLErrorMessage());
			Error("CTX make error");
		}
	    if (!fVerifyPeer){
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
        }
		ssl_warning = GetSSLWarningMessage();
		if (strlen(ssl_warning) > 0){
			 Warning(ssl_warning);
		}
	}
#endif
	while (TRUE) {
		if ((fd = AcceptLoop(soc,soc_len)) < 0) {
			continue;
		}
		if ((pid = fork()) > 0) {	/*	parent	*/
			close(fd);
		} else
		if (pid == 0) {	/*	child	*/
#ifdef	USE_SSL
			if (fSsl) {
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
			scr = InitSession();
			RemoteIP(fd,scr->host,SIZE_HOST);
			alarm(API_TIMEOUT_SEC);
			while (MainLoop(fpComm,scr));
			FinishSession(scr);
			DisconnectSysData();
// FIXME avoid segv gl protocol timeout
#if 0
			CloseNet(fpComm);
#endif
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
	SetCacheDir(CacheDir);
	SessionCallInit(NULL);
	LEAVE_FUNC;
}
