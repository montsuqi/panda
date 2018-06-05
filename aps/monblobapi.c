#define MAIN

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "libmondai.h"
#include "directory.h"
#include "dbgroup.h"
#include "dbutils.h"
#include "monsys.h"
#include "bytea.h"
#include "option.h"
#include "enum.h"
#include "comm.h"
#include "gettext.h"
#include "message.h"
#include "debug.h"

#define RETRY 1

static char *Directory;
static char *ExportID;
static char *Socket;

static char *OutputFile;
static unsigned int LifeType;

static ARG_TABLE option[] = {
    {"dir", STRING, TRUE, (void *)&Directory, N_("directory file name")},
    {"socket", STRING, TRUE, (void *)&Socket, "Socket file"},
    {"export", STRING, TRUE, (void *)&ExportID, "Export ID"},
    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  Directory = NULL;
  ExportID = NULL;
  OutputFile = NULL;
  LifeType = 0;
}

static void InitSystem(void) {
  char *dir;
  InitMessage("monblobapi", NULL);
  if ((dir = getenv("MON_DIRECTORY_PATH")) != NULL) {
    Directory = dir;
  }
  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_NONE);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
}

static int SendValue(NETFILE *fp, ValueStruct *value) {
  int ret = 0;
  size_t size;

  if (value == NULL) {
    SendLength(fp, 0);
  } else {
    size = ValueByteLength(value);
    SendLength(fp, size);
    ret = Send(fp, ValueByte(value), size);
  }
  return ret;
}

static NETFILE *ConnectBlobAPI(char *socketfile) {
  NETFILE *fp;
  Port *port;
  int fd;

  fp = NULL;
  port = ParPort(socketfile, NULL);
  fd = ConnectSocket(port, SOCK_STREAM);
  DestroyPort(port);
  if (fd > 0) {
    fp = SocketToNet(fd);
  } else {
    Error("cannot connect blob api");
  }
  return fp;
}

static void DisconnectBlobAPI(NETFILE *fp) {
  if (fp != NULL && CheckNetFile(fp)) {
    CloseNet(fp);
  }
}

static char *file_export(DBG_Struct *dbg, char *id, char *socket) {
  static char *filename;
  char *str;
  json_object *obj;
  NETFILE *fp;
  ValueStruct *ret, *value;
  ValueStruct *retval;

  ret = monblob_export(dbg, id);
  obj = json_object_new_object();
  if (!ret) {
    fprintf(stderr, "[%s] is not registered\n", id);
    json_object_object_add(obj, "status", json_object_new_int(404));
    retval = NULL;
  } else {
    value = GetItemLongName(ret, "file_data");
    retval = unescape_bytea(dbg, value);
    char *id;
    id = ValueToString(GetItemLongName(ret, "id"), dbg->coding);
    json_object_object_add(obj, "id", json_object_new_string(id));
    filename = ValueToString(GetItemLongName(ret, "filename"), dbg->coding);
    json_object_object_add(obj, "filename", json_object_new_string(filename));
    char *content_type;
    content_type =
        ValueToString(GetItemLongName(ret, "content_type"), dbg->coding);
    if (content_type == NULL || strlen(content_type) == 0) {
      json_object_object_add(
          obj, "content-type",
          json_object_new_string("application/octet-stream"));
    } else {
      json_object_object_add(obj, "content-type",
                             json_object_new_string(content_type));
    }
    json_object_object_add(obj, "status", json_object_new_int(200));
  }
  str = (char *)json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
  fp = ConnectBlobAPI(socket);
  SendString(fp, str);
  SendValue(fp, retval);
  FreeValueStruct(retval);
  Flush(fp);
  DisconnectBlobAPI(fp);
  FreeValueStruct(ret);
  return filename;
}

static char *_monblob_export(DBG_Struct *dbg, char *id, char *socket) {
  char *filename;

  TransactionStart(dbg);
  filename = file_export(dbg, id, socket);
  TransactionEnd(dbg);

  return filename;
}

extern int main(int argc, char **argv) {
  struct stat sb;
  DBG_Struct *dbg;
  int i;
  SetDefault();
  GetOption(option, argc, argv, NULL);
  InitSystem();

  if ((Socket == NULL) || (ExportID == NULL)) {
    PrintUsage(option, argv[0], NULL);
    exit(1);
  }

  for (i = 0;; i++) {
    if (stat(Socket, &sb) != -1) {
      break;
    }
    if (i >= RETRY) {
      Error("%s %s", strerror(errno), Socket);
    }
    sleep(1);
  }
  if (!S_ISSOCK(sb.st_mode)) {
    Error("%s is not socket file", Socket);
  }

  dbg = GetDBG_monsys();
  dbg->dbt = NewNameHash();

  if (OpenDB(dbg) != MCP_OK) {
    exit(1);
  }
  _monblob_export(dbg, ExportID, Socket);
  CloseDB(dbg);

  return 0;
}
