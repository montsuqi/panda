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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>

#include "const.h"
#include "libmondai.h"
#include "term.h"
#include "message.h"
#include "debug.h"

extern char *RemoteIP(int sock, char *remote, size_t size) {
  socklen_t len;
  union sockaddr_s {
    struct sockaddr sa;
    struct sockaddr_in sa_in;
#ifdef USE_IPv6
    struct sockaddr_in6 sa_in6;
#endif
    struct sockaddr_storage sa_stor;
  } u;

  ENTER_FUNC;
  memclear(remote, size);
  if (sock == 0) {
    snprintf(remote, size, "something wrong;sock = 0");
  } else {
    len = sizeof(u);
    if ((getpeername(sock, &(u.sa), &len)) != 0) {
      Warning(("getpeername error: %s\n"), strerror(errno));
      snprintf(remote, size, "getpeername error");
      return remote;
    }
    switch (u.sa.sa_family) {
    case AF_UNIX:
      snprintf(remote, size, "AF_UNIX pid:%d", getpid());
      break;
    case AF_INET:
      snprintf(remote, size, "%s", inet_ntoa(u.sa_in.sin_addr));
      break;
#ifdef USE_IPv6
    case AF_INET6:
      inet_ntop(AF_INET6, &(u.sa_in6.sin6_addr), remote, size);
      break;
#endif
    default:
      break;
    }
  }
#ifdef DEBUG
  fprintf(STDERR, "remote ip = [%s]\n", remote);
#endif
  LEAVE_FUNC;
  return remote;
}
