/*	PANDA -- a simple transaction monitor

Copyright (C) 2002 Ogochan & JMA (Japan Medical Association).

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
#include	<signal.h>
#include	<string.h>
#include	<termio.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"
#include	"trid.h"
#include	"debug.h"

static	char	*EnBase = "AAAAAAAAAAAAAAAA";
extern	void
DecodeTRID(
	int		*ses,
	int		*count,
	char	*trid)
{
	int		i;
	char	*base;

	*ses = 0;
	*count = 0;
	base = EnBase;
	for	( i = 0 ; i < 32 ; i += 4, trid ++ ) {
		(*ses) <<= 4;
		*ses |= ( *trid - *base );
		base ++;
	}
	for	( i = 0 ; i < 32 ; i += 4, trid ++ ) {
		(*count) <<= 4;
		*count |= ( *trid - *base );
		base ++;
	}
}

extern	void
EncodeTRID(
	char	*trid,
	int		ses,
	int		count)
{
	int		i;
	unsigned	int		mask;
	char	*base;

	printf("ses = %d\n",ses);
	base = EnBase;
	mask = 0xF0000000;
	for	( i = 32 ; i > 0 ; i -= 4 , trid ++ ) {
		*trid = *base + ( ( ses & mask ) >> ( i - 4 ) );
		mask >>= 4;
		base ++;
	}
	mask = 0xF0000000;
	for	( i = 32 ; i > 0 ; i -= 4 , trid ++ ) {
		*trid = *base + ( ( count & mask ) >> ( i - 4 ) );
		mask >>= 4;
		base ++;
	}
	*trid = 0;
}

