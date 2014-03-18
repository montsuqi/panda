/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1989-1999 Ogochan.
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
#include	<sys/time.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>
#include	<stdarg.h>
#include	<stdlib.h>
#ifdef	USE_SYSLOG
#include	<syslog.h>
#endif
#include	"libmondai.h"
#ifdef	USE_MSGD
#include	"const.h"
#include	"socket.h"
#include	"port.h"
#include	"net.h"
#endif
#include	"message.h"

#ifndef	SIZE_LOG
#define	SIZE_LOG		8192
#endif
#define SIZE_FORMAT		256

#ifdef	USE_SYSLOG
static	int syslog_facility =  LOG_LOCAL1;
#endif

#ifdef	USE_MSGD
static	NETFILE	*fpLog = NULL;
#else
static	FILE	*fpLog = NULL;
#endif

Bool	fTimer = FALSE;

Bool	fNumericHOST = FALSE;

static	char	*Format = "%Y/%M/%D/%h:%m:%s %F:%f:%L:%B";

static	char	*Processid = "";

static	char	*FlagChar[]	= {
	"M",	"D",	"W",	"E",	"L",	"P",	"?"};

static	void	(*MessageFunction)(int level, char *file, int line, char *msg);

extern	void
_MessageLevelPrintf(
	int 	level,
	char	*file,
	int		line,
	char	*format,
	...)
{
	char	buff[SIZE_LOG - SIZE_FORMAT];
	va_list	va;

	va_start(va,format);
	vsnprintf(buff, sizeof(buff), format, va);
	va_end(va);
	_Message(level,file,line,buff);
}

#ifdef	USE_SYSLOG
static int 
SyslogLevel(
	int level)
{
	int syslog_level;

	switch (level){
	  case MESSAGE_MESSAGE:
		  syslog_level = LOG_INFO;     break;
	  case MESSAGE_DEBUG:
		  syslog_level = LOG_DEBUG;    break;
	  case MESSAGE_WARN:
		  syslog_level = LOG_WARNING;  break;
	  case MESSAGE_ERROR:
		  syslog_level = LOG_ERR;      break;
	  case MESSAGE_LOG:
		  syslog_level = LOG_NOTICE;   break;
	  case MESSAGE_PRINT:
		  syslog_level = LOG_INFO;     break;
	  default:
		  syslog_level = LOG_INFO;
	}
	return (syslog_level);
};
#endif

static	void
PutLog(
	char	*str)
{
	char	buff[SIZE_LOG];

	sprintf(buff,"%s",StringChop(str));
#ifdef	USE_MSGD
	Send(fpLog,buff,strlen(buff));
	Send(fpLog,"\n",1);
	Flush(fpLog);
#else
	fprintf(fpLog,"%s\n",buff);
	fflush(fpLog);
#endif
}

extern  void
_Message(
	int		level,
	char	*file,
	int		line,
	char	*msg)
{
	MessageFunction(level, file, line, msg);
}

extern	void
__Message(
	int		level,
	char	*file,
	int		line,
	char	*msg)
{
	char	buff[SIZE_LOG];
	char	*f
	,		*p
	,		*s;
	Bool	fOut
	,		fDot;
	struct	timeval	tv;
	struct	tm	Now;

#ifdef USE_SYSLOG
	syslog(SyslogLevel(level), "%s", msg);
#endif
	
	if		(  fpLog  !=  NULL  ) {
		gettimeofday(&tv,NULL);
		localtime_r((time_t *)&tv.tv_sec, &Now);
		p = buff;
		fOut = TRUE;
		for	( f = Format ; *f != 0 ; f ++ ) {
			if		(  *f  ==  '%'  ) {
				f ++;
				switch	(*f) {
				  case	'Y':
					p += sprintf(p,"%04d",Now.tm_year+1900);
					break;
				  case	'M':
					p += sprintf(p,"%02d",Now.tm_mon+1);
					break;
				  case	'D':
					p += sprintf(p,"%02d",Now.tm_mday);
					break;
				  case	'h':
					p += sprintf(p,"%02d",Now.tm_hour);
					break;
				  case	'm':
					p += sprintf(p,"%02d",Now.tm_min);
					break;
				  case	's':
					p += sprintf(p,"%02d",Now.tm_sec);
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
	char *tempformat, *tempfn;
#ifdef	USE_MSGD
	int		fd;
	Port	*port;
#endif

#ifdef USE_SYSLOG
	static char	buff[SIZE_LOG];

	snprintf(buff, SIZE_LOG, "%s/%s", PACKAGE, id);
	openlog(buff, LOG_PID, syslog_facility);
#endif

	if		(  fn  ==  NULL  ) {
		tempfn = getenv("LOG_FILE_NAME");
		if (tempfn != NULL) {
			fn = StrDup(tempfn);
		}
	}
	fpLog = NULL;
	Processid = StrDup(id);
	tempformat = getenv("LOG_DATA_FORMAT");
	if (tempformat != NULL) {
		Format = StrDup(tempformat);
	}
#ifdef	USE_MSGD
	if		( fn  !=  NULL  ) {
		if		(  *fn  ==  '@'  ) {
			port = ParPort(fn+1,PORT_MSGD);
			if		(  ( fd = ConnectSocket(port,SOCK_STREAM) )  >=  0  ) {
				fpLog = SocketToNet(fd);
				if		(  Format  ==  NULL  ) {
					Format = "%F:%i:%f:%L:%B";
				}
			}
			DestroyPort(port);
		} else {
			if		(  ( fd = open(fn,O_WRONLY|O_CREAT|O_TRUNC,0600) )  >=  0  ) {
				fpLog = FileToNet(fd);
			}
		}
	}
	if		(  fpLog  ==  NULL  ) {
		if		(  Format  ==  NULL  ) {
			Format = "%Y/%M/%D/%h:%m:%s %F:%f:%L:%B";
		}
		fpLog = FileToNet(STDOUT_FILENO);
	}
#else
	if		(	(  fn  ==  NULL  )
			||	(  ( fpLog = fopen(fn,"w+") )  ==  NULL  ) )	{
		fpLog = stdout;
	}
#endif
	if		(  Format  ==  NULL  ) {
		Format = "%Y/%M/%D/%h:%m:%s %F:%f:%L:%B";
	}
	MessageFunction = __Message;
}

extern void
SetMessageFunction(
	void	(*func)(int level, char *file, int line, char *msg))
{
	MessageFunction = func;
}

extern long
GetNowTime(void)
{
	struct	timeval	tv;
	
	gettimeofday(&tv,NULL);
	return tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}

extern	void
TimerPrintf(
	long	start,
	long	end,
	char	*format,
	...)
{
	char	buff[SIZE_LOG - SIZE_FORMAT];
	va_list	va;

	if (fTimer){
		va_start(va,format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		printf(" %6ld(ms) %s", (end - start), buff);
	}
}

extern	void
Time(
	char	*str)
{
	static long last = 0;
	long now;

	now = GetNowTime();
	fprintf(stderr,"%6ld(ms) %s\n",now - last,str);
	last = now;
}
