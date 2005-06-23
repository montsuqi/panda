/*
PANDA -- a simple transaction monitor
Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	"defaults.h"
#include	<stdio.h>
#include	<string.h>
#include	<dlfcn.h>
#include    <sys/types.h>
#include	<sys/stat.h>

#include	"defaults.h"
#include	"const.h"
#include	"libmondai.h"
#include	"load.h"
#include	"debug.h"

extern	void	*
LoadFile(
	char	*path,
	char	*name)
{
	char	buff[SIZE_BUFF]
		,	filename[SIZE_LONGNAME+1];
	char	*p
	,		*q;
	void	*ret;
	struct	stat	st;

ENTER_FUNC;
	strcpy(buff,path);
	p = buff;
	ret = NULL;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(filename,"%s/%s",p,name);
		if		(  stat(filename, &st)  ==  0  ) {
			dbgprintf("load [%s]",filename);
			if		(  ( ret = dlopen(filename,RTLD_GLOBAL | RTLD_LAZY) )  !=  NULL  )
				break;
		}
		p = q + 1;
	}	while	(	(  q    !=  NULL  )
				&&	(  ret  ==  NULL  ) );
LEAVE_FUNC;
	return	(ret);
}
