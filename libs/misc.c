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

/*
#define	TRACE
*/
/*
*	misc library module
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<malloc.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	"types.h"
#include	"misc.h"
#include	"debug.h"

static	size_t	total = 0;

extern	void	*
_xmalloc(
	size_t	size,
	char	*fn,
	int		line)
{	void	*ret;

	total += size; 
#ifdef	TRACE
	printf("xmalloc %s(%d),size = %d,",fn,line,size);fflush(stdout);
#endif
	if		(  ( ret = malloc(size) )  ==  NULL  )	{
		printf("no memory space!! %s(%d)\n",fn,line);
		exit(1);
	}
#ifdef	TRACE
	printf("allocate = %p, total = %d\n",ret,(int)total);fflush(stdout);
#endif
	return	(ret);
}

extern	void
_xfree(
	void	*p,
	char	*fn,
	int		line)
{
#ifdef	TRACE
	printf("xfree %s(%d)\n",fn,line);
#endif
#ifndef	OKI
	free(p);
#endif
}

static	POOL	*pNew;
extern	void	*
_GetArea(
	size_t	size,
	char	*fn,
	int		line)
{	POOL	*pool
	,		*top;
	void	*p;

#ifdef	TRACE
	printf("GetArea %s(%d)\t",fn,line);
#endif
	if		(  ( pool = (POOL *)xmalloc(size + sizeof(POOL)) )  ==  NULL  )	{
#ifdef DEBUG
		printf("no memory space!! %s(%d) %d\n",fn,line,size);
#endif
		abort();
	}
	top = pNew;
	pool->next = top->next;
	top->next = pool;
	p = (void *)&pool[1];

	memset(p,0,size);
#ifdef	TRACE
	printf("allocate = %p , size = %ld\n",pool,(long)size);
#endif
	return	(p);
}

extern	void
_ReleaseArea(
	void *p)
{
	POOL	*np
	,		*rp;

	if (!p) return;
dbgmsg(">_ReleaseArea");
	rp = pNew;
	while (rp->next) {
		np = rp->next;
		if ((&np[1]) == p) {
			rp->next = np->next;
			free(np);
			return;
		}
		rp = np;
	}
dbgmsg("<_ReleaseArea");
}

extern	POOL	*
MarkPool(void)
{
dbgmsg("MarkPool");
	return	(pNew);
}

extern	void
ReleasePool(
	POOL	*point)
{	POOL	*np
	,		*rp;

dbgmsg(">ReleasePool");
	np = (  point  ==  NULL ) ? pNew : point;
	rp = np->next;
	np->next = NULL;
	while	(  rp  !=  NULL  )	{
		np = rp->next;
		free(rp);
		rp = np;
	}
dbgmsg("<ReleasePool");
}

extern	void
InitPool(void)
{
dbgmsg(">InitPool");
	 pNew = (POOL *)xmalloc(sizeof(POOL));
	pNew->next = NULL;
dbgmsg("<InitPool");
}

extern	FILE	*
Fopen(
	char	*name,
	char	*mode)
{
	char	*p
	,		buff[MAXNAMLEN+1];

	for ( p = name ; ( p = strchr(p, '/') ) != NULL ; p ++ )	{
		memcpy(buff, name, p - name);
		buff[p - name] = 0;
		mkdir(buff,0755);
	}
	return	(fopen(name,mode));
}

#ifdef	__GNUC__
extern	int
stricmp(
	char	*s1,
	char	*s2)
{	int		ret;

	ret = 0;
	for	( ; *s1  !=  0 ; s1 ++ , s2 ++ )	{
		if		(  ( ret = toupper(*s1) - toupper(*s2) )  !=  0  )
			break;
	}
	return	(ret);
}
extern	int
strnicmp(
	char	*s1,
	char	*s2,
	size_t	l)
{	int		ret;

	ret = 0;
	for	( ; l > 0 ; l -- , s1 ++ , s2 ++ )	{
		if		(  ( ret = toupper(*s1) - toupper(*s2) )  !=  0  )
			break;
	}
	return	(ret);
}
#endif

extern	char	*
StrDup(
	char	*s)
{
	char	*str;

	if		(  s  !=  NULL  ) {
		str = xmalloc(strlen(s)+1);
		strcpy(str,s);
	} else {
		str = NULL;
	}
	return	(str);
}

extern	long
StrToInt(
	char	*str,
	size_t	len)
{	long	ret;

	ret = 0L;
	for	( ; len > 0 ; len -- )	{
		if		(  isdigit(*str)  )	{
			ret = ret * 10 + ( *str - '0' );
		}
		str ++;
	}
	return	(ret);
}

extern	long
HexToInt(
	char	*str,
	size_t	len)
{	long	ret;

	ret = 0L;
	for	( ; len > 0 ; len -- )	{
		ret <<= 4;
		if		(  isdigit(*str)  )	{
			ret += ( *str - '0' );
		} else
		if		(  isxdigit(*str)  )	{
			ret += ( toupper(*str) - 'A' + 10 );
		}
		str ++;
	}
	return	(ret);
}

extern	char	*
IntToStr(
	char	*str,
	long	val,
	size_t	len)
{
	str += len;
#if	0
	*str = 0;
#endif
	for	( ; len > 0 ; len -- )	{
		str --;
		*str = (char)(( val % 10 ) + '0');
		val /= 10;
	}
	return	(str);
}

extern	char	*
IntStrDup(
	int		val)
{
	char	buff[20];

	sprintf(buff,"%d",val);
	return	(StrDup(buff));
}

extern	void
MakeCobolX(
	char	*to,
	size_t	len,
	char	*from)
{
	for	( ; len > 0 ; len -- )	{
		if		(  *from  ==  0  )	{
			*to = ' ';
		} else {
			*to = *from ++;
		}
		to ++;
	}
	*to = 0;
}

extern	void
CopyCobol(
	char	*to,
	char	*from)
{
	while	(  !isspace(*from)  )	{
		*to ++ = *from ++ ;
	}
	*to = 0;
}

extern	void
PrintFixString(
	char	*s,
	int		len)
{	int		i;

	for	( i = 0 ; i < len ; i ++ )	{
		putchar(*s);
		s ++;
	}
}

extern	void
AdjustByteOrder(
	void	*to,
	void	*from,
	size_t	size)
#ifndef	BIG_ENDIAN
{
	memcpy(to,from,size);
}
#else
{	byte	*p
	,		*q;

	p = (byte *)to + size - 1;
	for	( q = from ; p >= (byte *)to ; p -- , q ++ )	{
		*p = *q;
	}
}
#endif
#if	0
extern	Bool
IsAlnum(
	int		c)
{	int		ret;

	ret = 	(	( c >= 0x20 )
			 && ( c <  0x7F ) ) ? TRUE : FALSE;
	return	(ret);
}
#endif
