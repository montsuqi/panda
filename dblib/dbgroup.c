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

/*
#define	TIMER
#define	DEBUG
#define	TRACE
*/

#define DBGROUP
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <sys/time.h>
#include <assert.h>

#include "defaults.h"
#include "enum.h"
#include "libmondai.h"
#include "directory.h"
#include "DDparser.h"
#include "DBparser.h"
#include "dbgroup.h"
#include "debug.h"

extern void DBGroup_Init(RecordStruct **db, RecordStructMeta **meta) {
  ThisDB = db;
  ThisDBMeta = meta;
}

extern void InitializeCTRL(DBCOMM_CTRL *ctrl) {
  *ctrl->func = '\0';
  *ctrl->rname = '\0';
  *ctrl->pname = '\0';
  *ctrl->user = '\0';
  *ctrl->term = '\0';
  ctrl->rno = 0;
  ctrl->pno = 0;
  ctrl->rcount = 0;
  ctrl->blocks = 0;
  ctrl->rc = 0;
  ctrl->limit = 1;
  ctrl->redirect = 1;
  ctrl->usage = 1;
  ctrl->time = 0;
  ctrl->rec = NULL;
  ctrl->value = NULL;
  ctrl->src = NULL;
}

static void SetApplicationName(DBG_Struct *dbg, char *appname) {
  if (appname != NULL) {
    dbg->appname = appname;
  }
}

extern void InitDB_Process(char *appname) {
  int i;
  for (i = 0; i < ThisEnv->cDBG; i++) {
    SetApplicationName(ThisEnv->DBG[i], appname);
  }
}

static DB_FUNC LookupFUNC(DBG_Struct *dbg, char *funcname) {
  DB_FUNC func;

  assert(dbg);
  assert(dbg->func);

  func = (DB_FUNC)g_hash_table_lookup(dbg->func->table, funcname);
  return func;
}

static void SetDBAudit(DBG_Struct *dbg) {
  ValueStruct *audit;
  LargeByteString *lbs;

  if (dbg->auditlog > 0) {
    if (ThisEnv->auditrec->value != NULL) {
      audit = ThisEnv->auditrec->value;
      SetValueInteger(GetItemLongName(audit, "ticket_id"), dbg->ticket_id);
      lbs = dbg->last_query;
      if ((lbs != NULL) && (LBS_Size(lbs) > 0)) {
        SetValueString(GetItemLongName(audit, "exec_query"), LBS_Body(lbs),
                       dbg->coding);
      }
    }
  }
}
static void CheckErrCount(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  if (ctrl->rc >= 0) {
    dbg->errcount = 0;
  } else if (ctrl->rc < 0) {
    dbg->errcount++;
  }
  if (dbg->errcount >= MAX_ERR_COUNT) {
    Warning("Error count reached limit\n");
    exit(2);
  }
}

static void _DBRecordFunc(char *rname, RecordStruct *rec, char *fname) {
  DB_Struct *db = RecordDB(rec);
  DBG_Struct *dbg = db->dbg;
  int ix;
  DB_Operation *op;

  if ((ix = (int)(long)g_hash_table_lookup(db->opHash, fname)) > 0) {
    if ((op = db->ops[ix - 1]) != NULL) {
      if (!(*dbg->func->primitive->record)(dbg, fname, rec)) {
        Warning("function not found [%s]\n", fname);
      }
    }
  }
}

static int _ExecFunction(DBG_Struct *dbg, char *funcname, DBCOMM_CTRL *ctrl,
                         Bool fAll) {
  typedef void (*Operation_FUNC)(DBG_Struct *, DBCOMM_CTRL *);
  Operation_FUNC opfunc;

  if ((!fAll) && (dbg->dbt == NULL)) {
    return MCP_OK;
  }
  opfunc = (Operation_FUNC)LookupFUNC(dbg, funcname);
  if (opfunc == NULL) {
    Warning("function not found [%s]\n", funcname);
    return MCP_BAD_FUNC;
  }
  /* _DBOPEN,_DBDISCONNECT,_DBSTART,_DBCOMMIT... */
  (*opfunc)(dbg, ctrl);
  dbgprintf("ctrl.rc   = [%d]", ctrl->rc);
  if ((dbg->func->primitive->record != NULL) && (dbg->dbt != NULL)) {
    /* _DBRECORD */
    g_hash_table_foreach(dbg->dbt, (GHFunc)_DBRecordFunc, funcname);
  }
  return ctrl->rc;
}

static int ExecFunction(DBG_Struct *dbg, char *funcname, Bool fAll) {
  DBCOMM_CTRL ctrl;
  int i;

  InitializeCTRL(&ctrl);
  dbgprintf("func  = [%s]", funcname);
  dbgprintf("fAll = [%s]", fAll ? "TRUE" : "FALSE");
  if (dbg == NULL) {
    for (i = 0; i < ThisEnv->cDBG; i++) {
      ctrl.rc += _ExecFunction(ThisEnv->DBG[i], funcname, &ctrl, fAll);
    }
  } else {
    dbgprintf("name  = [%s]", dbg->name);
    ctrl.rc = _ExecFunction(dbg, funcname, &ctrl, fAll);
  }
  return (ctrl.rc);
}

extern ValueStruct *ExecDBESCAPE(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                 RecordStruct *rec, ValueStruct *args) {
  DB_FUNC func;
  ValueStruct *ret = NULL;

  func = LookupFUNC(dbg, "DBESCAPE");
  if (func != NULL) {
    ret = (*func)(dbg, ctrl, rec, args);
  }
  return ret;
}

extern ValueStruct *ExecDBESCAPEBYTEA(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                      RecordStruct *rec, ValueStruct *args) {
  DB_FUNC func;
  ValueStruct *ret = NULL;

  func = LookupFUNC(dbg, "DBESCAPEBYTEA");
  if (func != NULL) {
    ret = (*func)(dbg, ctrl, rec, args);
  }
  return ret;
}

extern ValueStruct *ExecDBUNESCAPEBYTEA(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                                        RecordStruct *rec, ValueStruct *args) {
  DB_FUNC func;
  ValueStruct *ret = NULL;

  func = LookupFUNC(dbg, "DBUNESCAPEBYTEA");
  if (func != NULL) {
    ret = (*func)(dbg, ctrl, rec, args);
  }
  return ret;
}

extern int ExecDBOP(DBG_Struct *dbg, char *sql, Bool fRed) {
  int rc;
  rc = dbg->func->primitive->exec(dbg, sql, fRed);
  return (rc);
}

extern int ExecRedirectDBOP(DBG_Struct *dbg, char *sql, Bool fRed) {
  int rc;
  rc = dbg->func->primitive->exec(dbg, sql, fRed);
  return (rc);
}

extern ValueStruct *ExecDBQuery(DBG_Struct *dbg, char *sql, Bool fRed) {
  ValueStruct *ret;
  ret = dbg->func->primitive->query(dbg, sql, fRed);
  return ret;
}

extern ValueStruct *ExecDB_Process(DBCOMM_CTRL *ctrl, RecordStruct *rec,
                                   ValueStruct *args) {
  DB_FUNC func;
  DBG_Struct *dbg;
  ValueStruct *ret;
  long start, end;

  ret = NULL;
  ctrl->rc = 0;
  start = GetNowTime();
  dbgprintf("func  = [%s]", ctrl->func);
  if (ctrl->fDBOperation) {
    ctrl->rc = ExecFunction(NULL, ctrl->func, FALSE);
  } else {
    if (rec == NULL) {
      Warning("bad function [%s:%s:%s] null record\n", ctrl->func, ctrl->rname,
              ctrl->pname);
      ctrl->rc = MCP_BAD_FUNC;
    } else {
      dbg = RecordDB(rec)->dbg;
      func = LookupFUNC(dbg, ctrl->func);
      if (func == NULL) {
        /* _DBACCESS */
        func = (DB_FUNC)dbg->func->primitive->access;
      }
      ret = (*func)(dbg, ctrl, rec, args);
      CheckErrCount(dbg, ctrl);
      SetDBAudit(dbg);
    }
    if (ctrl->rc < 0) {
      Warning("bad function [%s:%s:%s] rc = %d\n", ctrl->func, ctrl->rname,
              ctrl->pname, ctrl->rc);
    }
  }
  end = GetNowTime();
  ctrl->time = (end - start);
  TimerPrintf(start, end, "%s:%s:%s\n", ctrl->func, ctrl->rname, ctrl->pname);
  return (ret);
}

static int ExecDBG_Operation(DBG_Struct *dbg, char *funcname) {
  int rc;
  rc = ExecFunction(dbg, funcname, FALSE);
  return rc;
}

extern int TransactionStart(DBG_Struct *dbg) {
  int rc;

  rc = ExecDBG_Operation(dbg, "DBSTART");
  return (rc);
}

extern int TransactionEnd(DBG_Struct *dbg) {
  int rc;

  rc = ExecDBG_Operation(dbg, "DBCOMMIT");
  return (rc);
}

extern int GetDBRedirectStatus(int newstatus) {
  int dbstatus;
  DBG_Struct *dbg, *rdbg;
  int i;
  dbstatus = DB_STATUS_NOCONNECT;
  for (i = 0; i < ThisEnv->cDBG; i++) {
    dbg = ThisEnv->DBG[i];
    if (dbg->redirect != NULL) {
      rdbg = dbg->redirect;
      if (dbstatus < rdbg->dbstatus) {
        dbstatus = rdbg->dbstatus;
      }
    }
  }
  return dbstatus;
}

extern void RedirectError(void) {
  DBG_Struct *dbg;
  int i;
  for (i = 0; i < ThisEnv->cDBG; i++) {
    dbg = ThisEnv->DBG[i];
    CloseDB_RedirectPort(dbg);
  }
}

extern int OpenDB(DBG_Struct *dbg) { return ExecDBG_Operation(dbg, "DBOPEN"); }

extern int OpenRedirectDB(DBG_Struct *dbg) {
  return ExecFunction(dbg, "DBOPEN", TRUE);
}

extern int CloseRedirectDB(DBG_Struct *dbg) {
  return ExecFunction(dbg, "DBDISCONNECT", TRUE);
}

extern int TransactionRedirectStart(DBG_Struct *dbg) {
  return ExecFunction(dbg, "DBSTART", FALSE);
}

extern int TransactionRedirectEnd(DBG_Struct *dbg) {
  int rc;

  rc = ExecFunction(dbg, "DBCOMMIT", FALSE);

  return rc;
}

extern int CloseDB(DBG_Struct *dbg) {
  return ExecDBG_Operation(dbg, "DBDISCONNECT");
}

static void to_upperstr(char *org) {
  char ustr[SIZE_OTHER];
  char *str, *up;

  str = org;
  up = ustr;
  while ((*up = toupper(*str)) != 0) {
    up++;
    str++;
  }
  strcpy(org, ustr);
}

static char *GetMONDB_ENV(DBG_Struct *dbg, char *itemname) {
  char buff[SIZE_OTHER];
  char *str;

  sprintf(buff, "%s%s%s", MONDB, dbg->name, itemname);
  to_upperstr(buff);
  str = getenv(buff);
  return str;
}

extern Port *GetDB_Port(DBG_Struct *dbg) {
  Port *port;

  port = dbg->server->port;
  return (port);
}

extern char *GetDB_Host(DBG_Struct *dbg) {
  char *host;

  if (DB_Host != NULL) {
    host = DB_Host;
  } else {
    if ((host = GetMONDB_ENV(dbg, "HOST")) == NULL) {
      host = IP_HOST((GetDB_Port(dbg)));
    }
  }
  return (host);
}

extern char *GetDB_PortName(DBG_Struct *dbg) {
  Port *port;
  char *portname;

  if (DB_Port != NULL) {
    portname = DB_Port;
  } else {
    if ((portname = GetMONDB_ENV(dbg, "PORT")) == NULL) {
      port = GetDB_Port(dbg);
      if ((port != NULL) && (port->type == PORT_IP)) {
        portname = IP_PORT(port);
      } else {
        portname = NULL;
      }
    }
  }
  return (portname);
}

extern int GetDB_PortMode(DBG_Struct *dbg) {
  Port *port;
  int mode;

  port = GetDB_Port(dbg);
  if ((port != NULL) && port->type == PORT_UNIX) {
    mode = UNIX_MODE(port);
  } else {
    mode = 0;
  }
  return (mode);
}

extern char *GetDB_DBname(DBG_Struct *dbg) {
  char *name = NULL;
  if (DB_Name != NULL) {
    name = DB_Name;
  } else {
    if ((name = GetMONDB_ENV(dbg, "NAME")) == NULL) {
      name = dbg->server->dbname;
    }
  }
  return (name);
}

extern char *GetDB_User(DBG_Struct *dbg) {
  char *user = NULL;
  if (DB_User != NULL) {
    user = DB_User;
  } else {
    if ((user = GetMONDB_ENV(dbg, "USER")) == NULL) {
      user = dbg->server->user;
    }
  }
  return (user);
}

extern char *GetDB_Pass(DBG_Struct *dbg) {
  char *pass = NULL;
  if (DB_Pass != NULL) {
    pass = DB_Pass;
  } else {
    if ((pass = GetMONDB_ENV(dbg, "PASS")) == NULL) {
      pass = dbg->server->pass;
    }
  }
  return (pass);
}

extern char *GetDB_Crypt(DBG_Struct *dbg) {
  char *crypto = NULL;

  if (ThisEnv->CryptoPass != NULL) {
    crypto = ThisEnv->CryptoPass;
  } else {
    crypto = GetMONDB_ENV(dbg, "CRYPTOPASS");
  }
  return (crypto);
}

extern char *GetDB_Sslmode(DBG_Struct *dbg) {
  char *sslmode = NULL;
  if ((sslmode = GetMONDB_ENV(dbg, "SSLMODE")) == NULL) {
    sslmode = dbg->server->sslmode;
  }
  return (sslmode);
}

extern char *GetDB_Sslcert(DBG_Struct *dbg) {
  char *sslcert = NULL;
  if ((sslcert = GetMONDB_ENV(dbg, "SSLCERT")) == NULL) {
    sslcert = dbg->server->sslcert;
  }
  return (sslcert);
}

extern char *GetDB_Sslkey(DBG_Struct *dbg) {
  char *sslkey = NULL;
  if ((sslkey = GetMONDB_ENV(dbg, "SSLKEY")) == NULL) {
    sslkey = dbg->server->sslkey;
  }
  return (sslkey);
}

extern char *GetDB_Sslrootcert(DBG_Struct *dbg) {
  char *sslrootcert = NULL;
  if ((sslrootcert = GetMONDB_ENV(dbg, "SSLROOTCERT")) == NULL) {
    sslrootcert = dbg->server->sslrootcert;
  }
  return (sslrootcert);
}

extern char *GetDB_Sslcrl(DBG_Struct *dbg) {
  char *sslcrl = NULL;
  if ((sslcrl = GetMONDB_ENV(dbg, "SSLCRL")) == NULL) {
    sslcrl = dbg->server->sslcrl;
  }
  return (sslcrl);
}

extern Bool IsDBOperation(char *func) {
  Bool ret;
  if (!strcmp(func, "DBOPEN") || !strcmp(func, "DBCLOSE") ||
      !strcmp(func, "DBSTART") || !strcmp(func, "DBCOMMIT") ||
      !strcmp(func, "DBDISCONNECT")) {
    ret = TRUE;
  } else {
    ret = FALSE;
  }
  return ret;
}

extern Bool IsDBUpdateFunc(char *func) {
  if ((strncmp(func, "DBFETCH", 7) == 0) ||
      (strncmp(func, "DBCLOSECURSOR", 13) == 0)) {
    return FALSE;
  }
  return TRUE;
}

static ValueStruct *GetTableFuncValue(RecordStruct *rec, DBCOMM_CTRL *ctrl) {
  ValueStruct *value;
  PathStruct *path;
  DB_Operation *op;
  int pno, ono;

  value = rec->value;
  if ((strlen(ctrl->pname) > 0) &&
      ((pno = (int)(long)g_hash_table_lookup(RecordDB(rec)->paths,
                                             ctrl->pname)) != 0)) {
    pno--;
    path = RecordDB(rec)->path[pno];
    ctrl->usage = path->usage;
    value = (path->args != NULL) ? path->args : value;
    if ((strlen(ctrl->func) > 0) && ((ono = (int)(long)g_hash_table_lookup(
                                          path->opHash, ctrl->func)) != 0)) {
      op = path->ops[ono - 1];
      value = (op->args != NULL) ? op->args : value;
    }
  } else {
    pno = 0;
  }
  ctrl->pno = pno;
  return (value);
}

extern int GetTableFuncData(RecordStruct **rec, ValueStruct **value,
                            DBCOMM_CTRL *ctrl) {
  int rno;

  *value = NULL;
  *rec = NULL;

  ctrl->fDBOperation = FALSE;
  if (strlen(ctrl->rname) <= 0) {
    return 0;
  }

  if ((rno = (int)(long)g_hash_table_lookup(DB_Table, ctrl->rname)) != 0) {
    ctrl->rno = rno - 1;
    *rec = ThisDB[rno - 1];
    *value = GetTableFuncValue(*rec, ctrl);
    return 1;
  }
  return 0;
}

extern Bool SetDBCTRLValue(DBCOMM_CTRL *ctrl, char *pname) {
  int pno, ono;
  ValueStruct *value;
  PathStruct *path;
  DB_Operation *op;

  if (!pname) {
    Warning("path name not set.\n");
    return FALSE;
  }
  strncpy(ctrl->pname, pname, SIZE_NAME);
  if (ctrl->rec == NULL) {
    return FALSE;
  }
  value = ctrl->rec->value;
  if ((pno = (int)(long)g_hash_table_lookup(RecordDB(ctrl->rec)->paths,
                                            pname)) != 0) {
    pno--;
    path = RecordDB(ctrl->rec)->path[pno];
    ctrl->usage = path->usage;
    value = (path->args != NULL) ? path->args : value;
    if ((strlen(ctrl->func) > 0) && ((ono = (int)(long)g_hash_table_lookup(
                                          path->opHash, ctrl->func)) != 0)) {
      op = path->ops[ono - 1];
      ctrl->src = path->ops[ono - 1]->proc;
      value = (op->args != NULL) ? op->args : value;
    }
  } else {
    pno = 0;
  }
  ctrl->pno = pno;
  ctrl->value = value;
  return TRUE;
}

extern Bool SetDBCTRLRecord(DBCOMM_CTRL *ctrl, char *rname) {
  int rno;
  Bool rc = FALSE;
  if (!rname) {
    Warning("table name not set.\n");
    return rc;
  }
  strncpy(ctrl->rname, rname, SIZE_NAME);
  ctrl->fDBOperation = FALSE;
  if ((rno = (int)(long)g_hash_table_lookup(DB_Table, ctrl->rname)) != 0) {
    ctrl->rno = rno - 1;
    ctrl->rec = ThisDB[ctrl->rno];
    if (GetDBRecMemSave() && ThisDBMeta != NULL && ctrl->rec == NULL) {
      ctrl->rec = DB_Parser(ThisDBMeta[ctrl->rno]->name,ThisDBMeta[ctrl->rno]->gname,TRUE);
      ThisDB[ctrl->rno] = ctrl->rec;
    }
    rc = TRUE;
  } else {
    Warning("The table name of [%s] is not found.\n", rname);
  }
  return rc;
}

#ifdef DEBUG
extern void DumpDB_Node(DBCOMM_CTRL *ctrl) {
  printf("func   = [%s]\n", ctrl->func);
  printf("rname  = [%s]\n", ctrl->rname);
  printf("pname  = [%s]\n", ctrl->pname);
  printf("blocks = %d\n", ctrl->blocks);
  printf("rno    = %d\n", ctrl->rno);
  printf("pno    = %d\n", ctrl->pno);
  printf("count  = %d\n", ctrl->rcount);
  printf("limit  = %d\n", ctrl->limit);
}
#endif

extern RecordStruct *BuildDBCTRL(void) {
  RecordStruct *rec;
  char *buff, *p;

  buff = (char *)xmalloc(SIZE_BUFF);
  p = buff;
  p += sprintf(p, "dbctrl	{");
  p += sprintf(p, "rc int;");
  p += sprintf(p, "count int;");
  p += sprintf(p, "limit int;");
  p += sprintf(p, "func	varchar(%d);", SIZE_FUNC);
  p += sprintf(p, "rname	varchar(%d);", SIZE_NAME);
  p += sprintf(p, "pname	varchar(%d);", SIZE_NAME);
  sprintf(p, "};");
  rec = ParseRecordMem(buff);

  return (rec);
}

static void CopyValuebyName(ValueStruct *to, char *to_name, ValueStruct *from,
                            char *from_name) {
  CopyValue(GetItemLongName(to, to_name), GetItemLongName(from, from_name));
}

extern RecordStruct *SetAuditRec(ValueStruct *mcp, RecordStruct *rec) {
  time_t now;
  struct tm tm_now;
  ValueStruct *audit;

  audit = rec->value;
  now = time(NULL);
  localtime_r(&now, &tm_now);
  SetValueDateTime(GetItemLongName(audit, "exec_date"), tm_now);
  CopyValuebyName(audit, "func", mcp, "func");
  CopyValuebyName(audit, "result", mcp, "rc");
  CopyValuebyName(audit, "username", mcp, "dc.user");
  CopyValuebyName(audit, "term", mcp, "dc.term");
  CopyValuebyName(audit, "windowname", mcp, "dc.window");
  CopyValuebyName(audit, "widget", mcp, "dc.widget");
  CopyValuebyName(audit, "event", mcp, "dc.event");
  CopyValuebyName(audit, "comment", mcp, "db.logcomment");
  return (rec);
}

extern void AuditLog(ValueStruct *mcp) {
  int i;
  DBG_Struct *dbg;
  ValueStruct *ret;
  DB_FUNC func;
  RecordStruct *rec;
  rec = ThisEnv->auditrec;
  for (i = 0; i < ThisEnv->cDBG; i++) {
    dbg = ThisEnv->DBG[i];
    if (dbg->auditlog > 0) {
      rec = SetAuditRec(mcp, rec);
      if ((func = LookupFUNC(dbg, "DBAUDITLOG")) != NULL) {
        ret = (*func)(dbg, NULL, rec, rec->value);
        if (ret != NULL) {
          FreeValueStruct(ret);
        }
      }
    }
  }
}

extern void SetDBConfig(const char *file) {
  char *buf, *_buf, **elem;
  size_t size;
  int i;

  if (file == NULL) {
    return;
  }
  if (!g_file_get_contents(file, &_buf, &size, NULL)) {
    Error("can not read %s", file);
  }

  buf = g_malloc0(size + 1);
  memcpy(buf, _buf, size);
  g_free(_buf);

  elem = g_strsplit_set(buf, ":\n", -1);
  g_free(buf);
  for (i = 0; elem[i] != NULL; i++) {
  }
  if (i >= 5) {
    DB_Host = g_strdup(elem[0]);
    DB_Port = g_strdup(elem[1]);
    DB_Name = g_strdup(elem[2]);
    DB_User = g_strdup(elem[3]);
    DB_Pass = g_strdup(elem[4]);
  } else {
    Warning(
        "invalid DBConfig format. <Host>:<Port>:<Database>:<User>:<Password>");
  }
  g_strfreev(elem);
}
