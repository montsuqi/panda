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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	<sys/socket.h>
#include	<sys/select.h>
#include	<time.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"libmondai.h"
#include	"comm.h"
#include	"dbgroup.h"
#include	"dblib.h"
#include	"queue.h"
#include	"socket.h"
#include	"message.h"
#include	"dblog.h"
#include	"authstub.h"
#include	"debug.h"
#include	"dbmaster_thread.h"


#define DBMASTER_TIMEOUT_SEC 1


typedef struct {
	DBMasterThread *ctx;
	int client_fd;
} WorkerArgs;

extern DBMasterThread *
NewDBMasterThread(
	DBG_INSTANCE *dbg,
	char *auth_url,
	Port *port,
	int backlog)
{
	DBMasterThread *ctx = New(DBMasterThread);
	memset(ctx, 0, sizeof(*ctx));
	
	ctx->log = Open_DBLog(dbg, dbg->logTableName);
	if (!ctx->log) {
		Error("open logfile failed");
		DestroyDBMasterThread(&ctx);
		return NULL;
	}
	ctx->auth_url = NewURL();
	ParseURL(ctx->auth_url, auth_url, "file");
	
	ctx->server_fd = InitServerPort(port, backlog);
	if (ctx->server_fd < 0) {
		Error("open logfile failed");
		DestroyDBMasterThread(&ctx);		
		return NULL;
	}
	ctx->server = SocketToNet(ctx->server_fd);
	ctx->main_thread_active = FALSE;
	
	return ctx;
}

extern void
DestroyDBMasterThread(
	DBMasterThread **ctx)
{
	if (*ctx == NULL) {
		return;
	}

	if ((*ctx)->log) {
		Close_DBLog(&(*ctx)->log);
		(*ctx)->log = NULL;
	}
	if ((*ctx)->auth_url) {
		FreeURL((*ctx)->auth_url);
		(*ctx)->auth_url = NULL;
	}
	xfree(*ctx);
	*ctx = NULL;
}

static Bool
SendLogFunc(
	void *userdata,
	DBLogRecord *rec)
{
	NETFILE *client = (NETFILE *)userdata;
	
	SendPacketClass(client, DBREPLICATION_COMMAND_RECORD);
	if (!client->fOK) { goto _io_error; }
	
	SendUInt64(client, rec->no);
	if (!client->fOK) { goto _io_error; }
	
	SendLBS(client, rec->data);
	if (!client->fOK) { goto _io_error; }
	
	SendLBS(client, rec->checkdata);
	if (!client->fOK) { goto _io_error; }
	
	return TRUE;
	
_io_error:
	return FALSE;
}

static Bool
Authentication(
	DBMasterThread *ctx,	
	NETFILE *client,
	int *ver)
{
	PacketClass cmd;
	Bool ret = FALSE;
	LargeByteString *user;
	LargeByteString *pass;
ENTER_FUNC;

	user = NewLBS();
	pass = NewLBS();
	
	cmd = RecvPacketClass(client);
	if (!client->fOK) { goto _io_error;  }	
	if (cmd != DBREPLICATION_COMMAND_AUTH) { goto _io_error; }
	
	*ver = RecvUInt(client);
	if (!client->fOK) { goto _io_error;  }
	
	RecvLBS(client, user);
	if (!client->fOK) { goto _io_error;  }
	
	RecvLBS(client, pass);
	if (!client->fOK) { goto _io_error;  }
	
	if (AuthUser(ctx->auth_url, LBS_Body(user), LBS_Body(pass), NULL, NULL)) {
		SendPacketClass(client, DBREPLICATION_COMMAND_OK);
		if (client->fOK) {
			ret = TRUE;
		}
	} else {
		SendPacketClass(client, DBREPLICATION_COMMAND_NG);
	}

_io_error:
	FreeLBS(user);
	FreeLBS(pass);
LEAVE_FUNC;
	return ret;
}

static Bool
RecvRequest(
	int ver,
	DBMasterThread *ctx,	
	NETFILE *client,
	DBLogNo *no)
{
	PacketClass cmd;
	Bool ret = FALSE;
ENTER_FUNC;

	cmd = RecvPacketClass(client); ON_IO_ERROR(client, io_error);
	if (cmd != DBREPLICATION_COMMAND_REQ) { 
		goto io_error; 
	}
	
	*no = RecvUInt64(client); ON_IO_ERROR(client, io_error);
	ret = TRUE;
	
io_error:
LEAVE_FUNC;	
	return ret;
}

static Bool
SendLogs(
	int ver,
	DBMasterThread *ctx,	
	NETFILE *client,
	DBLogNo no)
{
	Bool ret;
ENTER_FUNC;	
	ret = Foreach_DBLog(
		ctx->log,
		no + 1LL, DBLOG_DEFAULT_NO,
		SendLogFunc, client
	);
LEAVE_FUNC;
	return ret;
}

static void *
LogSendWorkerMain(
	void *userdata)
{
	WorkerArgs *args = (WorkerArgs *)userdata;
	DBMasterThread *ctx = args->ctx;
	NETFILE *client = SocketToNet(args->client_fd);
	DBLogNo no;
	Bool ret;
	int ver;
		
	if (client == NULL) {
		return NULL;
	}

	ret = Authentication(ctx, client, &ver);
	if (!ret) {
		goto end_worker;
	}

	ret = RecvRequest(ver, ctx, client, &no);
	if (!ret) {
		goto end_worker;
	}

	ret = SendLogs(ver, ctx, client, no);
	if (!ret) {
		goto end_worker;
	}
	
	SendPacketClass(client, DBREPLICATION_COMMAND_EOR);
	
end_worker:
	CloseNet(client);
	xfree(args);
	
	return NULL;
}

static void
StartLogSendWorker(
	DBMasterThread *ctx,
	int client_fd)
{
	pthread_t worker;
	WorkerArgs *args = New(WorkerArgs);
	
	args->ctx = ctx;
	args->client_fd = client_fd;
	
	pthread_create(&worker, NULL, LogSendWorkerMain, args);
	pthread_detach(worker);
}

static void
UnsetActiveFlagMain(
	void *userdata)
{
	DBMasterThread *ctx = (DBMasterThread *)userdata;
	ctx->main_thread_active = FALSE;
}

static void *
DBMasterThreadMain(
	void *userdata)
{
	DBMasterThread *ctx = (DBMasterThread *)userdata;
	fd_set read_fdset, except_fdset;
	struct timeval tv;
	
	pthread_cleanup_push(UnsetActiveFlagMain, ctx);
	ctx->main_thread_active = TRUE;
	
	FD_ZERO(&read_fdset);
	FD_ZERO(&except_fdset);
	do {
		tv.tv_sec = DBMASTER_TIMEOUT_SEC;
		tv.tv_usec = 0;
		FD_CLR(ctx->server_fd, &read_fdset);
		FD_CLR(ctx->server_fd, &except_fdset);
		FD_SET(ctx->server_fd, &read_fdset);
		FD_SET(ctx->server_fd, &except_fdset);
		if (select(ctx->server_fd + 1, &read_fdset, NULL, &except_fdset, &tv) < 0) {
			Error("select failed");
			break;
		}
		if (FD_ISSET(ctx->server_fd, &except_fdset)) {
			Error("select failed");
			break;
		}
		if (FD_ISSET(ctx->server_fd, &read_fdset)) {
			int client_fd = accept(ctx->server_fd, NULL, NULL);
			if (client_fd >= 0) {
				StartLogSendWorker(ctx, client_fd);
			}
		}
	} while (!ctx->shutdown);
	
	pthread_cleanup_pop(1);
	
	return NULL;
}

extern void
StartDBMaster(
	DBMasterThread *ctx)
{
	ctx->shutdown = FALSE;
	if (!ctx->main_thread_active) {
		pthread_create(&ctx->main_thread, NULL, DBMasterThreadMain, ctx);
	}
}

extern void
JoinDBMaster(
	DBMasterThread *ctx)
{
	pthread_join(ctx->main_thread, NULL);
}


extern void
StopDBMaster(
	DBMasterThread *ctx)
{
	ctx->shutdown = TRUE;
	pthread_join(ctx->main_thread, NULL);
}

