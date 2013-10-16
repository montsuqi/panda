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
#include	<errno.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<netdb.h>

#include	"const.h"
#include	"libmondai.h"
#include	"term.h"
#include	"message.h"
#include	"debug.h"

extern	char	*
TermToHost(
	char	*term)
{
	struct	sockaddr_in		in;
#ifdef	USE_IPv6
	struct	sockaddr_in6	in6;
	char	buff[NI_MAXHOST];
	int		i;
#endif
	int		errcd, flags;
	char	*p;
	static	char	host[NI_MAXHOST+2];
	
ENTER_FUNC;
	p = term;
	flags = fNumericHOST ? NI_NUMERICHOST : 0;
	switch	(*p) {
	  case	'4':
		p ++;
		in.sin_port = HexToInt(p,4);
		p = strchr(p,':') + 1;
		in.sin_addr.s_addr = HexToInt(p,8);
		in.sin_family = AF_INET;
		if ((errcd = getnameinfo((struct sockaddr *)&in,sizeof(in),host
								 ,NI_MAXHOST,NULL,0,flags)) != 0) {
			Warning("getnameinfo error: %s\n",gai_strerror(errcd));
		}
		break;
#ifdef	USE_IPv6
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
		if ((errcd = getnameinfo((struct sockaddr *)&in6,sizeof(in6),buff
								 ,NI_MAXHOST,NULL,0,flags)) != 0) {
			Warning("getnameinfo error: %s\n",gai_strerror(errcd));
		}
		if	(	strchr(buff,':')	!=  NULL  ) {
			sprintf(host,"[%s]",buff);
		} else {
			strcpy(host,buff);
		}
		break;
#endif
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
	struct	tm	Now;
	socklen_t	len;
	static	char			name[SIZE_TERM+1];	//	SIZE_TERM == 64
	union sockaddr_s {
		struct  sockaddr 			sa;
		struct	sockaddr_in		sa_in;
#ifdef	USE_IPv6
		struct	sockaddr_in6		sa_in6;
#endif
		struct	sockaddr_storage	sa_stor;
	} u;
	struct	timeval	tv;

ENTER_FUNC;
	memclear(name,SIZE_TERM+1);
	if		(  sock  ==  0  ) {
		time(&nowtime);
		gettimeofday(&tv,NULL);
		localtime_r(&nowtime, &Now);
		Now.tm_year += 1900;
		sprintf(name,"T%04d%02d%02d:%02d%02d%02d.%06d",
				Now.tm_year,Now.tm_mon+1,Now.tm_mday,
				Now.tm_hour,Now.tm_min,Now.tm_sec,
				(int)tv.tv_usec);
	} else {
		len = sizeof(u);
		if ((getpeername(sock,&(u.sa),&len)) != 0 ){
			Warning(("getpeername error: %s\n"),strerror(errno));
			return name;
		}
		switch	(u.sa.sa_family) {
		  case	AF_UNIX:
			time(&nowtime);
			localtime_r(&nowtime, &Now);
			Now.tm_year += 1900;
			sprintf(name,"U%04d%02d%02d:%02d%02d%02d:%08X",
					Now.tm_year,Now.tm_mon+1,Now.tm_mday,
					Now.tm_hour,Now.tm_min,Now.tm_sec,
					getpid());
			break;
		  case	AF_INET:
			sprintf(name,"4%04X:%08X:%08X",
					(unsigned int)u.sa_in.sin_port,
					(unsigned int)(u.sa_in.sin_addr.s_addr),
					getpid());
			break;
#ifdef	USE_IPv6
		  case	AF_INET6:
			sprintf(name,"6%04X:%08X%08X%08X%08X%08X:%08X",
					(unsigned int)u.sa_in6.sin6_port,
					(unsigned int)(u.sa_in6.sin6_addr.s6_addr32[0]),
					(unsigned int)(u.sa_in6.sin6_addr.s6_addr32[1]),
					(unsigned int)(u.sa_in6.sin6_addr.s6_addr32[2]),
					(unsigned int)(u.sa_in6.sin6_addr.s6_addr32[3]),
					(unsigned int)(u.sa_in6.sin6_scope_id),
					getpid());
			break;
#endif
		  default:
			break;
		}
	}
#ifdef	DEBUG
	printf("term name = [%s]\n",name);
#endif
LEAVE_FUNC;
	return	name;
}

