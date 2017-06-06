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
#include	<libmondai.h>

#include	"utils.h"
#include	"tempdir.h"

#define		ELAPSED (2592000) // 30day

static char *TempDir = NULL;

void
InitTempDir()
{
	uuid_t u;
	gchar *dir,buf[64];

	dir = g_strconcat(g_get_home_dir(),"/.glclient/tmp",NULL);
	MakeDir(dir,0700);
	rm_r_old(dir,ELAPSED);
	uuid_generate(u);
	uuid_unparse(u,buf);
	TempDir = g_strconcat(dir,"/",buf,NULL);
	MakeDir(TempDir,0700);
	g_free(dir);
	fprintf(stderr,"tempdir: %s\n",TempDir);
}

char *
GetTempDir()
{
	return TempDir;
}

char *
MakeTempSubDir(
	const char*name)
{
	char *subdir;

	if (TempDir == NULL) {
		return NULL;
	}
	subdir = g_strconcat(TempDir,name,NULL);
	mkdir_p(subdir,0700);
	return subdir;
}
