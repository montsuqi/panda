/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

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
