/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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
*/
#define	DEBUG
#define	TRACE

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<libgen.h>
#include    <sys/socket.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"
#include	"const.h"
#include	"enum.h"
#include	"dirs.h"
#include	"tcp.h"
#include	"comm.h"
#include	"directory.h"
#include	"queue.h"
#include	"dbgroup.h"
#include	"aps_main.h"
#include	"apsio.h"
#include	"handler.h"
#include	"option.h"
#include	"debug.h"

static	Bool	fConnect;
static	sigset_t	hupset;

static	void
InitSystem(
	char	*name)
{
	int		i;

dbgmsg(">InitSystem");
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);
	pthread_sigmask(SIG_BLOCK,&hupset,NULL);

	InitDirectory(TRUE);
	SetUpDirectory(Directory,name,"");
	if		(  ( ThisLD = GetLD(name) )  ==  NULL  ) {
		fprintf(stderr,"LD \"%s\" not found.\n",name);
		exit(1);
	}
	if		(  ThisLD->home  !=  NULL  ) {
		chdir(ThisLD->home);
	}

	InitiateHandler();
	ThisDB = ThisLD->db;
	ThisDBT = ThisLD->DB_Table;

	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		InitializeValue(ThisLD->window[i]->value);
	}

	ReadyDC();
	if		(  ThisLD->cDB  >  0  ) {
		ReadyDB();
	}
dbgmsg("<InitSystem");
}

static	ProcessNode	*
MakeProcessNode(void)
{
	ProcessNode	*node;
	int			i;

dbgmsg(">MakeProcessNode");
	node = New(ProcessNode);
	node->mcprec = ThisEnv->mcprec;
	node->linkrec = ThisEnv->linkrec;
	node->sparec = ThisLD->sparec;
	node->cWindow = ThisLD->cWindow;
	node->scrrec = (ValueStruct **)xmalloc(sizeof(ValueStruct *) * node->cWindow);
	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		node->scrrec[i] = ThisLD->window[i]->value;
	}

	/*	get initialize memory area	*/

	*ValueString(GetItemLongName(node->mcprec,"private.pstatus")) 
		= '0' + APL_SESSION_LINK;
	*ValueString(GetItemLongName(node->mcprec,"private.pputtype")) = '0';
	*ValueString(GetItemLongName(node->mcprec,"private.prc")) = '0';
dbgmsg("<MakeProcessNode");
	return	(node);
}

static	void
FinishSession(
	ProcessNode	*node)
{
dbgmsg(">FinishSession");
	xfree(node);
dbgmsg("<FinishSession");
}

static	void
ExecuteDC(
	void	*para)
{
	int		fhWFC
	,		_fh;
	FILE	*fpWFC;

	ProcessNode	*node;
	WindowBind	*bind;
	int		ix;

dbgmsg(">ExecuteDC");
	if		(  !fConnect  )	{
		_fh = InitServerPort(PortNumber);
		if		(  ( fhWFC = accept(_fh,0,0) )  <  0  )	{
			Error("INET Domain Accept");
		}
		fpWFC = fdopen(fhWFC,"w+");
	} else {
		if		(  WfcPortNumber  ==  NULL  ) {
			WfcPortNumber = ThisLD->wfc->port;
		}
		if		(  WFC_Host  ==  NULL  ) {
			WFC_Host = ThisLD->wfc->host;
		}
#if	0
		printf("%s:%s\n",WFC_Host,WfcPortNumber);
#endif
		if		(  ( fhWFC = ConnectSocket(WfcPortNumber,SOCK_STREAM,WFC_Host) )
				   <  0  ) {
			Error("WFC not ready");
		}
		fpWFC = fdopen(fhWFC,"w+");
		SendString(fpWFC,ThisLD->name);
		if		(  RecvPacketClass(fpWFC)  !=  APS_OK  ) {
			Error("invalid LD name");
		}
	}
	InitAPSIO();

	node = MakeProcessNode();
	while	(TRUE) {
		if		(  !GetWFC(fpWFC,node)  )	break;
#ifdef	DEBUG
		printf("[%s]\n",ThisLD->name);
#endif
		if		(  ( ix = (int)g_hash_table_lookup(ThisLD->whash,
												   ValueString(GetItemLongName(node->mcprec,"dc.window"))))  !=  0  ) {
			bind = ThisLD->window[ix-1];
			printf("module [%s]\n",bind->module);
			if		(  bind->module  ==  NULL  )	break;
			strcpy(ValueString(GetItemLongName(node->mcprec,"dc.module")),bind->module);
			ExecDB_Function("DBSTART",NULL,NULL);
			ExecuteProcess(node);
			ExecDB_Function("DBCOMMIT",NULL,NULL);
			PutWFC(fpWFC,node);
		} else {
			printf("window [%s] not found.\n",ValueString(GetItemLongName(node->mcprec,"dc.window")));
			break;
		}
	}
	printf("exiting DC_Thread\n");
	fclose(fpWFC);
	shutdown(fhWFC, 2);
	FinishSession(node);
dbgmsg("<ExecuteDC");
}

static	void
ExecuteServer(void)
{
dbgmsg(">ExecuteServer");
	ExecuteDC(NULL);
dbgmsg("<ExecuteServer");
}

static	void
StopProcess(
	int		ec)
{
dbgmsg(">StopProcess");
	StopDC();
	CleanUp();
dbgmsg("<StopProcess");
	exit(ec);
}
static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},

	{	"connect",	BOOLEAN,	TRUE,	(void*)&fConnect,
		"コネクトモード(apsからwfcに接続する)"			},
	{	"host",		STRING,		TRUE,	(void*)&WFC_Host,
		"WFC稼働ホスト名"								},
	{	"wfcport",	STRING,		TRUE,	(void*)&WfcPortNumber,
		"WFC接続待ちポート番号"	 						},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"lddir",	STRING,		TRUE,	(void*)&LD_Dir,
		"データ定義格納ディレクトリ"	 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"path",		STRING,		TRUE,	(void*)&LibPath,
		"モジュールのロードパス"						},

	{	"dbhost",	STRING,		TRUE,	(void*)&DB_Host,
		"データベース稼働ホスト名"						},
	{	"dbport",	STRING,		TRUE,	(void*)&DB_Port,
		"データベース待機ポート番号"					},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"データベース名"								},
	{	"dbuser",	STRING,		TRUE,	(void*)&DB_User,
		"データベースのユーザ名"						},
	{	"dbpass",	STRING,		TRUE,	(void*)&DB_Pass,
		"データベースのパスワード"						},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	WfcPortNumber = NULL;
	WFC_Host = NULL;
	PortNumber = IntStrDup(PORT_APS_BASE);
	Back = 5;
	fConnect = FALSE;

	BaseDir = NULL;
	RecordDir = NULL;
	LD_Dir = NULL;
	BD_Dir = NULL;
	Directory = "./directory";
	LibPath = NULL;

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	int			rc;

	(void)signal(SIGHUP,(void *)StopProcess);

	SetDefault();
	fl = GetOption(option,argc,argv);

	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		InitSystem(fl->name);
		ExecuteServer();
		StopProcess(0);
		rc = 0;
	} else {
		rc = -1;
		fprintf(stderr,"LD名が指定されていません\n");
	}
	exit(rc);
}
