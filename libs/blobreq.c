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

#include "libmondai.h"
#include "net.h"
#include "comm.h"
#include "blobreq.h"
#include "message.h"
#include "debug.h"

static Bool RequestBLOB(NETFILE *fp, PacketClass op) {
  Bool rc;

  rc = FALSE;
  SendPacketClass(fp, SYSDATA_BLOB);
  ON_IO_ERROR(fp, badio);
  SendPacketClass(fp, op);
  ON_IO_ERROR(fp, badio);
  rc = TRUE;
badio:
  return (rc);
}

extern Bool RequestExportBLOB(NETFILE *fp, MonObjectType obj, char *fname) {
  char *buff;
  size_t size;
  if (RequestExportBLOBMem(fp, obj, &buff, &size)) {
    if (buff != NULL) {
      return g_file_set_contents(fname, buff, size, NULL);
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }
  return FALSE;
}

extern Bool RequestExportBLOBMem(NETFILE *fp, MonObjectType obj, char **out,
                                 size_t *size) {
  char *p;

  p = NULL;
  *out = NULL;
  RequestBLOB(fp, BLOB_EXPORT);
  ON_IO_ERROR(fp, badio);
  SendObject(fp, obj);
  ON_IO_ERROR(fp, badio);
  if (RecvPacketClass(fp) == BLOB_OK) {
    *size = RecvLength(fp);
    p = xmalloc(*size);
    Recv(fp, p, *size);
    ON_IO_ERROR(fp, badio);
    *out = p;
  }
  return (p != NULL);
badio:
  if (p != NULL) {
    xfree(p);
  }
  return FALSE;
}

extern MonObjectType RequestImportBLOB(NETFILE *fp, const char *fname) {
  MonObjectType obj;
  char *buff;
  obj = GL_OBJ_NULL;
  size_t size;
  if (g_file_get_contents(fname, &buff, &size, NULL)) {
    obj = RequestImportBLOBMem(fp, buff, size);
  }
  return (obj);
}

extern MonObjectType RequestImportBLOBMem(NETFILE *fp, char *in, size_t size) {
  MonObjectType obj;

  obj = GL_OBJ_NULL;
  RequestBLOB(fp, BLOB_IMPORT);
  ON_IO_ERROR(fp, badio);
  SendLength(fp, size);
  Send(fp, in, size);
  ON_IO_ERROR(fp, badio);
  obj = RecvObject(fp);
  ON_IO_ERROR(fp, badio);
badio:
  return (obj);
}
