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
#include	<gtk/gtk.h>

#define		MAIN
#include	"glclient.h"
#include	"protocol.h"
#include	"bd_config.h"
#include	"const.h"
#include	"widgetcache.h"
#include	"desktop.h"
#include	"bootdialog.h"
#include	"message.h"
#include	"debug.h"
#include	"action.h"
#include	"dialogs.h"
#include	"gettext.h"

static void GLMessage(int level, char *file, int line, char *msg);

static	void
MakeDirs(void)
{
#if 1
	gchar *template;
	gchar *tmpdir;
	gchar *p;

	tmpdir = g_strconcat(g_get_home_dir(),"/.glclient/tmp",NULL);
	MakeDir(tmpdir,0700);
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
	InitProtocol();
	LoadWidgetCache();
	InitTopWindow();

	RPC_GetServerInfo();
	RPC_StartSession();
	RPC_GetWindow();
	UpdateScreen();
	SetPingTimerFunc();
	UI_Main();

	return FALSE;
}

static void
StopClient ()
{
	RPC_EndSession();
	SaveWidgetCache();
}

static void 
GLMessage(int level, char *file, int line, char *msg)
{
	switch(level){
	  case MESSAGE_WARN:
		__Message(level, file, line, msg);
		break;
	  case MESSAGE_ERROR:
		ShowErrorDialog(msg);
		break;
	  default:
		__Message(level, file, line, msg);
		break;
	}
}

#define DEFAULT_PING_TIMER_PERIOD   (10000)
static	void
InitSystem()
{
	char *p;
	int delay;

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

	MakeDirs();
	InitDesktop();

	Session = g_new0(GLSession,1);
	RPCID(Session) = 0;
}

static	void
FinalSystem(void)
{
	if (!getenv("GLCLIENT_DONT_CLEAN_TEMP")) {
		rm_r(TempDir);
	}
}

static	void
ThisAskPass(
	gboolean fDialog)
{
	if (fPKCS11) {
		Pass = ShowAskPassDialog(_("pin:"));
	}
	if (fDialog) {
		return;
	} else {
		if (fSSL && !SaveCertPass) {
			Pass = ShowAskPassDialog(_("certificate password:"));
		}
		if (!fSSL && !SavePass) {
			Pass = ShowAskPassDialog(_("password:"));
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
	gboolean fDialog;
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
	} else {
		LoadConfigByDesc(ConfigName);
	}
	ThisAskPass(fDialog);

	InitStyle();
	SetMessageFunction(GLMessage);

	StartClient();
	StopClient();

	FinalSystem();
	return 0;
}
