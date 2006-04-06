/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2006 Ogochan.
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
#include	<glib.h>
#include	<errno.h>
#include	<iconv.h>

#include	"types.h"
#include	"libmondai.h"
#include	"HTClex.h"
#include	"debug.h"

#define	SIZE_CHARS		16

static	struct	{
	char	*str;
	int		token;
}	tokentable[] = {
	{	"!--"		,T_COMMENT	},
	{	""			,0			}
};

static	GHashTable	*Reserved;
extern	void
HTCLexInit(void)
{
	int		i;

	Reserved = NewNameHash();
	for	( i = 0 ; tokentable[i].token  !=  0 ; i ++ ) {
		g_hash_table_insert(Reserved,tokentable[i].str,(gpointer)tokentable[i].token);
	}
}

static	int
CheckReserved(
	char	*str)
{
	gpointer	p;
	int		ret;

	if		(  ( p = g_hash_table_lookup(Reserved,str) ) !=  NULL  ) {
#ifndef __alpha__ 	  
		ret = (int)p;
#else
		long tmp =(long)p;
		ret = (int)tmp;
#endif
	} else {
		ret = T_SYMBOL;
	}
	return	(ret);
}

extern	void
HTCSetCodeset(
	char	*codeset)
{
	iconv_t		cd;
	size_t		sbuff;
	size_t	sib
		,	sibo
		,	sob;
	char	*buff
		,	*istr
		,	*ostr;

	if		(  libmondai_i18n  ) {
		cd = iconv_open("utf8",codeset);
		sbuff = SIZE_BUFF;
		sibo = strlen(HTC_Memory);
		while	(TRUE) {
			buff = (char *)xmalloc(sbuff);
			sib = sibo;
			istr = HTC_Memory;
			ostr = buff;
			sob = sbuff;
			if		(  iconv(cd,&istr,&sib,&ostr,&sob)  ==  0  ) break;
			if		(  errno  ==  E2BIG  ) {
				xfree(buff);
				sbuff *= 2;
			} else {
				dbgprintf("error = %d\n",errno);
				break;
			}
		}
		*ostr = 0;
		iconv_close(cd);
		if		(  _HTC_Memory  !=  NULL  ) {
			xfree(_HTC_Memory);
		}
		HTC_Memory = buff;
		_HTC_Memory = buff;
	}
}

extern	int
GetChar(void)
{
	int		ret;

ENTER_FUNC;
	if		(  ( ret = *HTC_Memory ++ )  ==  0  ) {
		ret = -1;
	}
    if (ret == '\n')
        HTC_cLine++;
LEAVE_FUNC;
	return	(ret);
}

extern	void
UnGetChar(
	int		c)
{
	HTC_Memory --;
	*HTC_Memory = c;
}

#define	SKIP_SPACE	while	(  isspace( c = GetChar() ) ) {	\
						if		(  c  ==  '\n'  ) {	\
							c = ' ';\
						}	\
					}

extern	int
HTCLex(
	Bool	fSymbol)
{
	int		c
		,	c2;
	size_t	len;
	int		token;
	char	*s;

ENTER_FUNC;
  retry:
	SKIP_SPACE; 
	if		(  c  ==  '#'  ) {
		while	(  ( c = GetChar() )  !=  '\n'  );
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = HTC_ComSymbol;
		len = 0;
		while	(  ( c = GetChar() )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				switch	(c2 = GetChar()) {
				  case	'"':
					c = '"';
					break;
				  case	'n':
					c = '\n';
					break;
				  case	't':
					c = '\t';
					break;
				  default:
					UnGetChar(c2);
					break;
				}
			}
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
		}
		*s = 0;
		token = T_SCONST;
	} else
	if		(  c  ==  '\''  ) {
		s = HTC_ComSymbol;
		len = 0;
		while	(  ( c = GetChar() )  !=  '\''  ) {
			if		(  c  ==  '\\'  ) {
				switch	(c2 = GetChar()) {
				  case	'\'':
					c = '\'';
					break;
				  case	'n':
					c = '\n';
					break;
				  case	't':
					c = '\t';
					break;
				  default:
					UnGetChar(c2);
					break;
				}
			}
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
		}
		*s = 0;
		token = T_SCONST;
	} else
	if		(  c  ==  '-'  ) {
		s = HTC_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar();
		}	while	(  c  ==  '-'  );
		*s = 0;
		if		(  c  ==  '>'  ) {
			if		(  len  >  1  ) {
				token = T_COMMENTE;
			} else {
				UnGetChar(c);
				token = T_SYMBOL;
			}
		} else {
			UnGetChar(c);
			token = T_SYMBOL;
		}
	} else
	if		(	(  isalpha(c)  )
			||	(  isdigit(c)  )
			||	(  c  ==  '/'  )
			||	(  c  ==  '!'  ) ) {
		s = HTC_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar();
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '.' )
					 ||	(  c  ==  '_' )
					 ||	(  c  ==  '-' )
					 ||	(  c  ==  ':' ) );
		*s = 0;
		UnGetChar(c);
		if		(  fSymbol  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(HTC_ComSymbol);
		}
	} else {
		switch	(c) {
		  case	EOF:
			token = T_EOF;
			break;
		  default:
			token = c;
			break;
		}
	}
LEAVE_FUNC;
	return	(token);
}

