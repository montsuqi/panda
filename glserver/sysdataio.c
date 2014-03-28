/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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
#include	<unistd.h>

#include	"const.h"

#include	"glserver.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"comm.h"
#include	"comms.h"
#include	"socket.h"
#include	"wfcdata.h"
#include	"sysdatacom.h"
#include	"blobreq.h"
#include	"sysdbreq.h"
#include	"sysdataio.h"
#include	"enum.h"
#include	"net.h"
#include	"message.h"
#include	"debug.h"

static NETFILE *
ConnectSysData()
{
	NETFILE *fp;
	Port	*port;
	int		fd;
ENTER_FUNC;
	fp = NULL;
	port = ParPort(PortSysData, SYSDATA_PORT);
	fd = ConnectSocket(port, SOCK_STREAM);
	DestroyPort(port);
	if (fd > 0) {
		fp = SocketToNet(fd);
	} else {
		Error("cannot connect sysdata server");
	}
LEAVE_FUNC;
	return fp;
}

static void
DisconnectSysData(NETFILE *fp)
{
	if (fp != NULL && CheckNetFile(fp)) {
		CloseNet(fp);
	}
}

void
GLExportBLOB(
	MonObjectType	obj,
	char			**out,
	size_t			*size)
{
	NETFILE *fp;

	fp = ConnectSysData();
	RequestExportBLOBMem(fp,obj,out,size);
	DisconnectSysData(fp);
}

MonObjectType
GLImportBLOB(
	char	*in,
	size_t	size)
{
	NETFILE *fp;
	MonObjectType ret;

	fp = ConnectSysData();
	ret = RequestImportBLOBMem(fp,in,size);
	DisconnectSysData(fp);
	return ret;
}

static ValueStruct *val = NULL;

extern	gboolean
CheckSession(
	const char *term)
{
	ValueStruct *v;
	PacketClass rc;
	NETFILE *fp;
	gboolean ret;
ENTER_FUNC;
	ret = FALSE;
	fp = ConnectSysData();
	if (fp != NULL && CheckNetFile(fp)) {
		if (val == NULL) {
			val = RecParseValueMem(SYSDBVAL_DEF,NULL);
		}
		InitializeValue(val);
		v = GetRecordItem(val,"id");
		SetValueString(v,term,NULL);
		rc = SYSDB_GetMessage(fp,val);
		ret = rc == SESSION_CONTROL_OK;
	} else {
		Error("GetSessionMessage failure");
	}
	DisconnectSysData(fp);
LEAVE_FUNC;
	return ret;
}

extern	void
GetSessionMessage(
	const char *term,
	char **popup,
	char **dialog,
	char **abort)
{
	ValueStruct *v;
	PacketClass rc;
	NETFILE *fp;
ENTER_FUNC;
	fp = ConnectSysData();
	if (fp != NULL && CheckNetFile(fp)) {
		if (val == NULL) {
			val = RecParseValueMem(SYSDBVAL_DEF,NULL);
		}
		InitializeValue(val);
		v = GetRecordItem(val,"id");
		SetValueString(v,term,NULL);
		rc = SYSDB_GetMessage(fp,val);
		if (rc != SESSION_CONTROL_OK) {
			Error("GetSessionMessage failure");
		}
		v = GetRecordItem(val,"popup");
		*popup = g_strdup(ValueToString(v,NULL));
		v = GetRecordItem(val,"dialog");
		*dialog = g_strdup(ValueToString(v,NULL));
		v = GetRecordItem(val,"abort");
		*abort = g_strdup(ValueToString(v,NULL));
	} else {
		Error("GetSessionMessage failure");
	}
	DisconnectSysData(fp);
LEAVE_FUNC;
}

extern	void
ResetSessionMessage(
	const char *term)
{
	ValueStruct *v;
	PacketClass rc;
	NETFILE *fp;
ENTER_FUNC;
	fp = ConnectSysData();
	if (fp != NULL && CheckNetFile(fp)) {
		if (val == NULL) {
			val = RecParseValueMem(SYSDBVAL_DEF,NULL);
		}
		InitializeValue(val);
		v = GetRecordItem(val,"id");
		SetValueString(v,term,NULL);
		rc = SYSDB_ResetMessage(fp,val);
		if (rc != SESSION_CONTROL_OK) {
			Error("ResetSessionMessage failure");
		}
	} else {
		Error("ResetSessionMessage failure");
	}
	DisconnectSysData(fp);
LEAVE_FUNC;
}
