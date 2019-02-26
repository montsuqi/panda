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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <glib.h>

#include "const.h"
#include "libmondai.h"
#include "RecParser.h"
#include "DDparser.h"
#include "DBparser.h"
#include "enum.h"
#include "dirs.h"
#include "socket.h"
#include "net.h"
#include "comm.h"
#include "comms.h"
#include "directory.h"
#include "wfcdata.h"
#include "dblib.h"
#include "dbgroup.h"
#include "handler.h"
#include "apsio.h"
#include "aps_main.h"
#include "option.h"
#include "message.h"
#include "debug.h"

static char *WfcPortNumber;
static char AppName[128];

static int Sleep;
static Bool fConnectRetry;
static char *MiddleWareName;
static char *MiddleWareVersion;

static void InitSystem(char *name) {
  int i;

  MiddleWareName = getenv("__MCP_MIDDLEWARE_NAME");
  if (MiddleWareName == NULL) {
    MiddleWareName = "panda";
  }
  MiddleWareVersion = getenv("__MCP_MIDDLEWARE_VERSION");
  if (MiddleWareVersion == NULL) {
    MiddleWareVersion = PACKAGE_VERSION;
  }
  setenv("MCP_MIDDLEWARE_NAME", MiddleWareName, 1);
  setenv("MCP_MIDDLEWARE_VERSION", MiddleWareVersion, 1);

  InitDirectory();
  {
    char *env;
    env = getenv("MON_DB_REC_MEM_SAVE");
    if (env != NULL) {
      SetDBRecMemSave(env[0] == '1');
    }
    env = getenv("MON_SCR_REC_MEM_SAVE");
    if (env != NULL) {
      SetScrRecMemSave(env[0]=='1');
    }
  }
  SetUpDirectory(Directory, name, "", "", P_ALL);
  setenv("MON_DIRECTORY_PATH", Directory, 1);

  if ((ThisLD = GetLD(name)) == NULL) {
    Error("LD \"%s\" not found.", name);
  }
  if (ThisLD->home != NULL) {
    chdir(ThisLD->home);
  }
  ThisBD = NULL;
  ThisDBD = NULL;

  InitiateHandler();
  DBGroup_Init(ThisLD->db, ThisLD->dbmeta);
  DB_Table = ThisLD->DB_Table;
  TextSize = ThisLD->textsize;
  if (ThisEnv->mcprec != NULL) {
    InitializeValue(ThisEnv->mcprec->value);
  }
  if (ThisEnv->linkrec != NULL) {
    InitializeValue(ThisEnv->linkrec->value);
  }
  if (ThisLD->sparec != NULL) {
    InitializeValue(ThisLD->sparec->value);
  }

  for (i = 0; i < ThisLD->cBind; i++) {
    if ((ThisLD->binds[i] != NULL) && (ThisLD->binds[i]->rec != NULL)) {
      dbgprintf("[%s]", ThisLD->binds[i]->rec->name);
      InitializeValue(ThisLD->binds[i]->rec->value);
    }
  }
  ReadyDC();
}

static ProcessNode *MakeProcessNode(void) {
  ProcessNode *node;
  int i;
  ValueStruct *e;
  node = New(ProcessNode);
  node->mcprec = ThisEnv->mcprec;
  node->linkrec = ThisEnv->linkrec;
  node->sparec = ThisLD->sparec;
  node->cWindow = ThisLD->cWindow;
  node->cBind = ThisLD->cBind;
  node->bhash = ThisLD->bhash;
  node->textsize = ThisLD->textsize;
  node->scrrec =
      (RecordStruct **)xmalloc(sizeof(RecordStruct *) * node->cWindow);
  node->dbstatus = GetDBRedirectStatus(0);
  for (i = 0; i < node->cWindow; i++) {
    node->scrrec[i] = ThisLD->windows[i];
  }

  e = node->mcprec->value;
  SetValueString(GetItemLongName(e, "dc.middleware_name"), MiddleWareName,
                 NULL);
  SetValueString(GetItemLongName(e, "dc.middleware_version"), MiddleWareVersion,
                 NULL);
  return (node);
}

static void FinishSession(ProcessNode *node) {
  xfree(node->scrrec);
  xfree(node);
}

static void SetMCPEnv(ValueStruct *val) {
  setenv("MCP_WINDOW", ValueStringPointer(GetItemLongName(val, "dc.window")),
         1);
  setenv("MCP_WIDGET", ValueStringPointer(GetItemLongName(val, "dc.widget")),
         1);
  setenv("MCP_EVENT", ValueStringPointer(GetItemLongName(val, "dc.event")), 1);
  setenv("MCP_TENANT", ValueStringPointer(GetItemLongName(val, "dc.tenant")),
         1);
  setenv("MCP_TERM", ValueStringPointer(GetItemLongName(val, "dc.term")), 1);
  setenv("MCP_USER", ValueStringPointer(GetItemLongName(val, "dc.user")), 1);
  setenv("MCP_HOST", ValueStringPointer(GetItemLongName(val, "dc.host")), 1);
  setenv("MCP_TEMPDIR", ValueStringPointer(GetItemLongName(val, "dc.tempdir")),
         1);
}

static void MemSaveBegin(ProcessNode *node) {
  int i;
  unsigned long st,ed;
  st = GetNowTime();
  if (GetScrRecMemSave()) {
    for(i=0;i<node->cWindow;i++) {
      if (node->scrrec[i] == NULL) {
        node->scrrec[i] = ReadRecordDefine(ThisLD->windowsmeta[i]->name,FALSE);
      }
    }
  }
  ed = GetNowTime();
  TimerPrintf(st, ed, "MemSaveBegin");
}

static void MemSaveEnd(ProcessNode *node) {
  int i;
  unsigned long st,ed;
  st = GetNowTime();
  if (GetScrRecMemSave()) {
    for(i=0;i<node->cWindow;i++) {
      if (node->scrrec[i] != NULL) {
        FreeRecordStruct(node->scrrec[i]);
        node->scrrec[i] = NULL;
      }
    }
  }
  if (GetDBRecMemSave()) {
    for(i=0;i<ThisLD->cDB;i++) {
      if (ThisLD->db[i] != NULL) {
        FreeRecordStruct(ThisLD->db[i]);
        ThisLD->db[i] = NULL;
      }
    }
  }
  ed = GetNowTime();
  TimerPrintf(st, ed, "MemSaveEnd");
}

static int ExecuteServer(void) {
  int fhWFC, rc;
  Port *port;
  NETFILE *fpWFC;
  ProcessNode *node;
  WindowBind *bind;
  char *wname;
  PacketClass pc;

  if (WfcPortNumber == NULL) {
    port = ThisEnv->WfcApsPort;
  } else {
    port = ParPortName(WfcPortNumber);
  }
retry:
  fpWFC = NULL;
  rc = 0;
  while ((fhWFC = ConnectSocket(port, SOCK_STREAM)) < 0) {
    if (!fConnectRetry) {
      goto quit;
    }
    dbgmsg("WFC connection retry");
    sleep(1);
  }
  fpWFC = SocketToNet(fhWFC);
  SendStringDelim(fpWFC, ThisLD->name);
  SendStringDelim(fpWFC, "\n");

  pc = RecvPacketClass(fpWFC);
  switch (pc) {
  case APS_OK:
    break;
  case APS_NOT:
    if (!CheckNetFile(fpWFC)) {
      Warning("WFC connection lost");
      CloseNet(fpWFC);
      goto retry;
    }
    Error("invalid LD name");
    break;
  case APS_WAIT:
    CloseNet(fpWFC);
    sleep(1);
    Warning("connection suspended by wfc; retry");
    goto retry;
  }

  InitAPSIO(fpWFC);
  if (ReadyOnlineDB(AppName) < 0) {
    Error("Online DB is not ready");
  }
  node = MakeProcessNode();
  while (1) {
    MemSaveBegin(node);
    if (!GetWFC(fpWFC, node)) {
      Message("GetWFC failure");
      rc = -1;
      break;
    }
    if (node->messageType == MESSAGE_TYPE_CHECK) {
      continue;
    }
    wname =
        ValueStringPointer(GetItemLongName(node->mcprec->value, "dc.window"));
    dbgprintf("ld     = [%s]", ThisLD->name);
    dbgprintf("window = [%s]", wname);
    bind = (WindowBind *)g_hash_table_lookup(ThisLD->bhash, wname);
    if (bind == NULL) {
      Message("window [%s] not found.", wname);
      rc = 2;
      break;
    }
    SetValueString(GetItemLongName(node->mcprec->value, "dc.module"),
                   bind->module, NULL);
    if (node->dbstatus == DB_STATUS_REDFAILURE) {
      RedirectError();
    } else {
      node->dbstatus = GetDBRedirectStatus(0);
    }
    SetMCPEnv(node->mcprec->value);
    TransactionStart(NULL);
    ExecuteProcess(node);
    if (Sleep > 0) {
      sleep(Sleep);
    }
    TransactionEnd(NULL);
    PutWFC(fpWFC, node);
    MemSaveEnd(node);
  }
  MessageLogPrintf("exiting APS (%s)", ThisLD->name);
  MemSaveEnd(node);
  FinishSession(node);
quit:
  if (fpWFC != NULL) {
    CloseNet(fpWFC);
  }
  return (rc);
}

static void HungUp(void) { exit(15); }

static void StopProcess(void) {
  StopOnlineDB();
  CleanUpOnlineDB();
  StopDC();
  CleanUpOnlineDC();
}

static ARG_TABLE option[] = {
    {"wfcport", STRING, TRUE, (void *)&WfcPortNumber, "WFC waiting port"},

    {"base", STRING, TRUE, (void *)&BaseDir, "base environment directory path"},
    {"record", STRING, TRUE, (void *)&RecordDir, ".rec directory"},
    {"lddir", STRING, TRUE, (void *)&D_Dir, "LD directory path"},
    {"dir", STRING, TRUE, (void *)&Directory, "directory file name"},
    {"path", STRING, TRUE, (void *)&LibPath, "application module load path"},

    {"dbhost", STRING, TRUE, (void *)&DB_Host, "DB host name"},
    {"dbport", STRING, TRUE, (void *)&DB_Port, "DB port number"},
    {"db", STRING, TRUE, (void *)&DB_Name, "DB name"},
    {"dbuser", STRING, TRUE, (void *)&DB_User, "DB user name"},
    {"dbpass", STRING, TRUE, (void *)&DB_Pass, "DB password"},

    {"sleep", INTEGER, TRUE, (void *)&Sleep, "aps sleep time(for debug)"},
    {"timer", BOOLEAN, TRUE, (void *)&fTimer, "time measuring"},

    {"connect-retry", BOOLEAN, TRUE, (void *)&fConnectRetry,
     "WFC connection retry"},

    {"nocheck", BOOLEAN, TRUE, (void *)&fNoCheck,
     "no check dbredirector start"},
    {"noredirect", BOOLEAN, TRUE, (void *)&fNoRedirect, "no use dbredirector"},
    {"maxretry", INTEGER, TRUE, (void *)&MaxSendRetry,
     "max retry dbredirector"},
    {"retryint", INTEGER, TRUE, (void *)&RetryInterval,
     "dbredirector retry interval(sec)"},

    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  WfcPortNumber = NULL;
  fNoCheck = FALSE;
  fNoRedirect = FALSE;

  BaseDir = NULL;
  RecordDir = NULL;
  D_Dir = NULL;
  Directory = "./directory";
  LibPath = NULL;
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

extern int main(int argc, char **argv) {
  FILE_LIST *fl;
  int rc;
  struct sigaction sa;
  char *stderr_path, *stdout_path;

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

  prctl(PR_SET_PDEATHSIG, SIGHUP);

  stdout_path = getenv("APS_DEBUG_STDOUT_PATH");
  if (stdout_path != NULL) {
    freopen(stdout_path, "w", stdout);
  }

  stderr_path = getenv("APS_DEBUG_STDERR_PATH");
  if (stderr_path != NULL) {
    freopen(stderr_path, "w", stderr);
  }

  SetDefault();
  fl = GetOption(option, argc, argv, NULL);
  if ((fl != NULL) && (fl->name != NULL)) {
    snprintf(AppName, sizeof(AppName), "aps-%s", fl->name);
    InitMessage(AppName, NULL);
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
