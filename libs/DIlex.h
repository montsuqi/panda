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

#ifndef	_INC_DI_LEX_H
#define	_INC_DI_LEX_H
#include	<glib.h>

#define	SIZE_SYMBOL		255

#define	YYBASE			256
#define	T_EOF			(YYBASE +1)
#define	T_SYMBOL		(YYBASE +2)
#define	T_ICONST		(YYBASE +3)
#define	T_SCONST		(YYBASE +4)
#define	T_NAME			(YYBASE +5)
#define	T_LD			(YYBASE +6)
#define	T_LINKSIZE		(YYBASE +7)
#define	T_MULTI			(YYBASE +8)
#define	T_BD			(YYBASE +9)
#define	T_DBGROUP		(YYBASE +10)
#define	T_PORT			(YYBASE +11)
#define	T_USER			(YYBASE +12)
#define	T_PASS			(YYBASE +13)
#define	T_TYPE			(YYBASE +14)
#define	T_FILE			(YYBASE +15)
#define	T_REDIRECT		(YYBASE +16)
#define	T_REDIRECTPORT	(YYBASE +17)
#define	T_LDDIR			(YYBASE +18)
#define	T_BDDIR			(YYBASE +19)
#define	T_RECDIR		(YYBASE +20)
#define	T_BASEDIR		(YYBASE +21)
#define	T_PRIORITY		(YYBASE +22)
#define	T_LINKAGE		(YYBASE +23)
#define	T_STACKSIZE		(YYBASE +24)

#undef	GLOBAL
#ifdef	_DI_PARSER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
GLOBAL	char	DI_ComSymbol[SIZE_SYMBOL+1];
GLOBAL	int		DI_ComInt;
GLOBAL	int		DI_Token;
GLOBAL	FILE	*DI_File;
GLOBAL	char	*DI_FileName;
GLOBAL	int		DI_cLine;
#undef	GLOBAL

extern	int		DI_Lex(Bool fSymbol);
extern	void	DI_LexInit(void);

#endif
