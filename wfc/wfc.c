/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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
	glserverからの再接続
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
#include	<termio.h>
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
#include	"misc.h"

#include	"value.h"
#include	"comm.h"
#include	"dirs.h"
#include	"wfc.h"
#include	"mqthread.h"
#include	"corethread.h"
#include	"termthread.h"
#include	"option.h"
#include	"debug.h"

#include	"tcp.h"
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
	printf("window   = [%s]\n",data->ctrl->window);
	printf("widget   = [%s]\n",data->ctrl->widget);
	printf("event    = [%s]\n",data->ctrl->event);
	printf("term     = [%s]\n",data->ctrl->term);
	printf("user     = [%s]\n",data->ctrl->user);
dbgmsg("<DumpNode");
}
#endif

/*
 *	for WFC
 */
extern	int
PutAPS(
	APS_Node	*aps,
	SessionData	*data)
{
	MessageHeader	*hdr;
	int		i
	,		j;
	FILE	*fpAPS;
	int		ix;

dbgmsg(">PutAPS");
#ifdef	DEBUG
	printf("term = [%s]\n",data->ctrl->term);
	printf("user = [%s]\n",data->ctrl->user);
#endif
	fpAPS = NULL;
	ix = -1;
  retry:
	for	( i = 0 ; i < aps->nports ; i ++ ) {
		fpAPS = aps->fp[i];
		if		(  fpAPS  !=  NULL  ) {
			SendPacketClass(fpAPS,APS_PING);
			if		(  RecvPacketClass(fpAPS)  ==  APS_PONG  ) {
				hdr = data->hdr;
				SendPacketClass(fpAPS,APS_EVENTDATA);
				SendChar(fpAPS,hdr->status);
				SendString(fpAPS,hdr->term);
				SendString(fpAPS,hdr->user);
				SendString(fpAPS,hdr->window);
				SendString(fpAPS,hdr->widget);
				SendString(fpAPS,hdr->event);
				SendPacketClass(fpAPS,APS_MCPDATA);
				SendLBS(fpAPS,data->mcpdata);
				SendPacketClass(fpAPS,APS_SPADATA);
				SendLBS(fpAPS,data->spadata);
				SendPacketClass(fpAPS,APS_LINKDATA);
				SendLBS(fpAPS,data->linkdata);
				SendPacketClass(fpAPS,APS_SPADATA);
				SendLBS(fpAPS,data->spadata);
				SendPacketClass(fpAPS,APS_SCRDATA);
				for	( j = 0 ; j < data->aps->ld->cWindow ; j ++ ) {
					SendLBS(fpAPS,data->scrdata[j]);
				}
				SendPacketClass(fpAPS,APS_END);
				ix = i;
				break;
			} else {
				shutdown(fileno(aps->fp[i]), 2);
				fclose(aps->fp[i]);
				aps->fp[i] = NULL;
			}
		}
	}
	if		(  i  ==  aps->nports  ) {
		dbgmsg("wait aps connect");
		pthread_mutex_lock(&aps->lock);
		do {
			for	( i = 0 ; i < aps->nports ; i ++ ) {
				if		(  aps->fp[i]  !=  NULL  )	break;
			}
			if		(  i  ==  aps->nports  ) {
				pthread_cond_wait(&aps->conn, &aps->lock);
			}
		}	while	(  i  ==  aps->nports  );
		pthread_mutex_unlock(&aps->lock);
		dbgmsg("release aps connect");
		goto	retry;
	}
dbgmsg("<PutAPS");
	return(ix);
}

extern	Bool
GetAPS_Control(
	FILE	*fpAPS,
	MessageHeader	*hdr)
{
	PacketClass	c;
	Bool		fSuc;

dbgmsg(">GetAPS_Control");
	switch	( c = RecvPacketClass(fpAPS) ) { 
	  case	APS_CTRLDATA:
		RecvString(fpAPS,hdr->term);
		RecvString(fpAPS,hdr->window);
		RecvString(fpAPS,hdr->widget);
		hdr->puttype = (char)RecvChar(fpAPS);
		fSuc = TRUE;
#ifdef	DEBUG
		printf("[%s][%c]\n",hdr->window,hdr->puttype);fflush(stdout);
#endif
		break;
	  default:
		dbgmsg("aps die ?");
		fSuc = FALSE;
		break;
	}
dbgmsg("<GetAPS_Control");
	return	(fSuc); 
}

extern	Bool
GetAPS_Value(
	FILE	*fpAPS,
	SessionData	*data,
	PacketClass	c)
{
	int		i
	,		nclose;
	Bool	fSuc;

dbgmsg(">GetAPS_Value");
	SendPacketClass(fpAPS,c);
	switch	(c) {
	  case	APS_CLSWIN:
		dbgmsg("CLSWIN");
		nclose = RecvInt(fpAPS); 
		for	( i = 0 ; i < nclose ; i ++ ) {
			RecvString(fpAPS,data->w.close[data->w.n].window);
			data->w.n ++;
		}
		fSuc = TRUE;
		break;
	  case	APS_MCPDATA:
		dbgmsg("MCPDATA");
		RecvLBS(fpAPS,data->mcpdata);
		fSuc = TRUE;
		break;
	  case	APS_LINKDATA:
		dbgmsg("LINKDATA");
		RecvLBS(fpAPS,data->linkdata);
		fSuc = TRUE;
		break;
	  case	APS_SPADATA:
		dbgmsg("SPADATA");
		RecvLBS(fpAPS,data->spadata);
		fSuc = TRUE;
		break;
	  case	APS_SCRDATA:
		dbgmsg("SCRDATA");
		for	( i = 0 ; i < data->cWindow ; i ++ ) {
			RecvLBS(fpAPS,data->scrdata[i]);
		}
		fSuc = TRUE;
		break;
	  case	APS_END:
		dbgmsg("END");
		fSuc = TRUE;
		break;
	  case	APS_PING:
		dbgmsg("PING");
		RecvPacketClass(fpAPS);
		fSuc = TRUE;
		break;
	  default:
		printf("class = [%X]\n",(int)c);
		dbgmsg("protocol error");
		fSuc = FALSE;
		break;
	}
dbgmsg("<GetAPS_Value");
	return	(fSuc); 
}

static	void
CatchSEGV(
	int		ec)
{
}

extern	void
ExecuteServer(void)
{
	int		_fhTerm
	,		_fhAps;
	fd_set	ready;
	int		maxfd;

dbgmsg(">ExecuteServer");
	signal(SIGSEGV,(void *)CatchSEGV);

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
	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,"","");
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
		"APS接続待ちポート番号"	 						},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"lddir",	STRING,		TRUE,	(void*)&LD_Dir,
		"データ定義格納ディレクトリ"	 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = IntStrDup(PORT_WFC);
	ApsPortNumber = IntStrDup(PORT_WFC_APS);
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	LD_Dir = NULL;
	BD_Dir = NULL;
	Directory = "./directory";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	int			rc;

	SetDefault();
	GetOption(option,argc,argv);

	InitSystem();
	ExecuteServer();
	CleanUp();
	rc = 0;
	return	(rc);
}
