/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<netinet/in.h>
#include	<fcntl.h>
#include	<time.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<netdb.h>

#include	"types.h"
#include	"const.h"
#include	"libmondai.h"
#include	"term.h"
#include	"debug.h"

extern	char	*
TermToHost(
	char	*term)
{
	struct	sockaddr_in		in;
	struct	sockaddr_in6	in6;
	char	*p;
	int		i;
	char	buff[NI_MAXHOST+2];
	static	char	host[NI_MAXHOST];
	char	port[NI_MAXSERV];

ENTER_FUNC;
	p = term;
	switch	(*p) {
	  case	'4':
		p ++;
		in.sin_port = HexToInt(p,4);
		p = strchr(p,':') + 1;
		in.sin_addr.s_addr = HexToInt(p,8);
		in.sin_family = AF_INET;
		getnameinfo((struct sockaddr *)&in,sizeof(in),host,NI_MAXHOST,port,NI_MAXSERV,0);
		break;
	  case	'6':
		p ++;
		in6.sin6_port = HexToInt(p,4);
		p = strchr(p,':') + 1;
		for	( i = 0 ; i < 4 ; i ++ ) {
			in6.sin6_addr.s6_addr32[i] = HexToInt(p,8);
			p += 8;
		}
		p ++;
		in6.sin6_scope_id = HexToInt(p,8);
		in6.sin6_family = AF_INET6;
		getnameinfo((struct sockaddr *)&in6,sizeof(in6),buff,NI_MAXHOST,port,NI_MAXSERV,0);
		if		(  strchr(buff,':')  !=  NULL  ) {
			sprintf(host,"[%s]",buff);
		} else {
			strcpy(host,buff);
		}
		break;
	  case	'U':	/*	UNIX domain	*/
		strcpy(host,"localhost");
		break;
	  default:
		*host = 0;
		break;
	}
LEAVE_FUNC;
	return	(host);
}

extern	char	*
TermName(
	int		sock)
{
	time_t		nowtime;
	struct	tm	*Now;
	socklen_t	len;
	static	char			name[SIZE_TERM+1];	//	SIZE_TERM == 64
	struct	sockaddr_storage	addr;
	struct	sockaddr_in		*in;
	struct	sockaddr_in6	*in6;
	struct	timeval	tv;

ENTER_FUNC;
	memclear(name,SIZE_TERM+1);
	if		(  sock  ==  0  ) {
		time(&nowtime);
		gettimeofday(&tv,NULL);
		Now = localtime(&nowtime);
		Now->tm_year += 1900;
		sprintf(name,"T%04d%02d%02d:%02d%02d%02d.%06d",
				Now->tm_year,Now->tm_mon+1,Now->tm_mday,
				Now->tm_hour,Now->tm_min,Now->tm_sec,
				(int)tv.tv_usec);
	} else {
		len = sizeof(struct sockaddr_storage);
		getpeername(sock,(struct sockaddr *)&addr,&len);
		switch	(((struct sockaddr *)&addr)->sa_family) {
		  case	AF_UNIX:
			time(&nowtime);
			Now = localtime(&nowtime);
			Now->tm_year += 1900;
			sprintf(name,"U%04d%02d%02d:%02d%02d%02d:%08X",
					Now->tm_year,Now->tm_mon+1,Now->tm_mday,
					Now->tm_hour,Now->tm_min,Now->tm_sec,
					getpid());
			break;
		  case	AF_INET:
			in = (struct sockaddr_in *)&addr;
			sprintf(name,"4%04X:%08X:%08X",
					(unsigned int)in->sin_port,
					(unsigned int)(in->sin_addr.s_addr),
					getpid());
			break;
		  case	AF_INET6:
			in6 = (struct sockaddr_in6 *)&addr;
			sprintf(name,"6%04X:%08X%08X%08X%08X%08X:%08X",
					(unsigned int)in6->sin6_port,
					(unsigned int)(in6->sin6_addr.s6_addr32[0]),
					(unsigned int)(in6->sin6_addr.s6_addr32[1]),
					(unsigned int)(in6->sin6_addr.s6_addr32[2]),
					(unsigned int)(in6->sin6_addr.s6_addr32[3]),
					(unsigned int)(in6->sin6_scope_id),
					getpid());
			break;
		  default:
			break;
		}
	}
#ifdef	DEBUG
	printf("term name = [%s]\n",name);
#endif
LEAVE_FUNC;
	return	(name); 
}

