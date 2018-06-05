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
#include <libmondai.h>

#include "enum.h"

#include "socket.h"
#include "net.h"
#include "comm.h"
#include "comms.h"
#include "directory.h"
#include "wfcdata.h"
#include "wfc.h"
#include "sysdatacom.h"
#include "blobserv.h"
#include "sysdbserv.h"
#include "message.h"
#include "debug.h"
#include "sysdatathread.h"

static void SysDataThread(void *para) {
  NETFILE *fp = para;
  PacketClass c;
  Bool fLoop;

  ENTER_FUNC;
  fLoop = TRUE;
  while (fLoop) {
    c = RecvChar(fp);
    ON_IO_ERROR(fp, badio);
    switch (c) {
    case SYSDATA_BLOB:
      dbgmsg("Call ServeBLOB");
      ServeBLOB(fp);
      ON_IO_ERROR(fp, badio);
      break;
    case SYSDATA_SYSDB:
      dbgmsg("Call ServeSysDB");
      ServeSysDB(fp);
      ON_IO_ERROR(fp, badio);
      break;
    case SYSDATA_END:
      dbgmsg("SYSDATA_END");
      fLoop = FALSE;
      break;
    default:
      dbgprintf("invalid packet[%d]", (int)c);
      fLoop = FALSE;
      break;
    }
  }
badio:
  CloseNet(fp);
  LEAVE_FUNC;
}

extern pthread_t ConnectSysData(int _fhSysData) {
  int fhSysData;
  pthread_t thr;
  pthread_attr_t attr;
  NETFILE *fp;
  ENTER_FUNC;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 256 * 1024);
  if ((fhSysData = accept(_fhSysData, 0, 0)) < 0) {
    Error("accept(2) failure:%s", strerror(errno));
  }
  fp = SocketToNet(fhSysData);
  pthread_create(&thr, NULL, (void *(*)(void *))SysDataThread, (void *)fp);
  pthread_detach(thr);
  LEAVE_FUNC;
  return (thr);
}

extern void InitSysData(void) {
  ENTER_FUNC;
  InitServeBLOB();
  LEAVE_FUNC;
}
