/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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
#include	<glib.h>

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

ENTER_FUNC;
	if		(  Recv(fp,&size,sizeof(size))  >  0  ) {
		Recv(fp,user,size);
		user[size] = 0;
		RecvnString(fp, sizeof(pass), pass);	ON_IO_ERROR(fp,badio);
		if		(  ( pw = AuthAuthUser(user,pass) )  ==  NULL  ) {
			SendBool(fp,FALSE);				ON_IO_ERROR(fp,badio);
		} else {
			SendBool(fp,TRUE);				ON_IO_ERROR(fp,badio);
			SendString(fp,pw->other);		ON_IO_ERROR(fp,badio);
		}
	}
  badio:
LEAVE_FUNC;
}

static	void
InitData(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
InitPasswd(void)
{
ENTER_FUNC;
	AuthLoadPasswd(PasswordFile);
LEAVE_FUNC;
}

static	void
InitSystem(void)
{
ENTER_FUNC;
	InitPasswd();
	InitData();
LEAVE_FUNC;
}

static	Bool
CheckPassword(void)
{
	struct  stat    stbuf;
	static  off_t stsize;
	static  time_t stmtime;
	static  time_t stctime;

	if		(stat(PasswordFile,&stbuf)  ==  0 )		{
		if	(	(stmtime != stbuf.st_mtime )
			 || (stctime != stbuf.st_ctime )
			 || (stsize != stbuf.st_size ) ) {
			stmtime = stbuf.st_mtime;
			stctime = stbuf.st_ctime;
			stsize = stbuf.st_size;
			return TRUE;
		}
	}
	return FALSE;
}

static	void
_InitPasswd(int dummy)
{
	Message("%s reload", PasswordFile);
	InitPasswd();
	CheckPassword();
}

static	void
OnChildExit(
	int		ec)
{
ENTER_FUNC;
	while( waitpid(-1, NULL, WNOHANG) > 0 );
	(void)signal(SIGCHLD, (void *)OnChildExit);
LEAVE_FUNC;
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

	PortNumber = PORT_GLAUTH;
	Back = 5;
	PasswordFile = "./passwd";
}

extern	void
ExecuteServer(void)
{
	int		pid;
	int		fd
	,		_fd;
	NETFILE	*fp;
	Port	*port;

ENTER_FUNC;
	port = ParPortName(PortNumber);
	_fd = InitServerPort(port,Back);
	CheckPassword();

	while	(TRUE)	{
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			printf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}
		if	(	CheckPassword() )	{
			_InitPasswd(0);
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
LEAVE_FUNC;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	InitMessage("glauth",NULL);
	SetDefault();
	(void)GetOption(option,argc,argv,NULL);

#ifdef	DEBUG
#endif
	InitNET();
	(void)signal(SIGHUP, _InitPasswd);
	(void)signal(SIGCHLD, (void *)OnChildExit);

	InitSystem();
	ExecuteServer();
	return	(0);
}
