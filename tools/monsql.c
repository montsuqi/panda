/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "libmondai.h"
#include "directory.h"
#include "dbgroup.h"
#include "monsys.h"
#include "gettext.h"
#include "option.h"
#include "enum.h"
#include "message.h"
#include "debug.h"

#define DefaultOutput "JSON"

static char *Directory;
static char *DBConfig;
static char *DBG_Name;
static char *Command;
static char *SQLFile;
static char *Output;
static Bool Redirect = FALSE;

static ARG_TABLE option[] = {
    {"dir", STRING, TRUE, (void *)&Directory, N_("directory file name")},
    {"dbg", STRING, TRUE, (void *)&DBG_Name, N_("db group name")},
    {"r", BOOLEAN, TRUE, (void *)&Redirect, N_("redirect query")},
    {"c", STRING, TRUE, (void *)&Command, N_("run only single command")},
    {"f", STRING, TRUE, (void *)&SQLFile, N_("execute commands from file")},
    {"o", STRING, TRUE, (void *)&Output,
     N_("out put type [JSON] [XML] [CSV]...")},
    {"dbconfig", STRING, TRUE, (void *)&DBConfig,
     "database connection config file"},
    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  Directory = "./directory";
  DBConfig = NULL;
  Redirect = FALSE;
}

static void InitSystem(void) {
  char *dir;
  InitMessage("monsql", NULL);
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

static char *ReadSQLFile(DBG_Struct *dbg, char *sqlfile) {
  char buff[SIZE_BUFF];
  LargeByteString *lbs;
  char *sql;
  FILE *fp;

  if ((fp = fopen(sqlfile, "r")) == NULL) {
    Warning("[%s] can not open file", sqlfile);
    return NULL;
  }
  lbs = NewLBS();
  while (fgets(buff, SIZE_BUFF, fp) != NULL) {
    LBS_EmitString(lbs, buff);
  }
  LBS_EmitEnd(lbs);
  fclose(fp);

  sql = Coding_monsys(dbg, LBS_Body(lbs));
  LBS_Clear(lbs);

  return sql;
}

static int FileCommands(DBG_Struct *dbg, Bool redirect, char *sqlfile) {
  int rc;
  char *sql;

  rc = OpenDB(dbg);
  if (rc != MCP_OK) {
    return rc;
  }
  TransactionStart(dbg);
  sql = ReadSQLFile(dbg, sqlfile);
  if (sql) {
    ExecDBOP(dbg, sql, redirect);
    xfree(sql);
  }
  rc = TransactionEnd(dbg);
  CloseDB(dbg);
  return rc;
}

static void OutPutValue(char *type, ValueStruct *value) {
  ValueStruct *invalue;
  ConvFuncs *conv;
  static CONVOPT *ConvOpt;
  char *buf;

  if (!value) {
    invalue = NewValue(GL_TYPE_ARRAY);
  } else if (IS_VALUE_RECORD(value)) {
    invalue = NewValue(GL_TYPE_ARRAY);
    ValueAddArrayItem(invalue, 0, value);
  } else {
    invalue = value;
  }
  if (!type) {
    type = DefaultOutput;
  }
  conv = GetConvFunc(type);
  if (conv == NULL) {
    Error("Output type error. [%s] does not support.", type);
  }
  ConvOpt = NewConvOpt();
  ConvSetRecName(ConvOpt, "RECORD");
  ConvOpt->fNewLine = TRUE;

  buf = xmalloc(conv->SizeValue(ConvOpt, invalue));
  conv->PackValue(ConvOpt, buf, invalue);
  printf("%s\n", buf);
  xfree(buf);
}

static int SingleCommand(DBG_Struct *dbg, Bool redirect, char *str) {
  int rc;
  ValueStruct *ret;
  char *sql;

  rc = OpenDB(dbg);
  if (rc != MCP_OK) {
    return rc;
  }
  TransactionStart(dbg);

  sql = Coding_monsys(dbg, str);
  ret = ExecDBQuery(dbg, sql, redirect);
  OutPutValue(Output, ret);
  FreeValueStruct(ret);
  xfree(sql);

  rc = TransactionEnd(dbg);
  CloseDB(dbg);
  return rc;
}

extern int main(int argc, char **argv) {
  Bool rc = 0;
  DBG_Struct *dbg;

  SetDefault();
  GetOption(option, argc, argv, NULL);
  InitSystem();

  if (DBG_Name != NULL) {
    dbg = GetDBG(DBG_Name);
  } else {
    dbg = GetDBG_monsys();
  }

  if (!dbg) {
    Error("DBG [%s] does not found.", DBG_Name);
  }
  dbg->dbt = NewNameHash();

  if (Command) {
    rc = SingleCommand(dbg, Redirect, Command);
  } else if (SQLFile) {
    rc = FileCommands(dbg, Redirect, SQLFile);
  }
  return rc;
}
