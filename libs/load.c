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
	,		filename[SIZE_BUFF];
	char	*p
	,		*q;
	void	*ret;
	struct	stat	st;

dbgmsg(">LoadFile");
	strcpy(buff,path);
	p = buff;
	ret = NULL;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(filename,"%s/%s",p,name);
		if		(  stat(filename, &st)  ==  0  ) {
			MessagePrintf("load [%s]",filename);
			if		(  ( ret = dlopen(filename,RTLD_GLOBAL | RTLD_LAZY) )  !=  NULL  )
				break;
		}
		p = q + 1;
	}	while	(	(  q    !=  NULL  )
				&&	(  ret  ==  NULL  ) );
dbgmsg("<LoadFile");
	return	(ret);
}
