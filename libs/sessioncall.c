/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	<signal.h>
#include	<dlfcn.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>

#include	"const.h"
#include	"net.h"
#include	"defaults.h"
#include	"socket.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"load.h"
#include	"sessioncall.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"front.h"
#include	"driver.h"
#include	"debug.h"

static	char	*TermPort = NULL;

static	NETFILE	*
OpenPanda(
	ScreenData	*scr,
	char		*arg)
{
	NETFILE	*fp;

ENTER_FUNC;
	fp = ConnectTermServer(TermPort,scr,arg);
	if		(  fp  ==  NULL  ) {
		fprintf(stderr,"can't connect wfc\n");
		exit(1);
	}
LEAVE_FUNC;
	return	(fp);
}

static	Bool
SendPanda(
	ScreenData	*scr,
	NETFILE		*fp)
{
	Bool		rc;
	ValueStruct	*value;
ENTER_FUNC;
	dbgprintf("ThisWindow = [%s]",scr->window);
	value = GetWindowValue(scr,scr->window);
	if (value == NULL) {
		Error("SendPanda invalid window[%s]",scr->window);
	}
#ifdef	DEBUG
	DumpValueStruct(value);
#endif
	rc = SendTermServer(fp,scr,value);
LEAVE_FUNC;
	return	rc; 
}


static	void
ClearPutType(
	char		*wname,
	WindowData	*win)
{
	win->PutType = SCREEN_NULL;
}

static	void
RecvPanda(
	ScreenData	*scr,
	NETFILE		*fp)
{
	char			old_window[SIZE_NAME+1];
	int				type;
	int				i;
	WindowControl	ctl;
ENTER_FUNC;
	strncpy(old_window,scr->window,SIZE_NAME);
	old_window[SIZE_NAME] = 0;
	if (RecvTermServerHeader(fp,scr,&type,&ctl)) {
		ON_IO_ERROR(fp,badio);
		if (scr->Windows != NULL) {
			g_hash_table_foreach(scr->Windows,(GHFunc)ClearPutType,NULL);
		}
		for	(i = 0 ; i < ctl.n ; i ++ ) {
			if (ctl.control[i].PutType == SCREEN_CLOSE_WINDOW) {
				PutWindow(scr,ctl.control[i].window,SCREEN_CLOSE_WINDOW);
			}
		}
		dbgprintf("type =     [%d]",type);
		dbgprintf("ThisWindow [%s]",old_window);
		dbgprintf("window     [%s]",scr->window);
		dbgprintf("user =     [%s]",scr->user);
		switch	(type) {
		  case	SCREEN_CHANGE_WINDOW:
			PutWindow(scr,old_window,SCREEN_CLOSE_WINDOW);
			type = SCREEN_NEW_WINDOW;
			break;
		  case	SCREEN_JOIN_WINDOW:
			type = SCREEN_CURRENT_WINDOW;
			break;
		  default:
			break;
		}
		if (RegisterWindow(scr,scr->window) != NULL) {
			PutWindow(scr,scr->window,type);
			RecvTermServerData(fp,scr);	ON_IO_ERROR(fp,badio);
		} else {
			scr->status = APL_SESSION_NULL; /*glserver exit*/
		}
	} else {
		Error("invalid LD; window = [%s]",scr->window);
	}
LEAVE_FUNC;
	return;
badio:
	exit(1);
}

void
SessionCallInit(
	const char *port)
{
	TermPort = (char*)port;
	if (TermPort == NULL) {
		TermPort = getenv("WFC_TERM_PORT");
	}
	if (TermPort == NULL) {
		TermPort = "/tmp/wfc.term";
	}
}

void
SessionLink(
	ScreenData 	*scr)
{
	NETFILE	*fp;
	char	*p;

ENTER_FUNC;
	/* extract LDNAME -> 'panda:<LDNAME>' */
	p = strchr(scr->cmd,':');
	if (p == NULL){
		p = scr->cmd;
	} else {
		p++;
	}
	while (*p && isspace(*p)) p++;

	fp = OpenPanda(scr,p);
	RecvPanda(scr,fp);
	CloseNet(fp);
	scr->fConnect = TRUE;
LEAVE_FUNC;
}

void
SessionMain(
	ScreenData 	*scr)
{
	Port	*port;
	int		fd;
	NETFILE	*fp;

ENTER_FUNC;
	port = ParPort(TermPort,PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if ( fd > 0 ){
		fp = SocketToNet(fd);
		if (SendPanda(scr,fp)) {
			RecvPanda(scr,fp);
			CloseNet(fp);
		} else {
			scr->status = APL_SESSION_RESEND;
		}
	} else {
		scr->status = APL_SESSION_RESEND;
	}
LEAVE_FUNC;
}

void
SessionExit(
	ScreenData 	*scr)
{
	Port	*port;
	int		fd;
	NETFILE	*fp;

ENTER_FUNC;
	port = ParPort(TermPort,PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if ( fd > 0 ){
		fp = SocketToNet(fd);
		SendTermServerEnd(fp,scr);
	} else {
		scr->status = APL_SESSION_NULL;
	}
LEAVE_FUNC;
}
