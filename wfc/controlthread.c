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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <glib.h>
#include <pthread.h>
#include <errno.h>

#include "enum.h"

#include "libmondai.h"
#include "socket.h"
#include "net.h"
#include "comm.h"
#include "comms.h"
#include "queue.h"
#include "directory.h"
#include "wfcdata.h"
#include "wfc.h"
#include "termthread.h"
#include "corethread.h"
#include "controlthread.h"
#include "message.h"
#include "debug.h"

static Bool InitSession(NETFILE *fp) {
  Bool ret;
  char name[SIZE_LONGNAME + 1];

  RecvStringDelim(fp, SIZE_LONGNAME, name);
  if (strcmp(name, ThisEnv->name) == 0) {
    SendPacketClass(fp, WFCCONTROL_OK);
    ret = TRUE;
  } else {
    SendPacketClass(fp, WFCCONTROL_NG);
    ret = FALSE;
  }
  MessageLog("control session start");
  return (ret);
}

static void FinishSession(void) {
  MessageLog("control session end");
}

static void ControlThread(void *para) {
  NETFILE *fp;
  Bool fExit;

  fp = SocketToNet((int)(long)para);

  if (InitSession(fp)) {
    fExit = FALSE;
    while (!fExit) {
      switch (RecvPacketClass(fp)) {
      case WFCCONTROL_STOP:
        fShutdown = TRUE;
        fExit = TRUE;
        break;
      case WFCCONTROL_END:
        fExit = TRUE;
        break;
      default:
        break;
      }
      ON_IO_ERROR(fp, badio);
    }
  }
badio:
  FinishSession();
  CloseNet(fp);
  pthread_exit(NULL);
}

extern pthread_t ConnectControl(int _fhControl) {
  int fhControl;
  pthread_t thr;

  if ((fhControl = accept(_fhControl, 0, 0)) < 0) {
    Error("accept(2) failure:%s", strerror(errno));
  }
  pthread_create(&thr, NULL, (void *(*)(void *))ControlThread,
                 (void *)(long)fhControl);
  pthread_detach(thr);
  return (thr);
}

extern void InitControl(void) {
}
