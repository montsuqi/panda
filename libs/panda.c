/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	"net.h"
#include	"libmondai.h"
#include	"applications.h"
#include	"DDparser.h"
#include	"wfc.h"
#include	"wfcio.h"
#include	"driver.h"
#include	"misc.h"
#include	"debug.h"

static	NETFILE	*fpPanda;
static	char	*PandaPort;

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

extern	void
pandaLink(
	char		*arg)
{
	char	window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];

dbgmsg(">pandaLink");
	OpenPanda(arg);
	RecvPanda(window,widget);
dbgmsg("<pandaLink");
}

extern	void
pandaMain(
	char		*arg)
{
	char	window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];

dbgmsg(">pandaMain");
	if		(  SendPanda()  ) {
		RecvPanda(window,widget);
	} else {
		CloseNet(fpPanda);
		exit(0);
	}
dbgmsg("<pandaMain");
}

#include	"option.h"
static	ARG_TABLE	option[] = {
	{	"panda",	STRING,		TRUE,	(void*)&PandaPort,
		"wfc待機ポート番号"								},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault()
{
	PandaPort = "localhost:9000";			/*	PORT_WFC	*/
}
extern	void
pandaInit(
	int		argc,
	char	**argv)
{
dbgmsg(">pandaInit");
	SetDefault();
	(void)GetOption(option,argc,argv);
dbgmsg("<pandaInit");
}
