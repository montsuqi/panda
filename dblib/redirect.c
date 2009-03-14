/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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
	if		(  dbg->fpLog  !=  NULL  ) {					
		SendPacketClass(dbg->fpLog,RED_DATA);	ON_IO_ERROR(dbg->fpLog,badio);
		SendLBS(dbg->fpLog,dbg->redirectData);	ON_IO_ERROR(dbg->fpLog,badio);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_OK  ) {
			Warning("Redirect Commit error");			
		}
	}
	rc = TRUE;		
badio:
	return rc;
}
#endif

static Bool
SendCommit_Redirect(
	DBG_Struct	*dbg)
{
	int rc = FALSE;
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_COMMIT);	ON_IO_ERROR(dbg->fpLog,badio);
		SendUInt64(dbg->fpLog, dbg->ticket_id); ON_IO_ERROR(dbg->fpLog,badio);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_OK  ) {		
			Warning("Redirect Commit error");			
		}
	}
	rc = TRUE;	
badio:
	return rc;
}

static Bool
SendVeryfyData_Redirect(
	DBG_Struct	*dbg)
{
	int rc = FALSE;
	if	( (dbg->fpLog  !=  NULL)
		  && ( dbg->redirectData !=  NULL)
		  && ( LBS_Size(dbg->redirectData) > 0 ) ) {
		SendPacketClass(dbg->fpLog,RED_DATA);	ON_IO_ERROR(dbg->fpLog,badio);
		SendUInt64(dbg->fpLog, dbg->ticket_id);	ON_IO_ERROR(dbg->fpLog,badio);
		SendLBS(dbg->fpLog,dbg->checkData);		ON_IO_ERROR(dbg->fpLog,badio);
		SendLBS(dbg->fpLog,dbg->redirectData);	ON_IO_ERROR(dbg->fpLog,badio);
	}
	rc = SendCommit_Redirect(dbg);
badio:
	return rc;
}

static void
ChangeDBStatus_Redirect(
	DBG_Struct	*dbg,
	int dbstatus)
{
	DBG_Struct	*rdbg;
	if ( dbg->redirect != NULL ){
		rdbg = dbg->redirect;
		rdbg->process[PROCESS_UPDATE].dbstatus = dbstatus;
	}
}

static Bool
RecvSTATUS_Redirect(
	DBG_Struct	*dbg)
{
	int dbstatus;
	int rc = FALSE;
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog, RED_STATUS);ON_IO_ERROR(dbg->fpLog,badio);
		dbstatus = RecvChar(dbg->fpLog);	ON_IO_ERROR(dbg->fpLog,badio);		
		ChangeDBStatus_Redirect(dbg, dbstatus);
	}
	rc = TRUE;
badio:
	return rc;
}

extern	void
OpenDB_RedirectPort(
	DBG_Struct	*dbg)
{
	int		fh;
	DBG_Struct	*rdbg;

ENTER_FUNC;
	dbgprintf("dbg [%s]\n",dbg->name);
	if		(	(  fNoRedirect  )
			||	(  dbg->redirect  ==  NULL  ) ) {
		dbg->fpLog = NULL;
		dbg->redirectData = NULL;
	} else {
		rdbg = dbg->redirect;
		if		( ( rdbg->redirectPort  ==  NULL ) 
		 || (( fh = ConnectSocket(rdbg->redirectPort,SOCK_STREAM) )  <  0 ) ) {
			Warning("loging server not ready");
			dbg->fpLog = NULL;
			dbg->redirectData = NULL;
			if ( !fNoCheck ){
				ChangeDBStatus_Redirect(dbg, DB_STATUS_REDFAILURE);
			}
		} else {
			dbg->fpLog = SocketToNet(fh);
			dbg->redirectData = NewLBS();
			if ( !RecvSTATUS_Redirect(dbg) ){
				CloseDB_RedirectPort(dbg);
			}
		}
	}
LEAVE_FUNC;
}

extern	void
CloseDB_RedirectPort(
	DBG_Struct	*dbg)
{
ENTER_FUNC;
	if (  dbg->redirect == NULL )
		return;
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_END);
		CloseNet(dbg->fpLog);
		dbg->fpLog = NULL;
	}
	if		(  dbg->redirectData  !=  NULL  ) {
		FreeLBS(dbg->redirectData);
		dbg->redirectData = NULL;
	}
LEAVE_FUNC;
}

extern	void
PutDB_Redirect(
	DBG_Struct	*dbg,
	char		*data)
{
ENTER_FUNC;
	if		(  dbg->redirectData  !=  NULL  ) {
		LBS_EmitString(dbg->redirectData,data);
	}
LEAVE_FUNC;
}

extern	void
PutCheckDataDB_Redirect(
	DBG_Struct	*dbg,
	char		*data)
{
ENTER_FUNC;
	LBS_EmitString(dbg->checkData,data);
LEAVE_FUNC;
}

extern	void
BeginDB_Redirect(
	DBG_Struct	*dbg)
{
ENTER_FUNC;
	if (  dbg->redirect == NULL )
		return;

	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_BEGIN);
		dbg->ticket_id = RecvUInt64(dbg->fpLog);
	}
	if		(  dbg->redirectData  !=  NULL  ) { 
		LBS_EmitStart(dbg->redirectData);
		LBS_EmitStart(dbg->checkData);
	}
LEAVE_FUNC;
}

extern	Bool
CheckDB_Redirect(
	DBG_Struct	*dbg)
{
	Bool	rc = TRUE;
ENTER_FUNC;
	if (  dbg->redirect == NULL )
		return rc;
	if		(  dbg->redirectData  !=  NULL  ) {
		if		(  dbg->fpLog  !=  NULL  ) {				
			SendPacketClass(dbg->fpLog,RED_PING);
			if		(  RecvPacketClass(dbg->fpLog)  !=  RED_PONG  ) {
				Warning("log server down?");
				ChangeDBStatus_Redirect(dbg, DB_STATUS_REDFAILURE);
				CloseDB_RedirectPort(dbg);
				rc = FALSE;
			}
		}
	}
LEAVE_FUNC;
	return	(rc);
}

extern	void
AbortDB_Redirect(
	DBG_Struct	*dbg)
{
ENTER_FUNC;
	if (  dbg->redirect == NULL )
		return;
	
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_ABORT);	ON_IO_ERROR(dbg->fpLog,badio);
		SendUInt64(dbg->fpLog, dbg->ticket_id);	ON_IO_ERROR(dbg->fpLog,badio);
	}
	if	(  dbg->redirectData  !=  NULL  ) {
		LBS_EmitStart(dbg->redirectData);
		LBS_EmitStart(dbg->checkData);
	}
badio:
LEAVE_FUNC;
}

extern	void
CommitDB_Redirect(
	DBG_Struct	*dbg)
{
	Bool rc = TRUE;

ENTER_FUNC;
	if (  dbg->redirect == NULL )
		return;
	rc = SendVeryfyData_Redirect(dbg);
	if ( rc ){
		rc = RecvSTATUS_Redirect(dbg);
	}
	if ( !rc ){
		CloseDB_RedirectPort(dbg);
	}
LEAVE_FUNC;
}

