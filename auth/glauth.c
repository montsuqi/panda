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
#define	MAIN
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<signal.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"auth.h"
#include	"libmondai.h"
#include	"socket.h"
#include	"comm.h"
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	int			Back;
static	char		*PortNumber;

static	void
Session(
	NETFILE	*fp)
{
	size_t	size;
	char	user[SIZE_USER+1]
	,		pass[SIZE_PASS+1];
	PassWord	*pw;

dbgmsg(">Session");
	if		(  Recv(fp,&size,sizeof(size))  >  0  ) {
		Recv(fp,user,size);
		user[size] = 0;
		RecvString(fp,pass);			ON_IO_ERROR(fp,badio);
		if		(  ( pw = AuthAuthUser(user,pass) )  ==  NULL  ) {
			SendBool(fp,FALSE);				ON_IO_ERROR(fp,badio);
		} else {
			SendBool(fp,TRUE);				ON_IO_ERROR(fp,badio);
			SendString(fp,pw->other);		ON_IO_ERROR(fp,badio);
		}
	}
  badio:
dbgmsg("<Session");
}

extern	void
ExecuteServer(void)
{
	int		pid;
	int		fd
	,		_fd;
	NETFILE	*fp;

dbgmsg(">ExecuteServer");
	_fd = InitServerPort(PortNumber,Back);

	while	(TRUE)	{
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			printf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}
		if		(  ( pid = fork() )  >  0  )	{	/*	parent	*/
			close(fd);
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
			close(_fd);
			fp = SocketToNet(fd);
			Session(fp);
			CloseNet(fp);
			exit(1);
		}
	}
dbgmsg("<ExecuteServer");
}

static	void
InitData(void)
{
dbgmsg(">InitData");
dbgmsg("<InitData");
}

static	void
InitPasswd(
	int		dummy)
{
	AuthLoadPasswd(PasswordFile);
}

static	void
InitSystem(void)
{
	InitPasswd(0);
	InitData();
}

static	void
OnChildExit(
	int		ec)
{
dbgmsg(">OnChildExit");
	while( waitpid(-1, NULL, WNOHANG) > 0 );
	(void)signal(SIGCHLD, (void *)OnChildExit);
dbgmsg("<OnChildExit");
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},
	{	"password",	STRING,		TRUE,	(void*)&PasswordFile,
		"パスワードファイル"	 						},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{

	PortNumber = IntStrDup(PORT_GLAUTH);
	Back = 5;
	PasswordFile = "./passwd";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	InitMessage("glauth",NULL);
	SetDefault();
	(void)GetOption(option,argc,argv);

#ifdef	DEBUG
#endif
	InitNET();
	(void)signal(SIGHUP, InitPasswd);
	(void)signal(SIGCHLD, (void *)OnChildExit);

	InitSystem();
	ExecuteServer();
	return	(0);
}
