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
