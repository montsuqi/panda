/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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
#include	"value.h"
#include	"BDlex.h"
#include	"debug.h"

typedef	struct	INCFILE_S	{
	struct	INCFILE_S	*next;
	fpos_t				pos;
	int					cLine;
	char				*fn;
}	INCFILE;
static	INCFILE		*ftop;

#define	GetChar(fp)		fgetc(fp)
#define	UnGetChar(fp,c)	ungetc((c),(fp))

static	struct	{
	char	*str;
	int		token;
}	tokentable[] = {
	{	"name"		,T_NAME		},
	{	"arraysize"	,T_ARRAYSIZE},
	{	"textsize"	,T_TEXTSIZE	},
	{	"db"		,T_DB		},
	{	"bind"		,T_BIND		},
	{	""			,0	}
};

static	GHashTable	*Reserved;

extern	void
BD_LexInit(void)
{
	int		i;

dbgmsg(">BD_LexInit");
	Reserved = NewNameHash();
	for	( i = 0 ; tokentable[i].token  !=  0 ; i ++ ) {
		g_hash_table_insert(Reserved,tokentable[i].str,(gpointer)tokentable[i].token);
	}
dbgmsg("<BD_LexInit");
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

#define	SKIP_SPACE	while	(  isspace( c = GetChar(BD_File) ) ) {	\
						if		(  c  ==  '\n'  ) {	\
							c = ' ';\
							BD_cLine ++;\
						}	\
					}

static	void
DoInclude(
	char	*fn)
{
	INCFILE	*back;
	char	name[SIZE_BUFF];
	char	buff[SIZE_BUFF];
	char	*p
	,		*q;

dbgmsg(">DoInclude");
	back = New(INCFILE);
	back->next = ftop;
	back->fn =  BD_FileName;
	fgetpos(BD_File,&back->pos);
	back->cLine = BD_cLine;
	ftop = back;
	fclose(BD_File);
	if		(  BD_Dir  !=  NULL  ) {
		strcpy(buff,BD_Dir);
	} else {
		strcpy(buff,".");
	}
	p = buff;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s",p,fn);
		if		(  ( BD_File = fopen(name,"r") )  !=  NULL  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );
	if		(  BD_File  ==  NULL  ) {
		Error("include not found");
	}
	BD_cLine = 1;
	BD_FileName = StrDup(name);
dbgmsg("<DoInclude");
}

static	void
ExitInclude(void)
{
	INCFILE	*back;

dbgmsg(">ExitInclude");
	fclose(BD_File);
	back = ftop;
	BD_File = fopen(back->fn,"r");
	xfree(BD_FileName);
	BD_FileName = back->fn;
	fsetpos(BD_File,&back->pos);
	BD_cLine = back->cLine;
	ftop = back->next;
	xfree(back);
dbgmsg("<ExitInclude");
}

static	void
ReadyDirective(void)
{
	char	buff[SIZE_BUFF];
	char	fn[SIZE_NAME+1];
	char	*s;
	int		c;

dbgmsg(">ReadyDirective");
	SKIP_SPACE;
	s = buff;
	do {
		*s = c;
		s ++;
	}	while	(  !isspace( c = GetChar(BD_File) )  );
	*s = 0;
	if		(  !stricmp(buff,"include")  ) {
		SKIP_SPACE;
		s = fn;
		switch	(c) {
		  case	'"':
			while	(  ( c = GetChar(BD_File) )  !=  '"'  ) {
				*s ++ = c;
			}
			*s = 0;
			break;
		  case	'<':
			while	(  ( c = GetChar(BD_File) )  !=  '>'  ) {
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
		UnGetChar(BD_File,c);
		while	(  ( c = GetChar(BD_File) )  !=  '\n'  );
		BD_cLine ++;
	}
dbgmsg("<ReadyDirective");
}

extern	int
BD_Lex(
	Bool	fSymbol)
{	int		c
	,		len;
	int		token;
	char	*s;

dbgmsg(">BD_Lex");
  retry:
	SKIP_SPACE; 
	if		(  c  ==  '#'  ) {
		ReadyDirective();
		goto	retry;
	}
	if		(  c  ==  '!'  ) {
		while	(  ( c = GetChar(BD_File) )  !=  '\n'  );
		BD_cLine ++;
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = BD_ComSymbol;
		len = 0;
		while	(  ( c = GetChar(BD_File) )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(BD_File);
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
	if		(  isalpha(c)  ) {
		s = BD_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(BD_File);
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '.' )
					 ||	(  c  ==  '_' ) );
		*s = 0;
		UnGetChar(BD_File,c);
		if		(  fSymbol  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(BD_ComSymbol);
		}
	} else
	if		(  isdigit(c)  )	{
		s = BD_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(BD_File);
		}	while	(  isdigit(c)  );
		*s = 0;
		UnGetChar(BD_File,c);
		token = T_ICONST;
		BD_ComInt = atoi(BD_ComSymbol);
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
dbgmsg("<BD_Lex");
	return	(token);
}

