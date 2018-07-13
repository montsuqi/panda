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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <glib.h>
#include <math.h>

#include "libmondai.h"
#define _COMM
#include "comm.h"
#include "debug.h"

extern void SendPacketClass(NETFILE *fp, PacketClass c) {
  nputc(c, fp);
  Flush(fp);
}

extern PacketClass RecvPacketClass(NETFILE *fp) {
  int tc;
  PacketClass c = (PacketClass)0x00;

  tc = ngetc(fp);
  if (tc >= 0) {
    c = tc;
  } else {
    Message("error RecvPacket");
  }
  return (c);
}

extern void SendLength(NETFILE *fp, size_t size) {
  Send(fp, &size, sizeof(size));
}

extern size_t RecvLength(NETFILE *fp) {
  size_t size;

  if (Recv(fp, &size, sizeof(size)) < 0) {
    size = 0;
  }
  return (size);
}

extern void SendString(NETFILE *fp, char *str) {
  size_t size;

  ENTER_FUNC;
  if (str != NULL) {
    size = strlen(str);
  } else {
    size = 0;
  }
  SendLength(fp, size);
  if (size > 0) {
    Send(fp, str, size);
  }
  LEAVE_FUNC;
}

extern void RecvString(NETFILE *fp, char *str) {
  size_t size;

  ENTER_FUNC;
  size = RecvLength(fp);
  Recv(fp, str, size);
  str[size] = 0;
  LEAVE_FUNC;
}

extern void RecvnString(NETFILE *fp, size_t size, char *str) {
  size_t lsize;
  ENTER_FUNC;
  lsize = RecvLength(fp);
  if (size >= lsize) {
    size = lsize;
    Recv(fp, str, size);
    str[size] = 0;
  } else {
    CloseNet(fp);
    Message("error size mismatch !");
  }
  LEAVE_FUNC;
}

extern char *RecvStringNew(NETFILE *fp) {
  size_t size;
  char *ret;
  size = RecvLength(fp);
  ret = xmalloc(size + 1);
  Recv(fp, ret, size);
  ret[size] = 0;
  return ret;
}

extern void SendLBS(NETFILE *fp, LargeByteString *lbs) {
  SendLength(fp, LBS_Size(lbs));
  if (LBS_Size(lbs) > 0) {
    Send(fp, LBS_Body(lbs), LBS_Size(lbs));
  }
}

extern void RecvLBS(NETFILE *fp, LargeByteString *lbs) {
  size_t size;

  size = RecvLength(fp);
  LBS_ReserveSize(lbs, size, FALSE);
  if (size > 0) {
    Recv(fp, LBS_Body(lbs), size);
  }
}

extern int RecvInt(NETFILE *fp) {
  int data;

  Recv(fp, &data, sizeof(data));
  return (data);
}

extern void SendInt(NETFILE *fp, int data) { Send(fp, &data, sizeof(data)); }

extern unsigned int RecvUInt(NETFILE *fp) {
  unsigned int data;

  Recv(fp, &data, sizeof(data));
  return (data);
}

extern void SendUInt(NETFILE *fp, unsigned int data) {
  Send(fp, &data, sizeof(data));
}

extern uint64_t RecvUInt64(NETFILE *fp) {
  uint64_t data;

  Recv(fp, &data, sizeof(data));
  return (data);
}

extern void SendUInt64(NETFILE *fp, uint64_t data) {
  Send(fp, &data, sizeof(data));
}

extern int RecvChar(NETFILE *fp) {
  unsigned char data;
  int ret;

  if (Recv(fp, &data, sizeof(data)) == sizeof(data)) {
    ret = data;
  } else {
    ret = -1;
  }
  return (ret);
}

extern void SendChar(NETFILE *fp, int data) {
  unsigned char buf;

  buf = (unsigned char)data;
  Send(fp, &buf, sizeof(buf));
}

extern Bool RecvBool(NETFILE *fp) {
  char buf[1];

  Recv(fp, buf, 1);
  return ((buf[0] == 'T') ? TRUE : FALSE);
}

extern void SendBool(NETFILE *fp, Bool data) {
  char buf[1];

  buf[0] = data ? 'T' : 'F';
  Send(fp, buf, 1);
}

extern void InitComm(void) {}
