/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<string.h>

#include	"types.h"
#include	"libmondai.h"
#include	"front.h"
#include	"debug.h"

extern	char	*
BlobCacheFileName(
	ValueStruct	*value)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"%s/%s",CacheDir,ValueToString(value,NULL));
	return	(buf);
}

extern	void
BlobCacheCleanUp(void)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"rm -f %s/*",CacheDir);
	system(buf);
}

extern	ScreenData	*
InitSession(void)
{
	ScreenData	*scr;

dbgmsg(">InitSession");
	InitPool();
	scr = NewScreenData();
	scr->status = APL_SESSION_LINK;
dbgmsg("<InitSession");
	return	(scr); 
}

