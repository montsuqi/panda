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
#define __FD_SETSIZE 8192
#define SOMAXCONN 1024
#define DEFAULT_LD "orca00"

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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h> /*	for	mknod	*/
#include <sys/prctl.h>
#include <unistd.h>
#include <glib.h>
#include <pthread.h>

#include "libmondai.h"
#include "RecParser.h"
#include "net.h"
#include "comm.h"
#include "dirs.h"
#include "wfcdata.h"
#include "wfc.h"
#include "mqthread.h"
#include "corethread.h"
#include "termthread.h"
#include "controlthread.h"
#include "sysdatathread.h"
#include "sessionctrl.h"
#include "dbgroup.h"
#include "option.h"
#include "message.h"
#include "debug.h"

#include "socket.h"
#include "queue.h"

#include "directory.h"

static char *ApsPortNumber;
static char *PortNumber;
static char *ControlPortNumber;
static int Back;
static char *Directory;

static Port *ApsPort;
static Port *WfcPort;
static Port *ControlPort;

static sigset_t SigMask;

#ifndef __USE_XOPEN2K
extern int pselect(int __nfds, fd_set *__restrict __readfds,
                   fd_set *__restrict __writefds,
                   fd_set *__restrict __exceptfds,
                   const struct timespec *__restrict __timeout,
                   const sigset_t *__restrict __sigmask);
#endif

#ifdef DEBUG
extern void DumpNode(SessionData *data) {
  printf("window   = [%s]\n", data->hdr->window);
  printf("widget   = [%s]\n", data->hdr->widget);
  printf("event    = [%s]\n", data->hdr->event);
  printf("term     = [%s]\n", data->hdr->term);
  printf("user     = [%s]\n", data->hdr->user);
}
#endif

extern LargeByteString *GetScreenData(SessionData *data, char *name) {
  LargeByteString *scrdata;
  RecordStruct *rec;

  if ((scrdata = (LargeByteString *)g_hash_table_lookup(data->scrpool, name)) ==
      NULL) {
    if ((rec = GetWindow(name)) != NULL) {
      scrdata = NewLBS();
      LBS_ReserveSize(scrdata, NativeSizeValue(NULL, rec->value), FALSE);
      NativePackValue(NULL, LBS_Body(scrdata), rec->value);
      g_hash_table_insert(data->scrpool, StrDup(name), scrdata);
    } else {
      scrdata = NULL;
    }
  }
  return (scrdata);
}

extern void ChangeLD(SessionData *data, LD_Node *newld) {
  int i;
  LargeByteString **scrdata;

  if (newld != NULL) {
    data->spa = g_hash_table_lookup(data->spadata, newld->info->name);
    if (data->spa == NULL) {
      if (newld->info->sparec != NULL) {
        data->spa = NewLBS();
        g_hash_table_insert(data->spadata, StrDup(newld->info->name),
                            data->spa);
        LBS_ReserveSize(data->spa,
                        NativeSizeValue(NULL, newld->info->sparec->value),
                        FALSE);
        NativePackValue(NULL, LBS_Body(data->spa), newld->info->sparec->value);
      }
    } else {
      InitializeValue(newld->info->sparec->value);
      NativePackValue(NULL, LBS_Body(data->spa), newld->info->sparec->value);
    }
    if (data->scrdata != NULL) {
      xfree(data->scrdata);
    }
    scrdata = (LargeByteString **)xmalloc(sizeof(LargeByteString *) *
                                          newld->info->cWindow);
    for (i = 0; i < newld->info->cWindow; i++) {
      scrdata[i] = GetScreenData(data, newld->info->windows[i]->name);
    }
    data->cWindow = newld->info->cWindow;
    data->scrdata = scrdata;
  }
  data->ld = newld;
  data->apsid = -1;
}

extern void ExecuteServer(void) {
  int _fhTerm, _fhAps, _fhControl, _fhSysData;
  fd_set ready, fds;
  int maxfd;
  int ret;
  struct timespec timeout;

  _fhTerm = InitServerPort(WfcPort, Back);
  maxfd = _fhTerm;
  _fhAps = InitServerPort(ApsPort, Back);
  maxfd = maxfd < _fhAps ? _fhAps : maxfd;
  if (ControlPort != NULL) {
    _fhControl = InitServerPort(ControlPort, Back);
    maxfd = maxfd < _fhControl ? _fhControl : maxfd;
  } else {
    _fhControl = -1;
  }
  if ((ThisEnv->sysdata != NULL) && (ThisEnv->sysdata->port != NULL)) {
    _fhSysData = InitServerPort(ThisEnv->sysdata->port, Back);
    maxfd = maxfd < _fhSysData ? _fhSysData : maxfd;
  } else {
    _fhSysData = -1;
  }

  timeout.tv_sec = 1;
  timeout.tv_nsec = 0;
  FD_ZERO(&ready);
  FD_SET(_fhTerm, &ready);
  FD_SET(_fhAps, &ready);
  if (_fhControl >= 0) {
    FD_SET(_fhControl, &ready);
  }
  if (_fhSysData >= 0) {
    FD_SET(_fhSysData, &ready);
  }
  while (!fShutdown) {
    memcpy(&fds, &ready, sizeof(fd_set));
    ret = pselect(maxfd + 1, &fds, NULL, NULL, &timeout, &SigMask);
    if (ret == -1) {
      if (errno == EINTR)
        continue;
      Error("select: %s", strerror(errno));
    }
    if (FD_ISSET(_fhTerm, &fds)) { /*	term connect	*/
      ConnectTerm(_fhTerm);
    }
    if (FD_ISSET(_fhAps, &fds)) { /*	APS connect	*/
      ConnectAPS(_fhAps);
    }
    if ((_fhControl >= 0) &&
        (FD_ISSET(_fhControl, &fds))) { /*	control connect		*/
      ConnectControl(_fhControl);
    }
    if ((_fhSysData >= 0) &&
        (FD_ISSET(_fhSysData, &fds))) { /*	sysdata connect		*/
      ConnectSysData(_fhSysData);
    }
  }
}

static void InitSystem(void) {
  fNumericHOST = TRUE;
  fShutdown = FALSE;
  InitDirectory();
  SetUpDirectory(Directory, NULL, "", "", P_LD);
  if (ThisEnv == NULL) {
    Error("DI file parse error.");
  }
  if (ThisEnv->InitialLD == NULL) {
    ThisEnv->InitialLD = StrDup(DEFAULT_LD);
  }
  if (ApsPortNumber == NULL) {
    ApsPort = ThisEnv->WfcApsPort;
  } else {
    ApsPort = ParPortName(ApsPortNumber);
  }
  if (PortNumber == NULL) {
    WfcPort = ThisEnv->TermPort;
  } else {
    WfcPort = ParPortName(PortNumber);
  }
  if (ControlPortNumber == NULL) {
    ControlPort = ThisEnv->ControlPort;
  } else {
    ControlPort = ParPortName(ControlPortNumber);
  }
  InitNET();
  InitSysData();
  InitMessageQueue();
  ReadyAPS();
  SetupMessageQueue();
  InitTerm();
  StartCoreThread();
  InitSessionCtrl();
  InitControl();
}

static void CleanUp(void) {
  CleanUNIX_Socket(ApsPort);
  CleanUNIX_Socket(WfcPort);
  CleanUNIX_Socket(ControlPort);
}

static void StopSystem(void) {
  fShutdown = TRUE;
  Message("receive stop signal");
  exit(0);
}

static ARG_TABLE option[] = {
    {"port", STRING, TRUE, (void *)&PortNumber, "waiting port name"},
    {"apsport", STRING, TRUE, (void *)&ApsPortNumber, "APS port name"},
    {"control", STRING, TRUE, (void *)&ControlPortNumber, "control port name"},
    {"back", INTEGER, TRUE, (void *)&Back, "connection waiting queue number"},

    {"base", STRING, TRUE, (void *)&BaseDir, "base directory"},
    {"record", STRING, TRUE, (void *)&RecordDir, "record directory"},
    {"ddir", STRING, TRUE, (void *)&D_Dir, "LD directory"},
    {"dir", STRING, TRUE, (void *)&Directory, "environment file name"},
    {"retry", INTEGER, TRUE, (void *)&MaxTransactionRetry,
     "maximun retry count"},
    {"sesnum", INTEGER, TRUE, (void *)&SesNum, "terminal cache number"},

    {NULL, 0, TRUE, NULL}};

static void SetDefault(void) {
  PortNumber = NULL;
  ApsPortNumber = NULL;
  Back = 5;
  BaseDir = NULL;
  RecordDir = NULL;
  D_Dir = NULL;
  Directory = "./directory";
  TempDirRoot = getenv("MCP_TEMPDIR_ROOT");
  if (TempDirRoot == NULL) {
    TempDirRoot = "/tmp/panda_root/";
  }
  ApiTempDirRoot = g_strdup_printf("%s/api/",TempDirRoot);
  MaxTransactionRetry = 0;
  ControlPort = NULL;
  SesNum = 0;
}

extern int main(int argc, char **argv) {
  int rc;
  sigset_t sigmask;
  struct sigaction sa;
  char *stdout_path, *stderr_path;

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &sigmask, &SigMask);

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags |= SA_RESTART;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGPIPE, &sa, NULL) != 0) {
    Error("sigaction(2) failure");
  }

  sa.sa_handler = (void *)StopSystem;
  if (sigaction(SIGHUP, &sa, NULL) != 0) {
    Error("sigaction(2) failure");
  }

  sa.sa_handler = SIG_IGN;
  if (sigaction(SIGUSR1, &sa, NULL) != 0) {
    Error("sigaction(2) failure");
  }

  sa.sa_handler = SIG_IGN;
  if (sigaction(SIGUSR2, &sa, NULL) != 0) {
    Error("sigaction(2) failure");
  }

  /* 親プロセス(monitor)停止で自動停止 */
  prctl(PR_SET_PDEATHSIG, SIGHUP);

  stdout_path = getenv("WFC_DEBUG_STDOUT_PATH");
  if (stdout_path != NULL) {
    freopen(stdout_path, "w", stdout);
  }

  stderr_path = getenv("WFC_DEBUG_STDERR_PATH");
  if (stderr_path != NULL) {
    freopen(stderr_path, "w", stderr);
  }

  InitMessage("wfc", NULL);

  SetDefault();
  GetOption(option, argc, argv, NULL);

  InitSystem();
  MessageLogPrintf("wfc start %s %s", PACKAGE_VERSION, PACKAGE_DATE);
  ExecuteServer();
  Message("wfc stop");
  CleanUp();
  rc = 0;
  return (rc);
}
