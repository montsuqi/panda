/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_COBOL_VALUE_H
#define	_COBOL_VALUE_H
extern	void	DumpCobol(char *name, char *p, size_t size);
extern	void	StringCobol2C(char *str, size_t size);
extern	void	StringC2Cobol(char *p, size_t size);
#define	IntegerC2Cobol	IntegerCobol2C

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
#endif
