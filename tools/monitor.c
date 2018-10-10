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

#define MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include "libmondai.h"
#include "directory.h"
#include "dbgroup.h"
#include "dirs.h"
#include "option.h"
#include "message.h"
#include "debug.h"

static char *Directory;
static char *TempDir;
static char *ApsPath;
static char *WfcPath;
static char *RedirectorPath;
static char *GlserverPath;
static char *DBLoggerPath;
static char *DBMasterPath;
static char *DBSlavePath;
static char *MONSetupPath;
static char *DDir;
static char *RecDir;
static char *ScrDir;
static char *MyHost;
static char *Log;
static Bool fQ;
static Bool fRedirector;
static Bool fNoApsConnectRetry;
static int Interval;
static int Wfcinterval;
static int MaxTransactionRetry;
static int Sleep;
static int SesNum;
static Bool fGlserver;
static Bool fGlSSL;
static Bool fVerifyPeer;
static char *GlAuth;
static char *GlCert;
static char *GlCAfile;

static Bool fDBMaster;
static Bool fDBLog;

static GList *ProcessList;
static volatile sig_atomic_t fLoop = TRUE;
static volatile sig_atomic_t fRestart = TRUE;
static volatile sig_atomic_t WfcRestartCount = 0;

#define MAX_WFC_RESTART_COUNT 5

#define PTYPE_NULL (unsigned char)0x00
#define PTYPE_APS (unsigned char)0x01
#define PTYPE_WFC (unsigned char)0x02
#define PTYPE_RED (unsigned char)0x04
#define PTYPE_GLS (unsigned char)0x08
#define PTYPE_LOG (unsigned char)0x10
#define PTYPE_MST (unsigned char)0x20
#define PTYPE_SLV (unsigned char)0x40

#define STATE_RUN 1
#define STATE_DOWN 2
#define STATE_STOP 3

typedef struct {
  pid_t pid;
  int state;
  unsigned char type;
  int argc;
  int interval;
  char **argv;
} Process;

#ifdef DEBUG
static void DumpCommand(char **argv) {
  int i;

  for (i = 0; argv[i] != NULL; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");
}

static void DumpProcess(Process *proc) {
  fprintf(stderr, "pid:%d status:%s\n", (int)proc->pid,
          proc->state == STATE_RUN ? "RUN" : "DOWN");
  DumpCommand(proc->argv);
}

static void DumpProcessAll(void) {
  int i;
  Process *proc;

  printf("*** process table dump ***\n");
  for (i = 0; i < g_list_length(ProcessList); i++) {
    proc = g_list_nth_data(ProcessList, i);
    DumpProcess(proc);
  }
  printf("**************************\n");
}
#endif

static ARG_TABLE option[] = {
    {"ApsPath", STRING, TRUE, (void *)&ApsPath, "aps command path"},
    {"WfcPath", STRING, TRUE, (void *)&WfcPath, "wfc command path"},
    {"RedPath", STRING, TRUE, (void *)&RedirectorPath,
     "redirector command path"},
    {"GlsPath", STRING, TRUE, (void *)&GlserverPath, "glserver command path"},

    {"DBLoggerPath", STRING, TRUE, (void *)&DBLoggerPath,
     "dblogger command path"},

    {"DBMasterPath", STRING, TRUE, (void *)&DBMasterPath,
     "dbmaster command path"},

    {"DBSlavePath", STRING, TRUE, (void *)&DBSlavePath, "dbslave command path"},
    {"MONSetupPath", STRING, TRUE, (void *)&MONSetupPath,
     "monsetup command path"},
    {"dir", STRING, TRUE, (void *)&Directory, "directory file name"},
    {"record", STRING, TRUE, (void *)&RecDir, "record directory"},
    {"screen", STRING, TRUE, (void *)&ScrDir, "screen directory"},
    {"ddir", STRING, TRUE, (void *)&DDir, "LD file directory"},
    {"tempdirroot", STRING, TRUE, (void *)&TempDir,
     "root of temporary directory"},

    {"redirector", BOOLEAN, TRUE, (void *)&fRedirector, "start dbredirector"},
    {"dblogger", BOOLEAN, TRUE, (void *)&fDBLog, "start dblogger"},
    {"dbmaster", BOOLEAN, TRUE, (void *)&fDBMaster, "start dbmaster"},
    {"nocheck", BOOLEAN, TRUE, (void *)&fNoCheck,
     "no check dbredirector start"},
    {"nosumcheck", BOOLEAN, TRUE, (void *)&fNoSumCheck,
     "no count dbredirector updates"},
    {"numeric", BOOLEAN, TRUE, (void *)&fNumericHOST,
     "Numeric form of the hostname"},
    {"sendretry", INTEGER, TRUE, (void *)&MaxSendRetry,
     "send retry dbredirector"},

    {"glserver", BOOLEAN, TRUE, (void *)&fGlserver, "start glserver"},
    {"glauth", STRING, TRUE, (void *)&GlAuth, "glserver authentication"},
    {"glssl", BOOLEAN, TRUE, (void *)&fGlSSL, "use ssl connection"},
    {"verifypeer", BOOLEAN, TRUE, (void *)&fVerifyPeer, "verify peer"},
    {"glcert", STRING, TRUE, (void *)&GlCert, "ssl certificate(p12)"},
    {"glcafile", STRING, TRUE, (void *)&GlCAfile, "ca certificate(pem)"},

    {"interval", INTEGER, TRUE, (void *)&Interval,
     "process start interval time"},
    {"wfcwait", INTEGER, TRUE, (void *)&Wfcinterval,
     "wfc start interval time(for slowCPU)"},
    {"sesnum", INTEGER, TRUE, (void *)&SesNum, "terminal session number"},

    {"myhost", STRING, TRUE, (void *)&MyHost, "my host name"},

    {"retry", INTEGER, TRUE, (void *)&MaxTransactionRetry,
     "transaction retry count"},
    {"no-aps-retry", BOOLEAN, TRUE, (void *)&fNoApsConnectRetry,
     "don't retry aps commection"},

    {"q", BOOLEAN, TRUE, (void *)&fQ, "show sub-program options"},
    {"timer", BOOLEAN, TRUE, (void *)&fTimer, "time measuring"},
    {"log", STRING, TRUE, (void *)&Log, "monitor log file name"},
    {"sleep", INTEGER, TRUE, (void *)&Sleep, "aps sleep time(for debug)"},

    {NULL, 0, FALSE, NULL, NULL}};

static Bool HerePort(Port *port) {
  Bool ret;
  char *host;

  if (port == NULL) {
    ret = FALSE;
  } else {
    switch (port->type) {
    case PORT_IP:
      if ((host = IP_HOST(port)) != NULL) {
        ret = (strcmp(host, MyHost) == 0) ? TRUE : TRUE; // what meaning?
      } else {
        ret = FALSE;
      }
      break;
    case PORT_UNIX:
      ret = TRUE;
      break;
    default:
      ret = FALSE;
    }
  }
  return (ret);
}

static void SetDefault(void) {
  ApsPath = NULL;
  WfcPath = NULL;
  RedirectorPath = NULL;
  GlserverPath = NULL;
  DBLoggerPath = NULL;
  DBMasterPath = NULL;
  DBSlavePath = NULL;
  MONSetupPath = NULL;
  Directory = "./directory";
  TempDir = "/tmp/panda_root/";
  DDir = NULL;
  RecDir = NULL;
  ScrDir = NULL;
  Log = NULL;
  Interval = 0;
  Wfcinterval = 0;
  MaxTransactionRetry = 0;
  MaxSendRetry = 3;
  Sleep = 0;

  MyHost = "localhost";

  fRedirector = FALSE;
  fNoCheck = FALSE;
  fNoSumCheck = FALSE;
  fNumericHOST = FALSE;
  fQ = FALSE;
  fTimer = FALSE;
  fNoApsConnectRetry = FALSE;
  SesNum = 0;

  fGlserver = FALSE;
  fGlSSL = FALSE;
  fVerifyPeer = FALSE;
  GlAuth = NULL;
  GlCert = NULL;
  GlCAfile = NULL;
  fDBLog = FALSE;
  fDBMaster = FALSE;
}

static void InitSystem(void) {
  ProcessList = NULL;
  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_NONE);
  setenv("MON_DIRECTORY_PATH", Directory, 1);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
  if (!MakeDir(TempDir, 0700)) {
    Error("cannot make TempDirRoot:%s", TempDir);
  }
  setenv("MCP_TEMPDIR_ROOT", TempDir, 1);
}

static void _execv(char *cmd, char **argv) {
#ifdef DEBUG
  DumpCommand(argv);
#endif
  if (execv(cmd, argv) < 0) {
    int errsv = errno;
    Error("%s: %s", strerror(errsv), cmd);
  }
}

static int StartProcess(Process *proc) {
  pid_t pid;
  char *msg, *p;
  int i;
  size_t size;

retry:
  if ((pid = fork()) > 0) {
    proc->pid = pid;
    proc->state = STATE_RUN;

    if (getenv("MONITOR_START_PROCESS_LOGGING") != NULL) {
      size = 0;
      for (i = 0; proc->argv[i] != NULL; i++) {
        size += strlen(proc->argv[i]) + 1;
      }
      msg = malloc(size + 10);
      memset(msg, 0, size + 10);
      p = msg;
      sprintf(p, "(%d) ", pid);
      p += strlen(p);
      for (i = 0; proc->argv[i] != NULL; i++) {
        memcpy(p, proc->argv[i], strlen(proc->argv[i]));
        p += strlen(proc->argv[i]);
        memcpy(p, " ", 1);
        p += 1;
      }
      Warning(msg);
      free(msg);
    }
  } else if (pid == 0) {
    if (proc->interval > 0) {
      sleep(proc->interval);
    }
    _execv(proc->argv[0], proc->argv);
  } else {
    Message("can't start process");
    goto retry;
  }
  return (pid);
}

static void InitRedirector(DBG_Struct *dbg) {
  int argc;
  char **argv;
  Process *proc;

  proc = New(Process);
  argv = (char **)xmalloc(sizeof(char *) * 15);
  proc->argv = argv;
  proc->type = PTYPE_RED;
  argc = 0;
  if (RedirectorPath != NULL) {
    argv[argc++] = RedirectorPath;
  } else if (ThisEnv->RedPath != NULL) {
    argv[argc++] = ThisEnv->RedPath;
  } else {
    argv[argc++] = SERVER_DIR "/dbredirector";
  }
  if (Directory != NULL) {
    argv[argc++] = "-dir";
    argv[argc++] = Directory;
  }
  if (DDir != NULL) {
    argv[argc++] = "-ddir";
    argv[argc++] = DDir;
  }
  if (RecDir != NULL) {
    argv[argc++] = "-record";
    argv[argc++] = RecDir;
  }
  if (fTimer) {
    argv[argc++] = "-timer";
  }
  if (dbg->sumcheck == 0) {
    argv[argc++] = "-nosumcheck";
  } else if (fNoSumCheck) {
    argv[argc++] = "-nosumcheck";
  }
  argv[argc++] = dbg->name;
  if (fQ) {
    argv[argc++] = "-?";
  }
  argv[argc++] = "-maxretry";
  argv[argc++] = IntStrDup(MaxSendRetry);
  proc->argc = argc;
  argv[argc++] = NULL;
  Message("start redirector:%s", dbg->name);
  proc->interval = Interval;
  dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;
  ProcessList = g_list_append(ProcessList, proc);
}

static void _InitRedirectors(DBG_Struct *dbg) {

  if (dbg->redirect != NULL && dbg->redirectorMode == REDIRECTOR_MODE_PATCH) {
    _InitRedirectors(dbg->redirect);
  }
  if (dbg->process[PROCESS_UPDATE].dbstatus != DB_STATUS_CONNECT) {
    InitRedirector(dbg);
  }
}

static void InitRedirectors(void) {
  int i;
  DBG_Struct *dbg;

  for (i = 0; i < ThisEnv->cDBG; i++) {
    ThisEnv->DBG[i]->process[PROCESS_UPDATE].dbstatus = DB_STATUS_UNCONNECT;
  }
  for (i = 0; i < ThisEnv->cDBG; i++) {
    dbg = ThisEnv->DBG[i];
    if (dbg->redirect != NULL && dbg->redirectorMode == REDIRECTOR_MODE_PATCH) {
      _InitRedirectors(dbg->redirect);
    }
  }
}

static void InitDBMaster(void) {
  int argc;
  char **argv;
  Process *proc;
  int back;
  int i;

  if (HerePort(ThisEnv->DBMasterPort)) {
    back = 0;
    for (i = 0; i < ThisEnv->cLD; i++) {
      back += ThisEnv->ld[i]->nports;
    }
    proc = New(Process);
    proc->type = PTYPE_MST;
    argv = (char **)xmalloc(sizeof(char *) * 24);
    proc->argv = argv;
    argc = 0;
    if (DBMasterPath != NULL) {
      argv[argc++] = DBMasterPath;
    } else if (ThisEnv->DBMasterPath != NULL) {
      argv[argc++] = ThisEnv->DBMasterPath;
    } else {
      argv[argc++] = SERVER_DIR "/dbmaster";
    }
    if (Directory != NULL) {
      argv[argc++] = "-dir";
      argv[argc++] = Directory;
    }
    if (DDir != NULL) {
      argv[argc++] = "-ddir";
      argv[argc++] = DDir;
    }
    if (RecDir != NULL) {
      argv[argc++] = "-record";
      argv[argc++] = RecDir;
    }
    if (ThisEnv->DBMasterAuth != NULL) {
      argv[argc++] = "-auth";
      argv[argc++] = ThisEnv->DBMasterAuth;
    }
    argv[argc++] = "-port";
    argv[argc++] = StrDup(StringPortName(ThisEnv->DBMasterPort));
    argv[argc++] = ThisEnv->DBMasterLogDBName;

    if (fQ) {
      argv[argc++] = "-?";
    }
    proc->argc = argc;
    argv[argc++] = NULL;
    proc->interval = Interval;
    ProcessList = g_list_append(ProcessList, proc);
  }
}

static void InitDBLog(DBG_Struct *dbg) {
  int argc;
  char **argv;
  Process *proc;

  proc = New(Process);
  argv = (char **)xmalloc(sizeof(char *) * 15);
  proc->argv = argv;
  proc->type = PTYPE_LOG;
  argc = 0;
  if (DBLoggerPath != NULL) {
    argv[argc++] = DBLoggerPath;
  } else if (ThisEnv->DBLoggerPath != NULL) {
    argv[argc++] = ThisEnv->DBLoggerPath;
  } else {
    argv[argc++] = SERVER_DIR "/dblogger";
  }
  if (Directory != NULL) {
    argv[argc++] = "-dir";
    argv[argc++] = Directory;
  }
  if (DDir != NULL) {
    argv[argc++] = "-ddir";
    argv[argc++] = DDir;
  }
  if (RecDir != NULL) {
    argv[argc++] = "-record";
    argv[argc++] = RecDir;
  }
  if (fTimer) {
    argv[argc++] = "-timer";
  }
  if (fNoSumCheck) {
    argv[argc++] = "-nosumcheck";
  }
  argv[argc++] = dbg->name;
  if (fQ) {
    argv[argc++] = "-?";
  }
  argv[argc++] = "-maxretry";
  argv[argc++] = IntStrDup(MaxSendRetry);
  proc->argc = argc;
  argv[argc++] = NULL;
  proc->interval = Interval;
  dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;
  ProcessList = g_list_append(ProcessList, proc);
}

static void _InitDBLogs(DBG_Struct *dbg) {

  if (dbg->redirect != NULL && dbg->redirectorMode == REDIRECTOR_MODE_LOG) {
    _InitDBLogs(dbg->redirect);
  }
  if (dbg->process[PROCESS_UPDATE].dbstatus != DB_STATUS_CONNECT) {
    InitDBLog(dbg);
  }
}

static void InitDBLogs(void) {
  int i;
  DBG_Struct *dbg;

  for (i = 0; i < ThisEnv->cDBG; i++) {
    ThisEnv->DBG[i]->process[PROCESS_UPDATE].dbstatus = DB_STATUS_UNCONNECT;
  }
  for (i = 0; i < ThisEnv->cDBG; i++) {
    dbg = ThisEnv->DBG[i];
    if (dbg->redirect != NULL && dbg->redirectorMode == REDIRECTOR_MODE_LOG) {
      _InitDBLogs(dbg->redirect);
    }
  }
}

static void InitGlserver(void) {
  int argc;
  char **argv;
  Process *proc;

  proc = New(Process);
  argv = (char **)xmalloc(sizeof(char *) * 20);
  proc->argv = argv;
  proc->type = PTYPE_GLS;
  argc = 0;
  if (GlserverPath != NULL) {
    argv[argc++] = GlserverPath;
  } else {
    argv[argc++] = SERVER_DIR "/glserver";
  }
  if (RecDir != NULL) {
    argv[argc++] = "-record";
    argv[argc++] = RecDir;
  } else if (ThisEnv->RecordDir != NULL) {
    argv[argc++] = "-record";
    argv[argc++] = ThisEnv->RecordDir;
  }
  if (ScrDir != NULL) {
    argv[argc++] = "-screen";
    argv[argc++] = ScrDir;
  }
  if (GlAuth != NULL) {
    argv[argc++] = "-auth";
    argv[argc++] = GlAuth;
  }
  argv[argc++] = "-api";
  if (fNumericHOST) {
    argv[argc++] = "-numeric";
  }
  if (fGlSSL) {
    argv[argc++] = "-ssl";
    if (GlCert != NULL) {
      argv[argc++] = "-cert";
      argv[argc++] = GlCert;
    }
    if (GlCAfile != NULL) {
      argv[argc++] = "-CAfile";
      argv[argc++] = GlCAfile;
    }
    if (fVerifyPeer) {
      argv[argc++] = "-verifypeer";
    }
  }

  proc->argc = argc;
  argv[argc++] = NULL;
  proc->interval = Interval;
  ProcessList = g_list_append(ProcessList, proc);
}

static void _InitAps(LD_Struct *ld) {
  int argc;
  char **argv;
  Process *proc;
  int n;

  if (ThisEnv->mlevel != MULTI_APS) {
    ld->nports = 1;
  }
  dbgprintf("start %d servers", ld->nports);
  for (n = 0; n < ld->nports; n++) {
    if ((ld->ports[n] == NULL) || (HerePort(ld->ports[n]))) {
      proc = New(Process);
      argv = (char **)xmalloc(sizeof(char *) * 22);
      proc->argv = argv;
      proc->type = PTYPE_APS;
      argc = 0;
      if (ApsPath != NULL) {
        argv[argc++] = ApsPath;
      } else if (ThisEnv->ApsPath != NULL) {
        argv[argc++] = ThisEnv->ApsPath;
      } else {
        argv[argc++] = SERVER_DIR "/aps";
      }
      argv[argc++] = "-wfcport";
      argv[argc++] = StrDup(StringPortName(ThisEnv->WfcApsPort));
      if (Directory != NULL) {
        argv[argc++] = "-dir";
        argv[argc++] = Directory;
      }
      if (DDir != NULL) {
        argv[argc++] = "-ddir";
        argv[argc++] = DDir;
      }
      if (RecDir != NULL) {
        argv[argc++] = "-record";
        argv[argc++] = RecDir;
      }
      if (fTimer) {
        argv[argc++] = "-timer";
      }
      if (!fNoApsConnectRetry) {
        argv[argc++] = "-connect-retry";
      }
      argv[argc++] = ld->name;
      argv[argc++] = "-sleep";
      argv[argc++] = IntStrDup(Sleep);
      argv[argc++] = "-maxretry";
      argv[argc++] = IntStrDup(MaxSendRetry);

      if (fNoCheck) {
        argv[argc++] = "-nocheck";
      }

      if (fQ) {
        argv[argc++] = "-?";
      }
      proc->argc = argc;
      argv[argc++] = NULL;
      proc->interval = Interval;
      ProcessList = g_list_append(ProcessList, proc);
    }
  }
}

static void InitApss(void) {
  int i;
  char str[512];

  str[0] = 0;
  for (i = 0; i < ThisEnv->cLD; i++) {
    _InitAps(ThisEnv->ld[i]);
    strncat(str, ThisEnv->ld[i]->name, sizeof(str) - strlen(str) - 1);
    strncat(str, " ", sizeof(str) - strlen(str) - 1);
  }
  Message("start aps:%s", str);
}

static void InitWfc(void) {
  int argc;
  char **argv;
  Process *proc;
  int back;
  int i;

  if (HerePort(ThisEnv->WfcApsPort)) {
    back = 0;
    for (i = 0; i < ThisEnv->cLD; i++) {
      back += ThisEnv->ld[i]->nports;
    }
    proc = New(Process);
    proc->type = PTYPE_WFC;
    argv = (char **)xmalloc(sizeof(char *) * 24);
    proc->argv = argv;
    argc = 0;
    if (WfcPath != NULL) {
      argv[argc++] = WfcPath;
    } else if (ThisEnv->WfcPath != NULL) {
      argv[argc++] = ThisEnv->WfcPath;
    } else {
      argv[argc++] = SERVER_DIR "/wfc";
    }
    if (fTimer) {
      argv[argc++] = "-timer";
    }
    argv[argc++] = "-back";
    argv[argc++] = IntStrDup(back + 1);
    argv[argc++] = "-port";
    argv[argc++] = StrDup(StringPortName(ThisEnv->TermPort));
    argv[argc++] = "-apsport";
    argv[argc++] = StrDup(StringPortName(ThisEnv->WfcApsPort));
    if (SesNum > 0) {
      argv[argc++] = "-sesnum";
      argv[argc++] = IntStrDup(SesNum);
    }

    if (Directory != NULL) {
      argv[argc++] = "-dir";
      argv[argc++] = Directory;
    }
    if (DDir != NULL) {
      argv[argc++] = "-ddir";
      argv[argc++] = DDir;
    }
    if (RecDir != NULL) {
      argv[argc++] = "-record";
      argv[argc++] = RecDir;
    }
    if (MaxTransactionRetry > 0) {
      argv[argc++] = "-retry";
      argv[argc++] = IntStrDup(MaxTransactionRetry);
    }
    dbgmsg("*");
    if (fQ) {
      argv[argc++] = "-?";
    }
    proc->argc = argc;
    argv[argc++] = NULL;
    proc->interval = Wfcinterval;
    ProcessList = g_list_append(ProcessList, proc);
    Message("start wfc");
  }
}

static void InitServers(void) {
  if (fRedirector) {
    InitRedirectors();
  }
  if (fDBLog) {
    InitDBLogs();
  }
  if (fDBMaster) {
    InitDBMaster();
  }
  if (fGlserver) {
    InitGlserver();
  }
  InitWfc();
  InitApss();
}

static void KillProcess(unsigned char type, int sig) {
  int i;
  Process *proc;

  for (i = 0; i < g_list_length(ProcessList); i++) {
    proc = g_list_nth_data(ProcessList, i);
    if ((proc->type & type) != 0) {
      dbgprintf("kill -%d %d\n", sig, proc->pid);
      if (kill(proc->pid, sig) == -1) {
        Message("kill(2) failure: %s", strerror(errno));
      }
      proc->state = STATE_STOP;
    }
  }
}

static void StartSetup() {
  pid_t pid;
  char *cmd;
  int argc;
  char **argv;
  int status;

  if (MONSetupPath) {
    cmd = MONSetupPath;
  } else {
    cmd = BIN_DIR "/monsetup";
  }
  argv = (char **)xmalloc(sizeof(char *) * 4);
  argc = 0;
  argv[argc++] = cmd;
  argv[argc++] = "-dir";
  argv[argc++] = Directory;
  argv[argc++] = NULL;

  if ((pid = fork()) > 0) {
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status)) {
      Error("monsetup failure");
    }
  } else if (pid == 0) {
    _execv(cmd, argv);
  } else {
    Error("can't start monsetup");
  }
  xfree(argv);
}

static void StartServers() {
  int i;
  Process *proc;

  for (i = 0; i < g_list_length(ProcessList); i++) {
    proc = g_list_nth_data(ProcessList, i);
    StartProcess(proc);
  }
}

static void StopServers(void) {
  KillProcess(PTYPE_GLS, SIGHUP);
  KillProcess(PTYPE_APS, SIGHUP);
  KillProcess((PTYPE_WFC | PTYPE_RED | PTYPE_LOG | PTYPE_MST), SIGHUP);
  sleep(1);
}

static void StopSystem(void) {
  fLoop = FALSE;
  fRestart = FALSE;
  StopServers();
  Message("stop system");
}

static void RestartSystem(void) {
  fRestart = FALSE;
  StopServers();
  Message("restart system");
  WfcRestartCount = 0;
}

static Process *GetProcess(int pid) {
  Process *proc;
  int i;

  for (i = 0; i < g_list_length(ProcessList); i++) {
    proc = (Process *)g_list_nth_data(ProcessList, i);
    if (pid == proc->pid) {
      return proc;
    }
  }
  return NULL;
}

static char *GetCommandStr(Process *proc) {
  int i;
  int size;
  char *p;
  char *command = NULL;

  if (proc == NULL) {
    return command;
  }
  size = 0;
  for (i = 0; proc->argv[i] != NULL; i++) {
    size += 1 + strlen(proc->argv[i]);
  }
  if (size > 0) {
    p = command = xmalloc(size + 1);
    for (i = 0; proc->argv[i] != NULL; i++) {
      sprintf(p, "%s ", proc->argv[i]);
      p += strlen(proc->argv[i]) + 1;
    }
    *p = 0;
  }
  return command;
}

static void ProcessMonitor(void) {
  int pid;
  int status;
  Process *proc;
  struct sigaction sa;
  char *command;

  /* need for catch SYGCHLD  */
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_DFL;
  sa.sa_flags |= SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) != 0) {
    Error("sigaction(2) failure");
  }

  proc = NULL;
  while ((pid = waitpid(-1, &status, 0)) != -1) {
    proc = GetProcess(pid);
    if (proc == NULL) {
      Warning("[BUG] unknown process down:%d", pid);
      break;
    }
    if (proc->state == STATE_RUN) {
      proc->state = STATE_DOWN;
    }
    if (fLoop && fRestart) {
      if (proc->state == STATE_DOWN) {
        command = GetCommandStr(proc);
        if (WIFSIGNALED(status)) {
          Message("pid[%d] killed by signal %d;[%s]", (int)pid,
                  WTERMSIG(status), command);
        } else {
          Message("pid[%d] exit(%d);[%s]", (int)pid, WEXITSTATUS(status),
                  command);
        }
        xfree(command);
      }
    }
    switch (proc->type) {
    case PTYPE_APS:
      if (WIFEXITED(status) && WEXITSTATUS(status) < 2) {
        StopSystem();
      }
      break;
    case PTYPE_WFC:
      WfcRestartCount++;
      if (WfcRestartCount < MAX_WFC_RESTART_COUNT) {
        Message("wfc restart count:%d", WfcRestartCount);
      } else {
        Message("wfc restart count:%d,reached max count,does not restart",
                WfcRestartCount);
        StopSystem();
        break;
      }
    }
    if (fRestart) {
      /*What should do to set interval ?*/
      StartProcess(proc);
    }
  }
}

extern int main(int argc, char **argv) {
  struct sigaction sa;

  SetDefault();
  GetOption(option, argc, argv, NULL);
  InitMessage("monitor", Log);

  if (!fRedirector) {
    fNoCheck = TRUE;
  }

  InitSystem();
  Message("start system");

  InitServers();

  sa.sa_handler = (void *)RestartSystem;
  sa.sa_flags |= SA_RESTART;
  if (sigaction(SIGHUP, &sa, NULL) != 0) {
    Error("sigaction(2) failure");
  }

  StartSetup();

  while (fLoop) {
    fRestart = TRUE;
    StartServers();
    ProcessMonitor();
  }
  return (0);
}
