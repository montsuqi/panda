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
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>

#include	"types.h"

#include	"libmondai.h"
#include	"enum.h"
#include	"net.h"
#include	"comm.h"
#include	"keyvalue.h"
#include	"keyvaluecom.h"
#include	"keyvaluereq.h"
#include	"message.h"
#include	"debug.h"

extern	int
RequestKV(
	NETFILE	*fp,
	PacketClass	op,
	ValueStruct *args)
{
	LargeByteString *buff;
	int rc;
ENTER_FUNC;
	rc = MCP_BAD_OTHER;
	SendPacketClass(fp, TYPE_KV);	ON_IO_ERROR(fp,badio);
	SendPacketClass(fp,op);			ON_IO_ERROR(fp,badio);
	buff = NewLBS();
	LBS_ReserveSize(buff,NativeSizeValue(NULL,args),FALSE);
	NativePackValue(NULL, LBS_Body(buff), args);
	SendLBS(fp, buff);				ON_IO_ERROR(fp,badio);

	rc = RecvInt(fp);				ON_IO_ERROR(fp,badio);
	if (rc == MCP_OK) {
		RecvLBS(fp, buff);			ON_IO_ERROR(fp,badio);
		NativeUnPackValue(NULL,LBS_Body(buff), args);
		FreeLBS(buff);
	}
  badio:
LEAVE_FUNC;
	return	(rc);
}
