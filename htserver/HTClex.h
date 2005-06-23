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

#ifndef	_INC_HTC_LEX_H
#define	_INC_HTC_LEX_H
#include	<glib.h>

#define	YYBASE			256
#define	T_EOF			(YYBASE +1)
#define	T_SYMBOL		(YYBASE +2)
#define	T_SCONST		(YYBASE +3)
#define	T_ICONST		(YYBASE +4)
#define	T_YYBASE		(YYBASE+4)

#define	T_COMMENT		(T_YYBASE +1)
#define	T_COMMENTE		(T_YYBASE +2)

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
