/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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
#include	<string.h>

#include	"types.h"
#include	"libmondai.h"
#include	"front.h"
#include	"debug.h"

static	unsigned int	rseed;
extern	char	*
BlobCacheFileName(
	ValueStruct	*value)
{
	char	buf[SIZE_BUFF];

	if		(  ValueObjectFile(value)  ==  NULL  ) {
		if		(  IS_OBJECT_NULL(ValueObjectId(value))  ) {
			srand(rseed);
			sprintf(buf,"%s/%d",CacheDir,rand());
		} else {
			sprintf(buf,"%s/%s",CacheDir,ValueToString(value,NULL));
		}
		ValueObjectFile(value) = StrDup(buf);
	}
	return	(ValueObjectFile(value));
}

extern	void
BlobCacheCleanUp(void)
{
	char	buf[SIZE_BUFF];

	sprintf(buf,"rm -f %s/*",CacheDir);
	system(buf);
}

extern	ScreenData	*
InitSession(void)
{
	ScreenData	*scr;

ENTER_FUNC;
	InitPool();
	scr = NewScreenData();
	scr->status = APL_SESSION_LINK;
	rseed = (int)scr;
LEAVE_FUNC;
	return	(scr); 
}

