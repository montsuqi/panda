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

#define	MAIN
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>
#include	<signal.h>

#include	"gettext.h"
#include	"const.h"
#include	"dirs.h"
#include    "types.h"
#include    "enum.h"
#include	"RecParser.h"
#include	"option.h"
#include    "libmondai.h"
#include    "glterm.h"
#include    "socket.h"
#include    "net.h"
#include    "comm.h"
#include    "auth.h"
#include    "authstub.h"
#include    "applications.h"
#include    "driver.h"
#include    "front.h"
#include    "term.h"
#include    "dirs.h"
#include    "RecParser.h"
#include    "message.h"
#include    "debug.h"

char	*Term;
Bool	FixTerm;

static	ARG_TABLE	option[] = {
	{	"screen",	STRING,		TRUE,	(void*)&ScreenDir,
		N_("screen directory")	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		N_("record directory")		 					},
	{	"cache",	STRING,		TRUE,	(void*)&CacheDir,
		N_("BLOB cache directory")						},
	{	"sesdir",	STRING,		TRUE,	(void*)&SesDir,
		N_("Session directory")							},
	{	"term",		STRING,		TRUE,	(void*)&Term,
		N_("Terminal ID")								},
	{	"fixterm",	BOOLEAN,	TRUE,	(void*)&FixTerm,
		N_("Fix Terminal ID")			},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	ScreenDir = ".";
	RecordDir = ".";
	CacheDir = "cache";
	SesDir = "";
	Term = "";
	FixTerm = FALSE;
}

static	void
_DumpWindows(
	char			*name,
	WindowData		*win,
	gpointer		opaque)
{
	if(win){
		printf("---- window:	[%s],[%s]\n", name, win->name);
		if (win->rec) DumpValueStruct(win->rec->value);
	}
}

static	void
_DumpRecords(
	char			*name,
	RecordStruct	*rec,
	gpointer		opaque)
{
	if(rec) {
		printf("---- record:	[%s]\n", name);
		if (rec) DumpValueStruct(rec->value);
	}
}

static	void
DumpScreenData(ScreenData *scr)
{
ENTER_FUNC;
	printf("==== DumpScreenData\n");
	printf("	window:	[%s]\n", scr->window);
	printf("	widget:	[%s]\n", scr->widget);
	printf("	event:	[%s]\n", scr->event);
	printf("	cmd:	[%s]\n", scr->cmd);
	printf("	user:	[%s]\n", scr->user);
	printf("	term:	[%s]\n", scr->term);
	printf("	other:	[%s]\n", scr->other);
	printf("	status:	[%d]\n", scr->status);
	g_hash_table_foreach(scr->Records, (GHFunc)_DumpRecords, NULL);
	g_hash_table_foreach(scr->Windows, (GHFunc)_DumpWindows, NULL);
LEAVE_FUNC;
}

static	void
ExecuteServer(void)
{
	ScreenData	*scr;
	int			count = 0;
	char		fname[256];
	char		term[256];
ENTER_FUNC;
	signal(SIGCHLD,SIG_IGN);
	scr = InitSession();
	if ( FixTerm ) {
		strcpy(term, Term);
	} else {
		strcpy(term, TermName(0));
	}
	while ( 1 ) {
		sprintf(fname, "%s/%s_%06d.scr", SesDir, Term, count);
		if ( (scr = _LoadScreenData(fname, term) ) != NULL ) {
			ApplicationsCall(scr->status, scr);
			DumpScreenData(scr);
		} else {
			break;
		}
		count ++;
	}
	exit(0);
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

static	void
InitSystem(
	int		argc,
	char	**argv)
{
ENTER_FUNC;
	InitNET();
	InitData();
	ApplicationsInit(argc,argv);
LEAVE_FUNC;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	SetDefault();
	(void)GetOption(option,argc,argv,NULL);
	InitMessage("glserver_replay",NULL);
	InitSystem(argc,argv);
	Message("glserver replay start");
	ExecuteServer();
	Message("glserver replay end");
	return	(0);
}
