/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_D_LEX_H
#define	_INC_D_LEX_H
#include	<glib.h>
#include	"Lex.h"
/*	common	*/
#define	T_NAME			(T_YYBASE +1)
#define	T_ARRAYSIZE		(T_YYBASE +2)
#define	T_TEXTSIZE		(T_YYBASE +3)
#define	T_DB			(T_YYBASE +4)
#define	T_BIND			(T_YYBASE +5)
/*	LD only	*/
#define	T_DATA			(T_YYBASE +6)
#define	T_SPA			(T_YYBASE +7)
#define	T_WINDOW		(T_YYBASE +8)
#define	T_PORT			(T_YYBASE +9)
#define	T_HOST			(T_YYBASE +10)
#define	T_LINKAGE		(T_YYBASE +11)
#define	T_MGROUP		(T_YYBASE +12)
#define	T_HOME			(T_YYBASE +13)
#define	T_WFC			(T_YYBASE +14)
#define	T_CACHE			(T_YYBASE +15)
/*	handler	*/
#define	T_HANDLER		(T_YYBASE +16)
#define	T_CLASS			(T_YYBASE +17)
#define	T_SERIALIZE		(T_YYBASE +18)
#define	T_START			(T_YYBASE +19)
#define	T_LOCALE		(T_YYBASE +20)
#define	T_ENCODING		(T_YYBASE +21)
#define	T_LOADPATH		(T_YYBASE +22)

#undef	GLOBAL
#ifdef	_D_PARSER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
#undef	GLOBAL

#endif
