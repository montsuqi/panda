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
#include    <sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>

#include	"types.h"

#include	"libmondai.h"
#include	"enum.h"
#include	"net.h"
#include	"comm.h"
#include	"keyvalue.h"
#include	"keyvaluecom.h"
#include	"kvserv.h"
#include	"message.h"
#include	"debug.h"

extern	void
PassiveKV(
	NETFILE		*fp,
	KV_State	*state)
{
	LargeByteString *buff;
	ValueStruct *args;
	PacketClass c;
	int rc;
	size_t size;
ENTER_FUNC;
	rc = MCP_BAD_FUNC;
	c = RecvPacketClass(fp); 	ON_IO_ERROR(fp,badio);
	buff = NewLBS();
	RecvLBS(fp, buff);			ON_IO_ERROR(fp,badio);
	NativeUnPackValueNew(NULL, LBS_Body(buff), &args);
	if (args == NULL) {
		rc = MCP_BAD_ARG;
	} else {
		switch	(c) {
		case KV_GETVALUE:
			dbgmsg("GETVALUE");
			rc = KV_GetValue(state, args);
			break;
		case KV_SETVALUE:
			dbgmsg("SETVALUE");
			rc = KV_SetValue(state, args);
			break;
		case KV_SETVALUEALL:
			dbgmsg("SETVALUEALL");
			rc = KV_SetValueALL(state, args);
			break;
		case KV_LISTKEY:
			dbgmsg("LISTKEY");
			rc = KV_ListKey(state, args);
			break;
		case KV_LISTENTRY:
			dbgmsg("LISTENTRY");
			rc = KV_ListEntry(state, args);
			break;
		case KV_NEWENTRY:
			dbgmsg("NEWENTRY");
			rc = KV_NewEntry(state, args);
			break;
		case KV_DELETEENTRY:
			dbgmsg("DELETEENTRY");
			rc = KV_DeleteEntry(state, args);
			break;
		case KV_DUMP:
			dbgmsg("DUMP");
			rc = KV_Dump(state, args);
			break;
		default:
			Warning("invalid packet class[%d]", c);
			break;
		}
	}
	SendInt(fp, rc);		ON_IO_ERROR(fp,badio);
	if (rc == MCP_OK) {
		size = NativeSizeValue(NULL,args);
		LBS_ReserveSize(buff,size,FALSE);
		NativePackValue(NULL, LBS_Body(buff), args);
		SendLBS(fp, buff);		ON_IO_ERROR(fp,badio);
	}
	FreeLBS(buff);
	if (args != NULL) {
		FreeValueStruct(args);
	}
badio:
LEAVE_FUNC;
}
