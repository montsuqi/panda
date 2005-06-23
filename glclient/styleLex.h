/*
PANDA -- a simple transaction monitor
Copyright (C) 1998-1999 Ogochan.
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_STYLE_LEX_H
#define	_INC_STYLE_LEX_H
#include	<glib.h>

#define	SIZE_SYMBOL		255

#define	YYBASE			256
#define	T_EOF			(YYBASE +1)
#define	T_SYMBOL		(YYBASE +2)
#define	T_ICONST		(YYBASE +3)
#define	T_SCONST		(YYBASE +4)
#define	T_STYLE			(YYBASE +5)
#define	T_FONTSET		(YYBASE +6)
#define	T_FG			(YYBASE +7)
#define	T_BG			(YYBASE +8)
#define	T_BG_PIXMAP		(YYBASE +9)
#define	T_TEXT			(YYBASE +10)
#define	T_BASE			(YYBASE +11)
#define	T_LIGHT			(YYBASE +12)
#define	T_DARK			(YYBASE +13)
#define	T_MID			(YYBASE +14)

#define	T_NORMAL		(YYBASE +101)
#define	T_PRELIGHT		(YYBASE +102)
#define	T_INSENSITIVE	(YYBASE +103)
#define	T_SELECTED		(YYBASE +104)
#define	T_ACTIVE		(YYBASE +105)

#ifdef	_STYLE_PARSER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif

GLOBAL	char	StyleComSymbol[SIZE_SYMBOL+1];
GLOBAL	int		StyleComInt;
GLOBAL	int		StyleToken;
GLOBAL	FILE	*StyleFile;
GLOBAL	int		StylecLine;

extern	int		StyleLex(Bool fSymbol);
extern	void	StyleLexInit(void);

#endif
