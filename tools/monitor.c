/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*Directory;
static	char	*ApsPath;
static	char	*WfcPath;
static	char	*RedirectorPath;
static	char	*LDDir;
static	char	*BDDir;
static	char	*RecDir;
static	char	*MyHost;
static	char	*Log;
static	Bool	fQ;
static	Bool	fRedirector;
static	Bool	fRestart;
static	int		interval;
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
	char	**argv;
}	Process;

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
	argv[argc ++] = RedirectorPath;
	if		(  Directory  !=  NULL  ) {
		argv[argc ++] = "-dir";
		argv[argc ++] = Directory;
	}
	if		(  LDDir  !=  NULL  ) {
		argv[argc ++] = "-lddir";
		argv[argc ++] = LDDir;
	}
	if		(  BDDir  !=  NULL  ) {
		argv[argc ++] = "-bddir";
		argv[argc ++] = BDDir;
	}
	if		(  RecDir  !=  NULL  ) {
		argv[argc ++] = "-record";
		argv[argc ++] = RecDir;
	}
	argv[argc ++] = dbg->name;
	if		(  fQ  ) {
		argv[argc ++] = "-?";
	}
	proc->argc = argc;
	argv[argc ++] = NULL;
	pid = StartProcess(proc);
dbgmsg("<StartRedirector");
}

static	void
_StartRedirectors(
	DBG_Struct	*dbg)
{

	DBG_Struct	*dbgs[10];
	int			i;

	if		(  dbg->dbt  !=  NULL  ) {
		if		(  dbg->redirect  !=  NULL  ) {
			i = 0;
			do {
				dbgs[i] = dbg;
				i ++;
				dbg = dbg->redirect;
			}	while	(  dbg  !=  NULL  );
			i --;
			for	( ; i > 0 ; i -- ) {
				if		(  !strcmp(dbgs[i-1]->redirectPort->host,MyHost)  ) {
					StartRedirector(dbgs[i]);
				}
			}
#if	0
			StartRedirector(dbgs[0]);
#endif
		}
	}
}

static	void
StartRedirectors(void)
{
	int		i;
	DBG_Struct	*dbg;

	for	( i = 0 ; i < ThisEnv->cDBG ; i ++ ) {
		dbg = ThisEnv->DBG[i];
		_StartRedirectors(dbg);
	}
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
		if		(	(  ld->ports[n]  ==  NULL              )
				||	(  !strcmp(ld->ports[n]->host,MyHost)  ) )	{
			proc = New(Process);
			argv = (char **)xmalloc(sizeof(char *) * 19);
			proc->argv = argv;
			proc->type = PTYPE_APS;
			argc = 0;
			argv[argc ++] = ApsPath;
			argv[argc ++] = "-wfcport";
			argv[argc ++] = ThisEnv->WfcApsPort->port;
			argv[argc ++] = "-host";
			argv[argc ++] = ThisEnv->WfcApsPort->host;
			if		(  Directory  !=  NULL  ) {
				argv[argc ++] = "-dir";
				argv[argc ++] = Directory;
			}
			if		(  LDDir  !=  NULL  ) {
				argv[argc ++] = "-lddir";
				argv[argc ++] = LDDir;
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
	if		(  !strcmp(ThisEnv->WfcApsPort->host,MyHost)  ) {
		back = 0;
		for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
			back += ThisEnv->ld[i]->nports;
		}
		proc = New(Process);
		proc->type = PTYPE_WFC;
		argv = (char **)xmalloc(sizeof(char *) * 14);
		proc->argv = argv;
		argc = 0;
		argv[argc ++] = WfcPath;
		argv[argc ++] = "-back";
		argv[argc ++] = IntStrDup(back+1);
		argv[argc ++] = "-apsport";
		argv[argc ++] = ThisEnv->WfcApsPort->port;

		if		(  Directory  !=  NULL  ) {
			argv[argc ++] = "-dir";
			argv[argc ++] = Directory;
		}
		if		(  LDDir  !=  NULL  ) {
			argv[argc ++] = "-lddir";
			argv[argc ++] = LDDir;
		}
		if		(  RecDir  !=  NULL  ) {
			argv[argc ++] = "-record";
			argv[argc ++] = RecDir;
		}
		if		(  fQ  ) {
			argv[argc ++] = "-?";
		}
		proc->argc = argc;
		argv[argc ++] = NULL;
		StartProcess(proc);
		sleep(interval);
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
dbgmsg(">StartPrograms");
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL);

	StartServers();
#ifdef	DEBUG
	DumpProcess();
#endif
dbgmsg("<StartPrograms");
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
			fprintf(fpLog,"process down pid = %d Command =[%s]\n"
					,(int)pid,proc->argv[0]);
#ifdef	DEBUG
			DumpCommand(proc->argv);
#endif
			if		(  fRestart  ) {
				if		(  ( proc->type & PTYPE_WFC ) !=  0  ) {
					break;
				} else {
					g_int_hash_table_remove(ProcessTable,pid);
					StartProcess(proc);
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
	KillAllProcess((PTYPE_APS | PTYPE_RED | PTYPE_WFC),SIGKILL);
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
	{	"lddir",	STRING,		TRUE,	(void*)&LDDir,
		"LD定義格納ディレクトリ"	 					},
	{	"bddir",	STRING,		TRUE,	(void*)&BDDir,
		"BD定義格納ディレクトリ"	 					},

	{	"redirector",BOOLEAN,	TRUE,	(void*)&fRedirector,
		"dbredirectorを起動する"	 					},

	{	"restart",	BOOLEAN,	TRUE,	(void*)&fRestart,
		"aps異常終了時に再起動する"	 					},

	{	"wait",		INTEGER,	TRUE,	(void*)&interval,
		"プロセス操作時の待ち時間"	 					},

	{	"myhost",	STRING,		TRUE,	(void*)&MyHost,
		"自分のホスト名を指定する"	 					},

	{	"maxtran",	INTEGER,	TRUE,	(void*)&MaxTran,
		"apsの処理するトランザクション数を指定する"		},

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
	ApsPath = SERVER_DIR "/aps";
	WfcPath = SERVER_DIR "/wfc";
	RedirectorPath = SERVER_DIR "/dbredirector";

	Directory = "./directory";
	LDDir = NULL;
	RecDir = NULL;
	Log = NULL;
	interval = 2;
	MaxTran = 0;
	Sleep = 0;

	MyHost = "localhost";

	fRedirector = FALSE;
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

	InitSystem();
	StartPrograms();
	ProcessMonitor();
	StopSystem();
	WaitStop();

	return	(0);
}
