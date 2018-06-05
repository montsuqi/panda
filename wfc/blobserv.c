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
#include <sys/stat.h>
#include <glib.h>
#include <pthread.h>

#include "libmondai.h"
#include "net.h"
#include "comm.h"
#include "enum.h"
#include "sysdatacom.h"
#include "blobserv.h"
#include "dbgroup.h"
#include "bytea.h"
#include "dbutils.h"
#include "monsys.h"
#include "debug.h"

#define BLOBEXPIRE 2

static DBG_Struct *dbg;
static pthread_mutex_t lock;

extern void InitServeBLOB() {
  dbg = GetDBG_monsys();
  dbg->dbt = NewNameHash();
  if (OpenDB(dbg) != MCP_OK) {
    exit(1);
  }
  pthread_mutex_init(&lock, NULL);
}

static void BLOBEXPORT(NETFILE *fp) {
  MonObjectType obj;
  ssize_t size;
  char *buff;

  dbgmsg("BLOB_EXPORT");
  TransactionStart(dbg);
  obj = RecvObject(fp);
  ON_IO_ERROR(fp, badio);
  if (blob_export_mem(dbg, obj, &buff, &size)) {
    SendPacketClass(fp, BLOB_OK);
    ON_IO_ERROR(fp, badio);
    SendLength(fp, size);
    ON_IO_ERROR(fp, badio);
    Send(fp, buff, size);
    xfree(buff);
  } else {
    SendPacketClass(fp, BLOB_NOT);
    ON_IO_ERROR(fp, badio);
  }
badio:
  TransactionEnd(dbg);
  return;
}

static void BLOBIMPORT(NETFILE *fp) {
  MonObjectType obj;
  ssize_t ssize;
  unsigned char *buff;

  dbgmsg("BLOB_IMPORT");
  TransactionStart(dbg);
  ssize = RecvLength(fp);
  ON_IO_ERROR(fp, badio);
  buff = xmalloc(ssize);
  Recv(fp, buff, ssize);
  ON_IO_ERROR(fp, badio);
  obj = blob_import_mem(dbg, 0, "blobserv.bin", NULL, 0, buff, ssize);
  xfree(buff);
  SendObject(fp, obj);
  ON_IO_ERROR(fp, badio);
badio:
  TransactionEnd(dbg);
  return;
}

extern void ServeBLOB(NETFILE *fp) {
  ENTER_FUNC;
  pthread_mutex_lock(&lock);
  switch (RecvPacketClass(fp)) {
  case BLOB_EXPORT:
    BLOBEXPORT(fp);
    break;
  case BLOB_IMPORT:
    BLOBIMPORT(fp);
    break;
  default:
    break;
  }
  pthread_mutex_unlock(&lock);
  LEAVE_FUNC;
}
