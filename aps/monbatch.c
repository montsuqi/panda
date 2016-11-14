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
#include	"dbutils.h"
#include	"monsys.h"
#include	"gettext.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*Directory;

static	volatile Bool exit_flag = FALSE;
extern char **environ;

typedef struct {
	DBG_Struct	*dbg;
	LargeByteString	*name;
	LargeByteString *value;
} name_value_t;

static	ARG_TABLE	option[] = {
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		N_("directory file name")						},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

void
signal_handler (int signo )
{
	if (!exit_flag) {
		exit_flag = TRUE;
		killpg(0, SIGHUP);
	}
}

void
alrm_handler (int signo)
{
	exit_flag = TRUE;
	killpg(0, SIGKILL);
}

void
chld_handler (int signo)
{
	/* dummy */
	/* Auto in waitpid is executed If you do not have to register */
	/* (default Shell.c of montsuqi) */
}

static	void
InitSystem(void)
{
	char *dir;
	InitMessage("monbatch",NULL);

	if ( (dir = getenv("MON_DIRECTORY_PATH")) != NULL ) {
		Directory = dir;
	}
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,P_NONE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
}

static void
insert_table(
	char *name,
	char *value,
	name_value_t *kv)
{
	char *evalue;

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
	if (value != NULL) {
		evalue = Escape_monsys(kv->dbg, value);
		LBS_EmitString(kv->value, evalue);
		xfree(evalue);
	}
	LBS_EmitChar(kv->value,'\'');
}

static int
registdb(
	DBG_Struct	*dbg,
	GHashTable *table)
{
	char *sql;
	size_t sql_len = SIZE_SQL;
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

	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len, "INSERT INTO %s (%s) VALUES (%s);",
			 BATCH_TABLE,
			 (char *)LBS_Body(kv.name),(char *)LBS_Body(kv.value));
	ExecDBOP(dbg, sql, FALSE, DB_UPDATE);

	snprintf(sql, sql_len, "INSERT INTO %s (%s) VALUES (%s);",
			 BATCH_LOG_TABLE,
			 (char *)LBS_Body(kv.name),(char *)LBS_Body(kv.value));
	ExecDBOP(dbg, sql, TRUE, DB_UPDATE);

	xfree(sql);
	FreeLBS(kv.name);
	FreeLBS(kv.value);

	TransactionEnd(dbg);
	CloseDB(dbg);

	return 0;
}

static int
unregistdb(
	DBG_Struct	*dbg,
	char *batch_id,
	json_object *cmd_results)
{
	json_object *child;
	int		rc;
	char	*message;
	char	endtime[50];
	char	*sql, *exec_record;
	size_t sql_len = SIZE_SQL;

	timestamp(endtime, sizeof(endtime));
	child = json_object_object_get(cmd_results,"rc");
	if (CheckJSONObject(child,json_type_int)) {
		rc = json_object_get_int(child);
	} else {
		rc = 999;
	}
	child = json_object_object_get(cmd_results,"result_message");
	if (CheckJSONObject(child,json_type_string)) {
		message = (char *)json_object_get_string(child);
	} else {
		message = "";
	}

	OpenDB(dbg);
	TransactionStart(dbg);

	exec_record = Escape_monsys(dbg, json_object_to_json_string(cmd_results));
	sql_len = sql_len + strlen(exec_record);
	sql = (char *)xmalloc(sql_len);
	snprintf(sql, sql_len,
      "UPDATE %s SET endtime = '%s',rc = '%d', message = '%s', exec_record = '%s' WHERE id = '%s';",
			 BATCH_LOG_TABLE, endtime, rc, message, exec_record, batch_id);
	ExecDBOP(dbg, sql, TRUE, DB_UPDATE);

	snprintf(sql, sql_len, "DELETE FROM %s WHERE id = '%s';",
			 BATCH_TABLE, batch_id);
	ExecDBOP(dbg, sql, FALSE, DB_UPDATE);
	xfree(sql);

	TransactionEnd(dbg);
	CloseDB(dbg);

	return 0;
}

static	GHashTable *
get_batch_info(
	char *batch_id,
	pid_t pgid)
{
	char	pid_s[10];
	char	starttime[50];
	GHashTable *batch;

	batch = NewNameHash();

	g_hash_table_insert(batch, "id", batch_id);
	snprintf(pid_s, sizeof(pid_s), "%d", (int)pgid);
	g_hash_table_insert(batch, "pgid", StrDup(pid_s));

	timestamp(starttime, sizeof(starttime));
	g_hash_table_insert(batch, "starttime", StrDup(starttime));

	g_hash_table_insert(batch, "tenant", getenv("MCP_TENANT"));
	g_hash_table_insert(batch, "name", getenv("MON_BATCH_NAME"));
	g_hash_table_insert(batch, "comment", getenv("MON_BATCH_COMMENT"));
	g_hash_table_insert(batch, "groupname", getenv("MON_BATCH_GROUPNAME"));
	g_hash_table_insert(batch, "extra", getenv("MON_BATCH_EXTRA"));
	g_hash_table_insert(batch, "exwindow", getenv("MCP_WINDOW"));
	g_hash_table_insert(batch, "exwidget", getenv("MCP_WIDGET"));
	g_hash_table_insert(batch, "exevent", getenv("MCP_EVENT"));
	g_hash_table_insert(batch, "exterm", getenv("MCP_TERM"));
	g_hash_table_insert(batch, "exuser", getenv("MCP_USER"));
	g_hash_table_insert(batch, "exhost", getenv("MCP_HOST"));
	return batch;
}

static json_object *
exec_shell(
	pid_t pgid,
	int		argc,
	char	**argv)
{
	pid_t pid, wpid;
	char starttime[50], endtime[50];
	int i, rc = 0, rrc = 0;
	int status;
	char *cmdv[4];
	char *sh;
	char *error = NULL;
	char *str_results;
	json_object *cmd_results, *result, *child;

	cmd_results = json_object_new_object();
	json_object_object_add(cmd_results,"pgid",json_object_new_int((int)pgid));
	result = json_object_new_array();
	json_object_object_add(cmd_results,"command",result);
	for ( i=1; i<argc; i++ ) {
		child = json_object_new_object();
		timestamp(starttime, sizeof(starttime));
		json_object_object_add(child,"starttime",json_object_new_string(starttime));
		if ( ( pid = fork() ) == 0 ) {
			sh = "/bin/sh";
			cmdv[0] = sh;
			cmdv[1] = "-c";
			cmdv[2] = argv[i];
			cmdv[3] = NULL;
			execve(sh, cmdv, environ);
		} else if ( pid < 0) {
			error = strerror(errno);
			rc = -1;
			break;
		}
		wpid = waitpid(pid, &status, 0);
		if (wpid < 0) {
			error = strerror(errno);
			rc = -1;
			break;
		}
		if (WIFEXITED(status)) {
			rc = WEXITSTATUS(status);
		} else if (WIFSIGNALED(status)) {
			rc = -WTERMSIG(status);
		} else {
			rc = status;
		}
		rrc += rc;
		timestamp(endtime, sizeof(endtime));
		json_object_object_add(child,"pid",json_object_new_int((int)pid));
		json_object_object_add(child,"name",json_object_new_string(argv[i]));
		json_object_object_add(child,"result",json_object_new_int(rc));
		json_object_object_add(child,"endtime",json_object_new_string(endtime));
		json_object_array_add(result,child);
		if (exit_flag) {
			break;
		}
	}
	if (error) {
		str_results = error;
	} else if (exit_flag) {
		str_results = "signal stop";
	} else if (rrc != 0) {
		str_results = "some failure";
	} else {
		str_results = "ok";
	}

	json_object_object_add(cmd_results,"rc",json_object_new_int(rrc));
	json_object_object_add(cmd_results,"result_message",json_object_new_string(str_results));
	return cmd_results;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	struct sigaction sa;
	DBG_Struct	*dbg;
	char *batch_id;
	uuid_t uu;
	pid_t pgid;
	GHashTable *batch;
	json_object *cmd_results;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset (&sa.sa_mask);
	sa.sa_flags |= SA_RESTART;
	sa.sa_handler = signal_handler;
	if (sigaction(SIGHUP, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	sa.sa_handler = chld_handler;
	if (sigaction(SIGCHLD, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	sa.sa_handler = alrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) != 0) {
		Error("sigaction(2) failure");
	}

	alarm(BATCH_TIMEOUT);

	fl = GetOption(option,argc,argv,NULL);
	InitSystem();
	if ((fl == NULL) || (fl->name == NULL)) {
		PrintUsage(option,argv[0],NULL);
	}

	if ((batch_id = getenv("MON_BATCH_ID")) == NULL) {
		Error("MON_BATCH_ID has not been set");
	}
	if (uuid_parse(batch_id, uu) == -1) {
		Error("MON_BATCH_ID is invalid");
	}

	dbg = GetDBG_monsys();
	pgid = getpgrp();
	batch = get_batch_info(batch_id, pgid);
	registdb(dbg, batch);

	cmd_results = exec_shell(pgid, argc, argv);

	unregistdb(dbg, batch_id, cmd_results);

	return 0;
}
