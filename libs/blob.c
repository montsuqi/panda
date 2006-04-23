/*
 * PANDA -- a simple transaction monitor
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
#include	<unistd.h>
#include	<string.h>
#include    <sys/types.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<pthread.h>
#include	<fcntl.h>
#include	<errno.h>

#include	"types.h"

#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"queue.h"
#include	"directory.h"
#include	"blob.h"
#include	"blob_v1.h"
#include	"blob_v2.h"
#include	"message.h"
#include	"debug.h"

extern	int
VersionBLOB(
	char	*space)
{
	FILE	*fp;
	char	magic[SIZE_BLOB_HEADER];
	char	name[SIZE_LONGNAME+1];
	int		version;

ENTER_FUNC;
	if		(  space  ==  NULL  ) {
		version = BLOB_VERSION;
	} else {
		snprintf(name,SIZE_LONGNAME+1,"%s/blob.env",space);
		version = 0;
		if		(  ( fp = Fopen(name,"r") )  !=  NULL  ) {
			fread(magic,SIZE_BLOB_HEADER,1,fp);
			if		(  !memcmp(magic,BLOB_V1_HEADER,SIZE_BLOB_HEADER)  ) {
				version = 1;
			}
		}
	}
LEAVE_FUNC;
	return	(version);
}

