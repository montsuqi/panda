/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>

#include "libmondai.h"
#include "RecParser.h"
#include "enum.h"
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
#include "audit.h"
#include "debug.h"

static char *PortNumber;
static int Back;
static char *Directory;
static char AppName[128];

static DBG_Struct *ThisDBG;
static DBG_Struct *AuditDBG;
static pthread_t _FileThread;

static pthread_mutex_t redlock;
static pthread_mutex_t ticketlock;
static pthread_cond_t redcond;

static GSList *TicketList;
static volatile int TICKETID = 1;

static Bool fDbsyncstatus = TRUE;
volatile sig_atomic_t fShutdown = FALSE;
volatile sig_atomic_t fReopen = FALSE;
volatile static Bool fSync = FALSE;
volatile static int LOCKFD = 0;
volatile static int SYNCFD = 0;

#define DBLOG_PROGRAM "dblogger"
#define FILE_SEP '/'

static VeryfyData *NewVerfyData(void) {
  VeryfyData *veryfydata;
  veryfydata = New(VeryfyData);
  veryfydata->checkData = NewLBS();
  veryfydata->redirectData = NewLBS();
  return veryfydata;
}

static void FreeVeryfyData(VeryfyData *veryfydata) {
  FreeLBS(veryfydata->checkData);
  FreeLBS(veryfydata->redirectData);
  xfree(veryfydata);
  veryfydata = NULL;
}

static Ticket *NewTicket(void) {
  Ticket *ticket;
  ticket = New(Ticket);
  ticket->ticket_id = 0;
  ticket->fd = 0;
  ticket->veryfydata = NULL;
  ticket->auditlog = NULL;
  ticket->status = TICKET_BEGIN;
  return ticket;
}

static Ticket *LookupTicket(int ticket_id, int fd) {
  GSList *list;
  Ticket *ticket;

  if (ticket_id == 0) {
    Warning("Illegal ticket_id");
    return NULL;
  }
  pthread_mutex_lock(&ticketlock);
  list = TicketList;
  while (list) {
    if (list->data) {
      ticket = (Ticket *)list->data;
      if ((ticket->ticket_id == ticket_id) && (ticket->fd == fd)) {
        pthread_mutex_unlock(&ticketlock);
        return ticket;
      }
    }
    list = list->next;
  }
  pthread_mutex_unlock(&ticketlock);
  return NULL;
}

static void LockTicket(NETFILE *fpLog) {
  pthread_mutex_lock(&ticketlock);
  LOCKFD = fpLog->fd;
}

static void UnLockTicket(NETFILE *fpLog) {
  LOCKFD = 0;
  pthread_mutex_unlock(&ticketlock);
}

static void CleanLock(NETFILE *fpLog) {
  if ((LOCKFD > 0) && (LOCKFD == fpLog->fd)) {
    Message("clean lock %d\n", fpLog->fd);
    UnLockTicket(fpLog);
    LOCKFD = 0;
  }
}

static void SyncModeStart(NETFILE *fpLog) {
  Message("Sync mode start");
  fSync = TRUE;
  SYNCFD = fpLog->fd;
  pthread_cond_signal(&redcond);
}

static void SyncModeEnd(NETFILE *fpLog) {
  if (fSync) {
    fSync = FALSE;
    Message("Sync mode end");
  }
  SYNCFD = 0;
}

static void SyncWait(NETFILE *fpLog) {
  int i = 0;

  LockTicket(fpLog);
  while (g_slist_length(TicketList) > 0) {
    sleep(1);
    i++;
    if (i > 5) {
      Warning("The ticket processing doesn't end.");
      break;
    }
  }
  SendPacketClass(fpLog, RED_SYNC_WAIT);
  sleep(1);
  UnLockTicket(fpLog);
}

static void CleanSyncMode(NETFILE *fpLog) {
  if ((SYNCFD > 0) && (SYNCFD == fpLog->fd)) {
    Message("clean Sync mode");
    fSync = FALSE;
  }
  SYNCFD = 0;
}

static void OrderTicket(NETFILE *fpLog) {
  Ticket *ticket;
  ticket = NewTicket();
  ticket->fd = fpLog->fd;
  ticket->ticket_id = TICKETID++;
  TicketList = g_slist_append(TicketList, ticket);
  SendInt(fpLog, ticket->ticket_id);
}

static void RecvRedData(NETFILE *fpLog) {
  int ticket_id;
  Ticket *ticket;
  VeryfyData *veryfydata;
  veryfydata = NewVerfyData();
  ticket_id = RecvInt(fpLog);

  RecvLBS(fpLog, veryfydata->checkData);
  LBS_EmitEnd(veryfydata->checkData);

  RecvLBS(fpLog, veryfydata->redirectData);
  LBS_EmitEnd(veryfydata->redirectData);

  ticket = LookupTicket(ticket_id, fpLog->fd);
  if ((ticket != NULL) && (ticket->status == TICKET_BEGIN)) {
    ticket->veryfydata = veryfydata;
    ticket->status = TICKET_DATA;
  } else {
    FreeVeryfyData(veryfydata);
  }
}

static Ticket *DequeueTicket(void) {
  GSList *first = NULL;
  Ticket *ticket = NULL;
  while (ticket == NULL) {
    pthread_mutex_lock(&redlock);
    pthread_mutex_lock(&ticketlock);
    first = g_slist_nth(TicketList, 0);
    pthread_mutex_unlock(&ticketlock);
    if (first == NULL) {
      if (fShutdown || fReopen || fSync) {
        pthread_mutex_unlock(&redlock);
        break;
      }
    }
    if (first) {
      ticket = (Ticket *)(first->data);
      if ((ticket->status == TICKET_COMMIT) ||
          (ticket->status == TICKET_ABORT) ||
          (ticket->status == TICKET_AUDIT)) {
        pthread_mutex_lock(&ticketlock);
        TicketList = g_slist_remove_link(TicketList, first);
        pthread_mutex_unlock(&ticketlock);
        g_slist_free_1(first);
      } else {
        ticket = NULL;
      }
    }
    if (ticket == NULL) {
      pthread_cond_wait(&redcond, &redlock);
    }
    pthread_mutex_unlock(&redlock);
  }
  return ticket;
}

static void CommitTicket(NETFILE *fpLog) {
  int ticket_id;
  Ticket *ticket;

  ticket_id = RecvInt(fpLog);
  ticket = LookupTicket(ticket_id, fpLog->fd);
  if (ticket) {
    if (ticket->status != TICKET_ABORT) {
      ticket->status = TICKET_COMMIT;
    }
    SendPacketClass(fpLog, RED_OK);
  } else {
    Warning("The transaction (%d) is not found. List count (%d). Current "
            "maximum ID is (%d)",
            ticket_id, g_slist_length(TicketList), TICKETID);
    SendPacketClass(fpLog, RED_NOT);
  }
  pthread_cond_signal(&redcond);
}

static void AbortTicket(NETFILE *fpLog) {
  Ticket *ticket;
  int ticket_id;
  ticket_id = RecvInt(fpLog);
  ticket = LookupTicket(ticket_id, fpLog->fd);
  if (ticket) {
    ticket->status = TICKET_ABORT;
  }
  pthread_cond_signal(&redcond);
}

static void AllAbortTicket(NETFILE *fpLog) {
  GSList *list;
  Ticket *ticket;
  pthread_mutex_lock(&ticketlock);
  for (list = TicketList; list; list = list->next) {
    ticket = (Ticket *)list->data;
    if ((ticket != NULL) && (ticket->fd == fpLog->fd) &&
        ((ticket->status == TICKET_BEGIN) || (ticket->status == TICKET_DATA))) {
      ticket->status = TICKET_ABORT;
      dbgprintf("Auto abort %llu\n", ticket->ticket_id);
    }
  }
  pthread_mutex_unlock(&ticketlock);
  pthread_cond_signal(&redcond);
}

static void AllAllAbortTicket(void) {
  GSList *list;
  Ticket *ticket;
  pthread_mutex_lock(&ticketlock);
  for (list = TicketList; list; list = list->next) {
    ticket = (Ticket *)list->data;
    if ((ticket != NULL) &&
        ((ticket->status == TICKET_BEGIN) || (ticket->status == TICKET_DATA))) {
      ticket->status = TICKET_ABORT;
      dbgprintf("Auto abort %llu\n", ticket->ticket_id);
    }
  }
  pthread_mutex_unlock(&ticketlock);
}

static void AuditTicket(NETFILE *fpLog, LargeByteString *lbs) {
  Ticket *ticket;
  ticket = NewTicket();
  ticket->status = TICKET_AUDIT;
  ticket->auditlog = lbs;
  LockTicket(fpLog);
  TicketList = g_slist_append(TicketList, ticket);
  UnLockTicket(fpLog);
}

static void RecvAuditLog(NETFILE *fpLog) {
  LargeByteString *lbs;
  lbs = NewLBS();
  RecvLBS(fpLog, lbs);
  if (LBS_Size(lbs) > 0) {
    AuditTicket(fpLog, lbs);
  } else {
    FreeLBS(lbs);
  }
}

static void LogThread(void *para) {
  int fhLog = (int)(long)para;
  NETFILE *fpLog;
  PacketClass c;
  Bool fSuc = TRUE;
  dbgmsg("log thread!\n");
  fpLog = SocketToNet(fhLog);
  do {
    switch (c = RecvPacketClass(fpLog)) {
    case RED_BEGIN:
      OrderTicket(fpLog);
      break;
    case RED_DATA:
      RecvRedData(fpLog);
      break;
    case RED_COMMIT:
      CommitTicket(fpLog);
      break;
    case RED_ABORT:
      AbortTicket(fpLog);
      break;
    case RED_PING:
      SendPacketClass(fpLog, RED_PONG);
      break;
    case RED_STATUS:
      SendChar(fpLog, ThisDBG->dbstatus);
      break;
    case RED_LOCK:
      LockTicket(fpLog);
      break;
    case RED_UNLOCK:
      UnLockTicket(fpLog);
      break;
    case RED_AUDIT:
      RecvAuditLog(fpLog);
      break;
    case RED_SYNC_START:
      SyncModeStart(fpLog);
      break;
    case RED_SYNC_END:
      SyncModeEnd(fpLog);
      break;
    case RED_SYNC_WAIT:
      SyncWait(fpLog);
      break;
    case RED_END:
      fSuc = FALSE;
      break;
    default:
      SendPacketClass(fpLog, RED_NOT);
      fSuc = FALSE;
      break;
    }
  } while (!fShutdown && fSuc && fpLog->fOK);
  CleanLock(fpLog);
  CleanSyncMode(fpLog);
  AllAbortTicket(fpLog);
  CloseNet(fpLog);
  dbgmsg("log thread close!\n");
}

extern pthread_t ConnectServer(int _fh) {
  int fh;
  pthread_t thr;


  for (;;) {
    if ((fh = accept(_fh, 0, 0)) < 0) {
      if (errno == EINTR) {
        continue;
      }
      Error("accept: %s", strerror(errno));
    } else {
      break;
    }
  }
  pthread_create(&thr, NULL, (void *(*)(void *))LogThread, (void *)(long)fh);
  pthread_detach(thr);
  return (thr);
}

static void WriteLog(FILE *fp, char *state) {
  time_t nowtime;
  struct tm Now;
  if (fp != NULL) {
    time(&nowtime);
    localtime_r(&nowtime, &Now);
    fprintf(fp,
            "%s %04d/%02d/%02d/%02d:%02d:%02d/ ========== %s ========== %s\n",
            ThisDBG->func->commentStart, Now.tm_year + 1900, Now.tm_mon + 1,
            Now.tm_mday, Now.tm_hour, Now.tm_min, Now.tm_sec, state,
            ThisDBG->func->commentEnd);
    fflush(fp);
  }
}

static void WriteLogQuery(FILE *fp, char *query) {
  static int count = 0;
  time_t nowtime;
  struct tm Now;

  if (fp != NULL) {
    time(&nowtime);
    localtime_r(&nowtime, &Now);
    fprintf(fp, "%s %04d/%02d/%02d/%02d:%02d:%02d/%08d %s",
            ThisDBG->func->commentStart, Now.tm_year + 1900, Now.tm_mon + 1,
            Now.tm_mday, Now.tm_hour, Now.tm_min, Now.tm_sec, count,
            ThisDBG->func->commentEnd);
    fprintf(fp, "%s\n", query);
    fflush(fp);
    count++;
  }
}

static Bool ConnectDB(void) {
  Bool rc = TRUE;
  DBG_Struct *rdbg;
  int retry = 0;
  if (GetDB_DBname(ThisDBG) == NULL) {
    return rc;
  }
  if (ThisDBG->dbstatus != DB_STATUS_CONNECT) {
    OpenRedirectDB(ThisDBG);
    if (ThisDBG->redirect != NULL) {
      /* ReRedirect ReConnect */
      rdbg = ThisDBG->redirect;
      while (ThisDBG->fpLog == NULL) {
        retry++;
        if (retry > MaxSendRetry) {
          break;
        }
        sleep(1);
        OpenDB_RedirectPort(ThisDBG);
      }
      if (ThisDBG->fpLog != NULL) {
        Message("ReRedirect Server (%s) connected.", rdbg->name);
      } else {
        Warning("ReRedirect Server (%s) not found.", rdbg->name);
      }
    }
    if (ThisDBG->dbstatus == DB_STATUS_CONNECT) {
      Message("connect to database successed");
    } else {
      Message("connect to database failed");
      rc = FALSE;
    }
  }
  return rc;
}

void ReConnectDB(void) {
  int retry = 0;
  while (!ConnectDB()) {
    retry++;
    if (retry > MaxSendRetry) {
      break;
    }
    sleep(CONNECT_INTERVAL);
  }
  if (ThisDBG->dbstatus == DB_STATUS_UNCONNECT) {
    ThisDBG->dbstatus = DB_STATUS_FAILURE;
  }
}

static Bool DisConnectDB(void) {
  Bool rc = TRUE;
  CloseRedirectDB(ThisDBG);
  if (ThisDBG->dbstatus == DB_STATUS_DISCONNECT) {
    Message("disconnect to database successed");
  } else {
    rc = FALSE;
    Message("disconnect to database failed");
  }
  return rc;
}

static FILE *OpenLogFile(void) {
  FILE *fp = NULL;

  if (ThisDBG->file != NULL) {
    umask((mode_t)0077);
    if ((fp = fopen(ThisDBG->file, "a+")) == NULL) {
      Warning("can not open log file :%s", ThisDBG->file);
    }
  }
  return fp;
}

extern Bool ConnectAuditDB(void) {
  Bool rc = TRUE;
  if (AuditDBG == NULL) {
    return rc;
  }
  if (AuditDBG->dbstatus != DB_STATUS_CONNECT) {
    OpenRedirectDB(AuditDBG);
    if (AuditDBG->dbstatus == DB_STATUS_CONNECT) {
      Message("connect to audit database successed");
    } else {
      Message("connect to audit database failed");
      rc = FALSE;
    }
  }
  return rc;
}

static FILE *OpenAuditLogFile(void) {
  FILE *fp = NULL;

  if (AuditLogFile != NULL) {
    umask((mode_t)0077);
    if ((fp = fopen(AuditLogFile, "a+")) == NULL) {
      Warning("can not open audit log file :%s", AuditLogFile);
    }
  }
  return fp;
}

static int CloseLogFile(FILE *fp) { return fclose(fp); }

static FILE *ReopenSystem(FILE *fp) {
  FILE *rfp = NULL;
  DisConnectDB();
  if (fp != NULL) {
    CloseLogFile(fp);
  }
  ConnectDB();
  rfp = OpenLogFile();
  return rfp;
}

static int CheckRedirectData(LargeByteString *src, LargeByteString *dsc) {
  int rc = MCP_OK;
  if (strcmp(LBS_Body(src), LBS_Body(dsc)) == 0) {
    rc = MCP_OK;
  } else {
    Warning("CheckData difference %s<>%s", LBS_Body(src), LBS_Body(dsc));
    rc = MCP_BAD_OTHER;
  }
  return rc;
}

extern int WriteDB(LargeByteString *query, LargeByteString *orgcheck) {
  int rc;
  LargeByteString *redcheck = NULL;
  char buff[SIZE_BUFF];

  LBS_EmitStart(ThisDBG->checkData);
  rc = TransactionRedirectStart(ThisDBG);
  if (rc == MCP_OK) {
    rc = ExecRedirectDBOP(ThisDBG, LBS_Body(query), TRUE);
    LBS_EmitEnd(ThisDBG->checkData);
    redcheck = ThisDBG->checkData;
  }
  if (rc == MCP_OK) {
    if ((!fNoSumCheck) && (LBS_Size(orgcheck) > 0)) {
      rc = CheckRedirectData(orgcheck, redcheck);
      if (rc != MCP_OK) {
        snprintf(buff, SIZE_BUFF, "Difference for the update check %s...",
                 (char *)LBS_Body(query));
        Warning(buff);
      }
    }
  }
  if (rc == MCP_OK) {
    rc = TransactionRedirectEnd(ThisDBG);
  }
  return rc;
}

static int ExecDB(VeryfyData *veryfydata) {
  int rc = MCP_OK;
  if (ThisDBG->dbstatus == DB_STATUS_UNCONNECT) {
    ReConnectDB();
  }
  if (ThisDBG->dbstatus == DB_STATUS_CONNECT) {
    rc = WriteDB(veryfydata->redirectData, veryfydata->checkData);
  }
  if (rc != MCP_OK) {
    CloseRedirectDB(ThisDBG);
    if (rc == MCP_BAD_CONN) {
      ThisDBG->dbstatus = DB_STATUS_UNCONNECT;
    } else {
      ThisDBG->dbstatus = DB_STATUS_FAILURE;
    }
  }
  return ThisDBG->dbstatus;
}

static void ReRedirect(char *query, char *checkData) {
  if (ThisDBG->redirect == NULL)
    return;
  LockDB_Redirect(ThisDBG);
  BeginDB_Redirect(ThisDBG);
  UnLockDB_Redirect(ThisDBG);
  PutDB_Redirect(ThisDBG, query);
  CopyCheckDataDB_Redirect(ThisDBG, checkData);
  CommitDB_Redirect(ThisDBG);
}

static void WriteRedirectAuditLog(void) {
  VeryfyData *veryfydata;

  veryfydata = NewVerfyData();
  veryfydata->checkData = LBS_Duplicate(AuditDBG->checkData);
  veryfydata->redirectData = LBS_Duplicate(AuditDBG->redirectData);
  ExecDB(veryfydata);
  ReRedirect(LBS_Body(veryfydata->redirectData),
             LBS_Body(veryfydata->checkData));
  FreeVeryfyData(veryfydata);
}

static void SyncMode(FILE *fp) {
  DisConnectDB();
  ThisDBG->dbstatus = DB_STATUS_SYNC;
  while (fSync) {
    sleep(1);
  }
  ConnectDB();
  fDbsyncstatus = TRUE;
  WriteLog(fp, "The database synchronized again. ");
}

static void CheckFailure(FILE *fp) {
  char *failure = "DB synchronous failure";
  if (ThisDBG->dbstatus == DB_STATUS_FAILURE) {
    WriteLog(fp, failure);
    Warning(failure);
    ThisDBG->dbstatus = DB_STATUS_DISCONNECT;
    fDbsyncstatus = FALSE;
  }
}

extern void WriteAuditLog(FILE *afp, FILE *fp, Ticket *ticket) {
  int rc;
  static Bool ExistADBGAuditTable = FALSE;
  static Bool ExistTDBGAuditTable = FALSE;
  while (fSync) {
    Message("auditlog wait...");
    sleep(1);
  }

  if (ticket->auditlog != NULL) {
    if (LBS_Size(ticket->auditlog) > 0) {
      if (AuditDBG != NULL) {
        if (!ExistADBGAuditTable) {
          CheckAuditTable(AuditDBG);
          ExistADBGAuditTable = TRUE;
        }
        LBS_EmitStart(AuditDBG->redirectData);
        LBS_EmitStart(AuditDBG->checkData);
        TransactionStart(AuditDBG);
        rc = ExecDBOP(AuditDBG, LBS_Body(ticket->auditlog), TRUE);
        TransactionEnd(AuditDBG);
        LBS_EmitEnd(AuditDBG->redirectData);
        LBS_EmitEnd(AuditDBG->checkData);
        if (rc == MCP_OK) {
          if (!ExistTDBGAuditTable) {
            CheckAuditTable(ThisDBG);
            ExistTDBGAuditTable = TRUE;
          }
          WriteRedirectAuditLog();
          CheckFailure(fp);
        }
      }
      if (afp != NULL) {
        fprintf(afp, "%s\n", (char *)LBS_Body(ticket->auditlog));
        fflush(afp);
      }
    }
    FreeLBS(ticket->auditlog);
  }
}

static void HandleRedirector(VeryfyData *veryfydata) {
  if (ExecDB(veryfydata) == DB_STATUS_UNCONNECT) {
    /* Retry */
    ExecDB(veryfydata);
  }
}

static void FileThread(void *dummy) {
  VeryfyData *veryfydata;
  FILE *fp, *afp;
  Ticket *ticket;
  char header[SIZE_BUFF];
  fp = OpenLogFile();
  ConnectDB();
  afp = OpenAuditLogFile();
  ConnectAuditDB();

  strncpy(header, "dbredirector start", sizeof(header));
  if (GetDB_DBname(ThisDBG) == NULL) {
    strncat(header, "(No database)", sizeof(header) - strlen(header) - 1);
    /* ReRedirect */
    OpenDB_RedirectPort(ThisDBG);
    ThisDBG->dbstatus = DB_STATUS_NOCONNECT;
  }
  if (fNoSumCheck) {
    strncat(header, "(No sum check)", sizeof(header) - strlen(header) - 1);
  }
  WriteLog(fp, header);
  while (TRUE) {
    ticket = DequeueTicket();
    if (ticket == NULL) {
      if (fSync) {
        SyncMode(fp);
        continue;
      }
      if (fReopen) {
        fp = ReopenSystem(fp);
        WriteLog(fp, header);
        fReopen = FALSE;
        continue;
      } else {
        break;
      }
    }
    if (ticket->status == TICKET_COMMIT) {
      veryfydata = ticket->veryfydata;
      if (veryfydata != NULL) {
        if (LBS_Size(veryfydata->redirectData) > 0) {
	  HandleRedirector(veryfydata);
	  CheckFailure(fp);
          ReRedirect(LBS_Body(veryfydata->redirectData),
                     LBS_Body(veryfydata->checkData));
          WriteLogQuery(fp, LBS_Body(veryfydata->redirectData));
        }
        FreeVeryfyData(veryfydata);
      }
    } else if (ticket->status == TICKET_AUDIT) {
      WriteAuditLog(afp, fp, ticket);
    }
    xfree(ticket);
  }
  if (GetDB_DBname(ThisDBG) == NULL) {
    /* ReRedirect */
    CloseDB_RedirectPort(ThisDBG);
  }
  CleanUNIX_Socket(ThisDBG->redirectPort);
  WriteLog(fp, "dbredirector stop ");
  if (!fDbsyncstatus) {
    WriteLog(fp, "DB synchronous failure");
  }
}

extern void ExecuteServer(void) {
  int _fh;
  fd_set ready;
  int maxfd;

  pthread_mutex_init(&redlock, NULL);
  pthread_mutex_init(&ticketlock, NULL);
  pthread_cond_init(&redcond, NULL);

  _fh = InitServerPort(ThisDBG->redirectPort, Back);
  maxfd = _fh;

  pthread_create(&_FileThread, NULL, (void *(*)(void *))FileThread, NULL);

  while (!fShutdown) {
    FD_ZERO(&ready);
    FD_SET(_fh, &ready);
    if (select(maxfd + 1, &ready, NULL, NULL, NULL) < 0) {
      if (errno == EINTR) {
        pthread_cond_signal(&redcond);
        continue;
      }
      Error("select: ", strerror(errno));
    }
    if (FD_ISSET(_fh, &ready)) {
      ConnectServer(_fh);
    }
  }
  AllAllAbortTicket();
  pthread_cond_signal(&redcond);
  pthread_join(_FileThread, NULL);
}

#ifdef DEBUG
static void DumpDBG(char *name, DBG_Struct *dbg, void *dummy) {
  dbgprintf("name     = [%s]\n", dbg->name);
  dbgprintf("\ttype     = [%s]\n", dbg->type);
  dbgprintf("\tDB name  = [%s]\n", GetDB_DBname(dbg));
  dbgprintf("\tDB user  = [%s]\n", GetDB_User(dbg));
  dbgprintf("\tDB pass  = [%s]\n", GetDB_Pass(dbg));
  dbgprintf("\tDB sslmode  = [%s]\n", GetDB_Sslmode(dbg));

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
  if (dbg->redirect != NULL) {
    red_dbg = dbg->redirect;
    if (strcmp(red_dbg->name, red_name) == 0) {
      src_port = StrDup(StringPort(GetDB_Port(dbg)));
      dsc_port = StrDup(StringPort(GetDB_Port(red_dbg)));
      if ((dbname = GetDB_DBname(dbg)) != NULL) {
        dbg_dbname = dbname;
      }
      if ((dbname = GetDB_DBname(red_dbg)) != NULL) {
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

static void LookupAuditDBG(char *name, DBG_Struct *dbg, void *dummy) {
  if ((dbg->auditlog > 0) && (AuditDBG == NULL)) {
    AuditDBG = dbg;
  }
}

extern void SetAuditDBG(void) {
  g_hash_table_foreach(ThisEnv->DBG_Table, (GHFunc)LookupAuditDBG, NULL);
  /* The audit log is not issued with the dbredirector. */
  if (AuditDBG != NULL) {
    AuditDBG->auditlog = 0;
  }
}

void StopHandler(int no) {
  (void)no;
  fShutdown = TRUE;
  Message("receive stop signal");
}

void ReopenHandler(int no) {
  (void)no;
  fReopen = TRUE;
  Message("receive reopen signal");
}

extern void InitSystem(char *name, char *program) {
  struct sigaction sa;
  char *filename;
  InitNET();

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGPIPE, &sa, NULL);

  sa.sa_handler = StopHandler;
  sa.sa_flags |= SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);

  sa.sa_handler = ReopenHandler;
  sa.sa_flags |= SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, NULL);

  /* 親プロセス(monitor)停止で自動停止 */
  prctl(PR_SET_PDEATHSIG, SIGHUP);

  InitDirectory();
  SetUpDirectory(Directory, NULL, NULL, NULL, P_NONE);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
  InitDB_Process(AppName);

  CheckDBG(name);
  if (!fNoAudit) {
    SetAuditDBG();
  }

  if (PortNumber != NULL) {
    ThisDBG->redirectPort = ParPortName(PortNumber);
  }
  if (ThisDBG->redirectPort == NULL) {
    ThisDBG->redirectPort = ParPortName(PORT_REDIRECT);
  }
  dbgprintf("cmd filename => %s", program);
  filename = rindex(program, FILE_SEP);
  if (filename) {
    ++filename;
    dbgprintf("cmd filename => %s", filename);
  }
  //	TicketList = g_slist_alloc();
}

static ARG_TABLE option[] = {
    {"port", STRING, TRUE, (void *)&PortNumber, "waiting port name"},
    {"back", INTEGER, TRUE, (void *)&Back, "connection waiting queue number"},

    {"dir", STRING, TRUE, (void *)&Directory, "environment file name"},
    {"base", STRING, TRUE, (void *)&BaseDir, "base directory"},
    {"record", STRING, TRUE, (void *)&RecordDir, "record directory"},
    {"ddir", STRING, TRUE, (void *)&D_Dir, "*D directory"},

    {"host", STRING, TRUE, (void *)&DB_Host, "DB host name"},
    {"port", STRING, TRUE, (void *)&DB_Port, "DB port number"},
    {"db", STRING, TRUE, (void *)&DB_Name, "DB name"},
    {"user", STRING, TRUE, (void *)&DB_User, "DB user name"},
    {"pass", STRING, TRUE, (void *)&DB_Pass, "DB password"},

    {"auditlog", STRING, TRUE, (void *)&AuditLogFile, "audit log file name"},
    {"nocheck", BOOLEAN, TRUE, (void *)&fNoCheck,
     "no check dbredirector start"},
    {"noredirect", BOOLEAN, TRUE, (void *)&fNoRedirect,
     "don't use dbredirector"},
    {"nosumcheck", BOOLEAN, TRUE, (void *)&fNoSumCheck,
     "no count dbredirector updates"},
    {"noaudit", BOOLEAN, TRUE, (void *)&fNoAudit, "don't write audit log"},
    {"maxretry", INTEGER, TRUE, (void *)&MaxSendRetry,
     "send retry dbredirector"},
    {"retryint", INTEGER, TRUE, (void *)&RetryInterval,
     "retry interval of dbredirector(sec)"},

    {NULL, 0, FALSE, NULL, NULL}};

static void SetDefault(void) {
  PortNumber = NULL;
  Back = 5;
  BaseDir = NULL;
  RecordDir = NULL;
  D_Dir = NULL;
  Directory = "./directory";

  DB_User = NULL;
  DB_Pass = NULL;
  DB_Host = NULL;
  DB_Port = NULL;
  DB_Name = DB_User;

  AuditLogFile = NULL;
  fNoCheck = FALSE;
  fNoSumCheck = FALSE;
  fNoRedirect = FALSE;
  fNoAudit = FALSE;
  MaxSendRetry = 3;
  RetryInterval = 5;
}

extern int main(int argc, char **argv) {
  FILE_LIST *fl;
  char *name;

  InitMessage("dbredirector", NULL);

  SetDefault();
  fl = GetOption(option, argc, argv, NULL);
  if (fl && fl->name) {
    name = fl->name;
  } else {
    name = "";
  }
  snprintf(AppName, sizeof(AppName), "dbredirector-%s", name);
  InitSystem(name, argv[0]);

  Message("dbredirector start");
  ExecuteServer();
  Message("dbredirector end");
  return (0);
}
