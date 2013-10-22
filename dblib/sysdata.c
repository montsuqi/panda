/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan.
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
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<signal.h>

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"dbgroup.h"
#include	"sysdata.h"
#include	"comm.h"
#include	"comms.h"
#include	"sysdatacom.h"
#include	"redirect.h"
#include	"debug.h"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->process[PROCESS_UPDATE].conn)

extern	ValueStruct	*
SYSDATA_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int		fh
		,	rc;
	NETFILE	*fp;

ENTER_FUNC;
	fp = NULL;
	rc = MCP_BAD_OTHER;
	if (ThisEnv->sysdata != NULL &&
			ThisEnv->sysdata->port != NULL &&
			(fh = ConnectSocket(ThisEnv->sysdata->port,SOCK_STREAM)) >= 0) {
		fp = SocketToNet(fh);
	}
	OpenDB_RedirectPort(dbg);
	dbg->process[PROCESS_UPDATE].conn = (void *)fp;
	if		(  fp  !=  NULL  ) {
		dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;
		dbg->process[PROCESS_READONLY].dbstatus = DB_STATUS_NOCONNECT;
		rc = MCP_OK;
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
SYSDATA_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  dbg->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT ) { 
		SendPacketClass(NBCONN(dbg), SYSDATA_END);
		CloseNet(NBCONN(dbg));
		CloseDB_RedirectPort(dbg);
		dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_DISCONNECT;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
SYSDATA_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	BeginDB_Redirect(dbg); 
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
SYSDATA_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	CheckDB_Redirect(dbg);
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}
