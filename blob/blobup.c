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

#define	MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<glib.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<signal.h>
#include	"types.h"
#include	"libmondai.h"
#include	"blob.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*Space;

static	ARG_TABLE	option[] = {
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Space = NULL;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	int			fv
		,		tv;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("blobup",NULL);

	if		(  fl  !=  NULL  ) {
		Space = fl->name;
	}
	if		(  Space  !=  NULL  ) {
		tv = VersionBLOB(NULL);
		fv = VersionBLOB(Space);
		if		(  fv  ==  0  ) {
			fprintf(stderr,"invalid BLOB space\n");
			exit(1);
		}
		if		(  fv  ==  tv  ) {
			fprintf(stderr,"needress BLOB space upgrade\n");
			exit(1);
		}
	}

	return	(0);
}
