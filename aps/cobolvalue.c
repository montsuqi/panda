/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"

#include	"value.h"
#include	"cobolvalue.h"
#include	"debug.h"


extern	void
DumpCobol(
	char	*name,
	char	*p,
	size_t	size)
{
	int		i;
	if		(  name  !=  NULL  ) {
		printf("%s:[",name);
	}
	for	( i = 0  ; i  < size ; i ++, p ++ ) {
		printf("%c",*p);
	}
	printf("]\n");
}

extern	void
StringCobol2C(
	char	*str,
	size_t	size)
{
	char	*p;

	for	( p = str + size - 1 ; p >= str ; p -- ) {
		if		(  *p  ==  ' '  ) {
			*p = 0;
		} else
		if		(  *p  ==  0  ) {
			/*	*/
		} else
			break;
	}
}

extern	void
StringC2Cobol(
	char	*p,
	size_t	size)
{
	int		i;
	Bool	fEnd;

	fEnd = FALSE;
	for	( i = 0 ; i < size ; i ++, p ++ ) {
		if		(  *p  ==  0  )	{
			fEnd = TRUE;
		}
		if		(  fEnd  ) {
			*p = ' ';
		}
	}
}


