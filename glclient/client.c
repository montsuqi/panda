/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<termio.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#ifdef ENABLE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif
#include	<glade/glade.h>
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif
#define		MAIN

#include	"types.h"
#include	"option.h"
#include	"tcp.h"
#include	"misc.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"styleParser.h"
#include	"comm.h"
#include	"protocol.h"
#include	"debug.h"

static	char	*PortNumber;
static	char	*Host;
static	char	*Cache;
static	char	*Style;

static	void
InitData(void)
{
	InitPool();
}

static	void
InitApplications(void)
{
}

extern	void
InitSystem(void)
{
	InitData();
	InitApplications();
}

static	ARG_TABLE	option[] = {
	{	"host",		STRING,		TRUE,	(void*)&Host,
		"ホスト名"	 									},
	{	"port",		STRING,	TRUE,	(void*)&PortNumber,
		"ポート番号"									},
	{	"cache",	STRING,		TRUE,	(void*)&Cache,
		"キャッシュディレクトリ名"						},
	{	"style",	STRING,		TRUE,	(void*)&Style,
		"スタイルファイル名"							},
	{	"user",		STRING,		TRUE,	(void*)&User,
		"ユーザ名"										},
	{	"pass",		STRING,		TRUE,	(void*)&Pass,
		"パスワード"									},
	{	NULL,		0,			FALSE,	NULL,	NULL	},
};

static	void
SetDefault(void)
{
	PortNumber = IntStrDup(PORT_GLTERM);
	Host = "localhost";
	CurrentApplication = "demo";
	Cache = "cache";
	Style = "";
	User = getenv("USER");
	Pass = "";
}

extern	char	*
CacheFileName(
	char	*name)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"%s/%s/%s/%s",Cache,Host,PortNumber,name);
	return	(buf);
}

extern	void
ExitSystem(void)
{
	SendPacketClass(fpComm,GL_END);
	fclose(fpComm);
	exit(0);
}

static	void
bannar(void)
{
	printf("glclient ver %s\n",VERSION);
	printf("Copyright (c) 1998-1999 Masami Ogoshi <ogochan@nurs.or.jp>\n");
	printf("              2000-2002 Masami Ogoshi & JMA.\n");
}

extern	int
main(
	int		argc,
	char	**argv)
{
	int		fhPort;
	FILE_LIST	*fl;
	int		rc;
	char	buff[SIZE_BUFF];

	bannar();
	SetDefault();
	if		(  ( fl = GetOption(option,argc,argv) )  !=  NULL  ) {
		CurrentApplication = fl->name;
	}
	argc = 1;
	argv[1] = NULL;
	InitSystem();
#ifdef ENABLE_GNOME
	gnome_init("glclient", VERSION, argc, argv);
#ifdef	USE_PANDA
	gtkpanda_init(&argc,&argv);
#endif
	glade_gnome_init();
#else
	gtk_set_locale();
	gtk_init(&argc, &argv);
#ifdef	USE_PANDA
	gtkpanda_init(&argc,&argv);
#endif
	glade_init();
#endif

	StyleParserInit();
	sprintf(buff,"%s/gltermrc",getenv("HOME"));
	StyleParser(buff);
	StyleParser("gltermrc");
	if		(  *Style  !=  0  ) {
		StyleParser(Style);
	}

	if		(  ( fhPort = ConnectSocket(PortNumber,SOCK_STREAM,Host) )  <  0  ) {
		g_warning("can not connect server(server port not found)");
		return	(1);
	}
	fpComm = fdopen(fhPort,"w+");
	InitProtocol();

	if		(  !SendConnect(fpComm,CurrentApplication)  ) {
		rc = 1;
	} else {
		CheckScreens(fpComm,TRUE);
		(void)GetScreenData(fpComm);
dbgmsg(">gtk_main");
		gtk_main();
dbgmsg("<gtk_main");
		ExitSystem(); 
		rc = 0;
	}
	return	(rc);
}
