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
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"queue.h"
#include	"directory.h"
#include	"blob.h"
#include	"message.h"
#include	"debug.h"

static	char	*BLOB_Space;

extern	Bool
NewBLOB(
	MonObjectType	*obj,
	int				mode)
{
}

extern	Bool
OpenBLOB(
	MonObjectType	*obj,
	int				mode)
{
}

extern	Bool
CloseBLOB(
	MonObjectType	*obj)
{
}

extern	size_t
WriteBLOB(
	MonObjectType	*obj,
	byte			*buff,
	size_t			size)
{
}

extern	size_t
ReadBLOB(
	MonObjectType	*obj,
	byte			*buff,
	size_t			size)
{
}

extern	void
InitBLOB(
	char	*space)
{
	FILE	*fp;
	char	name[SIZE_LONGNAME+1];
	char	buff[SIZE_BUFF];

	sprintf(name,"%s/pid",space);
	if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
		if		(  fgets(buff,SIZE_BUFF,fp)  !=  NULL  ) {
			if		(  getpid() != atoi(buff)  ) {
				fprintf(stderr,"another process uses libpandablob. (%d)\n",atoi(buff));
				exit(1);
			}
		}
		fclose(fp);
	}
	if		(  ( fp = fopen(name,"w") )  !=  NULL  ) {
		fprintf(fp,"%d\n",getpid());
		fclose(fp);
	} else {
		fprintf(stderr,"can not make lock file(directory not writable?)\n");
		exit(1);
	}
	BLOB_Space = StrDup(space);
}

extern	void
FinishBLOB(void)
{
	char	name[SIZE_LONGNAME+1];

	sprintf(name,"%s/pid",BLOB_Space);
	unlink(name);
}
