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

/*
#define	DEBUG
#define	TRACE
*/
#include	<stdio.h>
#include	<sys/time.h>
#include	<time.h>
#include	<unistd.h>
#include	<stdarg.h>
#include	"message.h"

extern	void
MessageDebug(
	char	*file,
	int		line,
	char	*msg)
{
	_Message(MESSAGE_DEBUG,file,line,msg);
}

extern	void
MessagePrintf(
	char	*file,
	int		line,
	char	*format,
	...)
{
	char	buff[1024];
	va_list	va;

	va_start(va,format);
	vsprintf(buff,format,va);
	va_end(va);
	_Message(MESSAGE_DEBUG,file,line,buff);
}

static	void
MessageLog(
	char	*msg)
{
	struct	timeval	tv;
	struct	tm	Now;

	gettimeofday(&tv,NULL);
	Now = *localtime((time_t *)&tv.tv_sec);
	Now.tm_year += 1900;
	Now.tm_mon ++;

	printf("L:%04d%02d%02d %02d%02d%02d.%03d:%s\n",
		   Now.tm_year, Now.tm_mon, Now.tm_mday,
		   Now.tm_hour, Now.tm_min, Now.tm_sec,
		   (int)(tv.tv_usec/1000),
		   msg);
}

extern	void
_Message(
	int		level,
	char	*file,
	int		line,
	char	*msg)
{
	switch	(level) {
	  case	MESSAGE_MESSAGE:
		printf("M:%s:%d:%s\n",file,line,msg);
		break;
	  case	MESSAGE_DEBUG:
		printf("D:%s\n",msg);
		break;
	  case	MESSAGE_WARN:
		printf("W:%s:%d:%s\n",file,line,msg);
		break;
	  case	MESSAGE_ERROR:
		printf("E:%s:%d:%s\n",file,line,msg);
		break;
	  case	MESSAGE_LOG:
		MessageLog(msg);
		break;
	  default:
		printf("?:%s:%d:%s\n",file,line,msg);
		break;
	}
	fflush(stdout);
}

extern	void
InitMessage(void)
{
}
