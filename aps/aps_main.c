/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

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

#include	"const.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"enum.h"
#include	"dirs.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"directory.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"handler.h"
#include	"wfcdata.h"
#include	"apsio.h"
#include	"aps_main.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*WfcPortNumber;

static	int		MaxTran;
static	int		Sleep;
static	Bool	fConnectRetry;

static	void
InitSystem(
	char	*name)
{
	int		i;

ENTER_FUNC;
	InitDirectory();
	SetUpDirectory(Directory,name,"","",TRUE);
	if		(  ( ThisLD = GetLD(name) )  ==  NULL  ) {
		Error("LD \"%s\" not found.",name);
	}
	if		(  ThisLD->home  !=  NULL  ) {
		chdir(ThisLD->home);
	}
	ThisBD = NULL;
	ThisDBD = NULL;

	InitiateHandler();
	ThisDB = ThisLD->db;
	DB_Table = ThisLD->DB_Table;
	TextSize = ThisLD->textsize;
	CurrentUser = (char *)xmalloc(SIZE_USER);
	CurrentTerm = (char *)xmalloc(SIZE_TERM);
	InitializeValue(ThisEnv->mcprec->value);
	if		(  ThisEnv->linkrec  !=  NULL  ) {
		InitializeValue(ThisEnv->linkrec->value);
	}
	if		(  ThisLD->sparec  !=  NULL  ) {
		InitializeValue(ThisLD->sparec->value);
	}

	for	( i = 0 ; i < ThisLD->cBind ; i ++ ) {
		if		(	(  ThisLD->binds[i]  !=  NULL  )
				&&	(  ThisLD->binds[i]->rec  !=  NULL  ) ) {
			dbgprintf("[%s]",ThisLD->binds[i]->rec->name);
			InitializeValue(ThisLD->binds[i]->rec->value);
		}
	}
	ReadyDC();
LEAVE_FUNC;
}

static	ProcessNode	*
MakeProcessNode(void)
{
	ProcessNode	*node;
	int			i;

ENTER_FUNC;
	node = New(ProcessNode);
	node->mcprec = ThisEnv->mcprec;
	node->linkrec = ThisEnv->linkrec;
	node->sparec = ThisLD->sparec;
	node->cWindow = ThisLD->cWindow;
	node->cBind = ThisLD->cBind;
	node->bhash = ThisLD->bhash;
	node->textsize = ThisLD->textsize;
	node->scrrec = (RecordStruct **)xmalloc(sizeof(RecordStruct *) * node->cWindow);
	node->dbstatus = GetDBRedirectStatus(0);
	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		node->scrrec[i] = ThisLD->windows[i];
	}

	/*	get initialize memory area	*/

	SetValueInteger(GetItemLongName(node->mcprec->value,"private.pstatus"),APL_SESSION_LINK);
	SetValueInteger(GetItemLongName(node->mcprec->value,"private.pputtype"),0);
	SetValueInteger(GetItemLongName(node->mcprec->value,"private.prc"),0);
LEAVE_FUNC;
	return	(node);
}

static	void
FinishSession(
	ProcessNode	*node)
{
ENTER_FUNC;
	xfree(node->scrrec); 
	xfree(node);
LEAVE_FUNC;
}

static	int
ExecuteServer(void)
{
	int		fhWFC
		,	rc;
	Port	*port;
	NETFILE		*fpWFC;
	ProcessNode	*node;
	WindowBind	*bind;
	int		tran;
	char	wname[SIZE_LONGNAME+1];

ENTER_FUNC;
	if		(  WfcPortNumber  ==  NULL  ) {
		port = ThisEnv->WfcApsPort;
	} else {
		port = ParPortName(WfcPortNumber);
	}
  retry:
	fpWFC = NULL;
	rc = 0;
	while	(  ( fhWFC = ConnectSocket(port,SOCK_STREAM) )  <  0  ) {
		if		(  !fConnectRetry  )	goto	quit;
		dbgmsg("WFC connection retry");
		sleep(1);
	}
	fpWFC = SocketToNet(fhWFC);
	SendStringDelim(fpWFC,ThisLD->name);
	SendStringDelim(fpWFC,"\n");
	if		(  RecvPacketClass(fpWFC)  !=  APS_OK  ) {
		if		(  !CheckNetFile(fpWFC) ) {
			Warning("WFC connection lost");
			CloseNet(fpWFC);
			goto	retry;
		}
		Error("invalid LD name");
	}
	InitAPSIO(fpWFC);
	if ( ReadyOnlineDB(fpWFC) < 0 ){
		Error("Online DB is not ready");
	}
	node = MakeProcessNode();
	for	( tran = MaxTran;(	(  MaxTran  ==  0  )
						||	(  tran     >   0  ) ); tran -- ) {
		if		(  !GetWFC(fpWFC,node)	) {
			Message("GetWFC failure");
			rc = -1;
			break;
		}
		if		(  node->pstatus  ==  APL_SYSTEM_END  ) {
			if (node->messagetype == MESSAGE_TYPE_API) {
				continue;
			} else {
				Message("system stop");
				rc = 0;
			}
			break;
		}
		dbgprintf("ld     = [%s]",ThisLD->name);
		dbgprintf("window = [%s]",ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")));
		PureComponentName(ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")),wname);

		if		(  ( bind = (WindowBind *)g_hash_table_lookup(ThisLD->bhash,wname) )
				   ==  NULL  ) {
			Message("window [%s] not found.",wname);
			rc = 2;
			break;
		}
		SetValueString(GetItemLongName(node->mcprec->value,"dc.module"),bind->module,NULL);
		if ( node->dbstatus == DB_STATUS_REDFAILURE ) {
			RedirectError();
		} else {
			node->dbstatus = GetDBRedirectStatus(0);
		}
		strcpy(CurrentUser, ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.user")));
		strcpy(CurrentTerm, ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.term")));
		TransactionStart(NULL);
		ExecuteProcess(node);
		if		(  Sleep  >  0  ) {
			sleep(Sleep);
		}
		TransactionEnd(NULL);
		PutWFC(fpWFC,node);
	}
	MessageLogPrintf("exiting APS (%s)",ThisLD->name);
	FinishSession(node);
  quit:
#if	1
	if		(  fpWFC  !=  NULL  ) {
		CloseNet(fpWFC);
	}
#endif
LEAVE_FUNC;
	return	(rc);
}

static	void
HungUp(void)
{
	exit(15);
}

static	void
StopProcess(void)
{
ENTER_FUNC;
	StopOnlineDB(); 
	CleanUpOnlineDB();
	StopDC();
	CleanUpOnlineDC();
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"wfcport",	STRING,		TRUE,	(void*)&WfcPortNumber,
		"WFC waiting port"		 						},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"base environment directory path"	 			},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		".rec directory"				 				},
	{	"lddir",	STRING,		TRUE,	(void*)&D_Dir,
		"LD directory path"				 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"directory file name"	 						},
	{	"path",		STRING,		TRUE,	(void*)&LibPath,
		"application module load path"					},

	{	"dbhost",	STRING,		TRUE,	(void*)&DB_Host,
		"DB host name"									},
	{	"dbport",	STRING,		TRUE,	(void*)&DB_Port,
		"DB port number"								},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"DB name"										},
	{	"dbuser",	STRING,		TRUE,	(void*)&DB_User,
		"DB user name"									},
	{	"dbpass",	STRING,		TRUE,	(void*)&DB_Pass,
		"DB password"									},

	{	"maxtran",	INTEGER,	TRUE,	(void*)&MaxTran,
		"aps process transaction count"					},
	{	"cache",	INTEGER,	TRUE,	(void*)&nCache,
		"cache terminal number"							},

	{	"sleep",	INTEGER,	TRUE,	(void*)&Sleep,
		"aps sleep time(for debug)"						},
	{	"timer",	BOOLEAN,	TRUE,	(void*)&fTimer,
		"time measuring"								},

	{	"connect-retry",	BOOLEAN,	TRUE,	(void*)&fConnectRetry,
		"WFC connection retry"							},

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"no check dbredirector start"					},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"no use dbredirector"							},
	{	"maxretry",	INTEGER,	TRUE,	(void*)&MaxSendRetry,
		"max retry dbredirector"						},
	{	"retryint",	INTEGER,	TRUE,	(void*)&RetryInterval,
		"dbredirector retry interval(sec)"				},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	WfcPortNumber = NULL;
	fNoCheck = FALSE;
	fNoRedirect = FALSE;

	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	LibPath = NULL;
	MaxTran = 0;
	nCache = 0;
	Sleep = 0;

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;

	MaxSendRetry = 3;
	RetryInterval = 5;

	fTimer = FALSE;
	fConnectRetry = FALSE;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	int			rc;
	char		id[128];
	struct sigaction	sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = (void *)HungUp;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGHUP, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGUSR2, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		snprintf(id, sizeof(id), "aps-%s",fl->name);
		InitMessage(id,NULL);
		InitNET();
		InitSystem(fl->name);
		rc = ExecuteServer();
		StopProcess();
	} else {
		rc = -1;
		fprintf(stderr, "LD name is not specified.\n");
	}
	exit(rc);
}
