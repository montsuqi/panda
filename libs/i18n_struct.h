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

#ifndef	_I18N_STRUCT_H
#define	_I18N_STRUCT_H
#include	<stdio.h>
#include	"types.h"

typedef	struct	{
#ifdef	BIG_ENDIAN
	word	set;		/*	文字セット		*/
	word	code;		/*	文字コード		*/
#else
	word	code;		/*	文字コード		*/
	word	set;		/*	文字セット		*/
#endif
}	_Char;

#define	Char			dword

typedef	struct	STREAM_S	{
	union	{
		FILE	*fp;
		char	*str;
	}	source;
	int		(*GetByte)(struct STREAM_S *);
	int		(*PutByte)(struct STREAM_S *,int);
	void	(*Close)(struct STREAM_S *);
	int		bBack;
	Bool	fEOF;
	struct	{
		int		set;
		Char	cBack;
		int		G0
		,		G1
		,		G2
		,		G3;
		int		*GL
		,		*GR
		,		*GLB;
	}	code;
}	STREAM;

typedef	struct	CODE_SET_S{
	struct	CODE_SET_S	*next
	,					*nnext;
	char	*name;
	int		no;
	struct	ESCAPE_TABLE_S	*G[4];
	Bool	fGR;
}	CODE_SET;

typedef	struct	ESCAPE_TABLE_S	{
	struct	ESCAPE_TABLE_S	*next;
	char	*esc;
	CODE_SET	*set;
	int		dig;
}	ESCAPE_TABLE;

#endif
