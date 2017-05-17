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
#include	<fcntl.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<dirent.h>
#include	<uuid/uuid.h>
#include	<time.h>

#include	"glclient.h"
#include	"logger.h"

#ifndef	SIZE_LOG
#define	SIZE_LOG		8192
#endif
#define SIZE_FORMAT		256

static FILE *fp = NULL;
static int level = LOG_WARN;
static void (*ErrorFunc)(char *,...);

static void
rm_old_log(
	char *dname,
	unsigned long elapse)
{
	DIR *dir;
	struct dirent *ent;
	struct stat st;
	time_t now;
	char path[2048];

	if (dname == NULL) {
		fprintf(stderr,"dname null\n");
		return;
	}

	now = time(NULL);

	if ((dir = opendir(dname)) != NULL) {
		while((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] != '.') {
				snprintf(path,sizeof(path),"%s/%s",dname,ent->d_name);
				path[sizeof(path)-1] = 0;
				if (stat(path,&st) == 0) {
					if ((now - st.st_ctim.tv_sec) > elapse) {
						fprintf(stderr,"remove %s\n",path);
						//remove(path);
					}
				}
			}
		}
		closedir(dir);
	}
}

void
InitLogger()
{

	uuid_t u;
	gchar *dir,buf[64];

	dir = g_strconcat(g_get_home_dir(),"/.glclient/log",NULL);
	MakeDir(dir,0700);
	uuid_generate(u);
	uuid_unparse(u,buf);
	LogFile = g_strconcat(dir,"/",buf,".log",NULL);
	rm_old_log(dir,2592000); /* 30days */
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
	void (*func)(char *,...))
{
	ErrorFunc = func;
}

void
Error(
	char *format,
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
	char *file,
	int line,
	char *format,
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
		if (_level == LOG_ERROR) {
			exit(1);
		}
		return;
	}
	if (_level < level) {
		// nop
		return;
	}
	switch(_level) {
	case LOG_DEBUG:
		lp = "DEBUG";
		break;
	case LOG_WARN:
		lp = "WARN ";
		break;
	case LOG_INFO:
		lp = "INFO ";
		break;
	case LOG_ERROR:
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

	fprintf(fp,"%s %s %s:%d: %s\n",tbuf,lp,file,line,buf);

	if (_level == LOG_ERROR) {
		fclose(fp);
		exit(1);
	}
}
