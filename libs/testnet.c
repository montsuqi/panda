/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/time.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"socket.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*PortNumber;
static	int		Back;

static	Bool	fServer;

static	long	__ever
		,		__lap;

extern	void
_ProfileReset(void)
{
	struct	timeval	tv;

	gettimeofday(&tv,NULL);
	__lap = __ever = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}

extern	void
_ProfileLap(
	char	*s)
{
	struct	timeval	tv;
	long	now;

	printf("%s:",(s));
	gettimeofday(&tv,NULL);
	now = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
	printf("elapse %6ld(ms)\tlap %6ld(ms)\n",(now - __ever),(now - __lap));
	__lap = now;
}

static	void
ExecuteServer(void)
{
	Port	*port;
	int		_fd
		,	fd;
	size_t	len;
	NETFILE	*fp;
	byte	buff[SIZE_BUFF];

ENTER_FUNC;
	port = ParPortName(PortNumber);
	_fd = InitServerPort(port,Back);

	if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
		printf("_fd = %d\n",_fd);
		Error("INET Domain Accept");
	}
	fp = SocketToNet(fd);
	while	(  ( len = RecvLength(fp) )  >  0  ) {
		printf("len = [%d]\n",len);
		Recv(fp,buff,len);
		Send(fp,buff,len);
		SendLength(fp,len);
	}
	close(_fd);
	CloseNet(fp);
LEAVE_FUNC;
}

static	void
ExecuteClient(void)
{
	Port	*port;
	int		fd;
	int		i
		,	j;
	NETFILE	*fp;
	byte	sbuff[SIZE_BUFF]
		,	rbuff[SIZE_BUFF];

ENTER_FUNC;
	port = ParPortName(PortNumber);
	if		(  ( fd = ConnectSocket(port,SOCK_STREAM) )  <  0  ) {
		printf("can not connect server(server port not found)\n");
		exit(1);
	}

	for	( i = 0 ; i < SIZE_BUFF ; i ++ ) {
		sbuff[i] = i % 256;
	}
	fp = SocketToNet(fd);
	_ProfileReset();
	for	( i = 1 ; i < SIZE_BUFF ; i ++ ) {
		SendLength(fp,i);
		for	( j = 0 ; j < i ; j ++ ) {
			Send(fp,sbuff+j,1);
		}
		Recv(fp,rbuff,i);
		if		(  memcmp(sbuff,rbuff,i)  !=  0  ) {
			printf("date broken\n");
		}
		memclear(sbuff,SIZE_BUFF);
		RecvLength(fp);
	}
	SendLength(fp,0);
	_ProfileLap("stop");
	CloseNet(fp);
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},
	{	"server",	BOOLEAN,	TRUE,	(void*)&fServer,
		"start as server"								},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = "3151";
	Back = 5;

	fServer = FALSE;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("testnet",NULL);

	InitNET();

	if		(  fServer  ) {
		ExecuteServer();
	} else {
		ExecuteClient();
	}
	return	(0);
}
