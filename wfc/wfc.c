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

/*
  TODO
	wfc間通信
*/

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#define	_WFC

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>		/*	for	mknod	*/
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"value.h"
#include	"net.h"
#include	"comm.h"
#include	"dirs.h"
#include	"wfc.h"
#include	"mqthread.h"
#include	"corethread.h"
#include	"termthread.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

#include	"socket.h"
#include	"pty.h"
#include	"queue.h"

#include	"directory.h"

static	char	*ApsPortNumber;
static	char	*PortNumber;
static	int		Back;
static	char	*Directory;

#ifdef	DEBUG
extern	void
DumpNode(
	SessionData	*data)
{
dbgmsg(">DumpNode");
	printf("window   = [%s]\n",data->hdr->window);
	printf("widget   = [%s]\n",data->hdr->widget);
	printf("event    = [%s]\n",data->hdr->event);
	printf("term     = [%s]\n",data->hdr->term);
	printf("user     = [%s]\n",data->hdr->user);
dbgmsg("<DumpNode");
}
#endif

extern	void
ExecuteServer(void)
{
	int		_fhTerm
	,		_fhAps;
	fd_set	ready;
	int		maxfd;

dbgmsg(">ExecuteServer");
	_fhTerm = InitServerPort(PortNumber,Back);
	maxfd = _fhTerm;
	_fhAps = InitServerPort(ApsPortNumber,Back);
	maxfd = maxfd < _fhAps ? _fhAps : maxfd;

	while	(TRUE)	{
		dbgmsg("loop");
		FD_ZERO(&ready);
		FD_SET(_fhTerm,&ready);
		FD_SET(_fhAps,&ready);
		select(maxfd+1,&ready,NULL,NULL,NULL);
		if		(  FD_ISSET(_fhTerm,&ready)  ) {	/*	term connect	*/
			ConnectTerm(_fhTerm);
		}
		if		(  FD_ISSET(_fhAps,&ready)  ) {		/*	APS connect		*/
			ConnectAPS(_fhAps);
		}
	}
dbgmsg("<ExecuteServer");
}


static	void
InitSystem(void)
{
dbgmsg(">InitSystem");
	InitDirectory();
	SetUpDirectory(Directory,NULL,"","");
	if		(  ApsPortNumber  ==  NULL  ) {
		ApsPortNumber = ThisEnv->WfcApsPort->port;
	}
	if		(  PortNumber  ==  NULL  ) {
		PortNumber = ThisEnv->TermPort->port;
	}
	InitNET();
	InitMessageQueue();
	ReadyAPS();
	SetupMessageQueue();
	InitTerm();
	StartCoreThread();
dbgmsg("<InitSystem");
}

static	void
CleanUp(void)
{

}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"apsport",	STRING,		TRUE,	(void*)&ApsPortNumber,
		"APS接続待ちポート"								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"lddir",	STRING,		TRUE,	(void*)&D_Dir,
		"LD定義格納ディレクトリ"		 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},

	{	"retry",	INTEGER,	TRUE,	(void*)&MaxRetry,
		"トランザクションを再試行する時の上限数"		},

	{	NULL,		0,			TRUE,	NULL		 	}
};

static	void
SetDefault(void)
{
	PortNumber = NULL;
	ApsPortNumber = NULL;
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	MaxRetry = 0;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	int			rc;

	SetDefault();
	GetOption(option,argc,argv);
	InitMessage("wfc",NULL);

	InitSystem();
	ExecuteServer();
	CleanUp();
	rc = 0;
	return	(rc);
}
