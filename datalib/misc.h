/*	PANDA -- a simple transaction monitor

Copyright (C) 1989-2002 Ogochan.

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

#ifndef	_INC_MISC_H
#define	_INC_MISC_H

#include	<strings.h>
#include	"types.h"

#define	memclear(b,s)	memset((b),0,(s))
#define	strlcmp(s1,s2)	strncmp((s1),(s2),strlen(s2))
#define	strlicmp(s1,s2)	strnicmp((s1),(s2),strlen(s2))

/*	for memory management	*/
extern	void	*_xmalloc(size_t,char *,int);
extern	void	_xfree(void *,char *,int);
extern	void	*_GetArea(size_t,char *,int);
extern	void	_ReleaseArea(void *);
extern	POOL	*MarkPool(void);
extern	void	ReleasePool(POOL *);
extern	void	InitPool(void);
#define	New(s)				(s *)xmalloc(sizeof(s))
#define	xmalloc(s)			_xmalloc((s),__FILE__,__LINE__)
#define	xfree(p)			_xfree((p),__FILE__,__LINE__)
#if	1
#define	GetArea(s)		_GetArea((s),__FILE__,__LINE__)
#define	ReleaseArea(p)	_ReleaseArea(p)
#else
#define	GetArea(sts,s)		_xmalloc((s),__FILE__,__LINE__)
#define	ReleaseArea(sts,p)	_xfree(p)
#endif

extern	FILE	*Fopen(char *name, char *mode);
#ifdef	__GNUC__
extern	int		stricmp(char *s1, char *s2);
extern	int		strnicmp(char *s1, char *s2, size_t l);
#endif
extern	char	*StrDup(char *s);
extern	char	*IntStrDup(int val);
extern	long	StrToInt(char *str, size_t len);
extern	long	HexToInt(char *str, size_t len);
extern	char	*IntToStr(char *str, long val, size_t len);
extern	void	MakeCobolX(char *to, size_t len, char *from);
extern	void	CopyCobol(char *to, char *from);
extern	void	PrintFixString(char *s, int len);
extern	void	AdjustByteOrder(void *to, void *from, size_t size);
#endif
