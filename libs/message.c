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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<sys/time.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include	"libmondai.h"
#ifdef	USE_MSGD
#include	"const.h"
#include	"tcp.h"
#include	"net.h"
#endif
#include	"message.h"

#ifndef	SIZE_BUFF
#define	SIZE_BUFF		8192
#endif

#ifdef	USE_MSGD
static	NETFILE	*fpLog = NULL;
#else
static	FILE	*fpLog = NULL;
#endif

static	char	*Format = "%Y/%M/%D/%h:%m:%s %F:%f:%L:%B";

static	char	*Processid = "";

static	char	*FlagChar[]	= {
	"M",	"D",	"W",	"E",	"L",	"L",	"P",	"?"};

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
	_Message(MESSAGE_PRINT,file,line,buff);
}

static	void
PutLog(
	char	*str)
{
	char	buff[SIZE_BUFF];

	sprintf(buff,"%s\n",StringChop(str));
#ifdef	USE_MSGD
	Send(fpLog,buff,strlen(buff));
#else
	fprintf(fpLog,"%s",buff);
	fflush(fpLog);
#endif
}

extern	void
_Message(
	int		level,
	char	*file,
	int		line,
	char	*msg)
{
	char	buff[SIZE_BUFF];
	char	*f
	,		*p
	,		*s;
	Bool	fOut
	,		fDot;
	struct	timeval	tv;
	struct	tm	*Now;

	if		(  fpLog  !=  NULL  ) {
		gettimeofday(&tv,NULL);
		Now = localtime((time_t *)&tv.tv_sec);
		p = buff;
		fOut = TRUE;
		for	( f = Format ; *f != 0 ; f ++ ) {
			if		(  *f  ==  '%'  ) {
				f ++;
				switch	(*f) {
				  case	'Y':
					p += sprintf(p,"%04d",Now->tm_year+1900);
					break;
				  case	'M':
					p += sprintf(p,"%02d",Now->tm_mon+1);
					break;
				  case	'D':
					p += sprintf(p,"%02d",Now->tm_mday);
					break;
				  case	'h':
					p += sprintf(p,"%02d",Now->tm_hour);
					break;
				  case	'm':
					p += sprintf(p,"%02d",Now->tm_min);
					break;
				  case	's':
					p += sprintf(p,"%02d",Now->tm_sec);
					break;
				  case	'p':
					p += sprintf(p,"%03d",(int)(tv.tv_usec/1000));
					break;
				  case	'F':
					fDot = FALSE;
					if		(  *(f+1)  ==  '('  ) {
						f += 2;
						fOut = FALSE;
						while	(  *f  !=  ')'  ) {
							if		(  *f  ==  *FlagChar[level]  ) {
								fOut = TRUE;
							}
							if		(  *f  ==  '.'  ) {
								fDot = TRUE;
							}
							f ++;
						}
					}
					if		(  !fDot  ) {
						p += sprintf(p,"%s",FlagChar[level]);
					}
					break;
				  case	'i':
					p += sprintf(p,"%s",Processid);
					break;
				  case	'f':
					p += sprintf(p,"%s",file);
					break;
				  case	'L':
					p += sprintf(p,"%d",line);
					break;
				  case	'B':
					for	( s = msg ; *s != 0 ; s ++ ) {
						switch	(*s) {
						  case	'\\':
							p += sprintf(p,"\\\\");
							break;
						  case	'\n':
							p += sprintf(p,"\\n");
							break;
						  default:
							*p ++ = *s;
							break;
						}
					}
					break;
				  default:
					*p ++ = *f;
					break;
				}
			} else {
				*p ++ = *f;
			}
		}
		*p = 0;
		if		(  fOut  ) {
			PutLog(buff);
		}
	}
}

extern	void
InitMessage(
	char	*id,
	char	*fn)
{
#ifdef	USE_MSGD
	int		fd;
	Port	*port;
#endif

	if		(  fn  ==  NULL  ) {
		fn = getenv("LOG_FILE_NAME");
	}
	fpLog = NULL;
	Processid = StrDup(id);
	if		(  ( Format = getenv("LOG_DATA_FORMAT") )  ==  NULL  ) {
		Format = "%Y/%M/%D/%h:%m:%s %F:%f:%L:%B";
	}
#ifdef	USE_MSGD
	if		( fn  !=  NULL  ) {
		if		(  *fn  ==  '@'  ) {
			port = ParPort(fn+1,PORT_MSGD);
			if		(  ( fd = ConnectSocket(port->port,SOCK_STREAM,
											port->host) )  >=  0  ) {
				fpLog = SocketToNet(fd);
				Format = "%F:%i:%f:%L:%B";
			}
			DestroyPort(port);
		} else {
			if		(  ( fd = open(fn,O_WRONLY|O_CREAT) )  >=  0  ) {
				fpLog = FileToNet(fd);
			}
		}
	}
	if		(  fpLog  ==  NULL  ) {
		fpLog = FileToNet(STDOUT_FILENO);
	}
#else
	if		(  ( fpLog = fopen(fn,"w+") )  ==  NULL  ) {
		fpLog = stdout;
	}
#endif
}
