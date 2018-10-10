/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <glib.h>
#include <json.h>
#include <unistd.h>
#include <errno.h>

#include "const.h"
#include "net.h"
#include "defaults.h"
#include "socket.h"
#include "libmondai.h"
#include "wfcio.h"
#include "comm.h"
#include "debug.h"

static char *TermPort = NULL;

void WFCIO_SetTermPort(const char *port) { TermPort = StrDup(port); }

json_object *WFCIO_JSONRPC(json_object *req) {
  int fd;
  Port *port;
  NETFILE *fp;
  json_object *res;
  char *str;
  if (TermPort == NULL) {
    TermPort = getenv("WFC_TERM_PORT");
  }
  if (TermPort == NULL) {
    TermPort = "/tmp/wfc.term";
  }
  port = ParPort(TermPort, PORT_WFC);
  fd = ConnectSocket(port, SOCK_STREAM);
  DestroyPort(port);
  if (fd <= 0) {
    Error("could not connect wfc");
  }
  fp = SocketToNet(fd);
  SendString(
      fp, (char *)json_object_to_json_string_ext(req, JSON_C_TO_STRING_PLAIN));
  ON_IO_ERROR(fp, badio);
  str = RecvStringNew(fp);
  ON_IO_ERROR(fp, badio2);
  CloseNet(fp);
  res = json_tokener_parse(str);
  if (res == NULL || is_error(res)) {
    Error("WFCIO_JSONRPC invalid json object");
  }
  return res;
badio:
  Error("SendString error:%s", strerror(errno));
badio2:
  Error("RecvStringNew error:%s", strerror(errno));
}
