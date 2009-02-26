/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2005 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2006-2009 Ogochan & JMA (Japan Medical Association)
 *                                 & JFBA (Japan Federation of Bar Association)
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
*/
#define	DEBUG
#define	TRACE

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

static	char	*PandaPort;

static	NETFILE	*
OpenPanda(void)
{
	NETFILE	*fp;

ENTER_FUNC;
	if		(  ( fp = TermConnectServer(PandaPort) )  !=  NULL  ) {
		TermCreateSession(fp,ThisTerm,ThisUser,ThisLang,TRUE);
	} else {
		fprintf(stderr,"can't connect wfc\n");
		exit(1);
	}
LEAVE_FUNC;
	return	(fp);
}

static	Bool
SendPanda(
	NETFILE	*fp)
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
#ifdef	DEBUG
	DumpValueStruct(value);
#endif
	rc = FALSE;
	dbgprintf("ThisTerm = [%s]",ThisTerm);
	if		(  TermSendID(fp,ThisTerm)  ) {
		if		(  !TermSendStart(fp)  )	goto	quit;
		if		(  !TermSendDataHeader(fp,ThisLang,ThisWindow,ThisWidget,ThisEvent)  )	goto	quit;
		if		(  !TermSendDataBody(fp,ThisWindow,value)  )	goto	quit;
		rc = TRUE;
	}
  quit:;
LEAVE_FUNC;
	return	(rc); 
}


static	void
ClearPutType(
	char		*wname,
	WindowData	*win)
{
	win->PutType = SCREEN_NULL;
}

static	Bool
RecvPanda(
	NETFILE	*fp,
	char	*user,
	char	*window,
	char	*widget,
	char	*lang)
{
	int		type;
	int		i;
	WindowControl	ctl;
	WindowData		*win;
	char	msg[SIZE_LONGNAME+1];
	Bool	rc;

ENTER_FUNC;
	rc = FALSE;
	if		(  TermRequestHeader(fp,user,lang,window,widget,&type,&ctl)  ) {
		ON_IO_ERROR(fp,badio);
		if		(  ThisScreen->Windows  !=  NULL  ) {
			g_hash_table_foreach(ThisScreen->Windows,(GHFunc)ClearPutType,NULL);
		}
		for	( i = 0 ; i < ctl.n ; i ++ ) {
			(void)PutWindowByName(ctl.control[i].window,ctl.control[i].PutType);
		}
		dbgprintf("type =     [%d]",type);
		dbgprintf("ThisWindow [%s]",ThisWindow);
		dbgprintf("window     [%s]",window);
		dbgprintf("user =     [%s]",user);
		dbgprintf("lang =     [%s]",lang);
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
		win = PutWindowByName(window,type);
		if		(  win  !=  NULL  )	{
			TermRecvServerData(fp,ThisScreen);	ON_IO_ERROR(fp,badio);
			strcpy(ThisWindow,window);
			strcpy(ThisWidget,widget);
			strcpy(ThisUser,user);
			if		(  *lang  !=  0  ) {
				strcpy(ThisLang,lang);
			}
			if		(  win->rec  ==  NULL  ) {
				ThisScreen->status = APL_SESSION_END;
			}
		} else {
			ThisScreen->status = APL_SESSION_END;
			//Error("Illegal windowData");
		}
		rc = TRUE;
	} else {
		snprintf(msg,SIZE_LONGNAME,"window = [%s]",window);
		MessageLog(msg);
		Error("invalid LD");
	  badio:
		exit(1);
	}
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
pandaLink(
	char		*arg)
{
	RecordStruct	*rec;
	Bool	ret;
	NETFILE	*fp;

ENTER_FUNC;
	ret = FALSE;
	if		(  ( fp = OpenPanda() )  !=  NULL  ) {
		if		(  !TermSendStart(fp)  )	goto	quit;
		if		(  !TermSwitchLD(fp,arg)  )	goto	quit;
		if		(  ( rec = GetWindowRecord(ThisWindow) )  !=  NULL  ) {
			TermSendDataBody(fp,ThisWindow,rec->value);
		}
		if		(  !TermSendExec(fp) )	goto	quit;
		if		(  !RecvPanda(fp,ThisUser,ThisWindow,ThisWidget,ThisLang)  )	goto	quit;
		if		(  !TermSendPrepare(fp)  )	goto	quit;
		if		(  !TermSendCommit(fp)  )	goto	quit;
		if		(  !TermBreakSession(fp)  )	goto	quit;
		ret = TRUE;
	}
  quit:;
	CloseNet(fp);
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
pandaMain(
	char		*arg)
{
	Bool	ret;
	NETFILE	*fp;

ENTER_FUNC;
	ret = FALSE;
	dbgprintf("pandaMain(%s)",arg);
	if		(  ( fp = TermConnectServer(PandaPort) )  !=  NULL  ) {
		if		(  *ThisWindow  ==  0  ) {
			if		(  !TermSendID(fp,ThisTerm)  )	goto	quit;
			if		(  !TermSendStart(fp)  )		goto	quit;
			if		(  !TermSwitchLD(fp,arg)  )		goto	quit;
		} else {
			if		(  !SendPanda(fp)  )	goto	quit;
		}
		if		(  !TermSendExec(fp) )		goto	quit;
		if		(  !RecvPanda(fp,ThisUser,ThisWindow,ThisWidget,ThisLang)  )	goto	quit;
		if		(  !TermSendPrepare(fp)  )	goto	quit;
		if		(  !TermSendCommit(fp)  )	goto	quit;
		if		(  !TermBreakSession(fp)  )	goto	quit;
		ret = TRUE;
	}
  quit:;
	CloseNet(fp);
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
pandaExit(
	char		*arg)
{
	Bool	ret;
	NETFILE	*fp;

ENTER_FUNC;
	ret = FALSE;
	if		(  ( fp = TermConnectServer(PandaPort) )  !=  NULL  ) {
		if		(  !TermSendID(fp,ThisTerm)  )	goto	quit;
		if		(  !TermSendEnd(fp)  )		goto	quit;
		ret = TRUE;
	}
  quit:;
	CloseNet(fp);
LEAVE_FUNC;
	return	(ret);
}

#include	"option.h"
static	ARG_TABLE	option[] = {
	{	"panda",	STRING,		TRUE,	(void*)&PandaPort,
		"wfc waiting port number"						},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault()
{
	PandaPort = "/tmp/wfc.term";			/*	PORT_WFC	*/
}
extern	void
pandaInit(
	int		argc,
	char	**argv)
{
ENTER_FUNC;
	SetDefault();
	(void)GetOption(option,argc,argv,NULL);
LEAVE_FUNC;
}
