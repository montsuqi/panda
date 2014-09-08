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

#define	MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/wait.h>
#include	<signal.h>
#include	<time.h>
#include	<errno.h>
#include	<uuid/uuid.h>

#include	"libmondai.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"monsys.h"
#include	"message.h"
#include	"debug.h"

static	char	*Directory;

static	volatile Bool exit_flag = FALSE;
extern char **environ;

#define BATCH_TABLE "monbatch"
#define BATCH_LOG_TABLE "monbatch_log"

typedef struct {
	DBG_Struct	*dbg;
	LargeByteString	*name;
	LargeByteString *value;
} name_value_t;

void signal_handler (int signo )
{
	exit_flag = TRUE;
	printf("stop signal\n");
}

static	void
InitSystem(void)
{
	InitMessage("monbatch",NULL);
	if ( (Directory = getenv("MCP_DIRECTORY_PATH")) != NULL ) {
		InitDirectory();
		SetUpDirectory(Directory,NULL,NULL,NULL,P_NONE);
		if		( ThisEnv == NULL ) {
			Error("DI file parse error.");
		}
	}
}

static void
insert_table(
	char *name,
	char *value,
	name_value_t *kv)
{
	ValueStruct		*v;

	if (LBS_Size(kv->name) > 0) {
		LBS_EmitChar(kv->name,',');
		LBS_EmitSpace(kv->name);
	}
	LBS_EmitString(kv->name, name);

	if (LBS_Size(kv->value) > 0) {
		LBS_EmitChar(kv->value,',');
		LBS_EmitSpace(kv->value);
	}
	LBS_EmitChar(kv->value,'\'');
	v = NewValue(GL_TYPE_CHAR);
	SetValueStringWithLength(v, value, strlen(value), NULL);
	LBS_EmitString(kv->value, ValueToString(v,kv->dbg->coding));
	FreeValueStruct(v);
	LBS_EmitChar(kv->value,'\'');
}

static int
registdb(
	DBG_Struct	*dbg,
	GHashTable *table)
{
	char *sql;
	size_t sql_len;
	name_value_t kv;

	if (!dbg) {
		return 0;
	}

	dbg->dbt = table;

	OpenDB(dbg);
	TransactionStart(dbg);

	kv.dbg = dbg;
	kv.name = NewLBS();
	kv.value = NewLBS();

	g_hash_table_foreach(table, (GHFunc)insert_table, &kv);
	LBS_EmitEnd(kv.name);
	LBS_EmitEnd(kv.value);

	sql_len = LBS_Size(kv.name) + LBS_Size(kv.value) + 40;
	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len, "INSERT INTO %s (%s) VALUES (%s);",
			 BATCH_TABLE,
			 (char *)LBS_Body(kv.name),(char *)LBS_Body(kv.value));
	printf("%s\n", sql);
	ExecDBOP(dbg, sql, DB_UPDATE);

	snprintf(sql, sql_len, "INSERT INTO %s (%s) VALUES (%s);",
			 BATCH_LOG_TABLE,
			 (char *)LBS_Body(kv.name),(char *)LBS_Body(kv.value));
	printf("%s\n", sql);
	ExecDBOP(dbg, sql, DB_UPDATE);

	xfree(sql);

	TransactionEnd(dbg);
	CloseDB(dbg);

	return 0;
}

static int
unregistdb(
	DBG_Struct	*dbg,
	pid_t pgid)
{
	time_t now;
	struct	tm	tm_now;
	char	date[50];
	char	*sql;
	size_t sql_len = 1024;

	now = time(NULL);
	localtime_r(&now, &tm_now);
	strftime(date, sizeof(date),
			 "%F %T %z", &tm_now);

	OpenDB(dbg);
	TransactionStart(dbg);

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len, "DELETE FROM batch WHERE pgid = '%d';", (int)pgid);
	printf("sql:%s\n", sql);
	ExecDBOP(dbg, sql, DB_UPDATE);
	snprintf(sql, sql_len, "UPDATE batch_log SET endtime = '%s' WHERE pgid = '%d';", date, (int)pgid);
	printf("sql:%s\n", sql);
	ExecDBOP(dbg, sql, DB_UPDATE);
	xfree(sql);

	TransactionEnd(dbg);
	CloseDB(dbg);

	printf("finish %d\n", pgid);
	return 0;
}

static	GHashTable *
get_batch_info(
	pid_t pgid)
{
	time_t now;
	struct	tm	tm_now;
	uuid_t	u;
	char	uuid[SIZE_TERM+1];
	char	pid_s[10];
	char	date[50];
	GHashTable *table;


	table = NewNameHash();

	uuid_generate(u);
	uuid_unparse(u, uuid);
	g_hash_table_insert(table, "id", StrDup(uuid));

	snprintf(pid_s, sizeof(pid_s), "%d", (int)pgid);
	g_hash_table_insert(table, "pgid", StrDup(pid_s));

	now = time(NULL);
	localtime_r(&now, &tm_now);
	strftime(date, sizeof(date),
			 "%F %T %z", &tm_now);
	g_hash_table_insert(table, "starttime", StrDup(date));

	g_hash_table_insert(table, "tenant", getenv("MCP_TENANT"));
	g_hash_table_insert(table, "name", getenv("MCP_BATCH_NAME"));
	g_hash_table_insert(table, "comment", getenv("MCP_BATCH_COMMENT"));
	g_hash_table_insert(table, "exwindow", getenv("MCP_WINDOW"));
	g_hash_table_insert(table, "exwidget", getenv("MCP_WIDGET"));
	g_hash_table_insert(table, "exevent", getenv("MCP_EVENT"));
	g_hash_table_insert(table, "exterm", getenv("MCP_TERM"));
	g_hash_table_insert(table, "exuser", getenv("MCP_USER"));
	g_hash_table_insert(table, "exhost", getenv("MCP_HOST"));
	return table;
}

extern  int
exec_shell(
	int		argc,
	char	**argv)
{
	pid_t pid, wpid;
	int i, rc = 0;
	int status;
	char *cmdv[4];
	char *sh;

	for ( i=1; i<argc; i++ ) {
		if ( ( pid = fork() ) == 0 ) {
			sh = "/bin/sh";
			cmdv[0] = sh;
			cmdv[1] = "-c";
			cmdv[2] = argv[i];
			cmdv[3] = NULL;
			execve(sh, cmdv, environ);
		} else if ( pid < 0) {
			perror("fork");
			rc = -1;
			break;
		}
		wpid = waitpid(pid, &status, 0);
		if (wpid < 0) {
			perror("waitpid");
			rc = -1;
			break;
		}
		if (WIFEXITED(status)) {
			rc = WEXITSTATUS(status);
		} else {
			rc = status;
		}
		printf("status=%d\n", rc);
		if (exit_flag) {
			printf("signal exit\n");
			break;
		}
	}
	return rc;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	int rc;
	pid_t pgid;
	struct sigaction sa;
	DBG_Struct	*dbg;
	GHashTable *batch;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = signal_handler;
	sa.sa_flags |= SA_RESTART;
	if (sigaction(SIGHUP, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	InitSystem();

	dbg = GetDBG_monsys();
	pgid = getpgrp();
	batch = get_batch_info(pgid);

	registdb(dbg, batch);

	rc = exec_shell(argc, argv);
	if (rc == 0) {
		printf("OK\n");
	}else {
		printf("NG\n");
	}
	unregistdb(dbg, pgid);
	return 0;
}
