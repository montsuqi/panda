/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef	_INC_SQL_LEX_H
#define	_INC_SQL_LEX_H
#include	<glib.h>
#include	"Lex.h"

#define	SIZE_SYMBOL		255

#define	T_SQL			(T_YYBASE +1)
#define	T_INTO			(T_YYBASE +2)
#define	T_INSERT		(T_YYBASE +3)
#define	T_LIKE			(T_YYBASE +4)
#define	T_ILIKE			(T_YYBASE +5)
#define	T_SELECT		(T_YYBASE +6)
#define	T_DECLARE		(T_YYBASE +7)
#define	T_WHERE			(T_YYBASE +8)

#ifdef	_SQL_PARSER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif

#undef	GLOBAL

extern	int		SQL_Lex(CURFILE *in, Bool fName);
extern	void	SQL_LexInit(void);
extern	void	UnGetChar(CURFILE *in, int c);
extern	int		GetChar(CURFILE *in);

#endif
