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
#include	<uuid/uuid.h>
#include	<time.h>
#include	<libgen.h>
#include	<gtk/gtk.h>
#include	<gtkpanda/gtkpanda.h>

#define		MAIN
#include	"glclient.h"
#include	"protocol.h"
#include	"bd_config.h"
#include	"const.h"
#include	"widgetcache.h"
#include	"desktop.h"
#include	"bootdialog.h"
#include	"action.h"
#include	"gettext.h"
#include	"logger.h"
#include	"utils.h"
#include	"tempdir.h"
#include	"dialogs.h"

extern	void 
SetSessionTitle(
	const char *title)
{
	if (TITLE(Session)) {
		xfree(TITLE(Session));
	}
	TITLE(Session) = StrDup(title);
}

extern	void
SetSessionBGColor(
	const char *bgcolor)
{
	if (BGCOLOR(Session)) {
		xfree(BGCOLOR(Session));
	}
	BGCOLOR(Session) = StrDup(bgcolor);
}

static void
StartPushClient()
{
	pid_t pid;
	char *pusher;

	pusher = getenv("GLPUSH_PUSHER_URI");
	if (pusher == NULL) {
		pusher = GLP_GetPusherURI(GLP(Session));
	}
	if (pusher == NULL || strlen(pusher) <= 0 ) {
		UsePushClient = FALSE;
		return;
	}
	setenv("GLPUSH_PUSHER_URI"   ,pusher,1);
	setenv("GLPUSH_REST_URI"     ,GLP_GetRESTURI(GLP(Session)),1);
	setenv("GLPUSH_SESSION_ID"   ,GLP_GetSessionID(GLP(Session)),1);
	setenv("GLPUSH_API_USER"     ,User,1);
	setenv("GLPUSH_API_PASSWORD" ,Pass,1);

	setenv("GLPUSH_LOGFILE" ,GetLogFile(),1);
	setenv("GLPUSH_TEMPDIR" ,GetTempDir(),1);

	if (fSSL) {
		setenv("GLPUSH_CERT"              ,CertFile,1);
		setenv("GLPUSH_CERT_KEY"          ,CertKeyFile,1);
		setenv("GLPUSH_CERT_KEY_PASSWORD" ,CertPass,1);
		setenv("GLPUSH_CA_FILE"           ,CAFile,1);
	}

	if (g_file_test(PushClientCMD,G_FILE_TEST_IS_EXECUTABLE)) {
		pid = fork();
		if (pid == 0) {
			execl(PushClientCMD,PushClientCMD,NULL);
		} else if (pid < 0) {
			Warning("fork error:%s",strerror(errno));
		} else {
			Info("%s(%d) fork",PushClientCMD,(int)pid);
			PushClientPID = pid;
			UsePushClient = TRUE;
		}
	} else {
		Warning("%s does not exists",PushClientCMD);
	}
}

static void
StopPushClient()
{
	if (!UsePushClient) {
		return ;
	}
	if (kill(PushClientPID,SIGINT) == 0) {
		Info("%s(%d) kill",PushClientCMD,(int)PushClientPID);
	} else {
		Info("kill error:%s",strerror(errno));
	}
}

static gboolean
StartClient()
{
	GLP(Session) = InitProtocol(AuthURI,User,Pass);
#if USE_SSL
	if (fPKCS11) {
		GLP_SetSSLPKCS11(GLP(Session),PKCS11Lib,PIN);
	} else if (fSSL) {
		GLP_SetSSL(GLP(Session),CertFile,CertKeyFile,CertPass,CAFile);
	}
#endif
	THISWINDOW(Session) = NULL;
	WINDOWTABLE(Session) = NewNameHash();
	SCREENDATA(Session) = NULL;
	InitTopWindow();

	RPC_GetServerInfo(GLP(Session));
	RPC_StartSession(GLP(Session));
	if (SCREENDATA(Session) != NULL) {
		json_object_put(SCREENDATA(Session));
	}
	StartPushClient();
	SCREENDATA(Session) = RPC_GetWindow(GLP(Session));
	UpdateScreen();
	SetPingTimerFunc();
	UI_Main();

	return FALSE;
}

static void
StopClient ()
{
	RPC_EndSession(GLP(Session));
}

static	void
InitSystem()
{
	char *p;
	int delay;

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	InitTempDir();
	InitLogger("glclient2");
	InitDesktop();

	Session = g_new0(GLSession,1);

	if ((p = getenv("GLCLINET_LOG_LEVEL")) != NULL) {
		if (!strcasecmp(p,"debug")) {
			SetLogLevel(GL_LOG_DEBUG);
		}
	}

	if ((p = getenv("GLCLIENT_PING_TIMER_PERIOD")) != NULL) {
		PingTimerPeriod = atoi(p) * 1000;
		if (PingTimerPeriod < 1000) {
			PingTimerPeriod = DEFAULT_PING_TIMER_PERIOD;
		}
	} else {
		PingTimerPeriod = DEFAULT_PING_TIMER_PERIOD;
	}

	p = getenv ("GL_SEND_EVENT_DELAY");
	if (p) {
		delay = atoi(p);
		if (delay > 0) {
			fTimer = TRUE;
			TimerPeriod = delay;
		} else {
			fTimer = FALSE;
		}
	}

	if (getenv("GLCLIENT_DELAY_DRAW_WINDOW") != NULL) {
		DelayDrawWindow = TRUE;
	} else {
		DelayDrawWindow = FALSE;
	}
	if (getenv("GLCLIENT_CANCEL_SCALE_WINDOW") != NULL) {
		CancelScaleWindow = TRUE;
	} else {
		CancelScaleWindow = FALSE;
	}
}

static	void
FinalSystem(void)
{
	FinalProtocol(GLP(Session));
	FinalLogger();
	StopPushClient();
	if (!getenv("GLCLIENT_DONT_CLEAN_TEMP")) {
		rm_r(GetTempDir());
	}
}


static	void
ThisAskPass()
{
	if (fSSL) {
		if (fPKCS11) {
			if (!fSavePIN) {
				PIN = AskPINDialog(_("pin:"));
			}
			if (PIN == NULL) {
				exit(0);
			}
		} else {
			if (!SaveCertPass) {
				CertPass = AskPassDialog(_("certificate password:"));
			}
		}
	}
	if (!SavePass) {
		Pass = AskPassDialog(_("password:"));
	}
	if (Pass == NULL) {
		exit(0);
	}
}

extern void
LoadConfig (
	int n)
{
	if (!gl_config_have_config(n)) {
		Error("no server setting:%d",n);
	}

	AuthURI = g_strdup(gl_config_get_string(n,"authuri"));
	Style = g_strdup(gl_config_get_string(n,"style"));
	Gtkrc = g_strdup(gl_config_get_string(n,"gtkrc"));
	fDebug = gl_config_get_boolean(n,"debug");
	fKeyBuff = gl_config_get_boolean(n,"keybuff");
	User = g_strdup(gl_config_get_string(n,"user"));
	fIMKanaOff = gl_config_get_boolean(n,"im_kana_off");
	SavePass = gl_config_get_boolean(n,"savepassword");
	if (SavePass) {
		Pass = g_strdup(gl_config_get_string(n,"password"));
	} else {
		Pass = g_strdup("");
	} 
	setenv("GLPRINTER_CONFIG",gl_config_get_string(n,"printer_config"),1);

	fSSL = gl_config_get_boolean(n,"ssl");
	{
		gchar *oldauth;
		GRegex *reg;

		oldauth = AuthURI;
		reg = g_regex_new("http://",0,0,NULL);
		AuthURI = g_regex_replace(reg,oldauth,-1,0,"",0,NULL);
		g_free(oldauth);
		g_regex_unref(reg);

		oldauth = AuthURI;
		reg = g_regex_new("https://",0,0,NULL);
		AuthURI = g_regex_replace(reg,oldauth,-1,0,"",0,NULL);
		g_free(oldauth);
		g_regex_unref(reg);

		oldauth = AuthURI;
		if (fSSL) {
			AuthURI = g_strdup_printf("https://%s",oldauth);
		} else {
			AuthURI = g_strdup_printf("http://%s",oldauth);
		}
		g_free(oldauth);
	}
	CAFile = g_strdup(gl_config_get_string(n,"cafile"));
	CertFile = g_strdup(gl_config_get_string(n,"certfile"));
	CertKeyFile = g_strdup(gl_config_get_string(n,"certkeyfile"));
	Ciphers = g_strdup(gl_config_get_string(n,"ciphers"));
	SaveCertPass = gl_config_get_boolean(n,"savecertpassword");
	if (SaveCertPass) {
		CertPass = g_strdup(gl_config_get_string(n,"certpassword"));
	}

	fPKCS11 = gl_config_get_boolean(n,"pkcs11");
	PKCS11Lib = g_strdup(gl_config_get_string(n,"pkcs11lib"));
	PIN = g_strdup(gl_config_get_string(n,"pin"));
	fSavePIN = gl_config_get_boolean(n,"savepin");

	fTimer = gl_config_get_boolean(n,"timer");
	TimerPeriod = gl_config_get_int(n,"timerperiod");
	FontName = g_strdup(gl_config_get_string(n,"fontname"));
}

extern void
LoadConfigByDesc (
	const char *desc)
{
	int n;

	n = GetConfigIndexByDesc(desc);
	if (n == -1) {
		Error("could not found setting:%s",desc);
	}
	LoadConfig(n);
}

static gboolean fListConfig = FALSE;
static GOptionEntry entries[] =
{
	{ "list-config",'l',0,G_OPTION_ARG_NONE,&fListConfig,
		"show config list",NULL},
	{ "config",'c',0,G_OPTION_ARG_STRING,&ConfigName,
		"connect by the specified config",NULL},
	{ NULL}
};

extern	int
main(
	int argc,
	char **argv)
{
	GOptionContext *ctx;
	struct sigaction sa;
	char path[PATH_MAX];
	char *dname;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_DFL;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

	if (readlink("/proc/self/exe", path, sizeof(path)) <= 0){
		exit(1);
	}
	dname = dirname(path);
	PushClientCMD = g_strdup_printf("%s/gl-push-client",dname);

	InitSystem();
	gl_config_init();

	if (fListConfig) {
		ListConfig();
		exit(0);
	}

	fDialog = ConfigName == NULL ? TRUE : FALSE;

	UI_Init(argc,argv);
	if (fDialog) {
		BootDialogRun();
    	LoadConfig(gl_config_get_index());
	} else {
		LoadConfigByDesc(ConfigName);
	}

	if (fDebug) {
		SetLogLevel(GL_LOG_DEBUG);
	}

	InitStyle();
	ThisAskPass();

	StartClient();
	StopClient();

	FinalSystem();
	return 0;
}
