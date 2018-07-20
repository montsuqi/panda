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
#include <string.h>
#include <strings.h>
#include <glib.h>
#include <math.h>

#include "libmondai.h"
#include "comm.h"
#define _COMMS
#include "comms.h"
#include "debug.h"

extern void SendStringDelim(NETFILE *fp, char *str) {
  size_t size;

  if (str != NULL) {
    size = strlen(str);
  } else {
    size = 0;
  }
  if (size > 0) {
    Send(fp, str, size);
  }
}

extern void SendLargeString(NETFILE *fp, LargeByteString *lbs) {
  size_t size;
  char *str;

  ENTER_FUNC;
  str = LBS_Body(lbs);
  if (str != NULL) {
    size = strlen(str);
  } else {
    size = 0;
  }
  if (size > 0) {
    Send(fp, str, size);
  }
  LEAVE_FUNC;
}

extern Bool RecvStringDelim(NETFILE *fp, size_t size, char *str) {
  Bool rc;
  int c;
  char *p;

  p = str;
  while (((c = RecvChar(fp)) >= 0) && (c != '\n')) {
    *p++ = c;
    size--;
    if (size == 0)
      break;
  }
  *p = 0;
  if (c >= 0) {
    StringChop(str);
    rc = TRUE;
  } else {
    rc = FALSE;
  }
  return (rc);
}

extern Bool RecvLargeString(NETFILE *fp, LargeByteString *lbs) {
  Bool rc;
  int c;

  ENTER_FUNC;
  LBS_Clear(lbs);
  while (((c = RecvChar(fp)) >= 0) && (c != '\n')) {
    LBS_EmitChar(lbs, c);
  }
  LBS_EmitChar(lbs, 0);
  if (c >= 0) {
    rc = TRUE;
  } else {
    rc = FALSE;
  }
  LEAVE_FUNC;
  return (rc);
}
