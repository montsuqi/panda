/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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

