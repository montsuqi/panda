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
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/socket.h>	/*	for SOCK_STREAM	*/
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"libmondai.h"
#include	"net.h"
#include	"dbgroup.h"
#include	"redirect.h"
#include	"comm.h"
#include	"debug.h"

#if	0
static Bool
SendQueryData_Redirect(
	DBG_Struct	*dbg)
{
	int rc = FALSE;
		
	SendPacketClass(dbg->fpLog,RED_DATA);	ON_IO_ERROR(dbg->fpLog,badio);
	SendLBS(dbg->fpLog,dbg->redirectData);	ON_IO_ERROR(dbg->fpLog,badio);
	if		(  RecvPacketClass(dbg->fpLog)  ==  RED_OK  ) {
		rc = TRUE;
	}
badio:
	return rc;
}
#endif

static Bool
SendVeryfyData_Redirect(
	DBG_Instance	*dbg)
{
	int rc = FALSE;
		
ENTER_FUNC;
	SendPacketClass(dbg->fpLog,RED_DATA);	ON_IO_ERROR(dbg->fpLog,badio);
	SendString(dbg->fpLog,dbg->env->id);	ON_IO_ERROR(dbg->fpLog,badio);
	SendLBS(dbg->fpLog,dbg->checkData);		ON_IO_ERROR(dbg->fpLog,badio);
	SendLBS(dbg->fpLog,dbg->redirectData);	ON_IO_ERROR(dbg->fpLog,badio);
	if		(  RecvPacketClass(dbg->fpLog)  ==  RED_OK  ) {
		rc = TRUE;
	}
badio:
LEAVE_FUNC;
	return rc;
}

static Bool
RecvSTATUS_Redirect(
	DBG_Instance	*dbg)
{
	int rc = FALSE;
	char	status;

ENTER_FUNC;
	SendPacketClass(dbg->fpLog, RED_STATUS);		ON_IO_ERROR(dbg->fpLog,badio);
	SendString(dbg->fpLog,dbg->env->id);			ON_IO_ERROR(dbg->fpLog,badio);
	status = RecvChar(dbg->fpLog);	ON_IO_ERROR(dbg->fpLog,badio);
	switch	(status) {
	  case	DB_STATUS_NOCONNECT:
	  case	DB_STATUS_UNCONNECT:
	  case	DB_STATUS_FAILURE:
	  case	DB_STATUS_DISCONNECT:
	  case	DB_STATUS_REDFAILURE:
		dbg->update.dbstatus = status;
		break;
	  default:
		break;
	}
	rc = TRUE;
badio:
LEAVE_FUNC;
	return rc;
}

extern	void
OpenDB_RedirectPort(
	DBG_Instance	*dbg)
{
	int			fh;
	DBG_Class	*rdbg;

ENTER_FUNC;
	dbgprintf("dbg [%s]\n",dbg->class->name);
	if		(	(  fNoRedirect  )
			||	(  dbg->class->redirect  ==  NULL  ) ) {
		Warning("no redirect");
		dbg->fpLog = NULL;
		dbg->redirectData = NULL;
		dbg->checkData = NULL;
	} else {
		rdbg = dbg->class->redirect;
		if		(	( rdbg->redirectPort  ==  NULL ) 
				||	(  ( fh = ConnectSocket(rdbg->redirectPort,SOCK_STREAM) )  <  0  ) ) {
			Warning("loging server not ready");
			dbg->fpLog = NULL;
			dbg->redirectData = NULL;
			dbg->checkData = NULL;
			if ( !fNoCheck ){
				dbg->update.dbstatus = DB_STATUS_REDFAILURE;	
			}
		} else {
			dbgmsg("redirect start");
			dbg->fpLog = SocketToNet(fh);
			dbg->redirectData = NewLBS();
			dbg->checkData = NewLBS();
			if ( !RecvSTATUS_Redirect(dbg) ){
				CloseDB_RedirectPort(dbg);
			}
		}
	}
LEAVE_FUNC;
}

extern	void
CloseDB_RedirectPort(
	DBG_Instance	*dbg)
{
ENTER_FUNC;
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_END);
		CloseNet(dbg->fpLog);
	}
	if		(  dbg->redirectData  !=  NULL  ) {
		FreeLBS(dbg->redirectData);
		dbg->redirectData = NULL;
	}
	if		(  dbg->checkData  !=  NULL  ) {
		FreeLBS(dbg->checkData);
		dbg->checkData = NULL;
	}
LEAVE_FUNC;
}

extern	void
PutDB_Redirect(
	DBG_Instance	*dbg,
	char			*data)
{
ENTER_FUNC;
	if		(  dbg->redirectData  !=  NULL  ) {
		LBS_EmitString(dbg->redirectData,data);
	}
LEAVE_FUNC;
}

extern	void
PutCheckDataDB_Redirect(
	DBG_Instance	*dbg,
	char			*data)
{
ENTER_FUNC;
	if		(  dbg->checkData  !=  NULL  ) {
		LBS_EmitString(dbg->checkData,data);
	}
LEAVE_FUNC;
}

extern	void
BeginDB_Redirect(
	DBG_Instance	*dbg)
{
ENTER_FUNC;
	if		(  dbg->redirectData  !=  NULL  ) { 
		LBS_EmitStart(dbg->redirectData);
		LBS_EmitStart(dbg->checkData);
	}
LEAVE_FUNC;
}

extern	Bool
CheckDB_Redirect(
	DBG_Instance	*dbg)
{
	Bool	rc = TRUE;
ENTER_FUNC;
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_PING);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_PONG  ) {
			Warning("log server down?");
			dbg->update.dbstatus = DB_STATUS_REDFAILURE;
			CloseDB_RedirectPort(dbg);
			rc = FALSE;
		}
	}
LEAVE_FUNC;
	return	(rc);
}

extern	void
AbortDB_Redirect(
	DBG_Instance	*dbg)
{
ENTER_FUNC;
	if	(  dbg->redirectData  !=  NULL  ) {
		LBS_EmitStart(dbg->redirectData);
		LBS_EmitStart(dbg->checkData);
	}
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_ABORT);	ON_IO_ERROR(dbg->fpLog,badio);
		SendString(dbg->fpLog,dbg->env->id);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_OK  ) {
		  badio:
			dbg->update.dbstatus = DB_STATUS_REDFAILURE;
			CloseDB_RedirectPort(dbg);
		}
	}
LEAVE_FUNC;
}

extern	void
PrepareDB_Redirect(
	DBG_Instance	*dbg)
{
	Bool rc = TRUE;

ENTER_FUNC;
	if		(  dbg->redirectData  !=   NULL  )	{
		if ( LBS_Size(dbg->redirectData) > 0 ) {
			rc = SendVeryfyData_Redirect(dbg);
		}
		if ( rc )	{
			rc = RecvSTATUS_Redirect(dbg);
		}
		if ( !rc )	{
			CloseDB_RedirectPort(dbg);
		}
	}
LEAVE_FUNC;
}

extern	void
CommitDB_Redirect(
	DBG_Instance	*dbg)
{
	Bool rc = TRUE;

ENTER_FUNC;
	if	( dbg->fpLog !=  NULL) {
		SendPacketClass(dbg->fpLog,RED_COMMIT);	ON_IO_ERROR(dbg->fpLog,badio);
		SendString(dbg->fpLog,dbg->env->id);
		if		(  RecvPacketClass(dbg->fpLog)  ==  RED_OK  ) {
			rc = RecvSTATUS_Redirect(dbg);
		} else {
			rc = -1;
			Warning("redirect commit fail.");
		}
		if ( !rc )	{
		  badio:
			dbg->update.dbstatus = DB_STATUS_REDFAILURE;
			CloseDB_RedirectPort(dbg);
		}
	}
LEAVE_FUNC;
}

