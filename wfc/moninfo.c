/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>
#include <pthread.h>

#include "enum.h"
#include "libmondai.h"
#include "RecParser.h"
#include "net.h"
#include "comm.h"
#include "comms.h"
#include "const.h"
#include "dirs.h"
#include "wfcdata.h"
#include "dbgroup.h"
#include "sysdatacom.h"
#include "sysdbreq.h"
#include "socket.h"
#include "message.h"
#include "debug.h"

#define MAX_USER 100

static gchar *SysDataPort = SYSDATA_PORT;
static GOptionEntry entries[] = {{"port", 'p', 0, G_OPTION_ARG_STRING,
                                  &SysDataPort, "SystemDataThread port P", "P"},
                                 {NULL}};

enum {
  DUMP = 0,
  POPUP,
  POPUP_ALL,
  DIALOG,
  DIALOG_ALL,
  ABORT,
  ABORT_ALL,
  WHO,
  LAST
};

static struct _COMMAND {
  char *name;
  char *desc;
} command[] = {
    {"dump",
     "command 					- dump system database"},
    {"popup", "command <id> <message>	- send popup message"},
    {"popup-all", "command <message>		- send popup message"},
    {"dialog", "command <id> <message>	- send dialog message"},
    {"dialog-all", "command <message>		- send dialog message"},
    {"abort", "command <id> <message>	- send abort message"},
    {"abort-all", "command <message>		- send abort message"},
    {"who", "command					- show who is using"},
    {"", ""}};

static NETFILE *ConnectSysData() {
  int fd;
  NETFILE *fp;
  Port *port;
  ENTER_FUNC;
  fp = NULL;
  port = ParPort(SysDataPort, PORT_WFC);
  fd = ConnectSocket(port, SOCK_STREAM);
  DestroyPort(port);
  if (fd > 0) {
    fp = SocketToNet(fd);
  }
  if (fp == NULL) {
    printf("[ERROR] cannot connect sysdata thread\n");
    exit(1);
  }
  LEAVE_FUNC;
  return fp;
}

static void Dump() {
  NETFILE *fp;
  int i, j, n;
  ValueStruct *vals, *a, *v, *w;
  PacketClass rc;
  char *keys[] = {
      "id",          "user",        "host",         "agent",
      "window",      "widget",      "event",        "in_process",
      "create_time", "access_time", "process_time", "total_process_time",
      "count",       "popup",       "dialog",       "abort",
      NULL};
  ENTER_FUNC;
  printf("---\n");
  fp = ConnectSysData();
  vals = RecParseValueMem(SYSDBVALS_DEF, NULL);
  InitializeValue(vals);
  rc = SYSDB_GetDataAll(fp, &n, vals);
  a = GetRecordItem(vals, "values");
  if (rc == SESSION_CONTROL_OK) {
    for (i = 0; i < n; i++) {
      v = GetArrayItem(a, i);
      for (j = 0; keys[j] != NULL; j++) {
        w = GetRecordItem(v, keys[j]);
        if (j == 0) {
          printf("- :%s: %s\n", keys[j], ValueToString(w, NULL));
        } else {
          printf("  :%s: %s\n", keys[j], ValueToString(w, NULL));
        }
      }
    }
  }
  SendPacketClass(fp, SYSDATA_END);
  CloseNet(fp);
  FreeValueStruct(vals);
  exit(0);
  LEAVE_FUNC;
}

extern void Who(void) {
  NETFILE *fp;
  int i, n;
  ValueStruct *vals, *a, *v;
  PacketClass rc;
  ENTER_FUNC;
  fp = ConnectSysData();
  vals = RecParseValueMem(SYSDBVALS_DEF, NULL);
  InitializeValue(vals);
  rc = SYSDB_GetDataAll(fp, &n, vals);
  a = GetRecordItem(vals, "values");
  printf("|%-11s|%-17s|%-32s|%-32s|%-17s|%-21s|%-129s|\n", "USER", "HOST",
         "LOGIN@", "LAST@", "WINDOW", "AGENT", "ID");
  if (rc == SESSION_CONTROL_OK) {
    for (i = 0; i < n; i++) {
      v = GetArrayItem(a, i);
      printf("|%-11s|%-17s|%-32s|%-32s|%-17s|%-21s|%-129s|\n",
             ValueToString(GetRecordItem(v, "user"), NULL),
             ValueToString(GetRecordItem(v, "host"), NULL),
             ValueToString(GetRecordItem(v, "create_time"), NULL),
             ValueToString(GetRecordItem(v, "access_time"), NULL),
             ValueToString(GetRecordItem(v, "window"), NULL),
             ValueToString(GetRecordItem(v, "agent"), NULL),
             ValueToString(GetRecordItem(v, "id"), NULL));
    }
  }
  SendPacketClass(fp, SYSDATA_END);
  CloseNet(fp);
  FreeValueStruct(vals);
  exit(0);
  LEAVE_FUNC;
}

static void SendMessage(char *key, char *termid, char *message) {
  NETFILE *fp;
  ValueStruct *val, *v;
  ENTER_FUNC;
  fp = ConnectSysData();
  val = RecParseValueMem(SYSDBVAL_DEF, NULL);
  InitializeValue(val);
  v = GetRecordItem(val, "id");
  SetValueString(v, termid, NULL);
  v = GetRecordItem(val, key);
  SetValueString(v, message, NULL);
  SYSDB_SetMessage(fp, val);
  SendPacketClass(fp, SYSDATA_END);
  CloseNet(fp);
  FreeValueStruct(val);
  exit(0);
  LEAVE_FUNC;
}

static void SendMessageAll(char *key, char *message) {
  NETFILE *fp;
  ValueStruct *val, *v;
  ENTER_FUNC;
  fp = ConnectSysData();
  val = RecParseValueMem(SYSDBVAL_DEF, NULL);
  InitializeValue(val);
  v = GetRecordItem(val, key);
  SetValueString(v, message, NULL);
  SYSDB_SetMessageAll(fp, val);
  SendPacketClass(fp, SYSDATA_END);
  CloseNet(fp);
  FreeValueStruct(val);
  exit(0);
  LEAVE_FUNC;
}

static void print_usage() {
  int i;

  printf("%-16s%-16s\n\n", "command", "description");
  for (i = 0; i < LAST; i++) {
    printf("%-16s%-16s\n", command[i].name, command[i].desc);
  }
}

extern int main(int argc, char *argv[]) {
  int i;
  int c;
  GError *error = NULL;
  GOptionContext *context;

  InitMessage("moninfo", NULL);
  InitNET();
  RecParserInit();

  context = g_option_context_new("- montsuqi system database utility");
  g_option_context_add_main_entries(context, entries, NULL);
  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print("option parsing failed: %s\n", error->message);
    exit(1);
  }
  if (argc < 2) {
    g_print("[ERROR] need command\n");
    print_usage();
    exit(1);
  }
  c = LAST;
  for (i = 0; i < LAST; i++) {
    if (!strcmp(command[i].name, argv[1])) {
      c = i;
    }
  }
  if (c == POPUP || c == DIALOG || c == ABORT) {
    if (argc < 4) {
      print_usage();
      exit(1);
    }
  } else if (c == POPUP_ALL || c == DIALOG_ALL || c == ABORT_ALL) {
    if (argc < 3) {
      print_usage();
      exit(1);
    }
  }

  switch (c) {
  case DUMP:
    Dump();
    break;
  case POPUP:
    SendMessage("popup", argv[2], argv[3]);
    break;
  case POPUP_ALL:
    SendMessageAll("popup", argv[2]);
    break;
  case DIALOG:
    SendMessage("dialog", argv[2], argv[3]);
    break;
  case DIALOG_ALL:
    SendMessageAll("dialog", argv[2]);
    break;
  case ABORT:
    SendMessage("abort", argv[2], argv[3]);
    break;
  case ABORT_ALL:
    SendMessageAll("abort", argv[2]);
    break;
  case WHO:
    Who();
    break;
  }
  g_print("[ERROR] invalid command\n");
  print_usage();
  exit(1);
  return (0);
}
