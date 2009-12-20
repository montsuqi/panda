/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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
#define	TIMER
#define	DEBUG
#define	TRACE
*/

#define	DBGROUP
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<sys/file.h>
#include	<libpq-fe.h>
#include	<libpq/libpq-fs.h>

#include	"libmondai.h"
#include	"enum.h"
#include        "DIparser.h"
#include	"PostgreSQLlib.h"
#include	"PostgreSQLutils.h"
#include	"redirect.h"
#include	"debug.h"
#include	"dblog.h"

#define  DBLOG_DEFAULT_TABLE "montsuqi_dbmaster_log"

static PGconn *
db_connect(
	DBG_INSTANCE *dbg,
	Bool *connected)
{
	PGconn *conn;
ENTER_FUNC;		
	*connected = FALSE;
	if (IS_DB_STATUS_CONNECT(dbg)) {
		conn = PGCONN(dbg, DB_UPDATE);
		*connected = TRUE;
		dbgmsg("pg_connect already connection");
	} else {
		dbgmsg("pg_connect new connection");
		conn = pg_connect(dbg);
	}
LEAVE_FUNC;	
	return conn;
}


static Bool
db_transaction(
	PGconn *conn,
	char *cmd)
{
	PGresult *res = db_exec(conn, cmd);
	Bool ret = TRUE;
ENTER_FUNC;	
	
	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
		ret = FALSE;
	}
	if (res) {
		PQclear(res);
	}

LEAVE_FUNC;	
	return ret;
}


static Bool
db_begin(
	PGconn *conn)
{
	Bool ret;
ENTER_FUNC;	
	ret = db_transaction(conn, "BEGIN");
LEAVE_FUNC;	
	return ret;
}


static Bool
db_commit(
	PGconn *conn)
{
	Bool ret;
ENTER_FUNC;	
	ret =  db_transaction(conn, "COMMIT");
LEAVE_FUNC;	
	return ret;
}


static Bool
db_rollback(
	PGconn *conn)
{
	Bool ret;
ENTER_FUNC;	
	ret = db_transaction(conn, "ROLLBACK");
LEAVE_FUNC;	
	return ret;
}


static Bool
CreateLogTable(
	DBLogCtx *log)
{
	Bool connected = FALSE;
	PGconn *conn = db_connect(log->dbg, &connected);
	char sql[SIZE_BUFF + 1];
	PGresult *res;
	Bool exists;
	Bool commit = FALSE;
	Bool functran = FALSE;
	Bool ret = TRUE;

ENTER_FUNC;
	if (!conn){
		return FALSE;
	}
	if (PQtransactionStatus(conn) != PQTRANS_INTRANS) { 
		functran = TRUE;
		ret = db_begin(conn);
		if (!ret) {
			Warning("BEGIN: %s", PQerrorMessage(conn));
			return FALSE;
		}
	}

	snprintf(
		sql, SIZE_BUFF,
		"SELECT tablename FROM pg_tables "
		" WHERE tablename = '%s';",
		log->tablename);
	res = db_exec(conn, sql);
	if (res == NULL && PQresultStatus(res) != PGRES_TUPLES_OK) {
		Warning("%s: %s", sql, PQerrorMessage(conn));
		ret = FALSE;
		goto _disconnect;
	}	
	if (PQntuples(res) == 1) {
		exists = TRUE;
	} else {
		exists = FALSE;
	}
	
	if (!exists) {
		snprintf(sql, SIZE_BUFF,
			 "CREATE TABLE \"%s\" (	                 "
			 "	no		INT8 PRIMARY KEY "
			 "	,data		TEXT             "
			 "	,checkdata	TEXT             "
			 "	,applydate	TIMESTAMP        "
			 ");",
			 log->tablename); 
		if (!db_command(conn, sql)) {
			Warning("create table: %s: %s", sql, PQerrorMessage(conn));
			ret = FALSE;
			goto _disconnect;
		}
		snprintf(sql, SIZE_BUFF,
			 "CREATE INDEX \"%s_applydate_idx\" ON \"%s\" ( "
			 "	applydate                               "
			 ");",
			 log->tablename, log->tablename); 
		if (!db_command(conn, sql)) {
			Warning("create index: %s: %s", sql, PQerrorMessage(conn));
			ret = FALSE;
			goto _disconnect;
		}
		snprintf(sql, SIZE_BUFF,
			 "CREATE SEQUENCE \"%s_seq\" START 1;", log->tablename);
		
		if (!db_command(conn, sql)) {
			Warning("create sequence: %s: %s", sql, PQerrorMessage(conn));
			ret = FALSE;
			goto _disconnect;
		}
		commit = TRUE;
	}
	
_disconnect:
	if (functran) {
		if (commit) {
			db_commit(conn);
		} else {
			db_rollback(conn);
		}
	}
	if (res) {
		PQclear(res);
	}
	if (!connected) {
		pg_disconnect(log->dbg);
	}
LEAVE_FUNC;
	return ret;
}


static Bool
AppendRecord(
	DBLogCtx *log,
	DBLogNo no,
	char *data,
	char *checkdata)
{
	Bool connected = FALSE;
	PGconn *conn = db_connect(log->dbg, &connected);
	size_t data_len = strlen(data) * 2 + 1;
	size_t checkdata_len = strlen(checkdata) * 2 + 1;
	size_t sql_len = 1024 + data_len + checkdata_len + 1;
	char *esc_data = (char *)xmalloc(data_len);
	char *esc_checkdata = (char *)xmalloc(checkdata_len);
	char *sql = (char *)xmalloc(sql_len);
	Bool ret = TRUE;

ENTER_FUNC;	
	pg_escape(esc_data, data, data_len);
	pg_escape(esc_checkdata, checkdata, checkdata_len);

	if (no == DBLOG_DEFAULT_NO) {
		snprintf(sql, sql_len,
			 "INSERT INTO \"%s\" (no, data, checkdata) VALUES(nextval('%s_seq'), '%s','%s');",
			 log->tablename, log->tablename, esc_data, esc_checkdata);
	} else {
		snprintf(sql, sql_len,
			 "INSERT INTO \"%s\" (no, data, checkdata) VALUES(%" PRIu64 ",'%s','%s');",
			 log->tablename, no, esc_data, esc_checkdata);
	}
	pthread_mutex_lock(&log->mutex);
	if (!db_command(conn, sql)) {
		Warning("%s: %s", sql, PQerrorMessage(conn));
		ret = FALSE;
	} else {
		
	}
	pthread_mutex_unlock(&log->mutex);
	
	if (!connected) {
		pg_disconnect(log->dbg);
	}
	xfree(esc_data);
	xfree(esc_checkdata);
	xfree(sql);
LEAVE_FUNC;	
	return ret;
}


extern DBLogCtx *
Open_DBLog(
	DBG_INSTANCE *dbg,
	char *tablename)
{
	DBLogCtx *log;
	pthread_mutexattr_t attr;
	Bool ret;
ENTER_FUNC;
	if (!IS_DB_STATUS_CONNECT(dbg)) {
		if (OpenRedirectDB(dbg) == MCP_OK) {
			Message("connect to database successed");
		} else {
			Error("connect to database failed");
		}
	}
	
	log = New(DBLogCtx);
	log->dbg = dbg;
	log->tablename = StrDup(tablename != NULL ? tablename: DBLOG_DEFAULT_TABLE);

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&log->mutex, &attr);
	
	pthread_mutexattr_destroy(&attr);
	
	ret = CreateLogTable(log);
	if (!ret) {
		if (log) {
			Close_DBLog(&log);
		}
		return NULL;
	}
LEAVE_FUNC;
	return log;
}


extern void
Close_DBLog(
	DBLogCtx **plog)
{
	DBLogCtx *log;
	
ENTER_FUNC;
	if (*plog == NULL) {
		return ;
	}
	log = *plog;
	if (log->tablename) {
		xfree(log->tablename);
	}
	pthread_mutex_lock(&log->mutex);
	CloseRedirectDB(log->dbg);
	
	pthread_mutex_unlock(&log->mutex);
	pthread_mutex_destroy(&log->mutex);
	xfree(log);
	
	*plog = NULL;
LEAVE_FUNC;
}


extern Bool
Put_DBLog(
	DBLogCtx *log,
	char *data,
	char *checkdata)
{
	Bool ret;

ENTER_FUNC;	
	ret = AppendRecord(log, DBLOG_DEFAULT_NO, data, checkdata);
	if (!ret) {
		Error("dblog insert error");
	}

LEAVE_FUNC;
	return ret;
}

extern Bool
PutNo_DBLog(
	DBLogCtx *log,
	DBLogNo no,
	char *data,
	char *checkdata)
{
	Bool ret;

ENTER_FUNC;	
	ret = AppendRecord(log, no, data, checkdata);
	if (!ret) {
		Error("dblog insert error");
	}

LEAVE_FUNC;
	return ret;
}

static void
ClearRecord(DBLogRecord *rec)
{
ENTER_FUNC;	
	if (rec->data) {
		FreeLBS(rec->data);
	}
	if (rec->checkdata) {
		FreeLBS(rec->checkdata);
	}
	memset(rec, 0, sizeof(DBLogRecord));
LEAVE_FUNC;	
}


extern Bool
GetLatestNo_DBLog(
	DBLogCtx *log,
	DBLogNo *no)
{
	Bool connected = FALSE;
	PGconn *conn = db_connect(log->dbg, &connected);
	PGresult *res;
	char sql[SIZE_BUFF];
	Bool ret = TRUE;
ENTER_FUNC;

	*no = DBLOG_DEFAULT_NO;
	pthread_mutex_lock(&log->mutex);

	snprintf(
		sql, SIZE_BUFF,
"SELECT max(no) FROM \"%s\";\n",
		log->tablename);

	res = db_exec(conn, sql);
	if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
		Warning("%s: %s", sql, PQerrorMessage(conn));
		ret = FALSE;
	} else {
		if (PQntuples(res) == 1) {
			char *val = PQgetvalue(res, 0, 0);
			dbgprintf("DB latest no => %s", val);
			if (val && strlen(val) > 0) {
				*no = strtoll(val, NULL, 10);
			}
		} else {
			ret = FALSE;
		}
	}
	if (res) {
		PQclear(res);
	}
	
	pthread_mutex_unlock(&log->mutex);
	if (!connected) {
		pg_disconnect(log->dbg);
	}
LEAVE_FUNC;
	return ret;
}


static Bool
SetRecord(const PGresult *res, DBLogRecord *rec, int i)
{
	char *no, *data, *checkdata, *applydate;
	
ENTER_FUNC;
	no = PQgetvalue(res, i, 0);
	data = PQgetvalue(res, i, 1);
	checkdata = PQgetvalue(res, i, 2);
	applydate = PQgetvalue(res, i, 3);

	rec->no = strtoll(no, NULL, 10);
	rec->data = NewLBS();
	rec->checkdata = NewLBS();

	LBS_EmitString(rec->data, data);
	LBS_EmitEnd(rec->data);
	LBS_EmitString(rec->checkdata, checkdata);
	LBS_EmitEnd(rec->checkdata);

	rec->applydate = strtoll(applydate, NULL, 10);
	
LEAVE_FUNC;
	return TRUE;
}


extern Bool
Foreach_DBLog(
	DBLogCtx *log,
	DBLogNo start_no,
	DBLogNo end_no,
	DBLogForeachFunc func,
	void *userdata)
{
	Bool connected = FALSE;
	Bool ret = TRUE;
	PGconn *conn = db_connect(log->dbg, &connected);
	PGresult *res;
	DBLogRecord rec;
	char sql[SIZE_BUFF + 1];
	int i, ntuples;
	
ENTER_FUNC;

	if (start_no == DBLOG_DEFAULT_NO
	    && end_no == DBLOG_DEFAULT_NO) 
	{
		snprintf(sql, SIZE_BUFF,
			 "SELECT no, data, checkdata, EXTRACT(epoch from applydate) FROM \"%s\" "
			 "ORDER BY no;"
			 ,log->tablename);
	} else if (start_no == DBLOG_DEFAULT_NO) {
		snprintf(sql, SIZE_BUFF,
			 "SELECT no, data, checkdata, EXTRACT(epoch from applydate) FROM \"%s\" "			 
			 "WHERE no < %" PRIu64 " ORDER BY no;"
			 ,log->tablename, end_no);
	} else if (end_no == DBLOG_DEFAULT_NO) {
		snprintf(sql, SIZE_BUFF,
			 "SELECT no, data, checkdata, EXTRACT(epoch from applydate) FROM \"%s\" "
			 "WHERE no >= %" PRIu64 " ORDER BY no;"
			 ,log->tablename, start_no);
	} else {
		snprintf(sql, SIZE_BUFF,
			 "SELECT no, data, checkdata, EXTRACT(epoch from applydate) FROM \"%s\" "			 
			 " WHERE no >= %" PRIu64 " AND no < %" PRIu64 " ORDER BY no;"
			 ,log->tablename, start_no, end_no);
	}
	
	pthread_mutex_lock(&log->mutex);
	res = db_exec(conn, sql);
	if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
		ret = FALSE;
		goto _end;
	}
	ntuples = PQntuples(res);
	
	for (i = 0; i < ntuples; ++i) {
		Bool procret;
		
		SetRecord(res, &rec, i);

		dbgmsg("call foreach func");
		procret = (*func)(userdata, &rec);
		ClearRecord(&rec);
		
		if (!procret) {
			dbgmsg("foreach func stop");
			ret = FALSE;
			break;
		}
	}
_end:
	pthread_mutex_unlock(&log->mutex);
	if (res) {
		PQclear(res);
	}
	if (!connected) {
		pg_disconnect(log->dbg);
	}
LEAVE_FUNC;
	return ret;
}


extern Bool
ForeachUnapplied_DBLog(
	DBLogCtx *log,
	DBLogForeachFunc func,
	void *userdata)
{
	Bool connected;
	Bool ret = TRUE;
	PGconn *conn = db_connect(log->dbg, &connected);
	PGresult *res = NULL;
	DBLogRecord rec;
	char sql[SIZE_BUFF + 1];
	int i, ntuples;
ENTER_FUNC;

	pthread_mutex_lock(&log->mutex);
	
	snprintf(sql, SIZE_BUFF,
		 "SELECT no, data, checkdata, EXTRACT(epoch from applydate) "
		 "FROM \"%s\" "
		 "WHERE applydate IS NULL "
		 "ORDER BY no;"
		 , log->tablename);
	res = db_exec(conn, sql);
	if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
		ret = FALSE;
		goto _end;
	}
	ntuples = PQntuples(res);
	
	for (i = 0; i < ntuples; ++i) {
		Bool procret;
		
		SetRecord(res, &rec, i);
		
		dbgmsg("call foreach func");
		procret = (*func)(userdata, &rec);
		ClearRecord(&rec);
		
		if (!procret) {
			dbgmsg("foreach func stop");
			ret = FALSE;
			break;
		}
	}
_end:
	pthread_mutex_unlock(&log->mutex);
	if (res) {
		PQclear(res);
	}
	if (!connected) {
		pg_disconnect(log->dbg);
	}
LEAVE_FUNC;
	return TRUE;
}


extern Bool
UpdateApplyDate_DBLog(
	DBLogCtx *log,
	DBLogNo no)
{
	Bool connected = FALSE;
	PGconn *conn = db_connect(log->dbg, &connected);
	char sql[SIZE_BUFF + 1];
	Bool ret = TRUE;

ENTER_FUNC;

	snprintf(sql, SIZE_BUFF,
		 "UPDATE \"%s\" SET applydate = NOW() WHERE no = %" PRIu64 ";",
		 log->tablename, no
	);
	
	pthread_mutex_lock(&log->mutex);
	dbgmsg(sql);
	if (!db_command(conn, sql)) {
		Warning("%s: %s", sql, PQerrorMessage(conn));
		ret = FALSE;
	}
	pthread_mutex_unlock(&log->mutex);
	
LEAVE_FUNC;	
	return ret;
}


extern Bool
Truncate_DBLog(
	DBLogCtx *log,
	time_t expire_log_sec)
{
	Bool connected = FALSE;
	PGconn *conn = db_connect(log->dbg, &connected);
	char sql[SIZE_BUFF + 1];
	Bool ret = TRUE;
	uint64_t expire_log_sec64 = (uint64_t)expire_log_sec;

ENTER_FUNC;
	snprintf(sql, SIZE_BUFF,
		 "DELETE FROM \"%s\" WHERE applydate < NOW() - INTERVAL '%" PRIu64 " sec' AND no < (SELECT max(no) FROM \"%s\");",
		 log->tablename, expire_log_sec64, log->tablename
	);
	dbgmsg(sql);
	pthread_mutex_lock(&log->mutex);
	if (!db_command(conn, sql)) {
		Warning("%s: %s", sql, PQerrorMessage(conn));
		ret = FALSE;
	}
	pthread_mutex_unlock(&log->mutex);
LEAVE_FUNC;
	return ret;
}
