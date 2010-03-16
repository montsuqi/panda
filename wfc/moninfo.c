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

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<string.h>
#include    <sys/types.h>
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>

#include	"enum.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"const.h"
#include	"dirs.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
#include	"keyvaluereq.h"
#include	"socket.h"
#include	"message.h"
#include	"debug.h"

#define	MAX_USER	100

static	gchar	*SysDataPort = SYSDATA_PORT;
static	GOptionEntry entries[] = 
{
	{	"port",	'p',	0,	G_OPTION_ARG_STRING, &SysDataPort, "SystemDataThread port P", "P" },
	{	NULL	}
};

enum {
	DUMP = 0,
	SENDMSG,
	SENDMSGALL,
	ABORT,
	ABORTALL,
	WHO,
	LAST
};

static struct _COMMAND {
	char *name;
	char *desc;
} command[] = 
{
	{	"dump"		, "command            - dump system database"		},
	{	"sendmsg"	, "command ID MESSAGE - send message to client"		},
	{	"sendmsgall", "command MESSAGE    - send message to all client"	},
	{	"abort"		, "command ID MESSAGE - abort client"				},
	{	"abortall"	, "command MESSAGE    - abort all client"			},
	{	"who"		, "command            - show who is using"			},
	{	""			, ""												}
};

static	NETFILE *
ConnectSysData()
{
	int fd;
	NETFILE *fp;
	Port *port;
ENTER_FUNC;
	fp = NULL;
	port = ParPort(SysDataPort, PORT_WFC);
	fd = ConnectSocket(port, SOCK_STREAM);
	DestroyPort(port);
	if (fd > 0) {
		fp = SocketToNet(fd);
	}
	if (fp == NULL) {
		printf("[ERROR] cannot connect sysdata thread\n");
		exit(1);
	}
LEAVE_FUNC;
	return fp;
}

static	void
Dump(void)
{
	NETFILE *fp;
	ValueStruct *q;
	ValueStruct *id;
ENTER_FUNC;
	fp = ConnectSysData();
	q = KVREQ_NewQuery(1);
	if (KVREQ_Request(fp, KV_DUMP, q) == MCP_OK) {
		id = GetItemLongName(q, "id");
		printf("[SUCCESS] dump to %s\n", ValueToString(id, NULL));
	} else {
		printf("[ERROR] dump failure\n");
		exit(1);
	}
	FreeValueStruct(q);
	CloseNet(fp);
	exit(0);
LEAVE_FUNC;
}

static	void
SendMsg(
	char *id,
	char *msg,
	Bool fAbort,
	Bool fAll)
{
	NETFILE *fp;
	ValueStruct *q;
	char *key[2];
	char *value[2];
	int rc;
	
ENTER_FUNC;
	fp = ConnectSysData();
	key[0] = "message";
	value[0] = msg;
	key[1] = "abort";
	if (fAbort) {
		value[1] = "T";
	} else {
		value[1] = "";
	}
	q = KVREQ_NewQueryWithValue(id, 2, key, value);

	if (fAll) {
		rc = KVREQ_Request(fp, KV_SETVALUEALL, q);
	} else {
		SetValueStringWithLength(GetItemLongName(q,"id"), id, strlen(id), NULL);
		rc = KVREQ_Request(fp, KV_SETVALUE, q);
	}
	CloseNet(fp);
	FreeValueStruct(q);
	if (rc != MCP_OK) {
		if (fAbort) {
			if (fAll) {
				printf("[ERROR] abortall failure\n");
			} else {
				printf("[ERROR] abort failure\n");
			}
		} else {
			if (fAll) {
				printf("[ERROR] sendmsgall failure\n");
			} else {
				printf("[ERROR] sendmsg failure\n");
			}
		}
		exit(1);
	}
	exit(0);
LEAVE_FUNC;
}

static	void
ColumnPrint(
	int size,
	char *str)
{
	int j;
	int s;
	char *buff;

	s = strlen(str);
	if (s > size - 1) {
		printf("%s",str);
	} else {
		buff = xmalloc(size);
		strcpy(buff, str);
		for( j = s; j < size - 1; j++) {
			buff[j] = ' ';
		}
		buff[size - 1] = 0;
		printf("%s",buff);
		xfree(buff);
	}
}

extern	void
Who(void)
{
	NETFILE *fp;
	ValueStruct *list;
	ValueStruct *query;
	char lname[128];
	char *key[6] =  {
		"user",
		"host",
		"create_time",
		"access_time",
		"window",
		"agent"
	};
	char *value[6] = {"","","","","",""};
	int i;
ENTER_FUNC;
	fp = ConnectSysData();
	list = KVREQ_NewQuery(MAX_USER);
	if (KVREQ_Request(fp, KV_LISTENTRY, list) != MCP_OK) {
		printf("[ERROR] cannot get user list\n");
		exit(1);
	}
	ColumnPrint(11, "USER");
	ColumnPrint(17, "HOST");
	ColumnPrint(32, "LOGIN@");
	ColumnPrint(32, "LAST@");
	ColumnPrint(17, "WINDOW");
	ColumnPrint(20, "AGENT");
	ColumnPrint(129, "ID");
	printf("\n");
	for ( i = 0; i < ValueInteger(GetItemLongName(list, "num")); i++) {
		sprintf(lname, "query[%d].key", i);
		if (!strcmp("system", ValueToString(GetItemLongName(list,lname), NULL))) {
			continue;
		} 
		query = KVREQ_NewQueryWithValue(ValueToString(GetItemLongName(list,lname), NULL),6,key,value);
		if (KVREQ_Request(fp, KV_GETVALUE, query) == MCP_OK) {
			ColumnPrint(11, ValueToString(GetItemLongName(query,"query[0].value"), NULL));
			ColumnPrint(17, ValueToString(GetItemLongName(query,"query[1].value"), NULL));
			ColumnPrint(32, ValueToString(GetItemLongName(query,"query[2].value"), NULL));
			ColumnPrint(32, ValueToString(GetItemLongName(query,"query[3].value"), NULL));
			ColumnPrint(17, ValueToString(GetItemLongName(query,"query[4].value"), NULL));
			ColumnPrint(20, ValueToString(GetItemLongName(query,"query[5].value"), NULL));
			ColumnPrint(129, ValueToString(GetItemLongName(query,"id"), NULL));
			printf("\n");
		}
		FreeValueStruct(query);
	}
	FreeValueStruct(list);
	CloseNet(fp);
	exit(0);
LEAVE_FUNC;
}

static	void
print_usage()
{
	int i;

	ColumnPrint(16, "command");
	ColumnPrint(16, "description");
	printf("\n\n");
	for(i = 0; i < LAST; i++) {
		ColumnPrint(16, command[i].name);
		ColumnPrint(16, command[i].desc);
		printf("\n");
	}
}

extern	int
main(
	int		argc,
	char	*argv[])
{
	int i;
	int c;
	Bool fInvalid;
	GError	*error = NULL;
	GOptionContext *context;

	InitMessage("moninfo",NULL);
	InitNET();

	context = g_option_context_new("- montsuqi system database utility");
	g_option_context_add_main_entries (context, entries, NULL);
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_print ("option parsing failed: %s\n", error->message);
		exit (1);
	}
	if (argc < 2) {
		g_print("[ERROR] need command\n");
		print_usage();
		exit (1);
	}
	c = LAST;
	for (i = 0; i < LAST; i++) {
		if (!strcmp(command[i].name, argv[1])) {
			c = i;
		}
	}
	fInvalid = TRUE;
	switch (c) {
	case DUMP:
		Dump();
		break;
	case SENDMSG:
		if (argc >= 4) {
			SendMsg(argv[2], argv[3], FALSE, FALSE);
		}
		break;
	case SENDMSGALL:
		if (argc >= 3) {
			SendMsg(argv[2], argv[2], FALSE, TRUE);
		}
		break;
	case ABORT:
		if (argc >= 4) {
			SendMsg(argv[2], argv[3], TRUE, FALSE);
		}
		break;
	case ABORTALL:
		if (argc >= 3) {
			SendMsg(argv[2], argv[2], TRUE, TRUE);
		}
		break;
	case WHO:
		Who();
		break;
	}
	g_print("[ERROR] invalid command\n");
	print_usage();
	exit(1);
	return	(0);
}
