/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _DBLOG_H
#define _DBLOG_H

#include "libmondai.h"
#include "dblib.h"
#include <inttypes.h>
#include <pthread.h>

#define IS_DB_STATUS_CONNECT(dbg) ((dbg)->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT)
#define DBG_INSTANCE DBG_Struct

typedef uint64_t  DBLogNo; 
#define DBLOG_DEFAULT_NO     ((DBLogNo)-1)
#define DBLOG_APPLYDATE_ZERO ((time_t)0)

typedef struct {
	DBG_INSTANCE *dbg;
	char *tablename;
	pthread_mutex_t mutex;
} DBLogCtx;

typedef struct {
	DBLogNo no;
	LargeByteString *data;
	LargeByteString *checkdata;
	time_t  applydate;
} DBLogRecord;

typedef Bool (*DBLogForeachFunc)(void *userdata, DBLogRecord *rec);

extern DBLogCtx *Open_DBLog(DBG_INSTANCE *dbg, char *tablename);
extern void	Close_DBLog(DBLogCtx **log);
extern Bool	Put_DBLog(DBLogCtx *log, char *data, char *checkdata);
extern Bool	PutNo_DBLog(DBLogCtx *log, DBLogNo no, char *data, char *checkdata);
extern Bool	GetLatestNo_DBLog(DBLogCtx *log, DBLogNo *no);
extern Bool	Foreach_DBLog(DBLogCtx *log,
			      DBLogNo start_no, DBLogNo end_no,
			      DBLogForeachFunc func, void *userdata);
extern Bool	ForeachUnapplied_DBLog(DBLogCtx *log,
				       DBLogForeachFunc func, void *userdata);
extern Bool     UpdateApplyDate_DBLog(DBLogCtx *log, DBLogNo no);
extern Bool	Truncate_DBLog(DBLogCtx *log, time_t expire_log_sec);

#endif
