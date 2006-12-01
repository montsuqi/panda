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
	if		(  fpPanda  ==  NULL  ) {
		fprintf(stderr,"can't connect wfc\n");
		exit(1);
	}
LEAVE_FUNC;
}

static	Bool
SendPanda(void)
{
	RecordStruct	*rec;
	Bool		rc;
	char	buff[SIZE_LONGNAME+1];
	char	*p;
	ValueStruct	*value;

ENTER_FUNC;
	dbgprintf("ThisWindow = [%s]",ThisWindow);
	strcpy(buff,ThisWindow);
	rec = NULL;
	while	(  strlen(buff)  >  0  ) {
		if		(  ( rec = GetWindowRecord(buff) )  !=  NULL  )	break;
		if		(  ( p = strrchr(buff,'.') )  !=  NULL  ) {
			*p = 0;
		} else {
			rec = GetWindowRecord(buff);
			break;
		}
	}
	dbgprintf("Window = [%s]",buff);
	if		(  rec  !=  NULL  ) {
		dbgprintf("rec = [%s]",rec->name);
		value = rec->value;
	} else {
		value = NULL;
	}
	rc = SendTermServer(fpPanda,ThisTerm,ThisWindow,ThisWidget,ThisEvent,value);
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
	WindowData		*win;
	char	msg[SIZE_LONGNAME+1];
	void
	ClearPutType(
		char		*wname,
		WindowData	*win)
	{
		win->PutType = SCREEN_NULL;
	}

ENTER_FUNC;
	if		(  RecvTermServerHeader(fpPanda,user,window,widget,&type,&ctl)  ) {
		ON_IO_ERROR(fpPanda,badio);
		if		(  ThisScreen->Windows  !=  NULL  ) {
			g_hash_table_foreach(ThisScreen->Windows,(GHFunc)ClearPutType,NULL);
		}
		for	( i = 0 ; i < ctl.n ; i ++ ) {
			if		(  ctl.control[i].PutType  ==  SCREEN_CLOSE_WINDOW  ) {
				win = PutWindowByName(ctl.control[i].window,SCREEN_CLOSE_WINDOW);
			}
		}
		dbgprintf("type =     [%d]",type);
		dbgprintf("ThisWindow [%s]",ThisWindow);
		dbgprintf("window     [%s]",window);
		dbgprintf("user =     [%s]",user);
		switch	(type) {
		  case	SCREEN_CHANGE_WINDOW:
			win = PutWindowByName(ThisWindow,SCREEN_CLOSE_WINDOW);
			type = SCREEN_NEW_WINDOW;
			break;
		  case	SCREEN_JOIN_WINDOW:
			type = SCREEN_CURRENT_WINDOW;
			break;
		  default:
			break;
		}
		(void)SetWindowName(window);
		win = PutWindowByName(window,type);
		if		(  win  !=  NULL  )	{
			RecvTermServerData(fpPanda,ThisScreen);	ON_IO_ERROR(fpPanda,badio);
			strcpy(ThisWindow,window);
			strcpy(ThisWidget,widget);
			strcpy(ThisUser,user);
			if		(  win->rec  ==  NULL  ) {
				ThisScreen->status = APL_SESSION_END;
			}
		} else {
			ThisScreen->status = APL_SESSION_END;
			//Error("Illegal windowData");
		}
	} else {
		snprintf(msg,SIZE_LONGNAME,"window = [%s]",window);
		MessageLog(msg);
		Error("invalid LD");
	  badio:
		exit(1);
	}
LEAVE_FUNC;
}

extern	Bool
pandaLink(
	char		*arg)
{
	char	user[SIZE_NAME+1]	
	,		window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];

ENTER_FUNC;
	OpenPanda(arg);
	RecvPanda(user,window,widget);
	CloseNet(fpPanda);
LEAVE_FUNC;
	return	(TRUE);
}

extern	Bool
pandaMain(
	char		*arg)
{
	char	user[SIZE_NAME+1]	
	,		window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1];
	Port	*port;
	int		fd;
	Bool	ret;

ENTER_FUNC;
	port = ParPort(PandaPort,PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if ( fd > 0 ){
		fpPanda = SocketToNet(fd);
		if		(  SendPanda()  ) {
			RecvPanda(user,window,widget);
			CloseNet(fpPanda);
			ret = TRUE;
		} else {
			ret = FALSE;
		}
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
pandaExit(
	char		*arg)
{
	Port	*port;
	int		fd;
	Bool	ret;

ENTER_FUNC;
	port = ParPort(PandaPort,PORT_WFC);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	if ( fd > 0 ){
		fpPanda = SocketToNet(fd);
		SendTermServerEnd(fpPanda,ThisTerm);
		CloseNet(fpPanda);
		ret = TRUE;
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
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
