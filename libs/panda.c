/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
	char	msg[SIZE_LONGNAME+1];

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
		dbgprintf("user =     [%s]",user);
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
		snprintf(msg,SIZE_LONGNAME,"window = [%s]",window);
		MessageLog(msg);
		Error("invalid LD");
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
