/*
PANDA -- a simple transaction monitor
Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
		dbg->checkData = NULL;
	} else {
		rdbg = dbg->redirect;
		if		( ( rdbg->redirectPort  ==  NULL ) 
		 || (( fh = ConnectSocket(rdbg->redirectPort,SOCK_STREAM) )  <  0 ) ) {
			Warning("loging server not ready");
			dbg->fpLog = NULL;
			dbg->redirectData = NULL;
			dbg->checkData = NULL;
			dbg->dbstatus = REDFAILURE;
		} else {
			dbg->fpLog = SocketToNet(fh);
			dbg->redirectData = NewLBS();
			dbg->checkData = NewLBS();
			SendPacketClass(dbg->fpLog, RED_STATUS);
			dbg->dbstatus = RecvChar(dbg->fpLog);
		}
	}
LEAVE_FUNC;
}

extern	void
CloseDB_RedirectPort(
	DBG_Struct	*dbg)
{
ENTER_FUNC;
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_END);
		CloseNet(dbg->fpLog);
	}
	if		(  dbg->redirectData  !=  NULL  ) {
		FreeLBS(dbg->redirectData);
		dbg->redirectData = NULL;
		FreeLBS(dbg->checkData);
		dbg->checkData = NULL;
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
	if		(  dbg->checkData  !=  NULL  ) {
		LBS_EmitString(dbg->checkData,data);
	}
LEAVE_FUNC;
}

extern	void
BeginDB_Redirect(
	DBG_Struct	*dbg)
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
	DBG_Struct	*dbg)
{
	Bool	rc = TRUE;
ENTER_FUNC;
	if		(  dbg->redirectData  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_PING);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_PONG  ) {
			Warning("log server down?");
			dbg->dbstatus = REDFAILURE;
			CloseDB_RedirectPort(dbg);
			rc = FALSE;
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
	if	(  dbg->redirectData  !=  NULL  ) {
		LBS_EmitStart(dbg->redirectData);
		LBS_EmitStart(dbg->checkData);
	}
LEAVE_FUNC;
}

extern	void
CommitDB_Redirect(
	DBG_Struct	*dbg)
{
ENTER_FUNC;
	if		(  dbg->redirectData  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_CHECK);
		SendLBS(dbg->fpLog,dbg->checkData);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_OK  ) {
			CloseDB_RedirectPort(dbg);
		} 
		SendPacketClass(dbg->fpLog,RED_DATA);
		SendLBS(dbg->fpLog,dbg->redirectData);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_OK  ) {
			CloseDB_RedirectPort(dbg);
		}
		SendPacketClass(dbg->fpLog, RED_STATUS);
		dbg->dbstatus = RecvChar(dbg->fpLog);
	}
LEAVE_FUNC;
}

