/*	PANDA -- a simple transaction monitor

Copyright (C) 2002 Ogochan & JMA (Japan Medical Association).

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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<math.h>

#include	"types.h"
#include	"const.h"
#include	"misc.h"
#include	"LBSfunc.h"
#include	"debug.h"

extern	LargeByteString	*
NewLBS(void)
{
	LargeByteString	*lbs;

	lbs = New(LargeByteString);
	lbs->ptr = 0;
	lbs->size = SIZE_GLOWN;
	lbs->body = xmalloc(lbs->size);

	return	(lbs);
}

extern	void
FreeLBS(
	LargeByteString	*lbs)
{
	if		(  lbs  !=  NULL  ) {
		if		(  lbs->body  !=  NULL  ) {
			xfree(lbs->body);
		}
		xfree(lbs);
	}
}

extern	void
LBS_RequireSize(
	LargeByteString	*lbs,
	size_t			size,
	Bool			fKeep)
{
	byte	*body;

	if		(  lbs->size  <  size  ) {
		body = (byte *)xmalloc(size);
		if		(  fKeep  ) {
			memcpy(body,lbs->body,lbs->size);
		}
		xfree(lbs->body);
		lbs->body = body;
		lbs->size = size;
	}
	lbs->ptr = size;
}

extern	int
LBS_FetchByte(
	LargeByteString	*lbs)
{
	int		ret;

	if		(  lbs->ptr  <  lbs->size  ) {
		ret = lbs->body[lbs->ptr];
		lbs->ptr ++;
	} else {
		ret = -1;
	}
	return	(ret);
}

extern	int
LBS_FetchChar(
	LargeByteString	*lbs)
{
	int		ret;

	if		(  ( ret = LBS_FetchByte(lbs) )  ==  LBS_QUOTE_MSB  ) {
		ret = 0x80 | LBS_FetchByte(lbs);
	}
	if		(  ret  <  0  ) {
		ret = 0;
	}
	return	(ret);
}

extern	void	*
LBS_FetchPointer(
	LargeByteString	*lbs)
{
	int		ret;
	int		i;

	ret = 0;
	for	( i = 0 ; i < sizeof(void *) ; i ++ ) {
		ret <<= 8;
		ret |= LBS_FetchByte(lbs);
	}
	return	((void *)ret);
}

extern	int
LBS_FetchInt(
	LargeByteString	*lbs)
{
	int		ret;
	int		i;

	ret = 0;
	for	( i = 0 ; i < sizeof(void *) ; i ++ ) {
		ret <<= 8;
		ret |= LBS_FetchByte(lbs);
	}
	return	(ret);
}

extern	void
LBS_EmitStart(
	LargeByteString	*lbs)
{
	lbs->ptr = 0;
	memclear(lbs->body,lbs->size);
}

extern	void
LBS_Emit(
	LargeByteString	*lbs,
	byte			code)
{
	byte	*body;

	if		(  lbs->ptr  ==  lbs->size  ) {
		lbs->size += SIZE_GLOWN;
		body = (byte *)xmalloc(lbs->size);
		memcpy(body,lbs->body,lbs->ptr);
		xfree(lbs->body);
		lbs->body = body;
	}
	lbs->body[lbs->ptr] = code;
	lbs->ptr ++;
}

extern	void
LBS_EmitChar(
	LargeByteString	*lbs,
	char			c)
{
	if		(  ( c & 0x80 )  ==  0x80  ) {
		LBS_Emit(lbs,LBS_QUOTE_MSB);
		LBS_Emit(lbs,(c & 0x7F));
	} else {
		LBS_Emit(lbs,c);
	}
}

extern	void
LBS_EmitString(
	LargeByteString	*lbs,
	char			*str)
{
	while	(  *str  !=  0  ) {
		LBS_EmitChar(lbs,*str);
		str ++;
	}
}

extern	void
LBS_EmitPointer(
	LargeByteString	*lbs,
	void			*p)
{
	LBS_Emit(lbs,(((int)p & 0xFF000000) >> 24));
	LBS_Emit(lbs,(((int)p & 0x00FF0000) >> 16));
	LBS_Emit(lbs,(((int)p & 0x0000FF00) >>  8));
	LBS_Emit(lbs,(((int)p & 0x000000FF)      ));
}

extern	void
LBS_EmitInt(
	LargeByteString	*lbs,
	int				i)
{
	LBS_Emit(lbs,((i & 0xFF000000) >> 24));
	LBS_Emit(lbs,((i & 0x00FF0000) >> 16));
	LBS_Emit(lbs,((i & 0x0000FF00) >>  8));
	LBS_Emit(lbs,((i & 0x000000FF)      ));
}

extern	void
LBS_EmitFix(
	LargeByteString	*lbs)
{
	byte	*body;

	if		(  lbs->ptr  >  0  ) {
		body = (byte *)xmalloc(lbs->ptr);
		memcpy(body,lbs->body,lbs->ptr);
		xfree(lbs->body);
		lbs->body = body;
	} else {
		xfree(lbs->body);
		lbs->body = NULL;
	}
	lbs->size = lbs->ptr;
	lbs->ptr = 0;
}

extern	size_t
LBS_StringLength(
	LargeByteString	*lbs)
{
	size_t	size;
	int		c;

	size = 0;
	RewindLBS(lbs);
	while	(  ( c = LBS_FetchByte(lbs) )  >  0  ) {
		if		(  c  !=  LBS_QUOTE_MSB  ) {
			size ++;
		}
	}
	return	(size);
}

extern	char	*
LBS_ToString(
	LargeByteString	*lbs)
{
	char	*ret
	,		*p;

	ret = (char *)xmalloc(LBS_StringLength(lbs) + 1);
	p = ret;
	RewindLBS(lbs);
	while	(  ( *p = LBS_FetchChar(lbs) )  !=  0  ) {
		p ++;
	}
	*p = 0;
	return	(ret);
}
