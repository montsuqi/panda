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
#include	"types.h"
#include	"misc.h"
#include	"dirs.h"
#include	"libmondai.h"
#include	"HTClex.h"
#include	"debug.h"

typedef	struct	INCFILE_S	{
	struct	INCFILE_S	*next;
	fpos_t				pos;
	int					cLine;
	char				*fn;
}	INCFILE;
static	INCFILE		*ftop;

static	struct	{
	char	*str;
	int		token;
}	tokentable[] = {
	{	""			,0	}
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
		ret = (int)p;
	} else {
		ret = T_SYMBOL;
	}
	return	(ret);
}

extern	int
HTCGetChar(void)
{
	int		c;

	c = fgetc(HTC_File);
	if		(  c  ==  EOF  ) {
		c = T_EOF;
	}
	return	(c);
}

extern	void
HTCUnGetChar(
	int		c)
{
	ungetc(c,HTC_File);
}

#define	SKIP_SPACE	while	(  isspace( c = HTCGetChar() ) ) {	\
						if		(  c  ==  '\n'  ) {	\
							c = ' ';\
							HTC_cLine ++;\
						}	\
					}

static	void
DoInclude(
	char	*fn)
{
	INCFILE	*back;
	char	name[SIZE_BUFF];
	char		buff[SIZE_BUFF];
	char		*p
	,			*q;

dbgmsg(">DoInclude");
	back = New(INCFILE);
	back->next = ftop;
	back->fn =  HTC_FileName;
	fgetpos(HTC_File,&back->pos);
	back->cLine = HTC_cLine;
	ftop = back;
	fclose(HTC_File);
	strcpy(buff,".");	/*	*/
	p = buff;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s",p,fn);
		if		(  ( HTC_File = fopen(name,"r") )  !=  NULL  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );
	if		(  HTC_File  ==  NULL  ) {
		Error("include not found");
	}
	HTC_cLine = 1;
	HTC_FileName = StrDup(name);
dbgmsg("<DoInclude");
}

static	void
ExitInclude(void)
{
	INCFILE	*back;

dbgmsg(">ExitInclude");
	fclose(HTC_File);
	back = ftop;
	HTC_File = fopen(back->fn,"r");
	xfree(HTC_FileName);
	HTC_FileName = back->fn;
	fsetpos(HTC_File,&back->pos);
	HTC_cLine = back->cLine;
	ftop = back->next;
	xfree(back);
dbgmsg("<ExitInclude");
}

static	void
ReadyDirective(void)
{
	char	buff[SIZE_BUFF];
	char	fn[SIZE_BUFF];
	char	*s;
	int		c;

dbgmsg(">ReadyDirective");
	SKIP_SPACE;
	s = buff;
	do {
		*s = c;
		s ++;
	}	while	(  !isspace( c = HTCGetChar() )  );
	*s = 0;
	if		(  !stricmp(buff,"include")  ) {
		SKIP_SPACE;
		s = fn;
		switch	(c) {
		  case	'"':
			while	(  ( c = HTCGetChar() )  !=  '"'  ) {
				*s ++ = c;
			}
			*s = 0;
			break;
		  case	'<':
			while	(  ( c = HTCGetChar() )  !=  '>'  ) {
				*s ++ = c;
			}
			*s = 0;
			break;
		  default:
			*s = 0;
			break;
		}
		if		(  *fn  !=  0  ) {
			DoInclude(fn);
		}
	} else {
		HTCUnGetChar(c);
		while	(  ( c = HTCGetChar() )  !=  '\n'  );
		HTC_cLine ++;
	}
dbgmsg("<ReadyDirective");
}

extern	int
HTCLex(
	Bool	fSymbol)
{	int		c
	,		len;
	int		token;
	char	*s;

dbgmsg(">HTCLex");
  retry:
	SKIP_SPACE; 
	if		(  c  ==  '#'  ) {
		ReadyDirective();
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = HTC_ComSymbol;
		len = 0;
		while	(  ( c = HTCGetChar() )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = HTCGetChar();
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
	if		(	(  isalpha(c)  )
			||	(  isdigit(c)  )
			||	(  c  ==  '/'  ) ) {
		s = HTC_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = HTCGetChar();
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '.' )
					 ||	(  c  ==  '_' ) );
		*s = 0;
		HTCUnGetChar(c);
		if		(  fSymbol  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(HTC_ComSymbol);
		}
	} else {
		switch	(c) {
		  case	EOF:
			if		(  ftop  ==  NULL  )	{
				token = T_EOF;
			} else {
				ExitInclude();
				goto	retry;
			}
			break;
		  default:
			token = c;
			break;
		}
	}
dbgmsg("<HTCLex");
	return	(token);
}

