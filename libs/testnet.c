/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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
		printf("len = [%d]\n",(int)len);
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
		"ŝ|ň??"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"³?āŭť|Ď?" 							},
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
	fl = GetOption(option,argc,argv,NULL);
	InitMessage("testnet",NULL);

	InitNET();

	if		(  fServer  ) {
		ExecuteServer();
	} else {
		ExecuteClient();
	}
	return	(0);
}
