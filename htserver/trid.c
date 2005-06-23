/*
PANDA -- a simple transaction monitor
Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
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
#include	<stdio.h>
#include	"trid.h"

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

