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

/*
#define	NEW_SEQUENCE
*/

#define	_PANDA

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<string.h>
#include	"types.h"
#include	"const.h"
#include	"net.h"
#include	"libmondai.h"
#include	"applications.h"
#include	"DDparser.h"
#include	"wfcdata.h"
#include	"wfcio.h"
#include	"front.h"
#include	"driver.h"
#include	"debug.h"

static	NETFILE	*fpPanda;
static	char	*PandaPort;

static	void
OpenPanda(
	char	*arg)
{
ENTER_FUNC;
	fpPanda = ConnectTermServer(PandaPort,ThisTerm,ThisUser,TRUE,arg); 
LEAVE_FUNC;
}

static	Bool
SendPanda(void)
{
	RecordStruct	*rec;
	Bool		rc;

ENTER_FUNC;
	rec = GetWindowRecord(ThisWindow);
	rc = SendTermServer(fpPanda,ThisWindow,ThisWidget,ThisEvent,rec->value);
LEAVE_FUNC;
	return	(rc); 
}


static	void
RecvPanda(
	char	*user,
	char	*window,
	char	*widget)
{
	int		type;
	int		i;
	WindowControl	ctl;

ENTER_FUNC;
	if		(  RecvTermServerHeader(fpPanda,user,window,widget,&type,&ctl)  ) {
#ifdef	NEW_SEQUENCE
		for	( i = 0 ; i < ctl.n ; i ++ ) {
			type = ctl.control[i].PutType;
			switch	(type) {
			  case	SCREEN_CHANGE_WINDOW:
				(void)PutWindowByName(ThisWindow,SCREEN_CLOSE_WINDOW);
				type = SCREEN_NEW_WINDOW;
				break;
			  default:
				break;
			}
			PutWindowByName(ctl.control[i].window,type);
		}
#else
		for	( i = 0 ; i < ctl.n ; i ++ ) {
			if		(  ctl.control[i].PutType  ==  SCREEN_CLOSE_WINDOW  ) {
				PutWindowByName(ctl.control[i].window,SCREEN_CLOSE_WINDOW);
			}
		}
#endif
		dbgprintf("type =     [%d]",type);
		dbgprintf("ThisWindow [%s]",ThisWindow);
		dbgprintf("window     [%s]",window);
#ifndef	NEW_SEQUENCE
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
		(void)SetWindowName(window);
		(void)PutWindowByName(window,type);
#endif
		RecvTermServerData(fpPanda,ThisScreen);
		strcpy(ThisWindow,window);
		strcpy(ThisWidget,widget);
		strcpy(ThisUser,user);
	} else {
		MessagePrintf("invalid window [%s]\n",window);
		exit(1);
	}
LEAVE_FUNC;
}

extern	void
pandaLink(
	char		*arg)
{
	char	user[SIZE_NAME+1]	
	,		window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];

ENTER_FUNC;
	OpenPanda(arg);
	RecvPanda(user,window,widget);
LEAVE_FUNC;
}

extern	void
pandaMain(
	char		*arg)
{
	char	user[SIZE_NAME+1]	
	,		window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];

ENTER_FUNC;
	if		(  SendPanda()  ) {
		RecvPanda(user,window,widget);
	} else {
		CloseNet(fpPanda);
		exit(0);
	}
LEAVE_FUNC;
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
ENTER_FUNC;
	SetDefault();
	(void)GetOption(option,argc,argv);
LEAVE_FUNC;
}
