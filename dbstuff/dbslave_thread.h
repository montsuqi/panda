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

#ifndef	_DBSLAVE_THREAD_H
#define	_DBSLAVE_THREAD_H

#include "libmondai.h"
#include "comm.h"
#include "port.h"
#include <pthread.h>
#include "dbreplication.h"

typedef struct {
	DBG_INSTANCE *dbg; /* replica dbg */
	Port *server_port;
	DBG_INSTANCE  *log;/*local log db */
	int replication_sec;
	time_t expire_log_sec;
	char *user;
	char *pass;
	Bool no_checksum;

	/* internal resources */
	pthread_t main_thread;
	pthread_mutex_t mutex;
	pthread_cond_t timer;
	Bool main_thread_active;
	Bool shutdown;
} DBSlaveThread;

extern DBSlaveThread *NewDBSlaveThread(
	DBG_INSTANCE *dbg, DBG_INSTANCE *log_dbg, Port *server_port,
	char *user, char *pass,
	Bool no_checksum,
	int replication_sec,
	int expire_log_sec);
extern void 		DestroyDBSlaveThread(DBSlaveThread **ctx);
extern void		StartDBSlave(DBSlaveThread *ctx);
extern void		JoinDBSlave(DBSlaveThread *ctx);
extern void		StopDBSlave(DBSlaveThread *ctx);

#endif
