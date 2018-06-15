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

/*
#define	TRACE
#define	DEBUG
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
#include "comm.h"
#include "dbgroup.h"
#include "dblib.h"
#include "queue.h"
#include "socket.h"
#include "message.h"
#include "enum.h"
#include "dblog.h"
#include "authstub.h"
#include "debug.h"
#include "dbslave_thread.h"

#define DBSLAVE_VERSION_01_00_00 10000 // 01 00 00

typedef struct {
  DBSlaveThread *ctx;
  DBLogCtx *log;
} PatchFuncArg;

int TransactionRedirectRollback(DBG_INSTANCE *dbg) {
  CloseRedirectDB(dbg);
  return MCP_OK;
}

extern DBSlaveThread *NewDBSlaveThread(DBG_INSTANCE *dbg, DBG_INSTANCE *log_dbg,
                                       Port *server_port, char *user,
                                       char *pass, Bool no_checksum,
                                       int replication_sec,
                                       int expire_log_sec) {
  DBSlaveThread *ctx = New(DBSlaveThread);
  DBLogCtx *log;
  int rc;

  ENTER_FUNC;
  memset(ctx, 0, sizeof(*ctx));

  /* check db */
  log = Open_DBLog(log_dbg, log_dbg->logTableName);
  if (!log) {
    Error("open logdb failed");
    DestroyDBSlaveThread(&ctx);
    return NULL;
  }
  Close_DBLog(&log);
  ctx->log = log_dbg;
  ctx->dbg = dbg;
  if (!IS_DB_STATUS_CONNECT(ctx->dbg)) {
    rc = OpenRedirectDB(ctx->dbg);
    if (rc != MCP_OK) {
      Error("open db failed");
      DestroyDBSlaveThread(&ctx);
      return NULL;
    }
    CloseRedirectDB(ctx->dbg);
  }

  ctx->server_port = server_port;
  ctx->user = StrDup(user);
  ctx->pass = StrDup(pass);
  ctx->no_checksum = no_checksum;
  ctx->replication_sec = replication_sec;
  ctx->expire_log_sec = expire_log_sec;

  ctx->main_thread_active = FALSE;

  pthread_cond_init(&ctx->timer, NULL);
  pthread_mutex_init(&ctx->mutex, NULL);
  LEAVE_FUNC;
  return ctx;
}

extern void DestroyDBSlaveThread(DBSlaveThread **ctx) {
  ENTER_FUNC;
  if (*ctx == NULL) {
    return;
  }

  if ((*ctx)->dbg) {
    CloseRedirectDB((*ctx)->dbg);
    (*ctx)->dbg = NULL;
  }

  if ((*ctx)->log) {
    CloseRedirectDB((*ctx)->log);
    (*ctx)->log = NULL;
  }
  if ((*ctx)->user) {
    xfree((*ctx)->user);
    (*ctx)->user = NULL;
  }
  if ((*ctx)->pass) {
    xfree((*ctx)->pass);
    (*ctx)->pass = NULL;
  }
  xfree(*ctx);
  *ctx = NULL;
  LEAVE_FUNC;
}

static void UnsetActiveFlagMain(void *userdata) {
  DBSlaveThread *ctx = (DBSlaveThread *)userdata;
  ENTER_FUNC;
  ctx->main_thread_active = FALSE;
}

static Bool Authentication(DBSlaveThread *ctx, NETFILE *server) {
  PacketClass cmd;
  LargeByteString *user;
  LargeByteString *pass;
  Bool ret = TRUE;
  ENTER_FUNC;

  user = NewLBS();
  LBS_EmitString(user, ctx->user);
  LBS_EmitEnd(user);
  pass = NewLBS();
  LBS_EmitString(pass, ctx->pass);
  LBS_EmitEnd(pass);

  SendPacketClass(server, DBREPLICATION_COMMAND_AUTH);
  SendUInt(server, DBSLAVE_VERSION_01_00_00);
  SendLBS(server, user);
  SendLBS(server, pass);
  cmd = RecvPacketClass(server);
  if (!server->fOK || cmd != DBREPLICATION_COMMAND_OK) {
    Warning("authentication failed");
    ret = FALSE;
  }
  FreeLBS(user);
  FreeLBS(pass);
  LEAVE_FUNC;
  return ret;
}

static Bool SendLogRequest(DBLogNo latest_no, NETFILE *server) {
  ENTER_FUNC;
  SendPacketClass(server, DBREPLICATION_COMMAND_REQ);
  SendUInt64(server, latest_no);
  LEAVE_FUNC;
  if (!server->fOK) {
    Warning("server socket error");
    return FALSE;
  }
  return TRUE;
}

static void FreeRecordData(DBLogRecord *rec) {
  ENTER_FUNC;
  if (rec->data) {
    FreeLBS(rec->data);
  }
  if (rec->checkdata) {
    FreeLBS(rec->checkdata);
  }
  memset(rec, 0, sizeof(*rec));
  LEAVE_FUNC;
}

static Bool RecvRecordData(NETFILE *server, DBLogRecord *rec) {
  PacketClass cmd;
  ENTER_FUNC;
  FreeRecordData(rec);
  rec->data = NewLBS();
  rec->checkdata = NewLBS();

  cmd = RecvPacketClass(server);
  if (!server->fOK || cmd != DBREPLICATION_COMMAND_RECORD) {
    return FALSE;
  }

  rec->no = RecvUInt64(server);
  RecvLBS(server, rec->data);
  RecvLBS(server, rec->checkdata);
  if (!server->fOK) {
    return FALSE;
  }

  LEAVE_FUNC;
  return TRUE;
}

static Bool CheckRedirectData(LargeByteString *a, LargeByteString *b) {
  Bool ret;
  ENTER_FUNC;
  ret = strcmp(LBS_Body(a), LBS_Body(b)) == 0 ? TRUE : FALSE;
  LEAVE_FUNC;
  return ret;
}

static Bool PatchLogRecord(void *userdata, DBLogRecord *rec) {
  PatchFuncArg *arg = (PatchFuncArg *)userdata;
  DBSlaveThread *ctx = arg->ctx;
  DBLogCtx *log = arg->log;
  int rc;
  Bool ret;
  Bool update_ok = TRUE;

  ENTER_FUNC;
  rc = ExecRedirectDBOP(ctx->dbg, LBS_Body(rec->data), TRUE, DB_UPDATE);
  if (rc == MCP_OK) {
    if ((!ctx->no_checksum) && (LBS_Size(rec->checkdata) > 0)) {
      ret = CheckRedirectData(ctx->dbg->checkData, rec->checkdata);
      if (!ret) {
        update_ok = FALSE;
        dbgprintf("diff [%s] => [%s]", LBS_Body(rec->checkdata),
                  LBS_Body(ctx->dbg->checkData));
        Warning("Difference for the update check %s...",
                (char *)LBS_Body(rec->data));
      }
    }
  } else {
    update_ok = FALSE;
    Warning("update error: %s", (char *)LBS_Body(rec->data));
  }
  if (update_ok) {
    ret = UpdateApplyDate_DBLog(log, rec->no);
    if (!ret) {
      Warning("dblog update applydate error");
      update_ok = FALSE;
    }
  }

  LEAVE_FUNC;
  return update_ok;
}

static Bool RecvLogs(DBSlaveThread *ctx, DBLogCtx *log, NETFILE *server) {
  Bool ret;
  Bool update_ok = TRUE;
  DBLogRecord rec = {0};
  ENTER_FUNC;

  while (TRUE) {
    ret = RecvRecordData(server, &rec);
    if (!ret) {
      if (!server->fOK) {
        Warning("server socket error");
        update_ok = FALSE;
      }
      break;
    }
    ret = PutNo_DBLog(log, rec.no, LBS_Body(rec.data), LBS_Body(rec.checkdata));
    if (!ret) {
      Warning("local log error");
      update_ok = FALSE;
    }
  }
  FreeRecordData(&rec);
  LEAVE_FUNC;
  return update_ok;
}

static Bool CopyLog(DBSlaveThread *ctx) {
  int fd;
  NETFILE *server = NULL;
  DBLogNo latest_no;
  DBLogCtx *log;
  int rc;
  Bool ret = TRUE;
  ENTER_FUNC;

  log = Open_DBLog(ctx->log, ctx->log->logTableName);
  if (log == NULL) {
    Warning("open log db failed");
    ret = FALSE;
    goto end_tran;
  }

  rc = TransactionRedirectStart(log->dbg);
  if (rc) {
    ret = FALSE;
    goto end_tran;
  }

  GetLatestNo_DBLog(log, &latest_no);
  dbgprintf("latest no %" PRIu64, latest_no);

  fd = ConnectSocket(ctx->server_port, SOCK_STREAM);
  if (fd < 0) {
    Warning("connect error");
    ret = FALSE;
    goto end_tran;
  }
  server = SocketToNet(fd);

  ret = Authentication(ctx, server);
  if (!ret) {
    goto end_tran;
  }

  ret = SendLogRequest(latest_no, server);
  if (!ret) {
    goto end_tran;
  }

  ret = RecvLogs(ctx, log, server);
  if (!ret) {
    goto end_tran;
  }

end_tran:
  if (server != NULL) {
    CloseNet(server);
  }
  if (ret) {
    rc = TransactionRedirectEnd(log->dbg);
    if (rc == MCP_OK) {
      TransactionRedirectRollback(log->dbg);
    }
    if (rc == MCP_OK) {
      dbgmsg("commit");
    }
  } else {
    dbgmsg("rollback");
    TransactionRedirectRollback(ctx->dbg);
    if (log != NULL) {
      TransactionRedirectRollback(log->dbg);
    }
  }
  Close_DBLog(&log);
  LEAVE_FUNC;
  return ret;
}

static Bool PatchLog(DBSlaveThread *ctx) {
  DBLogCtx *log;
  int rc;
  Bool ret;
  PatchFuncArg arg;

  ENTER_FUNC;
  ret = FALSE;
  log = Open_DBLog(ctx->log, ctx->log->logTableName);
  if (log == NULL) {
    Warning("open log db failed");
    goto _end_tran;
  }
  if (!IS_DB_STATUS_CONNECT(ctx->dbg)) {
    OpenRedirectDB(ctx->dbg);
  }

  rc = TransactionRedirectStart(log->dbg);
  if (rc)
    goto _end_tran;
  rc = TransactionRedirectStart(ctx->dbg);
  if (rc)
    goto _end_tran;

  arg.ctx = ctx;
  arg.log = log;
  ret = ForeachUnapplied_DBLog(log, PatchLogRecord, &arg);
  if (!ret)
    goto _end_tran;

_end_tran:
  if (ret) {
    rc = TransactionRedirectEnd(ctx->dbg);
    if (rc == MCP_OK) {
      rc = TransactionRedirectEnd(log->dbg);
    }
    if (rc == MCP_OK) {
      dbgmsg("commit");
    }
  } else {
    dbgmsg("rollback");
    TransactionRedirectRollback(ctx->dbg);
    if (log != NULL) {
      TransactionRedirectRollback(log->dbg);
    }
  }
  Close_DBLog(&log);

  LEAVE_FUNC;
  return ret;
}

static Bool TruncateLog(DBSlaveThread *ctx) {
  DBLogCtx *log;
  Bool ret = TRUE;
  ENTER_FUNC;

  if (ctx->expire_log_sec == 0) {
    /* no truncate */
    LEAVE_FUNC;
    return TRUE;
  }

  log = Open_DBLog(ctx->log, ctx->log->logTableName);
  if (log == NULL) {
    ret = FALSE;
    Warning("open log db failed");
    goto end_tran;
  }
  ret = Truncate_DBLog(log, ctx->expire_log_sec);
  if (!ret) {
    Warning("truncate failed");
  }
end_tran:
  Close_DBLog(&log);
  LEAVE_FUNC;
  return ret;
}

static void *DBSlaveThreadMain(void *userdata) {
  DBSlaveThread *ctx = (DBSlaveThread *)userdata;
  struct timespec interval;
  Bool processing = TRUE;
  Bool ret;
  ENTER_FUNC;
  pthread_cleanup_push(UnsetActiveFlagMain, ctx);
  ctx->main_thread_active = TRUE;
  do {
    pthread_mutex_lock(&ctx->mutex);
    dbgmsg("wait for timeout.. ");
    interval.tv_sec = time(NULL) + ctx->replication_sec;
    interval.tv_nsec = 0;
    pthread_cond_timedwait(&ctx->timer, &ctx->mutex, &interval);
    if (ctx->shutdown) {
      processing = FALSE;
    } else {
      ret = CopyLog(ctx);
      if (!ret) {
        Warning("copy log error");
      }
      ret = PatchLog(ctx);
      if (!ret) {
        Warning("patch log error");
      }
      ret = TruncateLog(ctx);
      if (!ret) {
        Warning("patch log error");
      }
    }
    pthread_mutex_unlock(&ctx->mutex);
  } while (processing);
  pthread_cleanup_pop(1);
  LEAVE_FUNC;
  return NULL;
}

extern void StartDBSlave(DBSlaveThread *ctx) {
  ENTER_FUNC;
  if (!ctx->main_thread_active) {
    ctx->shutdown = FALSE;
    pthread_create(&ctx->main_thread, NULL, DBSlaveThreadMain, ctx);
  }
  LEAVE_FUNC;
}

extern void JoinDBSlave(DBSlaveThread *ctx) {
  ENTER_FUNC;
  pthread_join(ctx->main_thread, NULL);
  LEAVE_FUNC;
}

extern void StopDBSlave(DBSlaveThread *ctx) {
  ENTER_FUNC;
  pthread_mutex_lock(&ctx->mutex);
  ctx->shutdown = TRUE;
  pthread_cond_signal(&ctx->timer);
  pthread_mutex_unlock(&ctx->mutex);
  pthread_join(ctx->main_thread, NULL);
  LEAVE_FUNC;
}
