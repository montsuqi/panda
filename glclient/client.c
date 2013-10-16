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
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<dirent.h>
#ifdef	USE_SSL
#include	<openssl/crypto.h>
#include	<openssl/x509.h>
#include	<openssl/pem.h>
#include	<openssl/ssl.h>
#include	<openssl/err.h>
#ifdef  USE_PKCS11
#include	<openssl/engine.h>
#endif	//USE_PKCS11
#endif	//USE_SSL
#define		MAIN
#include	"glclient.h"
#include	"bd_config.h"
#include	"const.h"
#include	"socket.h"
#include	"glterm.h"
#include	"comm.h"
#include	"protocol.h"
#include	"widgetcache.h"
#include	"desktop.h"
#include	"message.h"
#include	"debug.h"
#include	"interface.h"
#include	"gettext.h"

static gboolean fDialog;
static void GLMessage(int level, char *file, int line, char *msg);

extern  void
mkdir_p(
	char    *dname,
	int    mode)
{
	gchar *fn, *p;

	fn = g_strdup(dname);
	if (g_path_is_absolute (fn))
		p = (gchar *) g_path_skip_root (fn);	
	else
		p = fn;	

	do {
		while (*p && !(G_DIR_SEPARATOR == (*p)))
			p++;
		if (!*p)
			p = NULL;
		else
			*p = '\0';

		if (fn)
			mkdir (fn, mode);

		if (p)
		{
			*p++ = G_DIR_SEPARATOR;
			while (*p && (G_DIR_SEPARATOR == (*p)))
				p++;
		}
    }
	while (p);

	g_free (fn);
}

static  void
MakeDir(char *dir)
{
	struct stat st;

	if (stat(dir, &st) == 0){
		if (S_ISDIR(st.st_mode)) {
			return ;
		} else {
			unlink (dir);
		}
	}
	mkdir_p (dir, 0700);
}

static	void
MakeDirs(void)
{
#if 1
	gchar *template;
	gchar *tmpdir;
	gchar *p;

	tmpdir = g_strconcat(g_get_home_dir(),"/.glclient/tmp",NULL);
	MakeDir(tmpdir);
	template = g_strconcat(tmpdir,"/XXXXXX",NULL);
	g_free(tmpdir);
	if ((p = mkdtemp(template)) == NULL) {
		Error(_("mkdtemp failure"));
	}
	TempDir = p; 
#else
	/* glib >= 2.26*/
	TempDir = g_mkdtemp(g_strdup("glclient_XXXXXX"));
#endif
}

extern	void SetSessionTitle(
	char *title)
{
	if ( TITLE(glSession) ) {
		xfree(TITLE(glSession));
	}
	TITLE(glSession) = StrDup(title);
}

extern	void
SetSessionBGColor(
	char *bgcolor)
{
	if ( BGCOLOR(glSession) ) {
		xfree(BGCOLOR(glSession));
	}
	BGCOLOR(glSession) = StrDup(bgcolor);
}

#ifdef	USE_SSL
static void
_MakeSSL_CTX()
{
#ifdef  USE_PKCS11
	if (fPKCS11 == TRUE){
		CTX(glSession) = MakeSSL_CTX_PKCS11(&ENGINE(glSession), PKCS11_Lib,Slot,CA_File,NULL/*capath*/,Ciphers);
	}
	else{
		CTX(glSession) = MakeSSL_CTX(NULL/*keyfile*/,CertFile,CA_File,NULL/*capth*/,Ciphers);
	}
#else
    CTX(glSession) = MakeSSL_CTX(NULL/*keyfile*/,CertFile,CA_File,NULL/*capath*/,Ciphers);
#endif 	//USE_PKCS11
}
#endif	//USE_SSL	

static Bool
MakeFPCOMM (int fd)
{
#ifdef	USE_SSL
    if (!fSsl) {
        FPCOMM(glSession) = SocketToNet(fd);
    } else {
		_MakeSSL_CTX();

        if (CTX(glSession) == NULL){
			UI_ErrorDialog(GetSSLErrorMessage());
        }
        if ((FPCOMM(glSession) = MakeSSL_Net(CTX(glSession),fd)) != NULL){
            if (StartSSLClientSession(FPCOMM(glSession), IP_HOST(glSession->port)) != TRUE){
				UI_ErrorDialog(GetSSLErrorMessage());
            }
        }
    }
#else
	FPCOMM(glSession) = SocketToNet(fd);
#endif	//USE_SSL
	return TRUE;
}

static gboolean
start_client ()
{
	int		fd;
	gchar *portnum;

	ConvertWidgetCache();
	LoadWidgetCache();
	UI_InitTopWindow();

	portnum = g_strconcat(Host,":",PortNum,NULL);
    glSession->port = ParPort(portnum,PORT_GLTERM);
	g_free(portnum);
	if (  ( fd = ConnectSocket(glSession->port,SOCK_STREAM) )  <  0  ) {
		UI_ErrorDialog(_("can not connect server(server port not found)"));
        return FALSE;
	}
	InitProtocol();
	if(MakeFPCOMM(fd) != TRUE) return FALSE;

	if (SendConnect(FPCOMM(glSession),CurrentApplication)) {
		CheckScreens(FPCOMM(glSession),TRUE);
		fInRecv = TRUE;
		(void)GetScreenData(FPCOMM(glSession));
		fInRecv = FALSE;
		UI_Main();
	}
	
	return FALSE;
}

static void
stop_client ()
{
	GL_SendPacketClass(FPCOMM(glSession),GL_END);
	if	(  fMlog  ) {
		MessageLog("connection end\n");
	}
    CloseNet(FPCOMM(glSession));
#ifdef	USE_SSL
    if (CTX(glSession) != NULL)
        SSL_CTX_free (CTX(glSession));
#ifdef  USE_PKCS11
    if (ENGINE(glSession) != NULL){
        ENGINE_free(ENGINE(glSession));
        ENGINE_cleanup();
    }
#endif	//USE_PKCS11
#endif	//USE_SSL
    DestroyPort (glSession->port);
	SaveWidgetCache();
}

static void 
GLMessage(int level, char *file, int line, char *msg)
{
	switch(level){
	  case MESSAGE_WARN:
	  case MESSAGE_ERROR:
		UI_ErrorDialog(msg);
		break;
	  default:
		__Message(level, file, line, msg);
		break;
	}
}

static	void
askpass(char *pass)
{
	if (!SavePass) {
		if(UI_AskPass(pass, SIZE_BUFF, _("input Password")) != -1) {
			Pass = pass;
		} else {
			exit(0);
		}
	}
}

static	void
CheckDevilspie()
{
	gchar *path;
	path =  g_strconcat(g_get_home_dir(), "/.devilspie/glclient.ds", NULL);
	remove(path);
    g_free(path);
}

#define DEFAULT_PING_TIMER_PERIOD   (3000)
static	void
InitSystem()
{
	char *p;

	InitMessage("glclient",NULL);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	ConfDir =  g_strconcat(g_get_home_dir(), "/.glclient", NULL);

	if ((p = getenv("GLCLIENT_PING_TIMER_PERIOD")) != NULL) {
		PingTimerPeriod = atoi(p) * 1000;
		if (PingTimerPeriod < 1000) {
			PingTimerPeriod = DEFAULT_PING_TIMER_PERIOD;
		}
	} else {
		PingTimerPeriod = DEFAULT_PING_TIMER_PERIOD;
	}

	MakeDirs();
	InitDesktop();
	CheckDevilspie();

	glSession = New(Session);
	FPCOMM(glSession) = NULL;
#ifdef	USE_SSL
	CTX(glSession) = NULL;
#ifdef  USE_PKCS11
	ENGINE(glSession) = NULL;
#endif	//USE_PKCS11
#endif	//USE_SSL
	TITLE(glSession) = NULL;
	BGCOLOR(glSession) = NULL;
}

static	void
FinishSystem(void)
{
	DIR *dir;
	struct dirent *ent;
	char path[1024];
	
	if (!getenv("GLCLIENT_DONT_CLEAN_TEMP")) {
		if ((dir = opendir(TempDir)) != NULL) {
			while((ent = readdir(dir)) != NULL) {
				if (ent->d_name[0] != '.') {
					snprintf(path,sizeof(path),"%s/%s",TempDir,ent->d_name);
					path[sizeof(path)-1] = 0;
					remove(path);
				}
			}
			closedir(dir);
			remove(TempDir);
		}
	}
}

static	void
ExecClient(int argc, char **argv)
{
	char		_password[SIZE_BUFF];
	char		*delay_str;
	int			delay;

	UI_Init(argc, argv);

	if (fDialog) {
		UI_BootDialogRun();
	} else {
		fDialog = FALSE;
		UI_load_config(ConfigName);
		askpass(_password);
	}

	UI_InitStyle();

	delay_str = getenv ("GL_SEND_EVENT_DELAY");
	if (delay_str) {
		delay = atoi(delay_str);
		if (delay > 0) {
			fTimer = TRUE;
			TimerPeriod = delay_str;
		} else {
			fTimer = FALSE;
		}
	}

	SetMessageFunction(GLMessage);

	InitNET();
#ifdef	USE_SSL
	SetAskPassFunction(UI_AskPass);
#endif
	start_client();
	stop_client();

	exit(0);
}

static gboolean fListConfig = FALSE;
static GOptionEntry entries[] =
{
	{ "list-config",'l',0,G_OPTION_ARG_NONE,&fListConfig,"show config list",NULL},
	{ "config",'c',0,G_OPTION_ARG_STRING,&ConfigName,"connect by the specified config",NULL},
	{ NULL}
};

extern	int
main(
	int		argc,
	char	**argv)
{
	GOptionContext *ctx;
	int status;
	pid_t pid;
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_DFL;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

    gl_config_init();
	gl_config_convert_config();

	if (fListConfig) {
		UI_list_config();
		exit(0);
	}

	fDialog = ConfigName == NULL ? TRUE : FALSE;

	InitSystem();
	if (fDialog) {
		while(1) {
			if ((pid = fork()) == 0) {
				ExecClient(argc, argv);
			} else if (pid > 0) {
				wait(&status);
				if (WEXITSTATUS(status) == 0) {
					break;
				}
			} else {
				Error("fork failed");
			}
		}
	} else {
		ExecClient(argc, argv);
	}
	FinishSystem();
	return 0;
}
