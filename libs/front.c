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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
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
#include	<glib.h>
#include	<netdb.h>

#include	"types.h"
#include	"libmondai.h"
#include	"socket.h"
#include	"front.h"
#include	"debug.h"

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

dbgmsg(">TermName");
	memclear(name,SIZE_TERM+1);
	if		(  sock  ==  0  ) {
		time(&nowtime);
		Now = localtime(&nowtime);
		Now->tm_year += 1900;
		sprintf(name,"T%04d%02d%02d:%02d%02d%02d:%08X",
				Now->tm_year,Now->tm_mon+1,Now->tm_mday,
				Now->tm_hour,Now->tm_min,Now->tm_sec,
				getpid());
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
dbgmsg("<TermName");
	return	(name); 
}

extern	char	*
BlobCacheFileName(
	ValueStruct	*value)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"%s/%s",CacheDir,ValueToString(value,NULL));
	return	(buf);
}

extern	void
BlobCacheCleanUp(void)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"rm -f %s/*",CacheDir);
	system(buf);
}

extern	ScreenData	*
InitSession(void)
{
	ScreenData	*scr;

dbgmsg(">InitSession");
	InitPool();
	scr = NewScreenData();
	scr->status = APL_SESSION_LINK;
dbgmsg("<InitSession");
	return	(scr); 
}

