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
#include <locale.h>
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
#include "gettext.h"
#include "message.h"
#include "debug.h"

static char *Directory;
static char *DBConfig;
static char *ImportFile;
static Bool List;
static Bool Blob;
static char *DeleteID;
static char *InfoID;
static char *CheckID;
static char *ExportID;
static char *OutputFile;
static unsigned int LifeType;

static ARG_TABLE option[] = {
    {"dir", STRING, TRUE, (void *)&Directory, N_("directory file name")},
    {"import", STRING, TRUE, (void *)&ImportFile, "Import file name"},
    {"export", STRING, TRUE, (void *)&ExportID, "Export ID"},
    {"blob", BOOLEAN, TRUE, (void *)&Blob, "user BLOB ID instead of ID"},
    {"list", BOOLEAN, TRUE, (void *)&List, "list of imported file"},
    {"delete", STRING, TRUE, (void *)&DeleteID, "Delete imported file"},
    {"info", STRING, TRUE, (void *)&InfoID, "display monblob Info"},
    {"check", STRING, TRUE, (void *)&CheckID, "check ID has data"},
    {"output", STRING, TRUE, (void *)&OutputFile, "output file name"},
    {"lifetype", INTEGER, TRUE, (void *)&LifeType, "lifetime type"},
    {"dbconfig", STRING, TRUE, (void *)&DBConfig,
     "database connection config file"},
    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  Directory = NULL;
  ImportFile = NULL;
  ExportID = NULL;
  DeleteID = NULL;
  InfoID = NULL;
  CheckID = NULL;
  OutputFile = NULL;
  DBConfig = NULL;
  LifeType = MON_LIFE_LONG;
  List = FALSE;
  Blob = FALSE;
  fTimer = TRUE;
}

static void InitSystem(void) {
  char *dir;
  InitMessage("monblob", NULL);
  if ((dir = getenv("MON_DIRECTORY_PATH")) != NULL) {
    Directory = dir;
  }
  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_NONE);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
  SetDBConfig(DBConfig);
}

static char *valstr(ValueStruct *value, char *name) {
  return ValueToString(GetItemLongName(value, name), NULL);
}

static void _list_print(ValueStruct *value) {
  printf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", valstr(value, "id"),
         valstr(value, "blobid"), valstr(value, "filename"),
         valstr(value, "size"), valstr(value, "content_type"),
         valstr(value, "lifetype"), valstr(value, "status"),
         valstr(value, "importtime"));
}

static Bool list_print(ValueStruct *ret) {
  int i;
  ValueStruct *value;

  if (ret == NULL) {
    return FALSE;
  }
  if (IS_VALUE_ARRAY(ret)) {
    for (i = 0; i < ValueArraySize(ret); i++) {
      value = ValueArrayItem(ret, i);
      _list_print(value);
    }
  } else if (IS_VALUE_RECORD(ret)) {
    _list_print(ret);
  }
  return TRUE;
}

static Bool all_list_print(DBG_Struct *dbg) {
  Bool rc;
  ValueStruct *ret;
  ret = monblob_list(dbg);
  rc = list_print(ret);
  FreeValueStruct(ret);
  return rc;
}

static Bool monblob_info_print(DBG_Struct *dbg, char *id) {
  Bool rc;
  ValueStruct *ret;

  ret = monblob_info(dbg, id);
  rc = list_print(ret);
  FreeValueStruct(ret);
  return rc;
}

static void _monblob_check_id(DBG_Struct *dbg, char *id) {
  if (monblob_check_id(dbg, id)) {
    printf("true");
    exit(0);
  } else {
    printf("false");
    exit(1);
  }
}

extern int main(int argc, char **argv) {
  DBG_Struct *dbg;
  char *id;
  Bool rc;

  setlocale(LC_CTYPE, "ja_JP.UTF-8");
  SetDefault();
  GetOption(option, argc, argv, NULL);
  InitSystem();

  if ((ImportFile == NULL) && (ExportID == NULL) && (List == FALSE) &&
      (DeleteID == NULL) && (InfoID == NULL) && (CheckID == NULL)) {
    PrintUsage(option, argv[0], NULL);
    exit(1);
  }

  if (ExportID != NULL && OutputFile == NULL) {
    printf("set -output\n");
    PrintUsage(option, argv[0], NULL);
    exit(1);
  }

  dbg = GetDBG_monsys();
  dbg->dbt = NewNameHash();

  if (OpenDB(dbg) != MCP_OK) {
    exit(1);
  }
  monblob_setup(dbg, FALSE);

  TransactionStart(dbg);
  if (List) {
    if (!all_list_print(dbg)) {
      exit(1);
    }
    CloseDB(dbg);
    return 0;
  }

  if (ImportFile) {
    int persist = 1;
    if (Blob) {
      LifeType = MON_LIFE_SHORT;
      persist = 0;
    }
    id = monblob_import(dbg, NULL, persist, ImportFile, NULL, LifeType);
    if (!id) {
      exit(1);
    }
    printf("%s\n", id);
    xfree(id);
  } else if (ExportID) {
    rc = monblob_export_file(dbg, ExportID, OutputFile);
    if (!rc) {
      exit(1);
    }
    if (Blob) {
      if (!monblob_delete(dbg, ExportID)) {
        exit(1);
      }
    }
    printf("export: %s\n", OutputFile);
  } else if (DeleteID) {
    if (!monblob_delete(dbg, DeleteID)) {
      exit(1);
    }
  } else if (InfoID) {
    if (!monblob_info_print(dbg, InfoID)) {
      exit(1);
    }
  } else if (CheckID) {
    _monblob_check_id(dbg, CheckID);
  }

  TransactionEnd(dbg);
  CloseDB(dbg);

  return 0;
}
