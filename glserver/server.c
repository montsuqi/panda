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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<string.h>
#include	<termio.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"enum.h"
#include	"misc.h"
#include	"tcp.h"
#include	"value.h"
#include	"glterm.h"
#include	"comm.h"
#include	"authstub.h"
#include	"applications.h"
#include	"driver.h"
#include	"glserver.h"
#include	"front.h"
#include	"dirs.h"
#include	"DDparser.h"
#include	"debug.h"

static	void
ClearWindows(
	gpointer	key,
	gpointer	value,
	gpointer	user_data)
{
	WindowData	*win = (WindowData *)value;

	xfree(key);
	FreeValueStruct(win->Value);
	xfree(win->name);
}

static	void
FinishSession(
	ScreenData	*scr)
{
	if		(	(  scr  !=  NULL  )
			&&	(  scr->Windows  !=  NULL  ) ) {
		g_hash_table_foreach(scr->Windows,(GHFunc)ClearWindows,NULL);
		g_hash_table_destroy(scr->Windows);
		xfree(scr);
	}
	ReleasePool(NULL);
	printf("session end\n");
}

static	Bool
CheckCache(
	FILE	*fpComm,
	char	*name,
	off_t	st_size,
	time_t	st_mtime)
{
	Bool	ret;
	int		klass;

dbgmsg(">CheckCache");
	SendPacketClass(fpComm,GL_QueryScreen);
	SendString(fpComm,name);
	SendLong(fpComm,(long)st_size);
	SendLong(fpComm,(long)st_mtime);
	fflush(fpComm);
	switch	(  klass = RecvPacketClass(fpComm)  ) {
	  case	GL_GetScreen:
		dbgmsg("GetScreen");
		ret = TRUE;
		break;
	  case	GL_NOT:
		dbgmsg("Not");
		ret = FALSE;
		break;
	  default:
		dbgmsg("error");
		ret = FALSE;
		break;
	}
dbgmsg("<CheckCache");
	return	(ret);
}
	
static	void
SendFile(
	FILE	*fpComm,
	char	*fname,
	char	*wname)
{
	char	buff[SIZE_BUFF];
	struct	stat	stbuf;
	size_t	size
	,		left;
	FILE	*fp;

	stat(fname,&stbuf);
	if		(  CheckCache(fpComm,
						  wname,
						  stbuf.st_size, stbuf.st_mtime)  ) {
		RecvString(fpComm,wname);	/*	dummy	*/
		fp = fopen(fname,"r");
		SendPacketClass(fpComm,GL_ScreenDefine);
		SendLong(fpComm,(long)stbuf.st_size);
		left = stbuf.st_size;
		do {
			if		(  left  >  SIZE_BUFF  ) {
				size = SIZE_BUFF;
			} else {
				size = left;
			}
			size = fread(buff,1,size,fp);
			if		(  size  >  0  ) {
				fwrite(buff,1,size,fpComm);
				left -= size;
			}
		}	while	(  left  >  0  );
		fflush(fpComm);
		fclose(fp);
	}
}

static	void
CheckScreen(
	char		*wname,
	WindowData	*win,
	FILE		*fpComm)
{
	char	fname[SIZE_BUFF]
	,			dir[SIZE_BUFF];
	struct	stat	stbuf;
	char	*p
	,		*q;
	Bool	fDone
	,		fExit;

dbgmsg(">CheckScreen");
	if		(  win->fNew  ) {
		strcpy(dir,ScreenDir);
		p = dir;
		fDone = FALSE;
		do {
			if		(  ( q = strchr(p,':') )  !=  NULL  ) {
				*q = 0;
				fExit = FALSE;
			} else {
				fExit = TRUE;
			}
			sprintf(fname,"%s/%s.glade",p,wname);
			if		(  stat(fname,&stbuf)  ==  0  ) {
				SendFile(fpComm,fname,wname);
				fDone = TRUE;
			}
			p = q + 1;
		}	while	(  !fExit  );
		if		(  !fDone  ) {
			printf("screen file not exists. [%s]\n",wname);
			exit(1);
		}
		win->fNew = FALSE;
	}
dbgmsg("<CheckScreen");
}

static	void
CheckScreens(
	FILE		*fpComm,
	ScreenData	*scr)
{
dbgmsg(">CheckScreens");
	g_hash_table_foreach(scr->Windows,(GHFunc)CheckScreen,fpComm);
	SendPacketClass(fpComm,GL_END);
dbgmsg("<CheckScreens");
}

static	void
SendWindow(
	char		*wname,
	WindowData	*win,
	FILE		*fpComm)
{
dbgmsg(">SendWindow");
	if		(  win->PutType  !=  SCREEN_NULL  ) {
		SendPacketClass(fpComm,GL_WindowName);
		SendString(fpComm,wname);
		SendInt(fpComm,win->PutType);
		switch	(win->PutType) {
		  case	SCREEN_CURRENT_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			if		(  win->Value  !=  NULL  ) {
				SendPacketClass(fpComm,GL_ScreenData);
				SendValue(fpComm,win->Value);
			} else {
				SendPacketClass(fpComm,GL_NOT);
			}
			break;
		  default:
			SendPacketClass(fpComm,GL_NOT);
			break;
		}
		win->PutType = SCREEN_NULL;
		fflush(fpComm);
	}
dbgmsg("<SendWindow");
}

static	void
SendScreenAll(
	FILE		*fpComm,
	ScreenData	*scr)
{
dbgmsg(">SendScreenAll");
	g_hash_table_foreach(scr->Windows,(GHFunc)SendWindow,fpComm);
	if		(	(  *scr->window  !=  0  )
			&&	(  *scr->widget  !=  0  ) ) {
		SendPacketClass(fpComm,GL_FocusName);
		SendString(fpComm,scr->window);
		SendString(fpComm,scr->widget);
	}
	SendPacketClass(fpComm,GL_END);
dbgmsg("<SendScreenAll");
}

static	Bool
SendScreenData(
	FILE		*fpComm,
	ScreenData	*scr)
{
	Bool	rc;

dbgmsg(">SendScreenData");
	if		(  RecvPacketClass(fpComm)  ==  GL_GetData  ) {
		if		(  RecvInt(fpComm)  ==  0  ) {	/*	get all data	*/
			dbgmsg("get all data");
			SendScreenAll(fpComm,scr);
			fflush(fpComm);
		} else {
			dbgmsg("get partial");
		}
		rc = TRUE;
	} else {
		rc = FALSE;
	}
dbgmsg("<SendScreenData");
	return	(rc);
}

static	void
RecvScreenData(
	FILE	*fpComm,
	ScreenData	*scr)
{
	int		c;
	char		wname[SIZE_NAME+1]
	,			name[SIZE_NAME+1];
	char		str[SIZE_BUFF];
	int			ival;
	Bool		bval;
	double		fval;
	Fixed		*xval;
	WindowData	*win;
	ValueStruct	*value;
	PacketDataType	type;

	while	(  RecvPacketClass(fpComm)  ==  GL_WindowName  ) {
		RecvString(fpComm,wname);
		win = g_hash_table_lookup(scr->Windows,wname);
		while	(  ( c = RecvPacketClass(fpComm) )  ==  GL_ScreenData  ) {
			RecvString(fpComm,name);
			value = GetItemLongName(win->Value,name+strlen(wname)+1);
			value->fUpdate = TRUE;
			type = RecvDataType(fpComm);
			switch	(type)	{
			  case	GL_TYPE_CHAR:
			  case	GL_TYPE_VARCHAR:
			  case	GL_TYPE_DBCODE:
			  case	GL_TYPE_TEXT:
				RecvString(fpComm,str);
				SetValueString(value,str);
				break;
			  case	GL_TYPE_NUMBER:
				xval = RecvFixed(fpComm);
				SetValueFixed(value,xval);
				xfree(xval->sval);
				xfree(xval);
				break;
			  case	GL_TYPE_INT:
				ival = RecvInt(fpComm);
				SetValueInteger(value,ival);
				break;
			  case	GL_TYPE_FLOAT:
				fval = RecvFloat(fpComm);
				SetValueFloat(value,fval);
				break;
			  case	GL_TYPE_BOOL:
				bval = RecvBool(fpComm);
				SetValueBool(value,bval);
				break;
			  default:
				break;
			}
		}
	}
}

static	Bool
SendScreen(
	FILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;

dbgmsg(">SendScreen");
	CheckScreens(fpComm,scr);
	ret = SendScreenData(fpComm,scr);
dbgmsg("<SendScreen");
	return	(ret);
}

static	Bool
MainLoop(
	FILE	*fpComm,
	ScreenData	*scr)
{
	Bool	ret;

	PacketClass	klass;
	char	pass[SIZE_PASS+1];
	char	ver[SIZE_NAME+1];

dbgmsg(">MainLoop");
	klass = RecvPacketClass(fpComm); 
	if		(  klass  !=  GL_Null  ) {
#ifdef	TRACE
		printf("class = %d\n",(int)klass);
#endif
		switch	(klass) {
		  case	GL_Connect:
			RecvString(fpComm,ver);
			RecvString(fpComm,scr->user);
			RecvString(fpComm,pass);
			RecvString(fpComm,scr->cmd);
			if		(  strcmp(ver,VERSION)  ) {
				SendPacketClass(fpComm,GL_E_VERSION);
				g_warning("reject client(invalid version)");
			} else
			if		(  AuthUser(scr->user,pass,scr->other)  ) {
				scr->Windows = NULL;
				ApplicationsCall(APL_SESSION_LINK,scr);
				if		(  scr->status  ==  APL_SESSION_NULL  ) {
					SendPacketClass(fpComm,GL_E_APPL);
				} else {
					SendPacketClass(fpComm,GL_OK);
					CheckScreens(fpComm,scr);
				}
			} else {
				g_warning("reject client(authentication error)");
				SendPacketClass(fpComm,GL_E_AUTH);
			}
			fflush(fpComm);
			break;
		  case	GL_Event:
			RecvString(fpComm,scr->window);
			RecvString(fpComm,scr->widget);
			RecvString(fpComm,scr->event);
			RecvScreenData(fpComm,scr);
			ApplicationsCall(APL_SESSION_GET,scr);
			break;
		  case	GL_ScreenData:
			/*	fatal error	*/
			scr->status = APL_SESSION_RESEND;
			break;
		  case	GL_END:
			scr->status = APL_SESSION_NULL;
			break;
		  default:
			printf("class = %X\n",klass);
			scr->status = APL_SESSION_NULL;
			break;
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
			ret = SendScreen(fpComm,scr);
			break;
		}
	} else {
		ret = FALSE;
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
	FILE	*fpComm;
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
			if		(  ( fpComm = fdopen(fh,"w+") )  ==  NULL  ) {
				close(fh);
				exit(1);
			}
			close(_fh);
			scr = InitSession();
			strcpy(scr->term,TermName(fh));
			while	(  MainLoop(fpComm,scr)  );
			FinishSession(scr);
			fclose(fpComm);
			shutdown(fh, 2);
			exit(0);
		}
	}
}

static	void
InitData(void)
{
dbgmsg(">InitData");
dbgmsg("<InitData");
}

extern	void
InitSystem(void)
{
dbgmsg(">InitSystem");
	InitData();
	ApplicationsInit();
dbgmsg("<InitSystem");
}
