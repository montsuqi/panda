/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).

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
		ret = (int)p;
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

