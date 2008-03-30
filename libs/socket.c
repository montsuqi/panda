/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1996-1999 Ogochan.
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
#include	<unistd.h>
#include	<fcntl.h>
#include	<errno.h>

#include    <sys/types.h>
#include    <sys/socket.h>
#include    <sys/stat.h>
#include	<sys/un.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include	<netinet/tcp.h>
#include	"socket.h"
#include	"debug.h"

#define	memclear(b,s)	memset((b),0,(s))

extern	void
SetNodelay(
	int		s)
{
#ifdef	SOL_TCP
	int		one;

	one = 1;
	setsockopt(s, SOL_TCP, TCP_NODELAY, (void *) &one, sizeof(one));
#endif
}

extern	int
BindIP_Socket(
	char	*port,
	int		type)
#ifdef	USE_IPv6
{
	int		s
	,		ld;
	int		rc;
	int		one;
	struct	addrinfo	*info
			,			hints
			,			*head;
	struct	sockaddr_in	*name;

ENTER_FUNC;
	memclear(&hints,sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = type;
	hints.ai_flags = AI_PASSIVE;
	if		(  ( rc = getaddrinfo(NULL,port,&hints,&info) )  !=  0  ) {
		printf("%s [%s]\n",gai_strerror(rc),port);
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
		fprintf(stderr,"port = [%s]\n",port);
		Error("error bind");
	}
LEAVE_FUNC;
	return	(ld);
}
#else
{	int		ld;
	struct	sockaddr_in	name;
	int		iport;
	int		one;

ENTER_FUNC;
	if		(  ( ld = socket(PF_INET,type,0) )  <  0  )	{
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
LEAVE_FUNC;
	return	(ld);
}
#endif

extern	int
BindUNIX_Socket(
	char	*name,
	int		type,
	int		mode)
{
	int		sock;
	struct	sockaddr_un	addr;
	size_t	alen;

ENTER_FUNC;
	if		(  ( sock = socket(PF_UNIX,type,0) )  <  0  )	{
		Error("error socket");
	}
	unlink(name);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path,name);
	alen = sizeof(addr.sun_family) + strlen(addr.sun_path);
	if		(  bind(sock,(struct sockaddr *)&addr,alen)  <  0  )	{
		Message("%s: %s", strerror(errno), name);
		close(sock);
		Error("UNIX Domain Bind");
	} else {
		chmod(name,mode);
	}
LEAVE_FUNC;
	return	(sock);
}

extern	int
ConnectIP_Socket(
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
			,			*head;
	struct	sockaddr_in	*name;

ENTER_FUNC;
#ifdef	DEBUG
	printf("port = %s\n",port);
	printf("type = %d\n",type);
	printf("host = %s\n",host);
#endif
	memclear(&hints,sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = type;
	if		(  ( rc = getaddrinfo(host,port,&hints,&info) )  !=  0  ) {
		printf("port = %s\n",port);
		printf("type = %d\n",type);
		printf("host = %s\n",host);
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
LEAVE_FUNC;
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

ENTER_FUNC;
	if		(  ( ld = socket(PF_INET,type,0) )  <  0  )	{
		//fprintf(stderr,"error socket");
		ld = -1;
	} else {
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
	}
LEAVE_FUNC;
	return	(ld);
}
#endif

extern	int
ConnectUNIX_Socket(
	char	*name,
	int		type)
{
	int		sock;
	size_t	alen;
	struct	sockaddr_un	addr;

ENTER_FUNC;
	if		(  ( sock = socket(PF_UNIX,type,0) )  <  0  )	{
		Error("error socket");
	}
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path,name);
	alen = sizeof(addr.sun_family) + strlen(addr.sun_path);

	if		(  connect(sock,(struct sockaddr *)&addr,alen)  <  0  )	{
		sock = -1;
	}
LEAVE_FUNC;
	return	(sock);
}

extern	int
ConnectSocket(
	Port	*port,
	int		type)
{
	int		fd;
	switch	(port->type) {
	  case	PORT_IP:
		fd = ConnectIP_Socket(IP_PORT(port),type,IP_HOST(port));
		break;
	  case	PORT_UNIX:
		fd = ConnectUNIX_Socket(UNIX_NAME(port),type);
		break;
	  default:
		fd = -1;
		break;
	}
	return	(fd);
}

extern	int
BindSocket(
	Port	*port,
	int		type)
{
	int		fd;
	switch	(port->type) {
	  case	PORT_IP:
		fd = BindIP_Socket(IP_PORT(port),type);
		break;
	  case	PORT_UNIX:
		fd = BindUNIX_Socket(UNIX_NAME(port),type,UNIX_MODE(port));
		break;
	  default:
		fd = -1;
		break;
	}
	return	(fd);
}

extern void
CleanUNIX_Socket(
		Port	*port)
{
	struct	stat	sb;

	if(port->type == PORT_UNIX) {
		if ((stat(port->adrs.a_unix.name,&sb) == 0 )
		&& ( S_ISSOCK(sb.st_mode))) {
			unlink(port->adrs.a_unix.name);
		}	
	}
}

