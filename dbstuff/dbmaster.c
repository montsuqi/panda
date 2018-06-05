/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>

#include "libmondai.h"
#include "RecParser.h"
#include "comm.h"
#include "dirs.h"
#include "redirect.h"
#include "dbgroup.h"
#include "dblib.h"
#include "directory.h"
#include "queue.h"
#include "socket.h"
#include "dbredirector.h"
#include "option.h"
#include "message.h"
#include "dblog.h"
#include "dbmaster_thread.h"
#include "debug.h"

static char *PortNumber;
static Port *DBMasterPort;
static int Back;
static char *Directory;
static DBG_Struct *ThisDBG;

static DBMasterThread *DBMasterServer;

static char *AuthURL;

extern void ExecuteServer(void) {
  ENTER_FUNC;
  StartDBMaster(DBMasterServer);
  JoinDBMaster(DBMasterServer);
  LEAVE_FUNC;
}

#ifdef DEBUG
static void DumpDBG(char *name, DBG_Struct *dbg, void *dummy) {
  dbgprintf("name     = [%s]\n", dbg->name);
  dbgprintf("\ttype     = [%s]\n", dbg->type);
  dbgprintf("\tDB name  = [%s]\n", GetDB_DBname(dbg, DB_UPDATE));
  dbgprintf("\tDB user  = [%s]\n", GetDB_User(dbg, DB_UPDATE));
  dbgprintf("\tDB pass  = [%s]\n", GetDB_Pass(dbg, DB_UPDATE));
  dbgprintf("\tDB sslmode  = [%s]\n", GetDB_Sslmode(dbg, DB_UPDATE));
  dbgprintf("\t   redirectorMode = [%d]\n", dbg->redirectorMode);

  if (dbg->file != NULL) {
    dbgprintf("\tlog file = [%s]\n", dbg->file);
  }
  if (dbg->redirect != NULL) {
    dbg = dbg->redirect;
    dbgprintf("\tredirect = [%s]\n", dbg->name);
  }
}
#endif

static void _CheckDBG(char *name, DBG_Struct *dbg, char *red_name) {
  DBG_Struct *red_dbg;
  char *src_port, *dsc_port;
  char *dbg_dbname = "", *red_dbg_dbname = "";
  char *dbname;
  ENTER_FUNC;
  if (dbg->redirect != NULL) {
    red_dbg = dbg->redirect;
    if (strcmp(red_dbg->name, red_name) == 0) {
      src_port = StrDup(StringPort(GetDB_Port(dbg, DB_UPDATE)));
      dsc_port = StrDup(StringPort(GetDB_Port(red_dbg, DB_UPDATE)));
      if ((dbname = GetDB_DBname(dbg, DB_UPDATE)) != NULL) {
        dbg_dbname = dbname;
      }
      if ((dbname = GetDB_DBname(red_dbg, DB_UPDATE)) != NULL) {
        red_dbg_dbname = dbname;
      }
      if ((strcmp(dbg->type, red_dbg->type) == 0) &&
          (strcmp(dbg_dbname, red_dbg_dbname) == 0) &&
          (strcmp(src_port, dsc_port) == 0)) {
        Error("The connection destination is same DB");
      }
      xfree(src_port);
      xfree(dsc_port);
    }
  }
  LEAVE_FUNC;
}

static void CheckDBG(char *name) {
#ifdef DEBUG
  g_hash_table_foreach(ThisEnv->DBG_Table, (GHFunc)DumpDBG, NULL);
#endif
  if ((ThisDBG = (DBG_Struct *)g_hash_table_lookup(ThisEnv->DBG_Table, name)) ==
      NULL) {
    Error("DB group not found");
  }
  g_hash_table_foreach(ThisEnv->DBG_Table, (GHFunc)_CheckDBG, name);
}

void StopSystem(int no) {
  Message("receive stop signal");
  StopDBMaster(DBMasterServer);
}

extern void InitSystem(char *name) {
  struct sigaction sa;
  ENTER_FUNC;
  InitNET();

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGPIPE, &sa, NULL);

  sa.sa_handler = StopSystem;
  sa.sa_flags |= SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);

  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_NONE);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
  InitDB_Process("dbmaster");
  CheckDBG(name);

  if (PortNumber == NULL) {
    DBMasterPort = ParPortName(PORT_MASTER);
  } else {
    DBMasterPort = ParPort(PortNumber, PORT_MASTER);
  }

  DBMasterServer = NewDBMasterThread(ThisDBG, AuthURL, DBMasterPort, Back);
  if (!DBMasterServer) {
    Error("start dbmaster thread failed");
  }
  LEAVE_FUNC;
}

static ARG_TABLE
    option[] =
        {{"port", STRING, TRUE, (void *)&PortNumber, "waiting port name"},
         {"back", INTEGER, TRUE, (void *)&Back,
          "connection waiting queue number"},

         {"dir", STRING, TRUE, (void *)&Directory, "environment file name"},
         {"base", STRING, TRUE, (void *)&BaseDir, "base directory"},
         {"record", STRING, TRUE, (void *)&RecordDir, "record directory"},
         {"ddir", STRING, TRUE, (void *)&D_Dir, "*D directory"},
         /*
                 {	"host",		STRING,		TRUE,	(void*)&DB_Host,
                         "DB host name"
            }, {	"port",		STRING,		TRUE,	(void*)&DB_Port,
                         "DB port number"
            }, {	"db",		STRING,		TRUE,	(void*)&DB_Name,
                         "DB name"
            }, {	"user",		STRING,		TRUE,	(void*)&DB_User,
                         "DB user name"
            }, {	"pass",		STRING,		TRUE,	(void*)&DB_Pass,
                         "DB password"
            },
         */
         {"auth", STRING, TRUE, (void *)&AuthURL, "authentication server"},

         {"nocheck", BOOLEAN, TRUE, (void *)&fNoCheck,
          "no check dbredirector start"},
         {"nosumcheck", BOOLEAN, TRUE, (void *)&fNoSumCheck,
          "no count dbredirector updates"},
         {"maxretry", INTEGER, TRUE, (void *)&MaxSendRetry,
          "send retry dbredirector"},
         {"retryint", INTEGER, TRUE, (void *)&RetryInterval,
          "retry interval of dbredirector(sec)"},
         /* slave-master options */
         {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  PortNumber = NULL;
  Back = 5;
  BaseDir = NULL;
  RecordDir = NULL;
  D_Dir = NULL;
  Directory = "./directory";
  AuthURL = "glauth://localhost:8001"; /*	PORT_GLAUTH	*/

  DB_User = NULL;
  DB_Pass = NULL;
  DB_Host = NULL;
  DB_Port = NULL;
  DB_Name = DB_User;

  fNoSumCheck = FALSE;
  MaxSendRetry = 3;
}

extern int main(int argc, char **argv) {
  FILE_LIST *fl;
  char *name;
  int i;

  SetDefault();
  fl = GetOption(option, argc, argv, NULL);

  InitMessage("dbmaster", NULL);

  for (i = 0; i < argc; ++i) {
    dbgprintf("%s ", argv[i]);
  }

  if (fl && fl->name) {
    name = fl->name;
  } else {
    name = "";
  }
  InitSystem(name);
  Message("dbmaster start");
  ExecuteServer();
  Message("dbmaster end");

  return (0);
}
