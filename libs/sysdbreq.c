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
#include "wfcdata.h"
#include "sysdbreq.h"
#include "message.h"
#include "debug.h"

static PacketClass SYSDB_RequestCommon(NETFILE *fp, PacketClass pc,
                                       ValueStruct *val) {
  PacketClass rc;
  LargeByteString *buf;
  ENTER_FUNC;
  rc = SESSION_CONTROL_NG;
  buf = NewLBS();
  LBS_ReserveSize(buf, NativeSizeValue(NULL, val), FALSE);
  NativePackValue(NULL, LBS_Body(buf), val);
  SendPacketClass(fp, SYSDATA_SYSDB);
  ON_IO_ERROR(fp, badio);
  SendPacketClass(fp, pc);
  ON_IO_ERROR(fp, badio);
  SendLBS(fp, buf);
  ON_IO_ERROR(fp, badio);
  rc = RecvPacketClass(fp);
  RecvLBS(fp, buf);
  NativeUnPackValue(NULL, LBS_Body(buf), val);
badio:
  FreeLBS(buf);
  LEAVE_FUNC;
  return rc;
}

PacketClass SYSDB_GetData(NETFILE *fp, ValueStruct *val) {
  return SYSDB_RequestCommon(fp, SYSDB_GET_DATA, val);
}

PacketClass SYSDB_GetMessage(NETFILE *fp, ValueStruct *val) {
  return SYSDB_RequestCommon(fp, SYSDB_GET_MESSAGE, val);
}

PacketClass SYSDB_ResetMessage(NETFILE *fp, ValueStruct *val) {
  return SYSDB_RequestCommon(fp, SYSDB_RESET_MESSAGE, val);
}

PacketClass SYSDB_SetMessage(NETFILE *fp, ValueStruct *val) {
  return SYSDB_RequestCommon(fp, SYSDB_SET_MESSAGE, val);
}

PacketClass SYSDB_SetMessageAll(NETFILE *fp, ValueStruct *val) {
  return SYSDB_RequestCommon(fp, SYSDB_SET_MESSAGE_ALL, val);
}

PacketClass SYSDB_GetDataAll(NETFILE *fp, int *size, ValueStruct *vals) {
  PacketClass rc;
  LargeByteString *buf;
  rc = SESSION_CONTROL_NG;
  buf = NewLBS();
  SendPacketClass(fp, SYSDATA_SYSDB);
  ON_IO_ERROR(fp, badio);
  SendPacketClass(fp, SYSDB_GET_DATA_ALL);
  ON_IO_ERROR(fp, badio);
  rc = RecvPacketClass(fp);
  *size = RecvInt(fp);
  RecvLBS(fp, buf);
  NativeUnPackValue(NULL, LBS_Body(buf), vals);
badio:
  FreeLBS(buf);
  return rc;
}
