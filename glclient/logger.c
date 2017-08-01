/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2011 JMA (Japan Medical Association).
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/stat.h>
#include	<sys/file.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<dirent.h>
#include	<uuid/uuid.h>
#include	<time.h>
#include	<libmondai.h>

#include	"logger.h"
#include	"utils.h"

#ifndef	SIZE_LOG
#define	SIZE_LOG		8192
#endif
#define SIZE_FORMAT		256

static char *LogFile = NULL;
static FILE *fp = NULL;
static int level = GL_LOG_WARN;
static void (*ErrorFunc)(const char *,...);

void
InitLogger(
	const char *prefix)
{
	struct tm cur;
	time_t t;
	uuid_t u;
	gchar *dir,_uuid[64],_time[64];

	dir = g_strconcat(g_get_home_dir(),"/.glclient/log",NULL);
	mkdir_p(dir,0700);

	t = time(NULL);
	gmtime_r(&t,&cur);
	strftime(_time,sizeof(_time),"%Y%m%d%H%M%S",&cur);

	uuid_generate(u);
	uuid_unparse(u,_uuid);
	LogFile = g_strconcat(dir,"/",prefix,"-",_time,"-",_uuid,".log",NULL);
	rm_r_old(dir,2592000); /* 30days */
	g_free(dir);
	fprintf(stderr,"LogFile: %s\n",LogFile);

	if (fp == NULL) {
		if ((fp = fopen(LogFile,"w")) == NULL) {
			fprintf(stderr,"fopen failure:%s\n",strerror(errno));
		} 
	} else {
		fprintf(stderr,"fp != null, perhaps call InitLogger again?\n");
	}
	ErrorFunc = NULL;
}

void
InitLogger_via_FileName(
	const char *filename)
{
	if (fp == NULL) {
		if ((fp = fopen(filename,"a")) == NULL) {
			fprintf(stderr,"fopen failure:%s\n",strerror(errno));
		} 
	} else {
		fprintf(stderr,"fp != null, perhaps call InitLogger again?\n");
	}
	ErrorFunc = NULL;
}

const char*
GetLogFile()
{
	return LogFile;
}

void
FinalLogger()
{
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
}

void 
SetLogLevel(
	int _level)
{
	level = _level;
}

void
SetErrorFunc(
	void (*func)(const char *,...))
{
	ErrorFunc = func;
}

void
Error(
	const char *format,
	...)
{
	va_list	va;

	va_start(va,format);
	if (ErrorFunc != NULL) {
		ErrorFunc(format,va);
	} else {
		_Error(format,va);
	}
	va_end(va);
}

void
logger(
	int _level,
	const char *file,
	int line,
	const char *format,
	...)
{
	struct	timeval	tv;
	struct	tm	now;
	char	buf[SIZE_LOG - SIZE_FORMAT],tbuf[256],*lp;
	va_list	va;

	if (fp == NULL) {
		va_start(va,format);
		vsnprintf(buf, sizeof(buf), format, va);
		va_end(va);
		fprintf(stderr,"%s\n",buf);
		if (_level == GL_LOG_ERROR) {
			exit(1);
		}
		return;
	}
	if (_level < level) {
		// nop
		return;
	}
	switch(_level) {
	case GL_LOG_DEBUG:
		lp = "DEBUG";
		break;
	case GL_LOG_WARN:
		lp = "WARN ";
		break;
	case GL_LOG_INFO:
		lp = "INFO ";
		break;
	case GL_LOG_ERROR:
		lp = "ERROR";
		break;
	default:
		fprintf(stderr,"invalid log level:%d\n",_level);
		return;
	}

	va_start(va,format);
	vsnprintf(buf, sizeof(buf), format, va);
	va_end(va);

	gettimeofday(&tv,NULL);
	localtime_r((time_t *)&tv.tv_sec, &now);
	strftime(tbuf,sizeof(tbuf),"%Y-%m-%dT%H:%M:%d%z",&now);

	flock(fileno(fp),LOCK_EX);
	fprintf(fp,"%s %s %s:%d: %s\n",tbuf,lp,file,line,buf);
	fprintf(stderr,"%s %s %s:%d: %s\n",tbuf,lp,file,line,buf);
	fflush(fp);
	flock(fileno(fp),LOCK_UN);

	if (_level == GL_LOG_ERROR) {
		fclose(fp);
		exit(1);
	}
}
