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
#define	TRACE
#define	DEBUG
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

#include	"types.h"
#include	"enum.h"
#include	"misc.h"
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
#include	"dirs.h"
#include	"DDparser.h"
#include	"debug.h"

static	void
FinishSession(
	ScreenData	*scr)
{
	ReleasePool(NULL);
	printf("session end\n");
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

dbgmsg(">RecvScreenData");
	do {
		RecvStringDelim(fpComm,SIZE_BUFF,buff);
		if		(	(  *buff                     !=  0     )
				&&	(  ( p = strchr(buff,':') )  !=  NULL  ) ) {
			*p = 0;
			DecodeName(wname,vname,buff);
			p ++;
			while	(  isspace(*p)  )	p ++;
			DecodeStringURL(str,p);
printf("wname = [%s]\n",wname);
printf("vname = [%s]\n",vname);
			if		(  ( win = g_hash_table_lookup(scr->Windows,wname) )  !=  NULL  ) {
				value = GetItemLongName(win->rec->value,vname);
				value->fUpdate = TRUE;
				SetValueString(value,str,"euc-jp");
			}
		} else
			break;
	}	while	(TRUE);
dbgmsg("<RecvScreenData");
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

dbgmsg(">WriteClient");
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
			SendValueString(fpComm,value,NULL,fName);
			if		(  fName  ) {
				SendStringDelim(fpComm,"\n");
			}
		} else {
			SendStringDelim(fpComm,"\n");
		}
	}	while	(TRUE);
dbgmsg("<WriteClient");
}

static	Bool
MainLoop(
	NETFILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;
	char	buff[SIZE_BUFF+1];
	char	*pass;
	char	*ver;
	char	*p
	,		*q;

dbgmsg(">MainLoop");
	RecvStringDelim(fpComm,SIZE_BUFF,buff);
	if		(  strncmp(buff,"Start: ",7)  ==  0  ) {
		dbgmsg("start");
		p = buff + 7;
		*(q = strchr(p,' ')) = 0;
		ver = p;
		p = q + 1;
		*(q = strchr(p,' ')) = 0;
		strcpy(scr->user,p);
		p = q + 1;
		*(q = strchr(p,' ')) = 0;
		pass = p;
		strcpy(scr->cmd,q+1);
		if		(  strcmp(ver,VERSION)  ) {
			SendStringDelim(fpComm,"Error: version\n");
			g_warning("reject client(invalid version)");
		} else
		if		(  AuthUser(scr->user,pass,scr->other)  ) {
			scr->Windows = NULL;
			ApplicationsCall(APL_SESSION_LINK,scr);
			if		(  scr->status  ==  APL_SESSION_NULL  ) {
				SendStringDelim(fpComm,"Error: application\n");
			} else {
				SendStringDelim(fpComm,"Connect: OK\n");
			}
		} else {
			SendStringDelim(fpComm,"Error: authentication\n");
			g_warning("reject client(authentication error)");
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
	if		(  strncmp(buff,"End",3)  ==  0  ) {
		dbgmsg("end");
		scr->status = APL_SESSION_NULL;
	} else {
		printf("invalid message [%s]\n",buff);
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
dbgmsg("<MainLoop");
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


	signal(SIGCHLD,SIG_IGN);

	_fh = InitServerPort(PortNumber,Back);
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
dbgmsg(">InitData");
	DD_ParserInit();
dbgmsg("<InitData");
}

extern	void
InitSystem(
	int		argc,
	char	**argv)
{
dbgmsg(">InitSystem");
	InitData();
	InitNET();
	ApplicationsInit(argc,argv);
dbgmsg("<InitSystem");
}
