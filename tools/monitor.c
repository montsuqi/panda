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

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<glib.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include    <sys/socket.h>
#include	<signal.h>
#include	<errno.h>
#include	"types.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*Directory;
static	char	*ApsPath;
static	char	*WfcPath;
static	char	*RedirectorPath;
static	char	*GlserverPath;
static	char	*DBLoggerPath;
static	char	*DBMasterPath;
static	char	*DBSlavePath;
static	char	*DDir;
static	char	*RecDir;
static	char	*ScrDir;
static	char	*MyHost;
static	char	*Log;
static	Bool	fQ;
static	Bool	fRedirector;
static	Bool	fTimer;
static	Bool	fNoApsConnectRetry;
static	int		Interval;
static	int		Wfcinterval;
static	int		MaxTran;
static	int		MaxTransactionRetry;
static	int		Sleep;
static	int		SesNum;
static	Bool	fGlserver;
static	Bool	fGlSSL;
static	char	*GlCacheDir;
static	char	*GlAuth;
static	char	*GlCert;
static	char	*GlCAfile;

static	Bool	fDBMaster;
static	Bool	fDBLog;

static	GList	*ProcessList;
static	volatile sig_atomic_t	fLoop = TRUE;

#define	PTYPE_NULL	(byte)0x00
#define	PTYPE_APS	(byte)0x01
#define	PTYPE_WFC	(byte)0x02
#define	PTYPE_RED	(byte)0x04
#define	PTYPE_GLS	(byte)0x08
#define	PTYPE_LOG	(byte)0x10
#define	PTYPE_MST	(byte)0x20
#define	PTYPE_SLV	(byte)0x40

#define	STATE_RUN		1
#define	STATE_DOWN		2
#define STATE_STOP		3

typedef	struct {
	pid_t	pid;
	int		state;
	byte	type;
	int		argc;
	char	**argv;
}	Process;

#ifdef	DEBUG
static	void
DumpCommand(
	char	**argv)
{
	int		i;

	for	( i = 0 ; argv[i]  !=  NULL ; i ++ ) {
		dbgprintf("%s ",argv[i]);
	}
	dbgmsg("");
}

static	void
DumpProcess(Process *proc)
{
	fprintf(stderr, "pid:%d status:%s\n",
		(int)proc->pid,
		proc->state == STATE_RUN ? "RUN" : "DOWN");
	DumpCommand(proc->argv);
}

static	void
DumpProcessAll(void)
{
	int 	i;
	Process	*proc;

	printf("*** process table dump ***\n");
	for(i=0;i<g_list_length(ProcessList);i++) {
		proc = g_list_nth_data(ProcessList,i);
		DumpProcess(proc);
	}
	printf("**************************\n");
}
#endif

static	ARG_TABLE	option[] = {
	{	"ApsPath",	STRING,		TRUE,	(void*)&ApsPath,
		"aps command path"		 						},
	{	"WfcPath",	STRING,		TRUE,	(void*)&WfcPath,
		"wfc command path"		 						},
	{	"RedPath",	STRING,		TRUE,	(void*)&RedirectorPath,
		"redirector command path"						},
	{	"GlsPath",	STRING,		TRUE,	(void*)&GlserverPath,
		"glserver command path"						},

	{	"DBLoggerPath",	STRING,		TRUE,	(void*)&DBLoggerPath,
		"dblogger command path"							},

	{	"DBMasterPath",	STRING,		TRUE,	(void*)&DBMasterPath,
		"dbmaster command path"							},

	{	"DBSlavePath",	STRING,		TRUE,	(void*)&DBSlavePath,
		"dbslave command path"							},

	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"directory file name"		 					},
	{	"record",	STRING,		TRUE,	(void*)&RecDir,
		"record directory"								},
	{	"screen",	STRING,		TRUE,	(void*)&ScrDir,
		"screen directory"								},
	{	"ddir",		STRING,		TRUE,	(void*)&DDir,
		"LD file directory"			 					},

	{	"redirector",BOOLEAN,	TRUE,	(void*)&fRedirector,
		"start dbredirector"		 					},
	{	"dblogger", BOOLEAN,	TRUE,	(void*)&fDBLog,
		"start dblogger"		 					},
	{	"dbmaster", BOOLEAN,	TRUE,	(void*)&fDBMaster,
		"start dbmaster"		 					},
	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"no check dbredirector start"					},
	{	"nosumcheck",BOOLEAN,	TRUE,	(void*)&fNoSumCheck,
		"no count dbredirector updates"					},
	{	"sendretry",	INTEGER,	TRUE,	(void*)&MaxSendRetry,
		"send retry dbredirector"						},

	{	"glserver",BOOLEAN,		TRUE,	(void*)&fGlserver,
		"start glserver"		 						},
	{	"glcache",	STRING,		TRUE,	(void*)&GlCacheDir,
		"glserver cache directory"		 				},
	{	"glauth",	STRING,		TRUE,	(void*)&GlAuth,
		"glserver authentication"						},
	{	"glssl",	BOOLEAN,	TRUE,	(void*)&fGlSSL,
		"use ssl connection"		 					},
	{	"glcert",	STRING,		TRUE,	(void*)&GlCert,
		"ssl certificate(p12)"							},
	{	"glcafile",	STRING,		TRUE,	(void*)&GlCAfile,
		"ca certificate(pem)"							},

	{	"interval",	INTEGER,	TRUE,	(void*)&Interval,
		"process start interval time" 					},
	{	"wfcwait",	INTEGER,	TRUE,	(void*)&Wfcinterval,
		"wfc start interval time(for slowCPU)"			},
	{	"sesnum",	INTEGER,	TRUE,	(void*)&SesNum,
		"terminal session number"							},
	{	"sesdir",	STRING,		TRUE,	(void*)&SesDir,
		"session keep directory"	 					},

	{	"myhost",	STRING,		TRUE,	(void*)&MyHost,
		"my host name"				 					},

	{	"maxtran",	INTEGER,	TRUE,	(void*)&MaxTran,
		"aps process transaction count"					},
	{	"retry",	INTEGER,	TRUE,	(void*)&MaxTransactionRetry,
		"transaction retry count"						},
	{	"no-aps-retry",	BOOLEAN,	TRUE,	(void*)&fNoApsConnectRetry,
		"don't retry aps commection"					},

	{	"q",		BOOLEAN,	TRUE,	(void*)&fQ,
		"show sub-program options"	 					},
	{	"timer",	BOOLEAN,	TRUE,	(void*)&fTimer,
		"time measuring"								},
	{	"log",		STRING,		TRUE,	(void*)&Log,
		"monitor log file name"							},
	{	"sleep",	INTEGER,	TRUE,	(void*)&Sleep,
		"aps sleep time(for debug)"						},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	ApsPath = NULL;
	WfcPath = NULL;
	RedirectorPath = NULL;
	GlserverPath = NULL;
	DBLoggerPath = NULL;
	DBMasterPath = NULL;
	DBSlavePath = NULL;
	
	Directory = "./directory";
	DDir = NULL;
	RecDir = NULL;
	ScrDir = NULL;
	Log = NULL;
	Interval = 0;
	Wfcinterval = 0;
	MaxTran = 0;
	MaxTransactionRetry = 0;
	MaxSendRetry = 3;
	Sleep = 0;

	MyHost = "localhost";

	fRedirector = FALSE;
	fNoCheck = FALSE;
	fNoSumCheck = FALSE;
	fQ = FALSE;
	fTimer = FALSE;
	fNoApsConnectRetry = FALSE;
	SesNum = 0;
	SesDir = NULL;

	fGlserver = FALSE;
	fGlSSL = FALSE;
	GlCacheDir = NULL;
	GlAuth = NULL;
	GlCert = NULL;
	GlCAfile = NULL;
	fDBLog = FALSE;
	fDBMaster = FALSE;
}

static	void
InitSystem(void)
{
ENTER_FUNC;
	ProcessList = NULL;
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,FALSE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
LEAVE_FUNC;
}

static	void
_execv(
	char	*cmd,
	char	**argv)
{
#ifdef	DEBUG
	DumpCommand(argv);
#endif
 	if (execv(cmd,argv) < 0 ){
 		int errsv = errno;	
 		Error("%s: %s", strerror(errsv), cmd);
 	}
}

static	int
StartProcess(
	Process	*proc,
	int		interval)
{
	pid_t	pid;
#ifdef	DEBUG
	int		i;
#endif

ENTER_FUNC;
  retry:
	if		(  ( pid = fork() )  >  0  ) {
		proc->pid = pid;
		proc->state = STATE_RUN;
#ifdef	DEBUG
		for	( i = 0 ; proc->argv[i]  !=  NULL ; i ++ ) {
			dbgprintf("%s ",proc->argv[i]);
		}
		dbgprintf("(%d)\n",pid);
#endif
	} else
	if		(  pid  ==  0  ) {
		if		(  interval  >  0  ) {
			sleep(interval);
		}
		_execv(proc->argv[0],proc->argv);
	} else {
		Message("can't start process");
		goto	retry;
	}
LEAVE_FUNC;
	return	(pid);
}

static	void
StartRedirector(
	DBG_Struct	*dbg)
{
	pid_t	pid;
	int		argc;
	char	**argv;
	Process	*proc;

ENTER_FUNC;
	proc = New(Process);
	argv = (char **)xmalloc(sizeof(char *) * 15);
	proc->argv = argv;
	proc->type = PTYPE_RED;
	argc = 0;
	if		(  RedirectorPath  !=  NULL  ) {
		argv[argc ++] = RedirectorPath;
	} else
	if		(  ThisEnv->RedPath  !=  NULL  ) {
		argv[argc ++] = ThisEnv->RedPath;
	} else {
		argv[argc ++] = SERVER_DIR "/dbredirector";
	}
	if		(  Directory  !=  NULL  ) {
		argv[argc ++] = "-dir";
		argv[argc ++] = Directory;
	}
	if		(  DDir  !=  NULL  ) {
		argv[argc ++] = "-ddir";
		argv[argc ++] = DDir;
	}
	if		(  RecDir  !=  NULL  ) {
		argv[argc ++] = "-record";
		argv[argc ++] = RecDir;
	}
	if		(  fTimer  ) {
		argv[argc ++] = "-timer";
	}		
	if		(  fNoSumCheck  ) {
		argv[argc ++] = "-nosumcheck";
	}
	argv[argc ++] = dbg->name;
	if		(  fQ  ) {
		argv[argc ++] = "-?";
	}
	argv[argc ++] = "-maxretry";
	argv[argc ++] = IntStrDup(MaxSendRetry);
	proc->argc = argc;
	argv[argc ++] = NULL;
	ProcessList = g_list_append(ProcessList, proc);
	Message("start redirector:%s",dbg->name);
	pid = StartProcess(proc,Interval);
	dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;
LEAVE_FUNC;
}

static	void
StartGlserver(void)
{
	pid_t	pid;
	int		argc;
	char	**argv;
	Process	*proc;

ENTER_FUNC;
	proc = New(Process);
	argv = (char **)xmalloc(sizeof(char *) * 20);
	proc->argv = argv;
	proc->type = PTYPE_GLS;
	argc = 0;
	if		(  GlserverPath  !=  NULL  ) {
		argv[argc ++] = GlserverPath;
	} else {
		argv[argc ++] = SERVER_DIR "/glserver";
	}
	if		(  RecDir  !=  NULL  ) {
		argv[argc ++] = "-record";
		argv[argc ++] = RecDir;
	} else if (ThisEnv->RecordDir != NULL) {
		argv[argc ++] = "-record";
		argv[argc ++] = ThisEnv->RecordDir;
	}
	if		(  ScrDir  !=  NULL  ) {
		argv[argc ++] = "-screen";
		argv[argc ++] = ScrDir;
	}
	if		(  GlCacheDir  !=  NULL  ) {
		argv[argc ++] = "-cache";
		argv[argc ++] = GlCacheDir;
	}
	if		(  GlAuth  !=  NULL  ) {
		argv[argc ++] = "-auth";
		argv[argc ++] = GlAuth;
	}
	argv[argc ++] = "-api";
	if		(  fGlSSL  ) {
		argv[argc ++] = "-ssl";
		if		(  GlCert  !=  NULL  ) {
			argv[argc ++] = "-cert";
			argv[argc ++] = GlCert;
		}
		if		(  GlCAfile  !=  NULL  ) {
			argv[argc ++] = "-CAfile";
			argv[argc ++] = GlCAfile;
		}
	}
	
	proc->argc = argc;
	argv[argc ++] = NULL;
	ProcessList = g_list_append(ProcessList, proc);
	Message("start glserver");
	pid = StartProcess(proc,Interval);
LEAVE_FUNC;
}

static	void
StartDBLog(
	DBG_Struct	*dbg)
{
	pid_t	pid;
	int		argc;
	char	**argv;
	Process	*proc;

ENTER_FUNC;
	proc = New(Process);
	argv = (char **)xmalloc(sizeof(char *) * 15);
	proc->argv = argv;
	proc->type = PTYPE_LOG;
	argc = 0;
	if		(  DBLoggerPath  !=  NULL  ) {
		argv[argc ++] = DBLoggerPath;
	} else
	if		(  ThisEnv->DBLoggerPath  !=  NULL  ) {
		argv[argc ++] = ThisEnv->DBLoggerPath;
	} else {
		argv[argc ++] = SERVER_DIR "/dblogger";
	}
	if		(  Directory  !=  NULL  ) {
		argv[argc ++] = "-dir";
		argv[argc ++] = Directory;
	}
	if		(  DDir  !=  NULL  ) {
		argv[argc ++] = "-ddir";
		argv[argc ++] = DDir;
	}
	if		(  RecDir  !=  NULL  ) {
		argv[argc ++] = "-record";
		argv[argc ++] = RecDir;
	}
	if		(  fTimer  ) {
		argv[argc ++] = "-timer";
	}
	if		(  fNoSumCheck  ) {
		argv[argc ++] = "-nosumcheck";
	}
	argv[argc ++] = dbg->name;
	if		(  fQ  ) {
		argv[argc ++] = "-?";
	}
	argv[argc ++] = "-maxretry";
	argv[argc ++] = IntStrDup(MaxSendRetry);
	proc->argc = argc;
	argv[argc ++] = NULL;
	pid = StartProcess(proc,Interval);
	dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;	
LEAVE_FUNC;
}

static	Bool
HerePort(
	Port	*port)
{
	Bool	ret;
	char	*host;

ENTER_FUNC;
	if		(  port  ==  NULL  ) {
		ret = FALSE;
	} else {
		switch	(port->type) {
		  case	PORT_IP:
			if ((host = IP_HOST(port)) != NULL) {
				ret = ( strcmp(host,MyHost) == 0 ) ? TRUE : TRUE; // what meaning?
			} else {
				ret = FALSE;
			}
			break;
		  case	PORT_UNIX:
			ret = TRUE;
			break;
		  default:
			ret = FALSE;
		}
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
StartDBMaster(void)
{
	int		argc;
	char	**argv;
	Process	*proc;
	int		back;
	int		i;

ENTER_FUNC;
	if		(  HerePort(ThisEnv->DBMasterPort)  ) {
		back = 0;
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			back += ThisEnv->ld[i]->nports;
		}
		proc = New(Process);
		proc->type = PTYPE_MST;
		argv = (char **)xmalloc(sizeof(char *) * 24);
		proc->argv = argv;
		argc = 0;
		if		(  DBMasterPath  !=  NULL  ) {
			argv[argc ++] = DBMasterPath;
		} else
		if		(  ThisEnv->DBMasterPath  !=  NULL  ) {
			argv[argc ++] = ThisEnv->DBMasterPath;
		} else {
			argv[argc ++] = SERVER_DIR "/dbmaster";
		}
		if		(  Directory  !=  NULL  ) {
			argv[argc ++] = "-dir";
			argv[argc ++] = Directory;
		}
		if		(  DDir  !=  NULL  ) {
			argv[argc ++] = "-ddir";
			argv[argc ++] = DDir;
		}
		if		(  RecDir  !=  NULL  ) {
			argv[argc ++] = "-record";
			argv[argc ++] = RecDir;
		}
		if		(  ThisEnv->DBMasterAuth  !=  NULL  ) {
			argv[argc ++] = "-auth";
			argv[argc ++] = ThisEnv->DBMasterAuth;
		}
		argv[argc ++] = "-port";
		argv[argc ++] = StrDup(StringPortName(ThisEnv->DBMasterPort));
		argv[argc ++] = ThisEnv->DBMasterLogDBName;
		
		if		(  fQ  ) {
			argv[argc ++] = "-?";
		}
		proc->argc = argc;
		argv[argc ++] = NULL;
		StartProcess(proc,Interval);
	}
LEAVE_FUNC;
}


static	void
_StartDBLogs(
	DBG_Struct *dbg)
{

ENTER_FUNC;
	if		(  dbg->redirect  !=  NULL  && dbg->redirectorMode == REDIRECTOR_MODE_LOG) {
		_StartDBLogs(dbg->redirect);
	}
	if		(  dbg->process[DB_UPDATE].dbstatus  !=  DB_STATUS_CONNECT  )	{	
		StartDBLog(dbg);
	}
LEAVE_FUNC;
}

static	void
StartDBLogs(void)
{
	int		i;
	DBG_Struct	*dbg;

ENTER_FUNC;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		ThisEnv->DBG[i]->process[DB_UPDATE].dbstatus = DB_STATUS_UNCONNECT;
	}
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		if		(  dbg->redirect  !=  NULL && dbg->redirectorMode == REDIRECTOR_MODE_LOG ) {
			_StartDBLogs(dbg->redirect);
		}
	}
LEAVE_FUNC;
}

static	void
_StartRedirectors(
	DBG_Struct	*dbg)
{

ENTER_FUNC;
	if		(  dbg->redirect  !=  NULL && dbg->redirectorMode == REDIRECTOR_MODE_PATCH ) {
		_StartRedirectors(dbg->redirect);
	}
	if		(  dbg->process[DB_UPDATE].dbstatus  !=  DB_STATUS_CONNECT  )	{
		StartRedirector(dbg);
	}
LEAVE_FUNC;
}

static	void
StartRedirectors(void)
{
	int		i;
	DBG_Struct	*dbg;

ENTER_FUNC;
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		ThisEnv->DBG[i]->process[DB_UPDATE].dbstatus = DB_STATUS_UNCONNECT;
	}
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		if		(  dbg->redirect  !=  NULL && dbg->redirectorMode == REDIRECTOR_MODE_PATCH ) {
			_StartRedirectors(dbg->redirect);
		}
	}
LEAVE_FUNC;
}

static	void
_StartAps(
	LD_Struct	*ld)
{
	int		argc;
	char	**argv;
	Process	*proc;
	int		n;

ENTER_FUNC;
	if		(  ThisEnv->mlevel  !=  MULTI_APS  ) {
		ld->nports = 1;
	}
	dbgprintf("start %d servers",ld->nports);
	for	( n = 0 ; n < ld->nports ; n ++ ) {
		if		(	(  ld->ports[n]  ==  NULL  )
				||	(  HerePort(ld->ports[n])  ) )	{
			proc = New(Process);
			argv = (char **)xmalloc(sizeof(char *) * 22);
			proc->argv = argv;
			proc->type = PTYPE_APS;
			argc = 0;
			if		(  ApsPath  !=  NULL  ) {
				argv[argc ++] = ApsPath;
			} else
			if		(  ThisEnv->ApsPath  !=  NULL  ) {
				argv[argc ++] = ThisEnv->ApsPath;
			} else {
				argv[argc ++] = SERVER_DIR "/aps";
			}
			argv[argc ++] = "-wfcport";
			argv[argc ++] = StrDup(StringPortName(ThisEnv->WfcApsPort));
			if		(  Directory  !=  NULL  ) {
				argv[argc ++] = "-dir";
				argv[argc ++] = Directory;
			}
			if		(  DDir  !=  NULL  ) {
				argv[argc ++] = "-ddir";
				argv[argc ++] = DDir;
			}
			if		(  RecDir  !=  NULL  ) {
				argv[argc ++] = "-record";
				argv[argc ++] = RecDir;
			}
			if		(  fTimer  ) {
				argv[argc ++] = "-timer";
			}
			if		(  !fNoApsConnectRetry  ) {
				argv[argc ++] = "-connect-retry";
			}
			argv[argc ++] = ld->name;
			argv[argc ++] = "-maxtran";
			argv[argc ++] = IntStrDup(MaxTran);
			argv[argc ++] = "-sleep";
			argv[argc ++] = IntStrDup(Sleep);
			argv[argc ++] = "-maxretry";
			argv[argc ++] = IntStrDup(MaxSendRetry);

			if		(  fNoCheck  ) {
				argv[argc ++] = "-nocheck";
			}

			if		(  fQ  ) {
				argv[argc ++] = "-?";
			}
			proc->argc = argc;
			argv[argc ++] = NULL;
			ProcessList = g_list_append(ProcessList, proc);
			StartProcess(proc,Interval);
		}
	}
LEAVE_FUNC;
}

static	void
StartApss(void)
{
	int		i;
	char	str[512];

ENTER_FUNC;
	str[0] = 0;
	for	(i = 0; i < ThisEnv->cLD; i++) {
		_StartAps(ThisEnv->ld[i]);
		strncat(str, ThisEnv->ld[i]->name,sizeof(str) - strlen(str)-1);
		strncat(str, " ",sizeof(str) - strlen(str)-1);
	}
	Message("start aps:%s",str);
LEAVE_FUNC;
}

static	void
StartWfc(void)
{
	int		argc;
	char	**argv;
	Process	*proc;
	int		back;
	int		i;

ENTER_FUNC;
	if		(  HerePort(ThisEnv->WfcApsPort)  ) {
		back = 0;
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			back += ThisEnv->ld[i]->nports;
		}
		proc = New(Process);
		proc->type = PTYPE_WFC;
		argv = (char **)xmalloc(sizeof(char *) * 24);
		proc->argv = argv;
		argc = 0;
		if		(  WfcPath  !=  NULL  ) {
			argv[argc ++] = WfcPath;
		} else
		if		(  ThisEnv->WfcPath  !=  NULL  ) {
			argv[argc ++] = ThisEnv->WfcPath;
		} else {
			argv[argc ++] = SERVER_DIR "/wfc";
		}
		if		(  fTimer  ) {
			argv[argc ++] = "-timer";
		}		
		argv[argc ++] = "-back";
		argv[argc ++] = IntStrDup(back+1);
		argv[argc ++] = "-port";
		argv[argc ++] = StrDup(StringPortName(ThisEnv->TermPort));
		argv[argc ++] = "-apsport";
		argv[argc ++] = StrDup(StringPortName(ThisEnv->WfcApsPort));
		if		(  SesNum  >  0  ) {
			argv[argc ++] = "-sesnum";
			argv[argc ++] = IntStrDup(SesNum);
		}
		if		(  SesDir  !=  NULL  ) {
			argv[argc ++] = "-sesdir";
			argv[argc ++] = SesDir;
		}

		if		(  Directory  !=  NULL  ) {
			argv[argc ++] = "-dir";
			argv[argc ++] = Directory;
		}
		if		(  DDir  !=  NULL  ) {
			argv[argc ++] = "-ddir";
			argv[argc ++] = DDir;
		}
		if		(  RecDir  !=  NULL  ) {
			argv[argc ++] = "-record";
			argv[argc ++] = RecDir;
		}
		if		(  MaxTransactionRetry  >  0  ) {
			argv[argc ++] = "-retry";
			argv[argc ++] = IntStrDup(MaxTransactionRetry);
		}
dbgmsg("*");
		if		(  fQ  ) {
			argv[argc ++] = "-?";
		}
		proc->argc = argc;
		argv[argc ++] = NULL;
		ProcessList = g_list_append(ProcessList, proc);
		Message("start wfc");
		StartProcess(proc,Wfcinterval);
	}
LEAVE_FUNC;
}


static	void
StartServers(void)
{
ENTER_FUNC;
	if		(  fRedirector  ) {
		StartRedirectors();
	}
	if		(  fGlserver  ) {
		StartGlserver();
	}
	if              (  fDBLog  ) {
		StartDBLogs();
	}
	if              (  fDBMaster  ) {
		StartDBMaster();
	}
	StartWfc();
	StartApss();
LEAVE_FUNC;
}

static	void
KillProcess(
	byte	type,
	int		sig)
{
	int		i;
	Process	*proc;

ENTER_FUNC;
	for(i = 0; i < g_list_length(ProcessList); i++) {
		proc = g_list_nth_data(ProcessList,i);
		if ((proc->type & type) != 0) {
			dbgprintf("kill -%d %d\n",sig,proc->pid);
			kill(proc->pid,sig);
			proc->state = STATE_STOP;
		}
	}
LEAVE_FUNC;
}


static	void
StopServers(void)
{
ENTER_FUNC;
	KillProcess(PTYPE_APS,SIGHUP);
	KillProcess(PTYPE_GLS,SIGHUP);
	KillProcess((PTYPE_WFC | PTYPE_RED| PTYPE_LOG | PTYPE_MST),SIGHUP);
LEAVE_FUNC;
}


static	void
StopSystem(void)
{
ENTER_FUNC;
	fLoop = FALSE;
	StopServers();
	Message("stop system");
LEAVE_FUNC;
}

static	void
RestartSystem(void)
{
ENTER_FUNC;
	StopServers();
	Message("restart system");
LEAVE_FUNC;
}

static	Process*
GetProcess(int pid)
{
	Process *proc;
	int i;

	for(i=0;i<g_list_length(ProcessList);i++) {
		proc = (Process*)g_list_nth_data(ProcessList,i);
		if (pid == proc->pid) {
			return proc;
		}
	}
	return NULL;
}
	
static	void
ProcessMonitor(void)
{
	int pid;
	int status;
	Process *proc;
	struct sigaction sa;

ENTER_FUNC;
	/* need for catch SYGCHLD  */
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_DFL;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	proc = NULL;
	while ((pid = waitpid(-1,&status,0)) != -1) {
		proc = GetProcess(pid);
		if (proc == NULL) {
			Error("[BUG] unknown process down:%d",pid);
			break;
		}
		if (proc->state == STATE_RUN) {
			proc->state = STATE_DOWN;
		}
		if (fLoop) {
			if (proc->state == STATE_DOWN) {
				if (WIFSIGNALED(status)) {
					Message("%s(%d) killed by signal %d"
							,proc->argv[0], (int)pid, WTERMSIG(status));
				} else {
					Message("process down pid = %d(%d) Command =[%s]\n"
							,(int)pid, WEXITSTATUS(status),proc->argv[0]);
				}
			}
		}
		switch	(proc->type) {
		case PTYPE_APS:
			if (WIFEXITED(status) && WEXITSTATUS(status) < 2) {
				StopSystem();
			}
			break;
		case PTYPE_WFC:
			if (WIFSIGNALED(status) || 
				(WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
				StopSystem();
			}
			break;
		}
		if (fLoop) {
			/*What should do to set interval ?*/
			StartProcess(proc,0);
		}
	}
LEAVE_FUNC;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST			*fl;
	struct sigaction	sa;

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("monitor",Log);

	if		(  !fRedirector  ) {
		fNoCheck = TRUE;
	}

	InitSystem();
	Message("start system");

	StartServers();

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = (void *)StopSystem;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGUSR1, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	sa.sa_handler = (void *)RestartSystem;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGHUP, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	while (fLoop) {
		ProcessMonitor();
	}
	return	(0);
}
