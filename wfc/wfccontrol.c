/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include    <sys/types.h>
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"value.h"
#include	"net.h"
#include	"comm.h"
#include	"dirs.h"
#include	"wfcdata.h"
#include	"controlthread.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

#include	"socket.h"

#include	"directory.h"

static	char	*ControlPort;
static	int		Back;
static	char	*Directory;

static	void
MainProc(void)
{
	NETFILE	*fp;

	if		(  ( fp = OpenPort(ControlPort,0) )  !=  NULL  ) {
		SendPacketClass(fp,WFCCONTROL_STOP);
		CloseNet(fp);
	}
}

static	void
InitSystem(void)
{
dbgmsg(">InitSystem");
#if	0
	InitDirectory();
	SetUpDirectory(Directory,NULL,"","");
#endif
	InitNET();
dbgmsg("<InitSystem");
}

static	void
CleanUp(void)
{
}

static	ARG_TABLE	option[] = {
	{	"control",	STRING,		TRUE,	(void*)&ControlPort,
		"制御ポート"									},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"lddir",	STRING,		TRUE,	(void*)&D_Dir,
		"LD定義格納ディレクトリ"		 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},

	{	NULL,		0,			TRUE,	NULL		 	}
};

static	void
SetDefault(void)
{
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	ControlPort = CONTROL_PORT;
}

extern	int
main(
	int		argc,
	char	**argv)
{

	SetDefault();
	GetOption(option,argc,argv);

	InitMessage("wfccontrol",NULL);

	InitSystem();
	MainProc();
	CleanUp();
	return	(0);
}
