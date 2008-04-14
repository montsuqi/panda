/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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
static	char	*DDir;
static	char	*RecDir;
static	char	*MyHost;
static	char	*Log;
static	Bool	fQ;
static	Bool	fRedirector;
static	Bool	fRestart;
static	Bool	fAllRestart;
static	Bool	fTimer;
static	Bool	fNoApsConnectRetry;
static	int		Interval;
static	int		wfcinterval;
static	int		MaxTran;
static	int		MaxTransactionRetry;
static	int		Sleep;
static	int		nCache;

static	GHashTable	*ProcessTable;

#define	PTYPE_NULL	(byte)0x00
#define	PTYPE_APS	(byte)0x01
#define	PTYPE_WFC	(byte)0x02
#define	PTYPE_RED	(byte)0x04

#define	STATE_RUN	1
#define	STATE_DOWN	2

typedef	struct {
	pid_t	pid;
	int		state;
	byte	type;
	int		argc;
	char	**argv;
}	Process;

static	void	StopSystem(void);

#ifdef	DEBUG
static	void
DumpCommand(
	char	**argv)
{
	int		i;

	for	( i = 0 ; argv[i]  !=  NULL ; i ++ ) {
		fprintf(fpLog,"%s ",argv[i]);
	}
	fprintf(fpLog,"\n");
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
	Process	*proc,
	int		interval)
{
	pid_t	pid;

ENTER_FUNC;
  retry:
	if		(  ( pid = fork() )  >  0  ) {
		proc->pid = pid;
		proc->state = STATE_RUN;
		g_int_hash_table_insert(ProcessTable,(long)pid,proc);
#ifdef	DEBUG
		for	( i = 0 ; proc->argv[i]  !=  NULL ; i ++ ) {
			fprintf(fpLog,"%s ",proc->argv[i]);
		}
		fprintf(fpLog,"(%d)\n",pid);
		fflush(fpLog);
#endif
	} else
	if		(  pid  ==  0  ) {
		if		(  interval  >  0  ) {
			sleep(interval);
		}
		_execv(proc->argv[0],proc->argv);
	} else {
		Message("can't start process\n");
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
	pid = StartProcess(proc,Interval);
	dbg->fConnect = CONNECT;
LEAVE_FUNC;
}

static	Bool
HerePort(
	Port	*port)
{
	Bool	ret;

ENTER_FUNC;
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
LEAVE_FUNC;
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
			StartProcess(proc,Interval);
		}
	}
LEAVE_FUNC;
}

static	void
StartApss(void)
{
	int		i;

ENTER_FUNC;
	for	( i = 0 ; i < ThisEnv->cLD ; i ++ ) {
		_StartAps(ThisEnv->ld[i]);
	}
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
		argv[argc ++] = "-cache";
		argv[argc ++] = IntStrDup(nCache);
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
		if		(  fQ  ) {
			argv[argc ++] = "-?";
		}
		proc->argc = argc;
		argv[argc ++] = NULL;
		StartProcess(proc,wfcinterval);
	}
LEAVE_FUNC;
}


static	void
StartServers(void)
{
ENTER_FUNC;
	ProcessTable = NewIntHash();
	if		(  fRedirector  ) {
		StartRedirectors();
	}
	StartWfc();
	StartApss();
LEAVE_FUNC;
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

ENTER_FUNC;
	kills.type = type;
	kills.sig = sig;
	g_int_hash_table_foreach(ProcessTable,(GHFunc)_KillProcess,(void *)&kills);
LEAVE_FUNC;
}

static	void
ProcessMonitor(void)
{
	pid_t	pid;
	int		status;
	Process	*proc;
	Bool	fStop;

ENTER_FUNC;
	signal(SIGCHLD, SIG_DFL);
	do {
		while	(  ( pid = waitpid(-1,&status,0) )  !=  -1  ) {
			dbgprintf("pid = %d is down",pid);
			if		(  ( proc = g_int_hash_table_lookup(ProcessTable,pid) )  !=  NULL  ) {
#if	0
				fprintf(fpLog,"process down pid = %d(%d) Command =[%s]\n"
						,(int)pid,WEXITSTATUS(status),proc->argv[0]);
#else
				if (WIFSIGNALED(status) ) {
					Message("%s(%d) killed by signal %d"
							,proc->argv[0], (int)pid, WTERMSIG(status));
				} else {
					Message("process down pid = %d(%d) Command =[%s]\n"
							,(int)pid, WEXITSTATUS(status),proc->argv[0]);
				}
#endif
				switch	(proc->type) {
				  case	PTYPE_APS:
					if		(	(  fRestart     )
							||	(  fAllRestart  ) ) {
						if		(	(  WIFEXITED(status)  )
								&&	(  WEXITSTATUS(status)  <  2  )	) {
							if		(  fAllRestart  ) {
								fStop = FALSE;
							} else {
								fStop = TRUE;
							}
						} else {
							fStop = FALSE;
						}
					} else {
						fStop = TRUE;
					}
					break;
				  default:
					if		(  fAllRestart  ) {
						fStop = FALSE;
					} else {
						fStop = TRUE;
					}
					break;
				}
				if		(  fStop  ) {
					StopSystem();
				} else {
					g_int_hash_table_remove(ProcessTable,pid);
					StartProcess(proc,Interval);
				}
			} else {
#if	0
				fprintf(fpLog,"unknown process down pid = %d(%d)\n"
						,(int)pid,WEXITSTATUS(status));
#else
				Message("unknown process down pid = %d(%d)\n"
 						,(int)pid,WEXITSTATUS(status));
#endif
			}
		}
	}	while	(TRUE);
LEAVE_FUNC;
}

static	void
InitSystem(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
WaitStop(void)
{
	while	(  waitpid(-1,0,WNOHANG)  >  0  );
}

static	void
StopSystem(void)
{
ENTER_FUNC;
	fRestart = FALSE;
	fAllRestart = FALSE;
	Message("Stop system");
	signal(SIGCHLD,(void *)WaitStop);
	KillAllProcess((PTYPE_APS | PTYPE_RED ),SIGKILL);
	KillAllProcess(PTYPE_WFC,SIGUSR1);
LEAVE_FUNC;
	exit(0);
}

static	void
StopApss(void)
{
ENTER_FUNC;
	sleep(Interval);
	KillAllProcess(PTYPE_APS,SIGKILL);
LEAVE_FUNC;
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
	{	"sendretry",	INTEGER,	TRUE,	(void*)&MaxSendRetry,
		"dbredirector送信の再試行数を指定する"			},

	{	"restart",	BOOLEAN,	TRUE,	(void*)&fRestart,
		"aps異常終了時に再起動する" 					},
	{	"allrestart",BOOLEAN,	TRUE,	(void*)&fAllRestart,
		"全ての子プロセス異常終了時に再起動する"	 	},

	{	"interval",	INTEGER,	TRUE,	(void*)&Interval,
		"プロセス操作時の待ち時間"	 					},
	{	"wfcwait",	INTEGER,	TRUE,	(void*)&wfcinterval,
		"wfc起動後の待ち時間(遅いCPU用)"				},
	{	"cache",	INTEGER,	TRUE,	(void*)&nCache,
		"terminal cache number"							},
	{	"sesdir",	STRING,		TRUE,	(void*)&SesDir,
		"セション変数保持ディレクトリ" 					},

	{	"myhost",	STRING,		TRUE,	(void*)&MyHost,
		"自分のホスト名を指定する"	 					},

	{	"maxtran",	INTEGER,	TRUE,	(void*)&MaxTran,
		"apsの処理するトランザクション数を指定する"		},
	{	"retry",	INTEGER,	TRUE,	(void*)&MaxTransactionRetry,
		"トランザクションを再試行する時の上限数"		},
	{	"no-aps-retry",	BOOLEAN,	TRUE,	(void*)&fNoApsConnectRetry,
		"APSの接続の再試行を行わない"					},

	{	"q",		BOOLEAN,	TRUE,	(void*)&fQ,
		"-?を指定する"				 					},
	{	"timer",	BOOLEAN,	TRUE,	(void*)&fTimer,
		"時間計測を行う"								},
	{	"log",		STRING,		TRUE,	(void*)&Log,
		"実行ログを取るファイル名を指定する"			},
	{	"sleep",	INTEGER,	TRUE,	(void*)&Sleep,
		"実行時間に足す処理時閨for debug)"				},

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
	Interval = 0;
	wfcinterval = 0;
	MaxTran = 0;
	MaxTransactionRetry = 0;
	MaxSendRetry = 3;
	Sleep = 0;

	MyHost = "localhost";

	fRedirector = FALSE;
	fNoCheck = FALSE;
	fNoSumCheck = FALSE;
	fRestart = FALSE;
	fAllRestart = FALSE;
	fQ = FALSE;
	fTimer = FALSE;
	fNoApsConnectRetry = FALSE;
	nCache = 100;
	SesDir = NULL;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
	if		(  fAllRestart  ) {
		fRestart = TRUE;
	}
	InitMessage("monitor",Log);

	if		(  !fRedirector  ) {
		fNoCheck = TRUE;
	}

	InitSystem();
	signal(SIGUSR1,(void *)StopSystem);
	signal(SIGHUP,(void *)StopApss);

	Message("Start system");
 	StartPrograms();

	ProcessMonitor();

	return	(0);
}
