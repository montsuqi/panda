/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>

#include	"types.h"
#include	"const.h"
#include	"misc.h"

#include	"value.h"
#include	"directory.h"
#include	"LBSfunc.h"
#include	"comm.h"
#include	"wfc.h"
#include	"aps_main.h"
#include	"apsio.h"
#include	"handler.h"
#include	"tcp.h"
#include	"debug.h"

static	LargeByteString	*iobuff;

extern	int
InitServerPort(
	char	*port)
{	int		fh;

dbgmsg(">InitServerPort");
	fh = BindSocket(port,SOCK_STREAM);

	if		(  listen(fh,Back)  <  0  )	{
		shutdown(fh, 2);
		Error("INET Domain listen");
	}
dbgmsg("<InitServerPort");
	return	(fh);
}

extern	void
InitAPSIO(void)
{
	iobuff = NewLBS();
}

/*
 *	for APS
 */
extern	Bool
GetWFC(
	FILE	*fpWFC,
	ProcessNode	*node)
{
	int			i;
	PacketClass	c;
	Bool		fSuc;
	Bool		fEnd;
	MessageHeader	hdr;

dbgmsg(">GetWFC");
	fEnd = FALSE; 
	fSuc = FALSE;
	while	(  !fEnd  ) {
		switch	(c = RecvPacketClass(fpWFC)) {
		  case	APS_EVENTDATA:
			dbgmsg("EVENTDATA");
			hdr.status = RecvChar(fpWFC);
			RecvString(fpWFC,hdr.term);
			RecvString(fpWFC,hdr.user);
			RecvString(fpWFC,hdr.window);
			RecvString(fpWFC,hdr.widget);
			RecvString(fpWFC,hdr.event);
#ifdef	DEBUG
			printf("status = [%c]\n",hdr.status);
			printf("term   = [%s]\n",hdr.term);
			printf("user   = [%s]\n",hdr.user);
			printf("window = [%s]\n",hdr.window);
			printf("widget = [%s]\n",hdr.widget);
			printf("event  = [%s]\n",hdr.event);
#endif
			fSuc = TRUE;
			break;
		  case	APS_MCPDATA:
			dbgmsg("MCPDATA");
			RecvLBS(fpWFC,iobuff);
			UnPackValue(iobuff->body,node->mcprec);
			*ValueString(GetItemLongName(node->mcprec,"private.pstatus")) = hdr.status;
			strcpy(ValueString(GetItemLongName(node->mcprec,"dc.term")),hdr.term);
			strcpy(ValueString(GetItemLongName(node->mcprec,"dc.user")),hdr.user);
			strcpy(ValueString(GetItemLongName(node->mcprec,"dc.window")),hdr.window);
			strcpy(ValueString(GetItemLongName(node->mcprec,"dc.widget")),hdr.widget);
			strcpy(ValueString(GetItemLongName(node->mcprec,"dc.event")),hdr.event);
			break;
		  case	APS_LINKDATA:
			dbgmsg("LINKDATA");
			RecvLBS(fpWFC,iobuff);
			UnPackValue(iobuff->body,node->linkrec);
			break;
		  case	APS_SPADATA:
			dbgmsg("SPADATA");
			RecvLBS(fpWFC,iobuff);
			UnPackValue(iobuff->body,node->sparec);
			break;
		  case	APS_SCRDATA:
			dbgmsg("SCRDATA");
			for	( i = 0 ; i < node->cWindow ; i ++ ) {
				RecvLBS(fpWFC,iobuff);
				UnPackValue(iobuff->body,node->scrrec[i]);
			}
			break;
		  case	APS_END:
			dbgmsg("END");
			*ValueString(GetItemLongName(node->mcprec,"private.prc")) = TO_CHAR(0);
			fEnd = TRUE;
			break;
		  case	APS_STOP:
			dbgmsg("STOP");
			fSuc = FALSE;
			fEnd = TRUE;
			break;
		  case	APS_PING:
			dbgmsg("PING");
			SendPacketClass(fpWFC,APS_PONG);
			break;
		  default:
			dbgmsg("default");
			SendPacketClass(fpWFC,APS_NOT);
			fSuc = FALSE;
			fEnd = TRUE;
			break;
		}
	}
dbgmsg("<GetWFC");
	return	(fSuc); 
}

extern	void
PutWFC(
	FILE	*fpWFC,
	ProcessNode	*node)
{
	int				i;
	PacketClass		c;
	Bool			fEnd;

dbgmsg(">PutWFC");
	SendPacketClass(fpWFC,APS_CTRLDATA);
	SendString(fpWFC,ValueString(GetItemLongName(node->mcprec,"dc.term")));
	SendString(fpWFC,ValueString(GetItemLongName(node->mcprec,"dc.window")));
	SendString(fpWFC,ValueString(GetItemLongName(node->mcprec,"dc.widget")));
	SendChar(fpWFC,*ValueString(GetItemLongName(node->mcprec,"private.pputtype")));
	fEnd = FALSE; 
	while	(  !fEnd  ) {
		switch	(c = RecvPacketClass(fpWFC)) {
		  case	APS_CLSWIN:
			dbgmsg("CLSWIN");
			SendInt(fpWFC,node->w.n);
			for	( i = 0 ; i < node->w.n ; i ++ ) {
				SendString(fpWFC,node->w.close[i].window);
			}
			break;
		  case	APS_MCPDATA:
			dbgmsg("MCPDATA");
			LBS_RequireSize(iobuff,SizeValue(node->mcprec,0,0),FALSE);
			PackValue(iobuff->body,node->mcprec);
			SendLBS(fpWFC,iobuff);
			break;
		  case	APS_LINKDATA:
			dbgmsg("LINKDATA");
			LBS_RequireSize(iobuff,SizeValue(node->linkrec,0,0),FALSE);
			PackValue(iobuff->body,node->linkrec);
			SendLBS(fpWFC,iobuff);
			break;
		  case	APS_SPADATA:
			dbgmsg("SPADATA");
			LBS_RequireSize(iobuff,SizeValue(node->sparec,0,0),FALSE);
			PackValue(iobuff->body,node->sparec);
			SendLBS(fpWFC,iobuff);
			break;
		  case	APS_SCRDATA:
			dbgmsg("SCRDATA");
			for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
				LBS_RequireSize(iobuff,
								SizeValue(node->scrrec[i],0,0),FALSE);
				PackValue(iobuff->body,node->scrrec[i]);
				SendLBS(fpWFC,iobuff);
			}
			break;
		  case	APS_END:
			dbgmsg("END");
			fEnd = TRUE;
			break;
		  case	APS_PING:
			dbgmsg("PING");
			SendPacketClass(fpWFC,APS_PONG);
			break;
		  default:
			dbgmsg("default");
			SendPacketClass(fpWFC,APS_NOT);
			fEnd = TRUE;
			break;
		}
	}
	fflush(fpWFC);
dbgmsg("<PutWFC");
}

