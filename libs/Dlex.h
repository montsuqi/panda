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

#ifndef	_INC_D_LEX_H
#define	_INC_D_LEX_H
#include	<glib.h>
#include	"Lex.h"
/*	common	*/
#define	T_NAME				(T_YYBASE +1)
#define	T_ARRAYSIZE			(T_YYBASE +2)
#define	T_TEXTSIZE			(T_YYBASE +3)
#define	T_DB				(T_YYBASE +4)
#define	T_BIND				(T_YYBASE +5)
/*	LD only	*/
#define	T_DATA				(T_YYBASE +6)
#define	T_SPA				(T_YYBASE +7)
#define	T_WINDOW			(T_YYBASE +8)
#define	T_PORT				(T_YYBASE +9)
#define	T_HOST				(T_YYBASE +10)
#define	T_LINKAGE			(T_YYBASE +11)
#define	T_MGROUP			(T_YYBASE +12)
#define	T_HOME				(T_YYBASE +13)
#define	T_CACHE				(T_YYBASE +15)
/*	handler	*/
#define	T_HANDLER			(T_YYBASE +16)
#define	T_CLASS				(T_YYBASE +17)
#define	T_SERIALIZE			(T_YYBASE +18)
#define	T_START				(T_YYBASE +19)
#define	T_LOCALE			(T_YYBASE +20)
#define	T_CODING			T_LOCALE
#define	T_ENCODING			(T_YYBASE +21)
#define	T_LOADPATH			(T_YYBASE +22)
#define	T_HANDLERPATH		(T_YYBASE +23)
#define	T_BIGENDIAN			(T_YYBASE +24)

#undef	GLOBAL
#ifdef	_D_PARSER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
#undef	GLOBAL

#endif
