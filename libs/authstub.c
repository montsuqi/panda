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
#define	MAIN
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
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
#include	"types.h"
#include	"misc.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"dirs.h"
#include	"tcp.h"
#include	"option.h"
#include	"debug.h"

extern	Bool
AuthUser(
	char	*user,
	char	*pass,
	char	*other)
{
	int		fh;
	NETFILE	*fp;
	Bool	rc;
	char	buff[SIZE_OTHER+1];

dbgmsg(">AuthUser");
#ifdef	DEBUG
	printf("Auth.prot = [%s]\n",Auth.protocol);
	printf("Auth.host = [%s]\n",Auth.host);
	printf("Auth.port = [%d]\n",Auth.port);
	printf("user      = [%s]\n",user);	fflush(stdout);
	printf("pass      = [%s]\n",pass);	fflush(stdout);
#endif
	if		(  !stricmp(Auth.protocol,"glauth")  ) {
		fh = ConnectSocket(Auth.port,SOCK_STREAM,Auth.host);
		fp = SocketToNet(fh);
		SendString(fp,user);
		SendString(fp,pass);
		if		(  ( rc = RecvBool(fp) )  ) {
			RecvString(fp,buff);
			if		(  other  !=  NULL  ) {
				strcpy(other,buff);
			}
		}
		CloseNet(fp);
	} else {
		rc = FALSE;
	}
#ifdef	DEBUG
	if		(  rc ) {
		printf("OK\n");
	} else {
		printf("NG\n");
	}
#endif
dbgmsg("<AuthUser");
	return	(rc);
}

#ifdef	MAIN
static	char		*AuthURL;
static	ARG_TABLE	option[] = {
	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		"認証サーバ"			 						},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	AuthURL = "glauth://localhost:9002";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	char	other[SIZE_OTHER+1];

	SetDefault();
	(void)GetOption(option,argc,argv);
	ParseURL(&Auth,AuthURL);
	if		(  AuthUser(argv[1],argv[2],other)  ) {
		printf("OK\n");
		printf("[%s]\n",other);
	} else {
		printf("NG\n");
	}
}
#endif
