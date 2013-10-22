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

#ifndef	_DBMASTER_THREAD_H
#define	_DBMASTER_THREAD_H

#include "libmondai.h"
#include "comm.h"
#include "port.h"
#include <pthread.h>
#include "dbreplication.h"

typedef struct {
	DBLogCtx *log;
	URL *auth_url;
	pthread_t main_thread;
	NETFILE *server;
	int server_fd;
	Bool shutdown;
	Bool main_thread_active;
} DBMasterThread;


extern DBMasterThread *	NewDBMasterThread(DBG_INSTANCE *dbg, char *auth_url, Port *port, int backlog);
extern void 			DestroyDBMasterThread(DBMasterThread **ctx);

extern void			StartDBMaster(DBMasterThread *ctx);
extern void			JoinDBMaster(DBMasterThread *ctx);
extern void			StopDBMaster(DBMasterThread *ctx);

#endif
