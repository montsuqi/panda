/*	PANDA -- a simple transaction monitor

Copyright (C) 1996-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

/*
*	TCP/IP module
*/
/*
#define		DEBUG
#define		TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<termios.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>

#include    <sys/types.h>
#include    <sys/socket.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	"misc.h"
#include	"value.h"
#include	"debug.h"

extern	int
BindSocket(
	char	*port,
	int		type)
#ifdef	USE_IPv6
{
	int		s
	,		ld;
	int		rc;
	struct	addrinfo	*info
			,			hints
			,			head;
	struct	sockaddr_in	*name;
dbgmsg(">BindSocket");
	memclear(&hints,sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = type;
	hints.ai_flags = AI_PASSIVE;
	if		(  ( rc = getaddrinfo(NULL,port,&hints,&info) )  !=  0  ) {
		printf("%s\n",gai_strerror(rc));
		Error("error resolv");
	}
	ld = -1;
	head = info;
	for	( ; info != NULL ; info = info->ai_next ) {
		name = (struct sockaddr_in *)info->ai_addr;
#ifdef	DEBUG
		printf("family   = %d\n",info->ai_family);
		printf("socktype = %d\n",info->ai_socktype);
		printf("protocol = %d\n",info->ai_protocol);
#endif
		s = socket(info->ai_family,info->ai_socktype,info->ai_protocol);
		one = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *) &one, sizeof(one));
		if		(  bind(s,info->ai_addr,info->ai_addrlen)  <  0  )	{
			close(s);
			continue;
		}
		ld = s;
		break;
	}
	freeaddrinfo(head);
	if		( ld  <  0  ) {
		Error("error bind");
	}
dbgmsg("<BindSocket");
	return	(ld);
}
#else
{	int		ld;
	struct	sockaddr_in	name;
	int		iport;

	if		(  ( ld = socket(AF_INET,type,0) )  <  0  )	{
		Error("error socket");
	}
	iport = atoi(port);
	one = 1;
	setsockopt(ld, SOL_SOCKET, SO_REUSEADDR, (void *) &one, sizeof(one));
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(iport);
	if		(  bind(ld,(struct sockaddr *)&name,sizeof(name))  <  0  )	{
		close(ld);
		Error("INET Domain Bind");
	}
	return	(ld);
}
#endif

extern	int
ConnectSocket(
	char	*port,
	int		type,
	char	*host)
#ifdef	USE_IPv6
{
	int		s
	,		ld;
	int		rc;
	struct	addrinfo	*info
			,			hints
			,			head;
	struct	sockaddr_in	*name;

dbgmsg(">ConnectSocket");
#ifdef	DEBUG
	printf("port = %s\n",port);
	printf("type = %d\n",type);
	printf("host = %s\n",host);
#endif
	memclear(&hints,sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = type;
	if		(  ( rc = getaddrinfo(host,port,&hints,&info) )  !=  0  ) {
		printf("%s\n",gai_strerror(rc));
		Error("error resolv");
	}
	ld = -1;
	head = info;
	for	( ; info != NULL ; info = info->ai_next ) {
		name = (struct sockaddr_in *)info->ai_addr;
		s = socket(info->ai_family,info->ai_socktype,info->ai_protocol);
		if		(  connect(s,info->ai_addr,info->ai_addrlen)  <  0  )	{
			close(s);
			continue;
		}
		ld = s;
		break;
	}
	freeaddrinfo(head);
dbgmsg("<ConnectSocket");
	return	(ld);
}
#else
{
	int		ld;
	int		iport;
	struct	sockaddr_in	*ptr;
	struct	sockaddr	addr;
	struct	hostent		*hp;
	struct	sockaddr_in	name;

dbgmsg(">ConnectSocket");
	if		(  ( ld = socket(AF_INET,type,0) )  <  0  )	{
		Error("error socket");
	}
	iport = atoi(port);
	name.sin_family = AF_INET;
	name.sin_port = htons(iport);

	if		(  ( hp = gethostbyname(host) )  !=  NULL  ) {
		ptr = (struct sockaddr_in *)&addr;
		memcpy(&name.sin_addr.s_addr,hp->h_addr,hp->h_length);
		if		(  connect(ld,(struct sockaddr *)&name,sizeof(name))  <  0  )	{
			ld = -1;
		}
	} else {
		ld = -1;
	}
dbgmsg("<ConnectSocket");
	return	(ld);
}
#endif

extern	FILE	*
OpenPort(
	char	*url,
	int		defport)
{
	int		fh;
	Port	*port;
	FILE	*fp;

	port = ParPort(url,defport);
	fh = ConnectSocket(port->port,SOCK_STREAM,port->host);
	DestroyPort(port);
	if		(  ( fp = fdopen(fh,"w+") )  ==  NULL  ) {
		close(fh);
	}
	return	(fp);
}

extern	int
InitServerPort(
	char	*port,
	int		back)
{	int		fh;

dbgmsg(">InitServerPort");
	fh = BindSocket(port,SOCK_STREAM);

	if		(  listen(fh,back)  <  0  )	{
		shutdown(fh, 2);
		Error("INET Domain listen");
	}
dbgmsg("<InitServerPort");
	return	(fh);
}

