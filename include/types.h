/*	PANDA -- a simple transaction monitor

Copyright (C) 1989-2003 Ogochan.

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
*	Global type define
*/

#ifndef	_INC_TYPES_H
#define	_INC_TYPES_H

#include	<sys/types.h>

#ifndef	byte
#define	byte		unsigned char
#endif

#ifndef	word
#define	word		unsigned short
#endif

#ifndef	dword
#define	dword		unsigned long
#endif

#ifndef	ulong
#define	ulong		unsigned long
#endif

#ifndef	Bool
#define	Bool		int
#endif

#ifndef	FALSE
#define	FALSE		0
#endif

#ifndef	TRUE
#define	TRUE		(!FALSE)
#endif

#define	IntToBool(v)	((v)?TRUE:FALSE)
#define	PRINT_BOOL(b)	((b) ? "T" : "F")

#define	TO_INT(x)	((x) - '0')
#define	TO_CHAR(x)	((x) + '0')

typedef	struct	POOL_S	{
	struct	POOL_S	*next;
#if defined(sparc)
	long dummy;					/* for 8byte alignment */
#endif
}	POOL;

#define	ALIGN_BYTES		sizeof(int)

#define	ROUND_ALIGN(p)	\
	((((p)%ALIGN_BYTES) == 0) ? (p) : (((p)/ALIGN_BYTES)+1)*ALIGN_BYTES)

#endif

