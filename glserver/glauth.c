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
#define	MAIN
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<termio.h>
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
#include	"misc.h"
#include	"auth.h"
#include	"comm.h"
#include	"dirs.h"
#include	"tcp.h"
#include	"option.h"
#include	"debug.h"

static	GHashTable	*FileHash;
static	int			Back;
static	char		*PortNumber;

static	int
InitServerPort(
	char	*port)
{	int		fh;

dbgmsg(">InitServerPort");
	fh = BindSocket(port,SOCK_STREAM);

	if		(  listen(fh,Back)  <  0  )	{
		shutdown(fh, 2);
		Error("INET Domain listen");
	}
dbgmsg("<InitServerPort");
	return	(fh);
}

static	void
SetFDs(
	int		fd,
	FILE	*fp,
	fd_set	*fds)
{
	FD_SET(fd,fds);
}

static	void
ReadFDs(
	int		fd,
	FILE	*fp,
	fd_set	*fds)
{
	size_t	size;
	char	user[SIZE_USER+1]
	,		pass[SIZE_PASS+1];
	PassWord	*pw;

dbgmsg(">ReadFDs");
	if		(  FD_ISSET(fd,fds)  ) {
		if		(  fread(&size,sizeof(size),1,fp)  >  0  ) {
			RecvStringBody(fp,user,size);
			RecvString(fp,pass);
			if		(  ( pw = AuthAuthUser(user,pass) )  ==  NULL  ) {
				SendBool(fp,FALSE);
			} else {
				SendBool(fp,TRUE);
				SendString(fp,pw->other);
			}
			fflush(fp);
		} else {
			fclose(fp);
			shutdown(fd,2);
			close(fd);
			g_hash_table_remove(FileHash,(void *)fd);
		}
	}
dbgmsg("<ReadFDs");
}

extern	void
ExecuteServer(void)
{
	int		fh
	,		_fh;

	fd_set		ready;
	int			maxfd;

dbgmsg(">ExecuteServer");
	_fh = InitServerPort(PortNumber);

	maxfd = _fh;
	
	while	(TRUE)	{
		FD_ZERO(&ready);
		FD_SET(_fh,&ready);
		g_hash_table_foreach(FileHash,(GHFunc)SetFDs,&ready);
		select(maxfd+1,&ready,NULL,NULL,NULL);
		if		(  FD_ISSET(_fh,&ready)  ) {	/*	connect	*/
			dbgmsg("connect");
			if		(  ( fh = accept(_fh,0,0) )  <  0  )	{
				printf("_fh = %d\n",_fh);
				Error("INET Domain Accept");
			}
			maxfd = ( maxfd > fh ) ? maxfd : fh;
			g_hash_table_insert(FileHash,(void *)fh,fdopen(fh,"w+"));
		}
		g_hash_table_foreach(FileHash,(GHFunc)ReadFDs,&ready);
	}
dbgmsg("<ExecuteServer");
}

static	guint
Hash(
	int	key)
{
	return	((guint)key);
}

static	gint
Compare(
	int		s1,
	int		s2)
{
	return	(s1 == s2);
}

static	void
InitData(void)
{
dbgmsg(">InitData");
	FileHash = g_hash_table_new((GHashFunc)Hash,(GCompareFunc)Compare);
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
	SetDefault();
	(void)GetOption(option,argc,argv);

#ifdef	DEBUG
#endif
	signal(SIGHUP,InitPasswd);

	InitSystem();
	ExecuteServer();
	return	(0);
}
