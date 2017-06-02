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

#include	<gtk/gtk.h>

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
#include	"dialogs.h"

static	void
MakeTempDir(void)
{
	uuid_t u;
	gchar *dir,buf[64];

	dir = g_strconcat(g_get_home_dir(),"/.glclient/tmp",NULL);
	MakeDir(dir,0700);
	uuid_generate(u);
	uuid_unparse(u,buf);
	TempDir = g_strconcat(dir,"/",buf,NULL);
	MakeDir(TempDir,0700);
	g_free(dir);
	fprintf(stderr,"tempdir: %s\n",TempDir);
}

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

static gboolean
StartClient ()
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
	LoadWidgetCache();
	InitTopWindow();

	RPC_GetServerInfo(GLP(Session));
	RPC_StartSession(GLP(Session));
	if (SCREENDATA(Session) != NULL) {
		json_object_put(SCREENDATA(Session));
	}
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
	SaveWidgetCache();
}

static	void
InitSystem()
{
	char *p;
	int delay;

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

	MakeTempDir();
	InitLogger();
	InitDesktop();

	Session = g_new0(GLSession,1);
}

static	void
FinalSystem(void)
{
	FinalProtocol(GLP(Session));
	FinalLogger();
	if (!getenv("GLCLIENT_DONT_CLEAN_TEMP")) {
		rm_r(TempDir);
	}
}


static	void
ThisAskPass()
{
	if (fPKCS11) {
		if (!fSavePIN) {
			PIN = AskPINDialog(_("pin:"));
		}
		if (PIN == NULL) {
			exit(0);
		}
		return;
	}
	if (fSSL) {
		if (!SaveCertPass) {
			Pass = AskPassDialog(_("certificate password:"));
		}
	} else {
		if (!SavePass) {
			Pass = AskPassDialog(_("password:"));
		}
	}
	if (Pass == NULL) {
		exit(0);
	}
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

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_DFL;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, entries, NULL);
	g_option_context_parse(ctx,&argc,&argv,NULL);

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
		SetLogLevel(LOG_DEBUG);
	}

	InitStyle();
	ThisAskPass();

	StartClient();
	StopClient();

	FinalSystem();
	return 0;
}
