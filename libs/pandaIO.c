/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

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

#define	_PANDA

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<string.h>
#include	"types.h"
#include	"const.h"
#include	"tcp.h"
#include	"value.h"
#include	"applications.h"
#include	"DDparser.h"
#include	"wfc.h"
#include	"wfcio.h"
#include	"pandaIO.h"
#include	"driver.h"
#include	"misc.h"
#include	"debug.h"

static	FILE	*fpPanda;

static	void
OpenPanda(
	char	*arg)
{
dbgmsg(">OpenPanda");
	fpPanda = ConnectTermServer(PandaPort,ThisTerm,ThisUser,TRUE,arg); 
dbgmsg("<OpenPanda");
}

static	Bool
SendPanda(void)
{
	ValueStruct	*value;
	Bool		rc;

dbgmsg(">SendPanda");
	value = GetWindowRecord(ThisWindow);
	rc = SendTermServer(fpPanda,ThisWindow,ThisWidget,ThisEvent,value);
dbgmsg("<SendPanda");
	return	(rc); 
}


static	void
RecvPanda(
	char	*window,
	char	*widget)
{
	int		type;
	WindowData	*win;
	int		i;
	CloseWindows	cls;

dbgmsg(">RecvPanda");
	if		(  RecvTermServerHeader(fpPanda,window,widget,&type,&cls)  ) {
		for	( i = 0 ; i < cls.n ; i ++ ) {
			PutWindowByName(cls.close[i].window,SCREEN_CLOSE_WINDOW);
		}
		win = SetWindowName(window);
		RecvTermServerData(fpPanda,win);
#ifdef	TRACE
		printf("type =     [%d]\n",type);
		printf("ThisWindow [%s]\n",ThisWindow);
		printf("window     [%s]\n",window);
#endif
		switch	(type) {
		  case	SCREEN_CHANGE_WINDOW:
			(void)PutWindowByName(ThisWindow,SCREEN_CLOSE_WINDOW);
			type = SCREEN_NEW_WINDOW;
			break;
		  case	SCREEN_JOIN_WINDOW:
			type = SCREEN_CURRENT_WINDOW;
			break;
		  default:
			break;
		}
		PutWindow(win,type);
		strcpy(ThisWindow,window);
		strcpy(ThisWidget,widget);
	} else {
		fprintf(stderr,"invalid window [%s]\n",window);
		exit(1);
	}
dbgmsg("<RecvPanda");
}

static	void
PandaIO_Link(
	char		*arg)
{
	char	window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];

dbgmsg(">PandaIO_Link");
	OpenPanda(arg);
	RecvPanda(window,widget);
dbgmsg("<PandaIO_Link");
}

static	void
PandaIO_Main(
	char		*arg)
{
	char	window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];

dbgmsg(">PandaIO_Main");
	if		(  SendPanda()  ) {
		RecvPanda(window,widget);
	} else {
		fclose(fpPanda);
		exit(0);
	}
dbgmsg("<PandaIO_Main");
}

extern	void
PandaIO_Init(void)
{
dbgmsg(">PandaIO_Init");
	ApplicationsRegist("panda",PandaIO_Link,PandaIO_Main);
	DD_ParserInit();
dbgmsg("<PandaIO_Init");
}
