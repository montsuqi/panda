/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_HTC_LEX_H
#define	_INC_HTC_LEX_H
#include	<glib.h>

#define	YYBASE			256
#define	T_EOF			(YYBASE +1)
#define	T_SYMBOL		(YYBASE +2)
#define	T_SCONST		(YYBASE +4)

#undef	GLOBAL
#ifdef	_HTC_PARSER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
GLOBAL	char	HTC_ComSymbol[SIZE_SYMBOL+1];
GLOBAL	char	*HTC_FileName;
GLOBAL	int		HTC_Token;
GLOBAL	FILE	*HTC_File;
GLOBAL	int		HTC_cLine;
GLOBAL	byte	*HTC_Memory;

GLOBAL	int		(*_HTCGetChar)(void);
GLOBAL	void	(*_HTCUnGetChar)(int c);

#undef	GLOBAL

extern	int		GetCharFile(void);
extern	void	UnGetCharFile(int c);
extern	int		HTCLex(Bool fSymbol);
extern	void	HTCLexInit(void);
extern	void	HTCSetCodeset(char *codeset);

#endif
