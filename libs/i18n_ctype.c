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

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	"types.h"
#include	"i18n_struct.h"
#include	"i18n_const.h"

#include	"misc.h"
#include	"i18n_code.h"
#include	"i18n_codeset.h"
#include	"debug.h"

extern	int
ToDigit(
	Char	c)
{	int		ret;

	if		(  c  ==  Char_EOF  )	{
		ret = 0;
	} else
	if		(	(  SET_Char(c)  ==  SET_ASCII  )
			 &&	(  isdigit(CODE_Char(c))       ) )	{
		ret = CODE_Char(c) - '0';
	} else {
		ret = 0;
	}
	return	(ret);
}

extern	Bool
IsSpace(
	Char	c)
{	Bool	ret;

	if		(  c  ==  Char_EOF  )	{
		ret = FALSE;
	} else
	if		(	(  SET_Char(c)  ==  SET_ASCII  )
			 &&	(  isspace(CODE_Char(c))       ) )	{
		ret = TRUE;
	} else {
		ret = FALSE;
	}
	return	(ret);
}

extern	Bool
IsDigit(
	Char	c)
{	Bool	ret;

	if		(  c  ==  Char_EOF  )	{
		ret = FALSE;
	} else
	if		(	(  SET_Char(c)  ==  SET_ASCII  )
			 &&	(  isdigit(CODE_Char(c))       ) )	{
		ret = TRUE;
	} else {
		ret = FALSE;
	}
	return	(ret);
}

extern	Bool
IsAlnum(
	Char	c)
{	Bool	ret;

	if		(  c  ==  Char_EOF  )	{
		ret = FALSE;
	} else
	if		(	(  SET_Char(c)  ==  SET_ASCII  )
			 &&	(  isalnum(CODE_Char(c))       ) )	{
		ret = TRUE;
	} else {
		ret = FALSE;
	}
	return	(ret);
}
