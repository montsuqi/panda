/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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

static	sigset_t	hupset;
static	int		MaxTran;
static	int		nPool;
static	int		Sleep;
static	Bool	fConnectRetry;

static	void
InitSystem(
	char	*name)
{
	int		i;

ENTER_FUNC;
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);
	pthread_sigmask(SIG_BLOCK,&hupset,NULL);
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
	node->dbstatus = DB_STATUS_NOCONNECT;
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

static	DB_Environment	**ConnectionPool;
static	GHashTable		*ConnectionHash;
static	void
InitConnectionPool(void)
{
	int		i;

ENTER_FUNC;
	ConnectionPool = (DB_Environment **)xmalloc(sizeof(DB_Environment *) * nPool);
	for	( i = 0 ; i < nPool ; i ++ ) {
		ConnectionPool[i] = NULL;
	}
	ConnectionHash = NewNameHash();
LEAVE_FUNC;
}

static	DB_Environment	*
GetConnection(
	char	*id)
{
	DB_Environment	*env;

	env = g_hash_table_lookup(ConnectionHash,id);

	return	(env);
}

static	void
CloseConnections(void)
{
	DB_Environment	*env;
	int		i;

	for	( i = 0 ; i < nPool ; i ++ ) {
		if		(  ( env = ConnectionPool[i] )  !=  NULL  ) {
			if		(  env->dbstatus  ==  DB_STATUS_PREPARE  ) {
			}
		}
	}
}

static	DB_Environment	*
GetFreeConnection(
	char	*id,
	NETFILE	*fpWFC)
{
	DB_Environment	*env;
	int		i;

ENTER_FUNC;
	if		(  ( env = GetConnection(id) )  ==  NULL  ) {
		dbgmsg("New connection");
		for	( i = 0 ; i < nPool ; i ++ ) {
			if		(	(  ConnectionPool[i]            !=  NULL               )
					&&	(	(  ConnectionPool[i]->dbstatus  ==  DB_STATUS_NOCONNECT  )
						||	(  ConnectionPool[i]->dbstatus  ==  DB_STATUS_CONNECT    ) ) )	break;
		}
		if		(  i  <  nPool  ) {
			dbgmsg("reuse pool");
			env = ConnectionPool[i];
			g_hash_table_remove(ConnectionHash,env->id);
			strcpy(env->id,id);
			g_hash_table_insert(ConnectionHash,env->id,env);
		} else {
			dbgmsg("new pool");
			for	( i = 0 ; i < nPool ; i ++ ) {
				if		(  ConnectionPool[i]  ==  NULL  )	break;
			}
			if		(  i  <  nPool  ) {
				dbgmsg("allocate");
				if		(  ( env = ReadyOnlineDB(fpWFC) )   ==  NULL  )	{
					Error("Online DB is not ready");
				}
				ConnectionPool[i] = env;
				strcpy(env->id,id);
				g_hash_table_insert(ConnectionHash,env->id,env);
			} else {
				Error("to many active transaction");
			}
		}
		env->dbstatus = DB_STATUS_CONNECT;
	}
LEAVE_FUNC;
	return	(env);
}

static	int
ProcessEvent(
	ProcessNode	*node,
	DB_Environment	*env)
{
	char	wname[SIZE_LONGNAME+1];
	WindowBind	*bind;
	int		rc;

	dbgprintf("ld     = [%s]",ThisLD->name);
	dbgprintf("window = [%s]",ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")));

	PureComponentName(ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")),wname);
	if		(  ( bind = (WindowBind *)g_hash_table_lookup(ThisLD->bhash,wname) )
			   !=  NULL  ) {
		if		(  bind->module  ==  NULL  ){
			Message("bind->module not found");
			rc = 2;
		} else {
			rc = 0;
			SetValueString(GetItemLongName(node->mcprec->value,"dc.module"),bind->module,NULL);
			if ( node->dbstatus == DB_STATUS_REDFAILURE ) {
				RedirectError();
			} else {
				node->dbstatus = GetDBStatus(env);
			}
			ExecuteProcess(node,env);
			if		(  Sleep  >  0  ) {
				sleep(Sleep);
			}
		}
	} else {
		Message("window [%s] not found.",wname);
		rc = 2;
	}
	return	(rc);
}

static	int
ExecuteServer(void)
{
	int		fhWFC
		,	rc;
	Port	*port;
	NETFILE		*fpWFC;
	ProcessNode	*node;
	int		tran;
	char	term[SIZE_TERM+1];
	DB_Environment	*env;
	int		c;

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
		Warning("WFC connection retry %s",StringPort(port));
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
	InitConnectionPool();
	InitAPSIO(fpWFC);
	node = MakeProcessNode();
	tran = MaxTran;
	while	(  RecvPacketClass(fpWFC)  ==  APS_REQ  ) {
		RecvnString(fpWFC, sizeof(term), term);			ON_IO_ERROR(fpWFC,quit);
		dbgprintf("term = [%s]\n",term);
		env = GetFreeConnection(term,fpWFC);
		if		(  ( c = RecvPacketClass(fpWFC) )  ==  APS_EVENT  ) {
			dbgmsg("APS_EVENT");
			if		(  GetWFC(fpWFC,node,term)	) {
				if		(  MaxTran  >  0  ) {
					tran --;
					if		(  tran  ==  0  )	break;
				}
				if		(  ( rc = ProcessEvent(node,env) )  ==  0  ) {
					PutWFC(fpWFC,node);
				}
			} else {
				Message("GetWFC failure");
				rc = -1;
				goto	quit;
			}
		} else {
			switch	(c)	{
			  case	APS_START:
				dbgmsg("APS_START");
				switch	(env->dbstatus) {
				  case	DB_STATUS_CONNECT:
					rc = TransactionAllStart(env);
					break;
				  case	DB_STATUS_START:
					rc = 0;
					break;
				  default:
					Error("transaction sequence is invalid(start) %02X",env->dbstatus);
					rc = -1;
				}
				break;
			  case	APS_PREPARE:
				dbgmsg("APS_PREPARE");
				if		(  env->dbstatus  ==  DB_STATUS_START  ) {
					rc = TransactionAllPrepare(env);
				} else
				if		(  env->dbstatus  ==  DB_STATUS_CONNECT  ) {
					(void)TransactionAllPrepare(env);
					rc = 0;
				} else {
					rc = -1;
					Error("transaction sequence is invalid(prepare) %02X",env->dbstatus);
				}
				break;
			  case	APS_COMMIT:
				dbgmsg("APS_COMMIT");
				if		(	(  env->dbstatus  ==  DB_STATUS_START    )
						||	(  env->dbstatus  ==  DB_STATUS_PREPARE  ) ) {
					rc = TransactionAllCommit(env);
				} else
				if		(  env->dbstatus  ==  DB_STATUS_CONNECT  ) {
					(void)TransactionAllCommit(env);
					rc = 0;
				} else {
					rc = -1;
					Error("transaction sequence is invalid(prepare) %02X",env->dbstatus);
				}
				break;
			  case	APS_ROLLBACK:
				dbgmsg("APS_ROLLBACK");
				if		(	(  env->dbstatus  ==  DB_STATUS_START    )
						||	(  env->dbstatus  ==  DB_STATUS_PREPARE  ) ) {
					rc = TransactionAllRollback(env);
				} else
				if		(  env->dbstatus  ==  DB_STATUS_CONNECT  ) {
					(void)TransactionAllRollback(env);
					rc = 0;
				} else {
					rc = -1;
					Error("transaction sequence is invalid(prepare) %02X",env->dbstatus);
				}
				break;
			}
			if		(  rc  ==  0  ) {
				SendPacketClass(fpWFC,APS_OK);		ON_IO_ERROR(fpWFC,quit);
			} else {
				SendPacketClass(fpWFC,APS_NOT);		ON_IO_ERROR(fpWFC,quit);
			}
		}
		if		(  node->pstatus  ==  APL_SYSTEM_END  ) {
			Message("system stop");
			rc = 0;
			break;
		}
		if		(  rc  !=  0  )	break;
	}
	StopOnlineDB(env); 
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
StopProcess(void)
{
ENTER_FUNC;
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
	{	"dbsslmode",	STRING,		TRUE,	(void*)&DB_Sslmode,
		"DB SSL mode"									},

	{	"maxtran",	INTEGER,	TRUE,	(void*)&MaxTran,
		"aps process transaction count"					},
	{	"cache",	INTEGER,	TRUE,	(void*)&nCache,
		"cache terminal number"							},
	{	"pool",	INTEGER,	TRUE,		(void*)&nPool,
		"max number of DB connection pool"				},

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
	nPool = 1;
	Sleep = 0;

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;
	DB_Sslmode = NULL;

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

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	(void)signal(SIGHUP,(void *)StopProcess);
	(void)signal(SIGUSR2, SIG_IGN);
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
