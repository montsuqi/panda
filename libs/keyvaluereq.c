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
#include	<libmondai.h>
#include	<RecParser.h>
#include	"enum.h"
#include	"net.h"
#include	"comm.h"
#include	"keyvalue.h"
#include	"keyvaluecom.h"
#include	"keyvaluereq.h"
#include	"message.h"
#include	"debug.h"

extern	int
KVREQ_Request(
	NETFILE	*fp,
	PacketClass	op,
	ValueStruct *args)
{
	LargeByteString *buff;
	int rc;
ENTER_FUNC;
	rc = MCP_BAD_OTHER;
	SendPacketClass(fp, SYSDATA_KV);	ON_IO_ERROR(fp,badio);
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

extern	ValueStruct *
KVREQ_NewQuery(
	int num)
{
	static Bool fInit = TRUE;
	ValueStruct *ret;
	char buff[512];
	const char *str = ""
"dummy	{"
	"id		char(%d);"
	"num	int;"
	"query	{"
		"key	char(%d);"
		"value	char(%d);"
	"}[%d];"
"};";
ENTER_FUNC;
	if (fInit) {
		RecParserInit();
		fInit = FALSE;
	}
	sprintf(buff, str, KVREQ_TEXT_SIZE, KVREQ_TEXT_SIZE, KVREQ_TEXT_SIZE, num);
	ret = RecParseValueMem(buff, NULL);
	InitializeValue(ret);
LEAVE_FUNC;
	return ret;
}

extern	ValueStruct	*
KVREQ_NewQueryWithValue(
	char *id, 
	int num, 
	char **keys, 
	char **values)
{
	ValueStruct *ret;
	ValueStruct *value;
	char lname[256];
	int i;
ENTER_FUNC;
	if (num <= 0) {
		return NULL;
	}
	ret = KVREQ_NewQuery(num);
	value = GetItemLongName(ret, "id");
	if (id != NULL) {
		SetValueStringWithLength(value, id, strlen(id), NULL);
	} else {
		SetValueStringWithLength(value, "", strlen(""), NULL);
	}
	value = GetItemLongName(ret, "num");
	ValueInteger(value) = num;

	for (i = 0; i < num ; i++) {
		sprintf(lname, "query[%d].key", i);
		value = GetItemLongName(ret, lname);
		if (keys[i] != NULL) {
			SetValueStringWithLength(value, keys[i], strlen(keys[i]), NULL);
		} else {
			SetValueStringWithLength(value, "", strlen(""), NULL);
		}
		sprintf(lname, "query[%d].value", i);
		value = GetItemLongName(ret, lname);
		if (values[i] != NULL) {
			SetValueStringWithLength(value, values[i], strlen(values[i]), NULL);
		} else {
			SetValueStringWithLength(value, "", strlen(""), NULL);
		}
	}
#ifdef TRACE
	DumpValueStruct(ret);
#endif
LEAVE_FUNC;
	return ret;
}

extern	int
KVREQ_GetNum(
	ValueStruct *query)
{
	ValueStruct *value;
ENTER_FUNC;
	value = GetItemLongName(query, "num");
	if (value == NULL) {
		Warning("cannot get num");
		return 0;
	}
LEAVE_FUNC;
	return ValueInteger(value);
}

extern	char *
KVREQ_GetKey(
	ValueStruct *query,
	int num)
{
	char lname[256];
	ValueStruct *value;
ENTER_FUNC;
	sprintf(lname, "query[%d].key", num);
	value = GetItemLongName(query, lname);
	if (value == NULL) {
		Warning("cannot get query[%d].key", num);
		return "";
	}
LEAVE_FUNC;
	return ValueToString(value, NULL);
}

extern	char *
KVREQ_GetValue(
	ValueStruct *query,
	int num)
{
	char lname[256];
	ValueStruct *value;
ENTER_FUNC;
	sprintf(lname, "query[%d].value", num);
	value = GetItemLongName(query, lname);
	if (value == NULL) {
		Warning("cannot get query[%d].value", num);
		return "";
	}
LEAVE_FUNC;
	return ValueToString(value, NULL);
}
