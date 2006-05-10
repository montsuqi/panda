/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
#include	<signal.h>
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
static	char	*DDir;
static	char	*RecDir;
static	char	*MyHost;
static	char	*Log;
static	Bool	fQ;
static	Bool	fRedirector;
static	Bool	fRestart;
static	int		interval;
static	int		wfcinterval;
static	int		MaxTran;
static	int		Sleep;

static	FILE		*fpLog;

static	GHashTable	*ProcessTable;

#define	PTYPE_NULL	(byte)0x00
#define	PTYPE_APS	(byte)0x01
#define	PTYPE_WFC	(byte)0x02
#define	PTYPE_RED	(byte)0x04

typedef	struct {
	pid_t	pid;
	byte	type;
	int		argc;
	int		count;
	char	**argv;
}	Process;

static	void	StopSystem(void);

#ifdef	DEBUG
static	void
DumpCommand(
	char	**argv)
{
	int		i;

	printf("command = [%s]\n",argv[0]);
	for	( i = 0 ; argv[i] != NULL ; i ++ ) {
		printf("\targ[%d] = [%s]\n",i,argv[i]);
	}
}

static	void
_DumpProcess(
	pid_t		pid,
	Process		*proc,
	void		*dummy)
{
	printf("pid = %d\n",(int)pid);
	DumpCommand(proc->argv);
}
static	void
DumpProcess(void)
{
	printf("*** process table dump ***\n");
	g_int_hash_table_foreach(ProcessTable,(GHFunc)_DumpProcess,NULL);
	printf("**************************\n");
}
#endif


static	void
_execv(
	char	*cmd,
	char	**argv)
{
#ifdef	DEBUG
	DumpCommand(argv);
#endif
	execv(cmd,argv);
}

static	int
StartProcess(
	Process	*proc)
{
	pid_t	pid;
	int		i;

dbgmsg(">StartProcess");
	if		(  ( pid = fork() )  >  0  ) {
		proc->pid = pid;
		g_int_hash_table_insert(ProcessTable,pid,proc);
		for	( i = 0 ; proc->argv[i]  !=  NULL ; i ++ ) {
			fprintf(fpLog,"%s ",proc->argv[i]);
		}
		fprintf(fpLog,"(%d)\n",pid);
		fflush(fpLog);
	} else {
		_execv(proc->argv[0],proc->argv);
	}
	sleep(interval);
dbgmsg("<StartProcess");
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

dbgmsg(">StartRedirector");
	proc = New(Process);
	argv = (char **)xmalloc(sizeof(char *) * 12);
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
	if		(  fNoSumCheck  ) {
		argv[argc ++] = "-nosumcheck";
	}
	argv[argc ++] = dbg->name;
	if		(  fQ  ) {
		argv[argc ++] = "-?";
	}
	proc->argc = argc;
	argv[argc ++] = NULL;
	pid = StartProcess(proc);
	dbg->fConnect = CONNECT;
dbgmsg("<StartRedirector");
}

static	Bool
HerePort(
	Port	*port)
{
	Bool	ret;

	if		(  port  ==  NULL  ) {
		ret = FALSE;
	} else {
		switch	(port->type) {
		  case	PORT_IP:
			ret = ( strcmp(IP_HOST(port),MyHost) == 0 ) ? TRUE : TRUE;
			break;
		  case	PORT_UNIX:
			ret = TRUE;
			break;
		  default:
			ret = FALSE;
		}
	}
	return	(ret);
}

static	void
_StartRedirectors(
	DBG_Struct	*dbg)
{

ENTER_FUNC;
	if		(  dbg->redirect  !=  NULL  ) {
		_StartRedirectors(dbg->redirect);
	}
	if		(  dbg->fConnect != CONNECT ) {
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
		ThisEnv->DBG[i]->fConnect = UNCONNECT;
	}
	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		if		(  dbg->redirect  !=  NULL  ) {
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

dbgmsg(">_StartAps");
	if		(  ThisEnv->mlevel  !=  MULTI_APS  ) {
		ld->nports = 1;
	}
	for	( n = 0 ; n < ld->nports ; n ++ ) {
		if		(	(  ld->ports[n]  ==  NULL  )
				||	(  HerePort(ld->ports[n])  ) )	{
			proc = New(Process);
			argv = (char **)xmalloc(sizeof(char *) * 20);
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
			argv[argc ++] = ld->name;
			argv[argc ++] = "-connect";
			argv[argc ++] = "-maxtran";
			argv[argc ++] = IntStrDup(MaxTran);
			argv[argc ++] = "-sleep";
			argv[argc ++] = IntStrDup(Sleep);

			if		(  fNoCheck  ) {
				argv[argc ++] = "-nocheck";
			}

			if		(  fQ  ) {
				argv[argc ++] = "-?";
			}
			proc->argc = argc;
			argv[argc ++] = NULL;
			StartProcess(proc);
		}
	}
dbgmsg("<_StartAps");
}

static	void
StartApss(void)
{
	int		i;

dbgmsg(">StartApss");
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		_StartAps(ThisEnv->ld[i]);
	}
dbgmsg("<StartApss");
}

static	void
StartWfc(void)
{
	int		argc;
	char	**argv;
	Process	*proc;
	int		back;
	int		i;

dbgmsg(">StartWfc");
	if		(  HerePort(ThisEnv->WfcApsPort)  ) {
		back = 0;
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			back += ThisEnv->ld[i]->nports;
		}
		proc = New(Process);
		proc->type = PTYPE_WFC;
		argv = (char **)xmalloc(sizeof(char *) * 19);
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
		argv[argc ++] = "-back";
		argv[argc ++] = IntStrDup(back+1);
		argv[argc ++] = "-port";
		argv[argc ++] = StrDup(StringPortName(ThisEnv->TermPort));
		argv[argc ++] = "-apsport";
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
		if		(  MaxRetry  >  0  ) {
			argv[argc ++] = "-retry";
			argv[argc ++] = IntStrDup(MaxRetry);
		}
		if		(  fQ  ) {
			argv[argc ++] = "-?";
		}
		proc->argc = argc;
		argv[argc ++] = NULL;
		StartProcess(proc);
		sleep(wfcinterval);
	}
dbgmsg("<StartWfc");
}


static	void
StartServers(void)
{
dbgmsg(">StartServers");
	ProcessTable = NewIntHash();
	if		(  fRedirector  ) {
		StartRedirectors();
	}
	StartWfc();
	StartApss();
dbgmsg("<StartServers");
}

static	void
StartPrograms(void)
{
ENTER_FUNC;
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,FALSE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
	StartServers();
#ifdef	DEBUG
	DumpProcess();
#endif
LEAVE_FUNC;
}

typedef	struct {
	byte	type;
	int		sig;
}	KILLALL;

static	void
_KillProcess(
	pid_t		pid,
	Process		*proc,
	KILLALL		*kills)
{
	if		(  ( kills->type & proc->type )  !=  0  ) {
		dbgprintf("kill -%d %d\n",kills->sig,pid);
		kill(pid,kills->sig);
	}
}

static	void
KillAllProcess(
	byte	type,
	int		sig)
{
	KILLALL	kills;

	kills.type = type;
	kills.sig = sig;
	g_int_hash_table_foreach(ProcessTable,(GHFunc)_KillProcess,(void *)&kills);
}

static	void
ProcessMonitor(void)
{
	pid_t	pid;
	int		status;
	Process	*proc;

	do {
		pid = wait(&status);
		if		(  pid  >  0  ) {
			proc = g_int_hash_table_lookup(ProcessTable,pid);
			fprintf(fpLog,"process down pid = %d(%d) Command =[%s]\n"
					,(int)pid,WEXITSTATUS(status),proc->argv[0]);
#ifdef	DEBUG
			DumpCommand(proc->argv);
#endif
			if		(  proc->type  ==  PTYPE_WFC  ) {
				StopSystem();
			}
			if		(  fRestart  ) {
				if		(  proc->type  ==  PTYPE_APS  ) {
					if		(	(  WIFEXITED(status)  )
							&&	(  WEXITSTATUS(status)  <  2  )	) {
						StopSystem();
					} else {
						g_int_hash_table_remove(ProcessTable,pid);
						StartProcess(proc);
					}
				}
			}
		}
	}	while	(TRUE);
}

static	void
InitSystem(void)
{
dbgmsg(">InitSystem");
	if		(  Log  !=  NULL  ) {
		if		(  ( fpLog = fopen(Log,"a+") )  ==  NULL  ) {
			fpLog = stdout;
		}
	} else {
		fpLog = stdout;
	}
dbgmsg("<InitSystem");
}

static	void
WaitStop(void)
{
	while	(  waitpid(-1,0,WNOHANG)  >  0  );
	exit(0);
}

static	void
StopSystem(void)
{
	fRestart = FALSE;
	signal(SIGCHLD,(void *)WaitStop);
	KillAllProcess((PTYPE_APS | PTYPE_RED ),SIGKILL);
	KillAllProcess(PTYPE_WFC,SIGUSR1);
	exit(0);
}

static	void
StopApss(void)
{
	sleep(interval);
	KillAllProcess(PTYPE_APS,SIGKILL);
}

static	ARG_TABLE	option[] = {
	{	"ApsPath",	STRING,		TRUE,	(void*)&ApsPath,
		"apsコマンドパス"		 						},
	{	"WfcPath",	STRING,		TRUE,	(void*)&WfcPath,
		"wfcコマンドパス"		 						},
	{	"RedPath",	STRING,		TRUE,	(void*)&RedirectorPath,
		"redirectorコマンドパス"						},

	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecDir,
		"レコードのあるディレクトリ"					},
	{	"ddir",		STRING,		TRUE,	(void*)&DDir,
		"LD定義格納ディレクトリ"	 					},

	{	"redirector",BOOLEAN,	TRUE,	(void*)&fRedirector,
		"dbredirectorを起動する"	 					},
	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"dbredirectorの起動をチェックしない"			},
	{	"nosumcheck",BOOLEAN,	TRUE,	(void*)&fNoSumCheck,
		"dbredirectorで更新数をチェックしない"			},

	{	"restart",	BOOLEAN,	TRUE,	(void*)&fRestart,
		"aps異常終了時に再起動する"	 					},

	{	"wait",		INTEGER,	TRUE,	(void*)&interval,
		"プロセス操作時の待ち時間"	 					},
	{	"wfcwait",	INTEGER,	TRUE,	(void*)&wfcinterval,
		"wfc起動後の待ち時間(遅いCPU用)"				},

	{	"myhost",	STRING,		TRUE,	(void*)&MyHost,
		"自分のホスト名を指定する"	 					},

	{	"maxtran",	INTEGER,	TRUE,	(void*)&MaxTran,
		"apsの処理するトランザクション数を指定する"		},
	{	"retry",	INTEGER,	TRUE,	(void*)&MaxRetry,
		"トランザクションを再試行する時の上限数"		},

	{	"q",		BOOLEAN,	TRUE,	(void*)&fQ,
		"-?を指定する"				 					},
	{	"log",		STRING,		TRUE,	(void*)&Log,
		"実行ログを取るファイル名を指定する"			},
	{	"sleep",	INTEGER,	TRUE,	(void*)&Sleep,
		"実行時間に足す処理時間(for debug)"				},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	ApsPath = NULL;
	WfcPath = NULL;
	RedirectorPath = NULL;

	Directory = "./directory";
	DDir = NULL;
	RecDir = NULL;
	Log = NULL;
	interval = 0;
	wfcinterval = 0;
	MaxTran = 0;
	MaxRetry = 0;
	Sleep = 0;

	MyHost = "localhost";

	fRedirector = FALSE;
	fNoCheck = FALSE;
	fNoSumCheck = FALSE;
	fRestart = FALSE;
	fQ = FALSE;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("monitor",NULL);

	signal(SIGUSR1,(void *)StopSystem);
	signal(SIGHUP,(void *)StopApss);

	if		(  !fRedirector  ) {
		fNoCheck = TRUE;
	}
	InitSystem();
	StartPrograms();
	ProcessMonitor();
	return	(0);
}
