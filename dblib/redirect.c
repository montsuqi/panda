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
	DBG_Struct	*rdbg;

dbgmsg(">OpenDB_RedirectPort");
	dbgprintf("dbg [%s]\n",dbg->name);
	if		(	(  fNoRedirect  )
			||	(  dbg->redirect  ==  NULL  ) ) {
		dbg->fpLog = NULL;
		dbg->redirectData = NULL;
	} else {
		rdbg = dbg->redirect;
		dbgprintf("redirect [%s] -> [%s]\n",dbg->name,rdbg->name);
		if		(  ( fh = ConnectIP_Socket(rdbg->redirectPort->port,SOCK_STREAM,
										rdbg->redirectPort->host) )
				   <  0  ) {
			Warning("loging server not ready");
			if		(  !fNoCheck  ) {
				exit(1);
			}
			dbg->fpLog = NULL;
			dbg->redirectData = NULL;
		} else {
			dbg->fpLog = SocketToNet(fh);
			dbg->redirectData = NewLBS();
		}
	}
	dbg->fConnect = FALSE;
dbgmsg("<OpenDB_RedirectPort");
}

extern	void
CloseDB_RedirectPort(
	DBG_Struct	*dbg)
{
dbgmsg(">CloseDB_RedirectPort");
	if		(  dbg->fpLog  !=  NULL  ) {
		SendPacketClass(dbg->fpLog,RED_END);
		CloseNet(dbg->fpLog);
	}
	if		(  dbg->redirectData  !=  NULL  ) {
		FreeLBS(dbg->redirectData);
		dbg->redirectData = NULL;
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
	int		i;

	if		(  dbg->redirectData  !=  NULL  ) {
		i = 0;
		do {
			SendPacketClass(dbg->fpLog,RED_PING);
			if		(  RecvPacketClass(dbg->fpLog)  !=  RED_PONG  ) {
				Warning("log server down?");
				CloseDB_RedirectPort(dbg);
				sleep(RetryInterval);
				OpenDB_RedirectPort(dbg);
				rc = FALSE;
				i ++;
			} else {
				rc = TRUE;
			}
		}	while	(	(  rc  ==  FALSE  )
					&&	(  i  <  MaxRetry  ) );
		if		(	(  !rc  )
				&&	(  !fNoCheck  ) ) {
			exit(1);
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
