/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	"monstring.h"
#include	"hash.h"
#include	"value.h"
#include	"DBlex.h"
#include	"DBparser.h"
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
	{	"primary"	,T_PRIMARY	},
	{	"path"		,T_PATH		},
	{	""			,0	}
};

static	GHashTable	*Reserved;

extern	void
DB_LexInit(void)
{
	int		i;

	Reserved = NewNameHash();
	for	( i = 0 ; tokentable[i].token  !=  0 ; i ++ ) {
		g_hash_table_insert(Reserved,tokentable[i].str,(gpointer)tokentable[i].token);
	}
	ftop = NULL;
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

#define	SKIP_SPACE	while	(  isspace( c = GetChar(DB_File) ) ) {	\
						if		(  c  ==  '\n'  ) {	\
							c = ' ';\
							DB_cLine ++;\
						}	\
					}


static	void
DoInclude(
	char	*fn)
{
	INCFILE	*back;
	char	name[SIZE_LONGNAME+1];
	char	buff[SIZE_LONGNAME+1];
	char	*p
	,		*q;

dbgmsg(">DoInclude");
	back = New(INCFILE);
	back->next = ftop;
	back->fn =  DB_FileName;
	fgetpos(DB_File,&back->pos);
	back->cLine = DB_cLine;
	ftop = back;
	fclose(DB_File);
	if		(  RecordDir  !=  NULL  ) {
		strcpy(buff,RecordDir);
	} else {
		strcpy(buff,".");
	}
	p = buff;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s",p,fn);
		if		(  ( DB_File = fopen(name,"r") )  !=  NULL  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );
	if		(  DB_File  ==  NULL  ) {
		Error("include not found");
	}
	DB_cLine = 1;
	DB_FileName = StrDup(name);
dbgmsg("<DoInclude");
}

static	void
ExitInclude(void)
{
	INCFILE	*back;

dbgmsg(">ExitInclude");
	fclose(DB_File);
	back = ftop;
	DB_File = fopen(back->fn,"r");
	xfree(DB_FileName);
	DB_FileName = back->fn;
	fsetpos(DB_File,&back->pos);
	DB_cLine = back->cLine;
	ftop = back->next;
	xfree(back);
dbgmsg("<ExitInclude");
}

static	void
ReadyDirective(void)
{
	char	buff[SIZE_LONGNAME+1];
	char	fn[SIZE_LONGNAME+1];
	char	*s;
	int		c;

dbgmsg(">ReadyDirective");
	SKIP_SPACE;
	s = buff;
	do {
		*s = c;
		s ++;
	}	while	(  !isspace( c = GetChar(DB_File) )  );
	*s = 0;
	if		(  !stricmp(buff,"include")  ) {
		SKIP_SPACE;
		s = fn;
		switch	(c) {
		  case	'"':
			while	(  ( c = GetChar(DB_File) )  !=  '"'  ) {
				*s ++ = c;
			}
			*s = 0;
			break;
		  case	'<':
			while	(  ( c = GetChar(DB_File) )  !=  '>'  ) {
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
		UnGetChar(DB_File,c);
		while	(  ( c = GetChar(DB_File) )  !=  '\n'  );
		DB_cLine ++;
	}
dbgmsg("<ReadyDirective");
}

extern	int
DB_Lex(
	Bool	fSymbol)
{	int		c
	,		len;
	int		token;
	char	*s;

dbgmsg(">DB_Lex");
  retry:
	SKIP_SPACE; 
	if		(  c  ==  '#'  ) {
		ReadyDirective();
		goto	retry;
	}
	if		(  c  ==  '!'  ) {
		while	(  ( c = GetChar(DB_File) )  !=  '\n'  );
		DB_cLine ++;
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = DB_ComSymbol;
		len = 0;
		while	(  ( c = GetChar(DB_File) )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(DB_File);
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
		s = DB_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(DB_File);
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '_' ) );
		*s = 0;
		UnGetChar(DB_File,c);
		if		(  fSymbol  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(DB_ComSymbol);
		}
	} else
	if		(  isdigit(c)  )	{
		s = DB_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(DB_File);
		}	while	(  isdigit(c)  );
		*s = 0;
		UnGetChar(DB_File,c);
		token = T_ICONST;
		DB_ComInt = atoi(DB_ComSymbol);
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
dbgmsg("<DB_Lex");
	return	(token);
}

