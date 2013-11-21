/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
#	include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"libmondai.h"
#include	"blobcache.h"
#include	"glserver.h"
#include	"debug.h"

extern	size_t
BlobCacheFileSize(
	ValueStruct	*value)
{
	struct stat st;

	if (!stat(BlobCacheFileName(value), &st)) {
		return st.st_size; 
	}
	return 0;
}

extern	char	*
BlobCacheFileName(
	ValueStruct	*value)
{
	char	buf[SIZE_BUFF];

	if		(  ValueObjectFile(value)  ==  NULL  ) {
		if		(  IS_OBJECT_NULL(ValueObjectId(value))  ) {
			sprintf(buf,"%s/%p",CacheDir, value);
		} else {
			sprintf(buf,"%s/%s",CacheDir,ValueToString(value,NULL));
		}
		ValueObjectFile(value) = StrDup(buf);
	}
	return	(ValueObjectFile(value));
}

extern	void
BlobCacheCleanUp()
{
	MakeDir(CacheDir,0700);
}
