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
#include	"value.h"
#include	"misc.h"
#include	"directory.h"
#include	"dirs.h"
#include	"option.h"
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

static	FILE		*fpLog;
static	pid_t		PidWfc;
static	GHashTable	*ProcessTable;

typedef	struct {
	pid_t	pid;
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

	proc = New(Process);
	argv = (char **)xmalloc(sizeof(char *) * 12);
	proc->argv = argv;
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
				dbgs[i ++] = dbg;
				dbg = dbg->redirect;
			}	while	(  dbg  !=  NULL  );
			i --;
			for	( ; i > 0 ; i -- ) {
				if		(  !strcmp(dbgs[i-1]->redirectPort->host,MyHost)  ) {
					StartRedirector(dbgs[i]);
				}
			}
			StartRedirector(dbgs[0]);
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

	for	( n = 0 ; n < ld->nports ; n ++ ) {
		if		(  !strcmp(ld->ports[n]->host,MyHost)  ) {
			proc = New(Process);
			argv = (char **)xmalloc(sizeof(char *) * 13);
			proc->argv = argv;
			argc = 0;
			argv[argc ++] = ApsPath;
			argv[argc ++] = "-port";
			argv[argc ++] = ld->ports[n]->port;
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
			if		(  fQ  ) {
				argv[argc ++] = "-?";
			}
			proc->argc = argc;
			argv[argc ++] = NULL;
			StartProcess(proc);
		}
	}
}

static	void
StartApss(void)
{
	int		i;

	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		_StartAps(ThisEnv->ld[i]);
	}
}

static	void
StartWfc(void)
{
	int		argc;
	char	**argv;
	Process	*proc;
	int		back;
	int		i;

	back = 0;
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		back += ThisEnv->ld[i]->nports;
	}
	proc = New(Process);
	argv = (char **)xmalloc(sizeof(char *) * 12);
	proc->argv = argv;
	argc = 0;
	argv[argc ++] = WfcPath;
	argv[argc ++] = "-back";
	argv[argc ++] = IntStrDup(back+1);

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
	PidWfc = StartProcess(proc);
}


static	void
StartServers(void)
{
	ProcessTable = NewIntHash();
	if		(  fRedirector  ) {
		StartRedirectors();
	}
	StartWfc();
	StartApss();
}

static	void
StartPrograms(void)
{
dbgmsg(">StartPrograms");
	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,NULL);

	StartServers();
#ifdef	DEBUG
	DumpProcess();
#endif
dbgmsg("<StartPrograms");
}

static	void
_KillProcess(
	pid_t		pid,
	Process		*proc,
	int			sig)
{
	kill(pid,sig);
}

static	void
KillAllProcess(
	int		sig)
{
	printf("*** process kill ***\n");
	g_int_hash_table_foreach(ProcessTable,(GHFunc)_KillProcess,(void *)sig);
	printf("**************************\n");
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
				if		(  pid  ==  PidWfc  ) {
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
StopSystem(void)
{
	fRestart = FALSE;
	kill(PidWfc,SIGKILL);
	sleep(interval);
	KillAllProcess(SIGKILL);
	sleep(interval);
	exit(0);
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

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	ApsPath = "/usr/local/panda/bin/aps";
	WfcPath = "/usr/local/panda/bin/wfc";
	RedirectorPath = "/usr/local/panda/bin/dbredirector";

	Directory = "./directory";
	LDDir = NULL;
	RecDir = NULL;
	Log = NULL;
	interval = 2;
	MaxTran = -1;

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
	signal(SIGHUP,(void *)StopSystem);
	InitSystem();
	StartPrograms();
	ProcessMonitor();
	StopSystem();

	return	(0);
}
