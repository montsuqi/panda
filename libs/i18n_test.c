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
#include	"i18n_code.h"

extern	int
main(
	int		argc,
	char	**argv)
{	STREAM	*is
	,		*os;
	Char	c;

	CodeInit(ENCODE_ISO2022);
#if	0
	is = CodeOpenString("koreha 漢字wo含んでいますkara douなるkaな");
#endif
	is = CodeOpenFile(argv[1],"r");
	os = CodeMakeFileStream(stdout);
	CodeSetEncode(os,ENCODE_ISO2022_);
	CodeSetEncode(is,ENCODE_ISO2022);
	while	(  ( c = CodeGetChar(is) )  !=  Char_EOF  )	{
		CodePutChar(os,c);
	}
	CodeClose(is);
	CodeClose(os);
	exit(0);
}
