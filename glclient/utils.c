/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2010 NaCl & JMA (Japan Medical Association).
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
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<dirent.h>
#include	<time.h>
#include	<errno.h>

#include	"utils.h"

void
get_human_bytes(size_t size, char *str)
{
	const double KB = 1024;
	const double MB = KB * KB;
	const double GB = MB * MB;
	
	if (size > GB) {
		sprintf(str, "%.1lf GB",(double)size/GB);
	} else if (size > MB) {
		sprintf(str, "%.1lf MB",(double)size/MB);
	} else if (size > KB) {
		sprintf(str, "%.1lf KB",(double)size/KB);
	} else {
		sprintf(str, "%ld Bytes",(unsigned long)size);
	}
}

gboolean
CheckAlreadyFile(
	char *filename)
{
	struct stat stbuf;
	uid_t euid;
	gid_t egid;
	gboolean rc = TRUE;

	euid = geteuid ();
	egid = getegid ();

    /* already file */
	if	(stat(filename, &stbuf) == 0 ){  
		rc = FALSE;
		/* Exception: not writable no error */	
		if (stbuf.st_uid == euid) {
			if (!(stbuf.st_mode & S_IWUSR)) {
				rc = TRUE;
			}
		} else if (stbuf.st_gid == egid) {
			if (!(stbuf.st_mode & S_IWGRP)) {
				rc = TRUE;
			}
		} else {
			if (!(stbuf.st_mode & S_IWOTH)) {
				rc = TRUE;
			}
		}
	}
	return (rc);
}

void
rm_r_old(
	const char *name,
	unsigned int elapsed)
{

	DIR *dir;
	struct dirent *ent;
	struct stat st;
	time_t now;
	char path[4096];

	if (name == NULL) {
		return;
	}

	now = time(NULL);

	if (stat(name,&st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			/* directory */
			if ((dir = opendir(name)) != NULL) {
				while((ent = readdir(dir)) != NULL) {
					if (ent->d_name[0] != '.') {
						snprintf(path,sizeof(path),"%s/%s",name,ent->d_name);
						path[sizeof(path)-1] = 0;
						rm_r_old(path,elapsed);
					}
				}
				closedir(dir);
				if ((now - st.st_ctim.tv_sec) > elapsed) {
					remove(name);
				}
			}
		} else {
			/* file */
			if ((now - st.st_ctim.tv_sec) > elapsed) {
				remove(name);
			}
		}
	}
}
