/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_DD_LEX_H
#define	_INC_DD_LEX_H
#include	<glib.h>

#define	SIZE_SYMBOL		255

#define	YYBASE			256
#define	T_EOF			(YYBASE +1)
#define	T_SYMBOL		(YYBASE +2)
#define	T_SCONST		(YYBASE +3)

#define	T_ICONST		(YYBASE +4)
#define	T_CHAR			(YYBASE +5)
#define	T_TEXT			(YYBASE +6)
#define	T_INT			(YYBASE +7)
#define	T_INPUT			(YYBASE +8)
#define	T_OUTPUT		(YYBASE +9)
#define	T_BOOL			(YYBASE +10)
#define	T_BYTE			(YYBASE +11)
#define	T_NUMBER		(YYBASE +12)
#define	T_PRIMARY		(YYBASE +13)
#define	T_PATH			(YYBASE +14)
#define	T_VARCHAR		(YYBASE +15)
#define	T_VIRTUAL		(YYBASE +16)
#define	T_DBCODE		(YYBASE +17)

#undef	GLOBAL
#ifdef	_DD_PARSER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif

GLOBAL	char	DD_ComSymbol[SIZE_SYMBOL+1];
GLOBAL	int		DD_ComInt;
GLOBAL	int		DD_Token;
GLOBAL	FILE	*DD_File;
GLOBAL	char	*DD_FileName;
GLOBAL	int		DD_cLine;
GLOBAL	Bool	fDD_Error;

#undef	GLOBAL

extern	int		DD_Lex(Bool fSymbol);
extern	void	DD_LexInit(void);


#endif
