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

static	iconv_t			ReadCd;
static	byte	obuff[SIZE_CHARS];
static	size_t	sob
		,		op;

static	GHashTable	*Reserved;
extern	void
HTCLexInit(void)
{
	int		i;

	Reserved = NewNameHash();
	for	( i = 0 ; tokentable[i].token  !=  0 ; i ++ ) {
		g_hash_table_insert(Reserved,tokentable[i].str,(gpointer)tokentable[i].token);
	}
	ReadCd = (iconv_t)0;
	op = 0;
	sob = SIZE_CHARS;
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
	if		(  libmondai_i18n  ) {
		if		(  ReadCd  !=  (iconv_t)0  ) {
			iconv_close(ReadCd);
		}
		ReadCd = iconv_open("utf8",codeset);
	}
}

extern	int
GetCharFile(void)
{
	int		c;
	byte	ibuff[SIZE_CHARS]
	,		*ic;
	char	*oc
	,		*istr;
	int		ret;
	size_t	count;
	int		rc;

ENTER_FUNC;
  retry:
	if		(  op  <  ( SIZE_CHARS - sob )  ) {
		ret = obuff[op];
		op ++;
	} else {
		op = 0;
		if		(  ( c = fgetc(HTC_File) )  !=  EOF  ) {
			if		(	(  libmondai_i18n  )
					&&	(  ReadCd  !=  (iconv_t)0  ) ) {
				ic = ibuff;
				do {
					*ic ++ = c;
					istr = ibuff;
					count = (size_t)(ic - ibuff);
					oc = obuff;
					sob = SIZE_CHARS;
					if		(  ( rc = iconv(ReadCd,&istr,&count,&oc,&sob) )  !=  0  ) {
						if		(  ( c = fgetc(HTC_File) )  ==  EOF  )	break;
					}
				}	while	(  rc  !=  0  );
				if		(  c  ==  EOF  ) {
					ret = -1;
				} else goto	retry;
			} else {
				sob = SIZE_CHARS;
				ret = c;
			}
		} else {
			ret = -1;
		}
	}
    if (ret == '\n')
        HTC_cLine++;
LEAVE_FUNC;
	return	(ret);
}

extern	void
UnGetCharFile(
	int		c)
{
	ungetc(c,HTC_File);
}

#define	SKIP_SPACE	while	(  isspace( c = _HTCGetChar() ) ) {	\
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

dbgmsg(">HTCLex");
  retry:
	SKIP_SPACE; 
	if		(  c  ==  '#'  ) {
		while	(  ( c = _HTCGetChar() )  !=  '\n'  );
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = HTC_ComSymbol;
		len = 0;
		while	(  ( c = _HTCGetChar() )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				switch	(c2 = _HTCGetChar()) {
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
					_HTCUnGetChar(c2);
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
		while	(  ( c = _HTCGetChar() )  !=  '\''  ) {
			if		(  c  ==  '\\'  ) {
				switch	(c2 = _HTCGetChar()) {
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
					_HTCUnGetChar(c2);
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
			c = _HTCGetChar();
		}	while	(  c  ==  '-'  );
		*s = 0;
		if		(  c  ==  '>'  ) {
			if		(  len  >  1  ) {
				token = T_COMMENTE;
			} else {
				_HTCUnGetChar(c);
				token = T_SYMBOL;
			}
		} else {
			_HTCUnGetChar(c);
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
			c = _HTCGetChar();
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '.' )
					 ||	(  c  ==  '_' )
					 ||	(  c  ==  '-' )
					 ||	(  c  ==  ':' ) );
		*s = 0;
		_HTCUnGetChar(c);
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
dbgmsg("<HTCLex");
	return	(token);
}

