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
#include	"front.h"
#include	"socket.h"
#include	"wfcdata.h"
#include	"sysdatacom.h"
#include	"blobreq.h"
#include	"sysdbreq.h"
#include	"sysdataio.h"
#include	"enum.h"
#include	"message.h"
#include	"debug.h"

static	NETFILE	*fp;

void
ConnectSysData()
{
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
}

void
DisconnectSysData()
{
	if (fp != NULL && CheckNetFile(fp)) {
		CloseNet(fp);
	}
}

static	void
_AccessBLOB(
	NETFILE		*fp,
	int			mode,
	ValueStruct	*value)
{
	int		i;

ENTER_FUNC;
	if		(  value  ==  NULL  )	return;
	if		(  IS_VALUE_NIL(value)  )	return;
	switch	(ValueType(value)) {
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			_AccessBLOB(fp, mode, ValueArrayItem(value,i));
		}
		break;
	  case	GL_TYPE_VALUES:
		for	( i = 0 ; i < ValueValuesSize(value) ; i ++ ) {
			_AccessBLOB(fp, mode, ValueValuesItem(value,i));
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			_AccessBLOB(fp, mode, ValueRecordItem(value,i));
		}
		break;
	  case	GL_TYPE_INT:
	  case	GL_TYPE_FLOAT:
	  case	GL_TYPE_BOOL:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_NUMBER:
		break;
	  case	GL_TYPE_OBJECT:
		switch(mode) {
		case BLOB_ACCESS_IMPORT:
			if (IS_OBJECT_NULL(ValueObjectId(value))) {
				if (BlobCacheFileSize(value) > 0) {
					ValueObjectId(value) = RequestImportBLOB(fp,BlobCacheFileName(value));
				}
			}
			break;
		case BLOB_ACCESS_EXPORT:
			if (!IS_OBJECT_NULL(ValueObjectId(value))) {
				RequestExportBLOB(fp,ValueObjectId(value),BlobCacheFileName(value));
			}
			break;
		}
		break;
	  case	GL_TYPE_ALIAS:
	  default:
		break;
	}
LEAVE_FUNC;
}


extern	void
AccessBLOB(
	int			mode,
	ValueStruct	*value)
{
ENTER_FUNC;
	if (fp != NULL && CheckNetFile(fp)) {
		_AccessBLOB(fp,mode,value);
	} else {
		Error("AccessBLOB failure");
	}
LEAVE_FUNC;
}

static ValueStruct *val = NULL;

extern	void
GetSessionMessage(
	const char *term,
	char **popup,
	char **dialog,
	char **abort)
{
	ValueStruct *v;
	PacketClass rc;
ENTER_FUNC;
	if (fp != NULL && CheckNetFile(fp)) {
		if (val == NULL) {
			val = RecParseValueMem(SYSDBVAL_DEF,NULL);
		}
		InitializeValue(val);
		v = GetRecordItem(val,"id");
		SetValueString(v,term,NULL);
		rc = SYSDB_GetMessage(fp,val);
		v = GetRecordItem(val,"popup");
		*popup = g_strdup(ValueToString(v,NULL));
		v = GetRecordItem(val,"dialog");
		*dialog = g_strdup(ValueToString(v,NULL));
		v = GetRecordItem(val,"abort");
		*abort = g_strdup(ValueToString(v,NULL));
	} else {
		Error("GetSessionMessage failure");
	}
LEAVE_FUNC;
}

extern	void
ResetSessionMessage(
	const char *term)
{
	ValueStruct *v;
	PacketClass rc;
ENTER_FUNC;
	if (fp != NULL && CheckNetFile(fp)) {
		if (val == NULL) {
			val = RecParseValueMem(SYSDBVAL_DEF,NULL);
		}
		InitializeValue(val);
		v = GetRecordItem(val,"id");
		SetValueString(v,term,NULL);
		rc = SYSDB_ResetMessage(fp,val);
	} else {
		Error("ResetSessionMessage failure");
	}
LEAVE_FUNC;
}
