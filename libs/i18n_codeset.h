/*	PANDA -- a simple transaction monitor

Copyright (C) 1996-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

#ifndef	_INC_CODESET_H
#define	_INC_CODESET_H
#include	"i18n_struct.h"

extern	CODE_SET		*AddCodeSet(char *,int,int);
extern	void			AddEscape(char *,CODE_SET *,int);
extern	ESCAPE_TABLE	*CheckESC(char *,size_t);
extern	void			InitCodeSet(void);
extern	word			GetCodeSet(char *);
extern	CODE_SET		*GetEscape(int);
extern	void			DumpCodeSet(CODE_SET *);
#endif
