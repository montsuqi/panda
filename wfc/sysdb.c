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
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>
#include	<time.h>

#include	"types.h"
#include	"enum.h"

#include	"libmondai.h"
#include	"lock.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"directory.h"
#include	"sysdb.h"
#include	"term.h"
#include	"keyvaluereq.h"
#include	"message.h"
#include	"debug.h"

static char *termkeys[SYSDB_TERM_LAST];

extern	void
SYSDB_Init()
{
	termkeys[SYSDB_TERM_USER] = "user";
	termkeys[SYSDB_TERM_HOST] = "host";
	termkeys[SYSDB_TERM_WINDOW] = "window";
	termkeys[SYSDB_TERM_WIDGET] = "widget";
	termkeys[SYSDB_TERM_EVENT] = "event";
	termkeys[SYSDB_TERM_CTIME] = "create_time";
	termkeys[SYSDB_TERM_ATIME] = "access_time";
	termkeys[SYSDB_TERM_PTIME] = "process_time";
	termkeys[SYSDB_TERM_TPTIME] = "total_process_time";
	termkeys[SYSDB_TERM_COUNT] = "count";
}

static	NETFILE *
ConnectSysData()
{
	int fd;
	NETFILE *fp;
ENTER_FUNC;
	fp = NULL;
	if (ThisEnv->sysdata != NULL && ThisEnv->sysdata->port != NULL) {
		if ((fd = ConnectSocket(ThisEnv->sysdata->port, SOCK_STREAM)) > 0) {
			fp = SocketToNet(fd);
		}
	}
	if (fp == NULL) {
		Warning("cannot connect sysdata thread");
	}
LEAVE_FUNC;
	return fp;
}

extern	ValueStruct	*
SYSDB_TERM_New(char *termid)
{
	ValueStruct *ret;
	NETFILE *fp;
	int i;
	char *keys[SYSDB_TERM_LAST];
	char *values[SYSDB_TERM_LAST];
ENTER_FUNC;
	ret = NULL;
	if ((fp = ConnectSysData()) != NULL) {
		for (i = 0; i < SYSDB_TERM_LAST; i++) {
			keys[i] = termkeys[i];
			if (i == SYSDB_TERM_HOST) {
				values[i] = TermToHost(termid);
			} else {
				values[i] = "";
			}
		}
		ret = KVREQ_NewQuery(termid,SYSDB_TERM_LAST,keys,values);
		KVREQ_Request(fp, KV_NEWENTRY, ret);
		SendPacketClass(fp, SYSDATA_END);
		CloseNet(fp);
	}
LEAVE_FUNC;
	return ret;
}

extern	void
SYSDB_TERM_Delete(
	ValueStruct *query)
{
	NETFILE *fp;
ENTER_FUNC;
	if ((fp = ConnectSysData()) != NULL) {
		KVREQ_Request(fp, KV_DELETEENTRY, query);
		SendPacketClass(fp, SYSDATA_END);
		CloseNet(fp);
	}
	FreeValueStruct(query);
LEAVE_FUNC;
}

extern	void
SYSDB_TERM_Update(
	ValueStruct *query)
{
	NETFILE *fp;
ENTER_FUNC;
	if ((fp = ConnectSysData()) != NULL) {
		KVREQ_Request(fp, KV_SETVALUE, query);
		SendPacketClass(fp, SYSDATA_END);
		CloseNet(fp);
	}
LEAVE_FUNC;
}

extern	void
SYSDB_TERM_SetValue(
	ValueStruct *query, 
	int keyid,
	char *data)
{
	ValueStruct *value;
	char lname[128];
ENTER_FUNC;
	sprintf(lname,"query[%d].value", keyid);
	if ((value = GetItemLongName(query, lname)) != NULL) {
		SetValueStringWithLength(value, data, strlen(data), NULL);
	}
LEAVE_FUNC;
}
