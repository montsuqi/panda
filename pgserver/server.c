/*
 * PANDA -- a simple transaction monitor
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
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<signal.h>
#include	<string.h>
#include	<unistd.h>
#include	<glib.h>

#include	"enum.h"
#include	"socket.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"comm.h"
#include	"comms.h"
#include	"authstub.h"
#include	"applications.h"
#include	"driver.h"
#include	"pgserver.h"
#include	"front.h"
#include	"term.h"
#include	"dirs.h"
#include	"RecParser.h"
#include	"debug.h"

typedef	struct {
	int		major;
	int		minor;
	int		micro;
}	tVersionNumber;

static	void
ParseVersion(
	char	*str,
	tVersionNumber	*ver)
{
	ver->major = 0;
	ver->minor = 0;
	ver->micro = 0;

	while	(	(  *str  !=  0    )
			&&	(  isdigit(*str)  ) )	{
		ver->major = ver->major * 10 + ( *str - '0' );
		str ++;
	}
	if		(  *str  !=  0  )	str ++;
	while	(	(  *str  !=  0    )
			&&	(  isdigit(*str)  ) )	{
		ver->minor = ver->minor * 10 + ( *str - '0' );
		str ++;
	}
	if		(  *str  !=  0  )	str ++;
	while	(	(  *str  !=  0    )
			&&	(  isdigit(*str)  ) )	{
		ver->micro = ver->micro * 10 + ( *str - '0' );
		str ++;
	}
	dbgprintf("%d.%d.%d",ver->major,ver->minor,ver->micro);
}

static	void
FinishSession(
	ScreenData	*scr)
{
	Message("[%s@%s] session end",scr->user, TermToHost(scr->term));
}

static	void
DecodeName(
	char	*wname,
	char	*vname,
	char	*p)
{
	while	(  isspace(*p)  )	p ++;
	while	(	(  *p  !=  0     )
			&&	(  *p  !=  '.'   )
			&&	(  !isspace(*p)  ) ) {
		*wname ++ = *p ++;
	}
	*wname = 0;
	p ++;
	while	(  isspace(*p)  )	p ++;
	while	(  *p  !=  0  ) {
		if		(  !isspace(*p)  ) {
			*vname ++ = *p;
		}
		p ++;
	}
	*vname = 0;
}

static	void
RecvScreenData(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	char	buff[SIZE_BUFF+1];
	char	vname[SIZE_BUFF+1]
	,		wname[SIZE_BUFF+1]
	,		str[SIZE_BUFF+1];
	char	*p;
	WindowData	*win;
	ValueStruct	*value;

ENTER_FUNC;
	do {
		RecvStringDelim(fpComm,SIZE_BUFF,buff);
		if		(	(  *buff                     !=  0     )
				&&	(  ( p = strchr(buff,':') )  !=  NULL  ) ) {
			*p = 0;
			DecodeName(wname,vname,buff);
			p ++;
			while	(  isspace(*p)  )	p ++;
			DecodeStringURL(str,p);
			dbgprintf("wname = [%s]\n",wname);
			dbgprintf("vname = [%s]\n",vname);
			if		(  ( win = g_hash_table_lookup(scr->Windows,wname) )  !=  NULL  ) {
				value = GetItemLongName(win->rec->value,vname);
				ValueIsUpdate(value);
				SetValueString(value,str,scr->encoding);
			}
		} else
			break;
	}	while	(TRUE);
LEAVE_FUNC;
}

static	void
WriteClient(
	NETFILE		*fpComm,
	ScreenData	*scr)
{
	char	name[SIZE_BUFF+1]
	,		vname[SIZE_BUFF+1]
	,		wname[SIZE_BUFF+1];
	WindowData	*win;
	ValueStruct	*value;
	char	*p;
	Bool	fName;

ENTER_FUNC;
	SendStringDelim(fpComm,"Event: ");
	SendStringDelim(fpComm,ThisWindow);
	SendStringDelim(fpComm,"/");
	SendStringDelim(fpComm,ThisWidget);
	SendStringDelim(fpComm,"\n");
	do {
		if		(  !RecvStringDelim(fpComm,SIZE_BUFF,name)  )	break;
		if		(  *name  ==  0  )	break;
		if		(  ( p = strchr(name,':') )  !=  NULL  ) {
			*p = 0;
			fName = FALSE;
		} else {
			fName = TRUE;
		}
		DecodeName(wname,vname,name);
		if		(  ( win = g_hash_table_lookup(scr->Windows,wname) )  !=  NULL  ) {
			value = GetItemLongName(win->rec->value,vname);
			SetValueName(name);
			SendValueString(fpComm,value,NULL,fName,FALSE,scr->encoding);
			if		(  fName  ) {
				SendStringDelim(fpComm,"\n");
			}
		} else {
			SendStringDelim(fpComm,"\n");
		}
	}	while	(TRUE);
LEAVE_FUNC;
}

static	Bool
MainLoop(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;
	char	buff[SIZE_BUFF+1];
	char	*pass;
	char	*p
		,	*q;
	tVersionNumber  ver;

ENTER_FUNC;
	RecvStringDelim(fpComm,SIZE_BUFF,buff);
	dbgprintf("[%s]",buff);
	if		(  strncmp(buff,"Start: ",7)  ==  0  ) {
		dbgmsg("start");
		p = buff + 7;
		*(q = strchr(p,' ')) = 0;
		ParseVersion(p,&ver);
		p = q + 1;
		*(q = strchr(p,' ')) = 0;
		strcpy(scr->user,p);
		p = q + 1;
		*(q = strchr(p,' ')) = 0;
		pass = p;
		strcpy(scr->cmd,q+1);
		if		(  ( ver.major <  1 ) ||
				   ( ( ver.major == 1 ) && ( ver.minor <  2 ) ) ||
				   ( ( ver.major == 1 ) && ( ver.minor == 2 ) && 
                     ( ver.micro < 5 ) ) ) {
			SendStringDelim(fpComm,"Error: version\n");
			Message("reject client(invalid version)");
		} else 
		if		(  AuthUser(&Auth,scr->user,pass,scr->other,NULL)  ) {
			ApplicationsCall(APL_SESSION_LINK,scr);
			if		(  scr->status  ==  APL_SESSION_NULL  ) {
				SendStringDelim(fpComm,"Error: application\n");
				Message("Error: application");
			} else {
				SendStringDelim(fpComm,"Connect: OK\n");
			}
		} else {
			SendStringDelim(fpComm,"Error: authentication\n");
			Message("reject client(authentication error)");
		}
	} else
	if		(  strncmp(buff,"Event: ",7)  ==  0  ) {
		dbgmsg("event");
		p = buff + 7;
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
			DecodeStringURL(scr->widget,p);
			DecodeStringURL(scr->event,q+1);
		} else {
			DecodeStringURL(scr->event,p);
			*scr->widget = 0;
		}
		RecvScreenData(fpComm,scr);
		ApplicationsCall(APL_SESSION_GET,scr);
	} else
	if		(  strncmp(buff,"Encoding: ",10)  ==  0  ) {
		dbgmsg("encoding");
		if		(  scr->encoding  !=  NULL  ) {
			xfree(scr->encoding);
		}
		scr->encoding = StrDup(buff+10);
		scr->status = APL_SESSION_RESEND;
	} else
	if		(  strncmp(buff,"End",3)  ==  0  ) {
		dbgmsg("end");
		scr->status = APL_SESSION_NULL;
	} else {
		if ( strlen(buff) > 0) {
			Warning("invalid message [%s]\n",buff);
		}
		scr->status = APL_SESSION_NULL;
	}
	while	(  scr->status  ==  APL_SESSION_LINK  ) {
		ApplicationsCall(scr->status,scr);
	}
	switch	(scr->status) {
	  case	APL_SESSION_NULL:
		ret = FALSE;
		break;
	  case	APL_SESSION_RESEND:
		ret = TRUE;
		break;
	  default:
		WriteClient(fpComm,scr);
		ret = TRUE;
		break;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
ExecuteServer(void)
{
	int		pid;
	int		fh
	,		_fh;
	NETFILE	*fpComm;
	ScreenData	*scr;
	Port	*port;

	signal(SIGCHLD,SIG_IGN);

	port = ParPortName(PortNumber);
	_fh = InitServerPort(port,Back);
	DestroyPort(port);
	while	(TRUE)	{
		if		(  ( fh = accept(_fh,0,0) )  <  0  )	{
			printf("_fh = %d\n",_fh);
			Error("INET Domain Accept");
		}

		if		(  ( pid = fork() )  >  0  )	{	/*	parent	*/
			close(fh);
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
			fpComm = SocketToNet(fh);
			close(_fh);
			scr = InitSession();
			strcpy(scr->term,TermName(fh));
			Message("[%s@%s] session start",scr->user, TermToHost(scr->term));
			while	(  MainLoop(fpComm,scr)  );
			FinishSession(scr);
			CloseNet(fpComm);
			exit(0);
		}
	}
}

static	void
InitData(void)
{
ENTER_FUNC;
	RecParserInit();
	BlobCacheCleanUp();
LEAVE_FUNC;
}

extern	void
InitSystem(
	int		argc,
	char	**argv)
{
ENTER_FUNC;
	InitData();
	InitNET();
	ApplicationsInit(argc,argv);
LEAVE_FUNC;
}
