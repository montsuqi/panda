/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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

dbgmsg(">OpenDB_RedirectPort");
	if		(	(  fNoRedirect  )
			||	(  dbg->redirectPort  ==  NULL  ) ) {
		dbg->fpLog = NULL;
		dbg->redirectData = NULL;
	} else {
		if		(  ( fh = ConnectSocket(dbg->redirectPort,SOCK_STREAM) )  <  0  ) {
			Warning("loging server not ready");
			dbg->fpLog = NULL;
			dbg->redirectData = NULL;
			if		(  !fNoCheck  ) {
				exit(1);
			}
		} else {
			dbg->fpLog = SocketToNet(fh);
			dbg->redirectData = NewLBS();
		}
	}
dbgmsg("<OpenDB_RedirectPort");
}

extern	void
CloseDB_RedirectPort(
	DBG_Struct	*dbg)
{
dbgmsg(">CloseDB_RedirectPort");
	if		(  dbg->fpLog  !=  NULL  ) {
		CloseNet(dbg->fpLog);
		FreeLBS(dbg->redirectData);
	}
dbgmsg("<CloseDB_RedirectPort");
}

extern	void
PutDB_Redirect(
	DBG_Struct	*dbg,
	char		*data)
{
dbgmsg(">PutDB_Redirect");
	if		(  dbg->redirectData  !=  NULL  ) {
		LBS_EmitString(dbg->redirectData,data);
		LBS_EmitString(dbg->redirectData,";"); 
	}
dbgmsg("<PutDB_Redirect");
}

extern	void
BeginDB_Redirect(
	DBG_Struct	*dbg)
{
dbgmsg(">BeginDB_Redirect");
	if		(  dbg->redirectData  !=  NULL  ) { 
		LBS_EmitStart(dbg->redirectData);
	}
dbgmsg("<BeginDB_Redirect");
}

extern	Bool
CheckDB_Redirect(
	DBG_Struct	*dbg)
{
	Bool	rc;

	if		(  dbg->redirectData  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_PING);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_PONG  ) {
			Warning("log server down?");
			if		(  !fNoCheck  ) {
				exit(1);
			}
			CloseNet(dbg->fpLog);
			dbg->fpLog = NULL;
			FreeLBS(dbg->redirectData);
			dbg->redirectData = NULL;
			rc = FALSE;
		} else {
			rc = TRUE;
		}
	} else {
		rc = TRUE;
	}
	return	(rc);
}

extern	void
CommitDB_Redirect(
	DBG_Struct	*dbg)
{
dbgmsg(">CommitDB_Redirect");
	if		(  dbg->redirectData  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_DATA);
		SendLBS(dbg->fpLog,dbg->redirectData);
		if		(  RecvPacketClass(dbg->fpLog)  !=  RED_OK  ) {
			Warning("log server down?");
			if		(  !fNoCheck  ) {
				exit(1);
			}
			CloseNet(dbg->fpLog);
			dbg->fpLog = NULL;
			FreeLBS(dbg->redirectData);
			dbg->redirectData = NULL;
		}
	}
dbgmsg("<CommitDB_Redirect");
}
