/*	PANDA -- a simple transaction monitor

Copyright (C) 1996-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

#include	<stdlib.h>
#include	<string.h>
#include	"types.h"
#include	"i18n_struct.h"
#include	"i18n_const.h"

#include	"misc.h"
#include	"i18n_code.h"
#include	"debug.h"

extern	size_t
CStrLen(
	Char	*str)
{	Char	*p;

	for	( p = str ; *p != Char_NULL ; p ++ );
	return	((size_t)(p-str));
}

extern	Char	*
CStrCpy(
	Char	*d,
	Char	*s)
{	Char	*r;

	r = d;
	while	(  ( *d ++ = *s ++ )  !=  Char_NULL );
	return	(r);
}

extern	Char	*
CStrDup(
	Char	*str)
{	Char	*ret;

	ret = (Char *)xmalloc(sizeof(Char)*(CStrLen(str)+1));
	CStrCpy(ret,str);
	return	(ret);
}

extern	long
CStrCmp(
	Char	*s1,
	Char	*s2)
{
	while	(	(  *s1  !=  Char_NULL  )
			 &&	(  *s2  !=  Char_NULL  )
			 &&	(  *s1  ==  *s2        ) ) {
		s1 ++;
		s2 ++;
	}
	return	(((long)*s1 - (long)*s2));
}

extern	long
CStrnCmp(
	Char	*s1,
	Char	*s2,
	size_t	len)
{	long	ret;

	len --;
	while	(  len  >  0  ) {
		if		(  *s1  !=  *s2  )	break;
		s1 ++;
		s2 ++;
		len --;
	}
	if		(  *s1  ==  0  )	{
		ret = -1;
	} else {
		ret = (long)*s1 - (long)*s2;
	}
	return	(ret);
}

extern	void
StrToCStr(
	Char	*cstr,
	char	*str,
	int		encode)
{	STREAM	*is;
	Char	*p
	,		c;

	is = CodeOpenString(str);
	CodeSetEncode(is,encode);
	p = cstr;
	while	(	(  ( c  = CodeGetChar(is) )  !=  Char_EOF   )
			 &&	(  c                         !=  Char_NULL  ) )	{
		*p ++ = c;
	}
	*p = Char_NULL;
	CodeClose(is);
}

