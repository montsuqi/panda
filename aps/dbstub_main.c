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
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>

#include "dirs.h"
#include "const.h"
#include "enum.h"
#include "dblib.h"
#include "dbgroup.h"
#include "RecParser.h"
#include "directory.h"
#include "handler.h"
#include "option.h"
#include "aps_main.h"
#include "message.h"
#include "debug.h"

static char *CommandParameter;
static char AppName[128];

static char *BD_Name;
static BatchBind *Bind;

static void InitData(char *name) {
  ENTER_FUNC;
  InitDirectory();
  SetUpDirectory(Directory, "", name, "", P_ALL);
  LEAVE_FUNC;
}

extern void InitSystem(char *name) {
  char *user = NULL;
  char *term = NULL;
  ENTER_FUNC;
  InitData(BD_Name);
  if ((ThisBD = GetBD(BD_Name)) == NULL) {
    Error("BD file not found.");
  }
  if (ThisBD->home != NULL) {
    chdir(ThisBD->home);
  }
  ThisLD = NULL;
  ThisDBD = NULL;
  InitiateBatchHandler();
  ThisDB = ThisBD->db;
  DB_Table = ThisBD->DB_Table;
  TextSize = ThisBD->textsize;
  if (ThisEnv->mcprec != NULL) {
    InitializeValue(ThisEnv->mcprec->value);
  }
  if (ThisEnv->mcprec != NULL) {
    SetValueString(GetItemLongName(ThisEnv->mcprec->value, "dc.module"), name,
                   NULL);
    if ((user = getenv("MCP_USER")) != NULL) {
      SetValueString(GetItemLongName(ThisEnv->mcprec->value, "dc.user"), user,
                     NULL);
    }
    if ((term = getenv("MCP_TERM")) != NULL) {
      SetValueString(GetItemLongName(ThisEnv->mcprec->value, "dc.term"), term,
                     NULL);
    }
  } else {
    Error("ThisEnv-> mcprec is NULL");
  }
  if ((Bind = g_hash_table_lookup(ThisBD->BatchTable, name)) == NULL) {
    Error("%s application is not in BD.", name);
  }
  InitDB_Process(AppName);
  ReadyHandlerDB(Bind->handler);
  LEAVE_FUNC;
}

static int ExecuteSubProcess(char *name) {
  int rc;
  ENTER_FUNC;
  dbgprintf("[%s][%s]\n", name, CommandParameter);
  rc = StartBatch(name, CommandParameter);
  LEAVE_FUNC;
  return (rc);
}

static void StopProcess(int ec) {
  ENTER_FUNC;
  StopHandlerDB(Bind->handler);
  CleanUpHandlerDB(Bind->handler);
  LEAVE_FUNC;
  exit(ec);
}

static void SegvProcess(int sn, siginfo_t *si, void *uc) {
  MessageLogPrintf("Received signal %d errno %d", si->si_signo, si->si_errno);
  switch (si->si_code) {
  case 1:
    MessageLogPrintf(" SI code = %d (Address not mapped to object)\n",
                     si->si_code);
    break;
  case 2:
    MessageLogPrintf(" SI code = %d (Invalid permissions for \
                       mapped object)\n",
                     si->si_code);
    break;
  default:
    MessageLogPrintf("SI code = %d (Unknown SI Code)\n", si->si_code);
    break;
  }
  Error("Fault addr = 0x%08x \n", si->si_addr);
}

static ARG_TABLE option[] = {
    {"host", STRING, TRUE, (void *)&DB_Host, "PostgreSQL稼働ホスト名"},
    {"port", STRING, TRUE, (void *)&DB_Port, "PostgreSQLポート番号"},

    {"base", STRING, TRUE, (void *)&BaseDir, "環境のベースディレクトリ"},
    {"record", STRING, TRUE, (void *)&RecordDir, "データ定義格納ディレクトリ"},
    {"ddir", STRING, TRUE, (void *)&D_Dir, "定義格納ディレクトリ"},
    {"bddir", STRING, TRUE, (void *)&D_Dir, "定義格納ディレクトリ(互換用)"},
    {"dir", STRING, TRUE, (void *)&Directory, "ディレクトリファイル"},
    {"path", STRING, TRUE, (void *)&LibPath, "モジュールのロードパス"},
    {"parameter", STRING, TRUE, (void *)&CommandParameter, "コマンドライン"},

    {"db", STRING, TRUE, (void *)&DB_Name, "データベース名"},
    {"user", STRING, TRUE, (void *)&DB_User, "ユーザ名"},
    {"pass", STRING, TRUE, (void *)&DB_Pass, "パスワード"},
    {"bd", STRING, TRUE, (void *)&BD_Name, "BD定義名"},

    {"nocheck", BOOLEAN, TRUE, (void *)&fNoCheck,
     "dbredirectorの起動をチェックしない"},
    {"noredirect", BOOLEAN, TRUE, (void *)&fNoRedirect,
     "dbredirectorを使わない"},
    {"maxretry", INTEGER, TRUE, (void *)&MaxSendRetry,
     "dbredirector送信の再試行数を指定する"},
    {"retryint", INTEGER, TRUE, (void *)&RetryInterval,
     "dbredirector送信の再試行の間隔を指定する(秒)"},

    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  BaseDir = NULL;
  RecordDir = NULL;
  D_Dir = NULL;
  Directory = "./directory";
  LibPath = NULL;

  DB_User = NULL;
  DB_Pass = NULL;
  DB_Host = NULL;
  DB_Port = NULL;
  DB_Name = DB_User;
  BD_Name = NULL;
  CommandParameter = "";

  fNoCheck = FALSE;
  fNoRedirect = FALSE;
  MaxSendRetry = 3;
  RetryInterval = 5;
}

extern int main(int argc, char **argv) {
  struct sigaction sa_segv, sa_stop;
  FILE_LIST *fl;
  int rc;

  ENTER_FUNC;
  InitNET();

  memset(&sa_segv, 0, sizeof(struct sigaction));
  sa_segv.sa_flags = SA_SIGINFO;
  sa_segv.sa_sigaction = SegvProcess;
  sigemptyset(&sa_segv.sa_mask);
  sigaction(SIGSEGV, &sa_segv, NULL);

  memset(&sa_stop, 0, sizeof(struct sigaction));
  sigemptyset(&sa_stop.sa_mask);
  sa_stop.sa_flags = 0;
  sa_stop.sa_handler = StopProcess;
  sigaction(SIGHUP, &sa_stop, NULL);

  SetDefault();
  fl = GetOption(option, argc, argv, NULL);
  InitMessage("dbstub", NULL);
  InitNET();

  if (BD_Name == NULL) {
    Error("BD name is not specified.");
  }
  if (fl == NULL) {
    Error("module name is not specified.");
  }
  snprintf(AppName, sizeof(AppName), "dbstub-%s", fl->name);
  InitSystem(fl->name);
  Message("module %s: %.20s", fl->name, CommandParameter);
  rc = ExecuteSubProcess(fl->name);
  StopProcess(rc);
  return (rc);
}
