/*	PANDA -- a simple transaction monitor

Copyright (C) 1989-2002 Ogochan.

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
*	debug macros
*/

#ifndef	_INC_DEBUG_H
#define	_INC_DEBUG_H
#include	<stdio.h>

#ifdef	TRACE
#define	dbgmsg(x)			printf("%s\n",(x));fflush(stdout)
#define	PASS(x)				printf("%s(%d):%s\n",__FILE__,__LINE__,(x))
#define	DUMP_VALUE(val)		printf("%s:%ld\n",#val,(long)(val))
#else
#define	dbgmsg(x)			/*	*/
#define	PASS(x)				/*	*/
#define	DUMP_VALUE(val)		/*	*/
#endif
#define	Error(x)			printf("Error: %s\n",(x));fflush(stdout);exit(1)
#define	Warning(x)			printf("Warning: %s\n",(x));fflush(stdout)

#endif
