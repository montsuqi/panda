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
#include	"DDlex.h"
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
	{	"bool"		,T_BOOL		},
	{	"byte"		,T_BYTE		},
	{	"char"		,T_CHAR		},
	{	"varchar"	,T_VARCHAR	},
	{	"input"		,T_INPUT	},
	{	"int"		,T_INT		},
	{	"number"	,T_NUMBER	},
	{	"output"	,T_OUTPUT	},
	{	"text"		,T_TEXT		},
	{	"primary"	,T_PRIMARY	},
	{	"path"		,T_PATH		},
	{	"virtual"	,T_VIRTUAL	},
	{	"dbcode"	,T_DBCODE	},
	{	""			,0	}
};

static	GHashTable	*Reserved;

extern	void
DD_LexInit(void)
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

#define	SKIP_SPACE	while	(  isspace( c = GetChar(DD_File) ) ) {	\
						if		(  c  ==  '\n'  ) {	\
							c = ' ';\
							DD_cLine ++;\
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
	back->fn =  DD_FileName;
	fgetpos(DD_File,&back->pos);
	back->cLine = DD_cLine;
	ftop = back;
	fclose(DD_File);
	strcpy(buff,RecordDir);
	p = buff;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s",p,fn);
		if		(  ( DD_File = fopen(name,"r") )  !=  NULL  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );
	if		(  DD_File  ==  NULL  ) {
		Error("include not found");
	}
	DD_cLine = 1;
	DD_FileName = StrDup(name);
dbgmsg("<DoInclude");
}

static	void
ExitInclude(void)
{
	INCFILE	*back;

dbgmsg(">ExitInclude");
	fclose(DD_File);
	back = ftop;
	DD_File = fopen(back->fn,"r");
	xfree(DD_FileName);
	DD_FileName = back->fn;
	fsetpos(DD_File,&back->pos);
	DD_cLine = back->cLine;
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
	}	while	(  !isspace( c = GetChar(DD_File) )  );
	*s = 0;
	if		(  !stricmp(buff,"include")  ) {
		SKIP_SPACE;
		s = fn;
		switch	(c) {
		  case	'"':
			while	(  ( c = GetChar(DD_File) )  !=  '"'  ) {
				*s ++ = c;
			}
			*s = 0;
			break;
		  case	'<':
			while	(  ( c = GetChar(DD_File) )  !=  '>'  ) {
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
		UnGetChar(DD_File,c);
		while	(  ( c = GetChar(DD_File) )  !=  '\n'  );
		DD_cLine ++;
	}
dbgmsg("<ReadyDirective");
}

extern	int
DD_Lex(
	Bool	fSymbol)
{	int		c
	,		len;
	int		token;
	char	*s;

dbgmsg(">DD_Lex");
  retry:
	SKIP_SPACE; 
	if		(  c  ==  '#'  ) {
		ReadyDirective();
		goto	retry;
	}
	if		(  c  ==  '!'  ) {
		while	(  ( c = GetChar(DD_File) )  !=  '\n'  );
		DD_cLine ++;
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = DD_ComSymbol;
		len = 0;
		while	(  ( c = GetChar(DD_File) )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(DD_File);
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
		s = DD_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(DD_File);
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '_' ) );
		*s = 0;
		UnGetChar(DD_File,c);
		if		(  fSymbol  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(DD_ComSymbol);
		}
	} else
	if		(  isdigit(c)  )	{
		s = DD_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(DD_File);
		}	while	(  isdigit(c)  );
		*s = 0;
		UnGetChar(DD_File,c);
		token = T_ICONST;
		DD_ComInt = atoi(DD_ComSymbol);
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
#ifdef	DEBUG
	printf("token = ");
	switch	(token) {
	  case	T_SYMBOL:
		printf("symbol (%s)\n",DD_ComSymbol);
		break;
	  case	T_BYTE:
		printf("[byte]\n");
		break;
	  case	T_CHAR:
		printf("[char]\n");
		break;
	  case	T_INT:
		printf("[int]\n");
		break;
	  case	T_TEXT:
		printf("[text]\n");
		break;
	  case	T_ICONST:
		printf("iconst (%d)\n",DD_ComInt);
		break;
	  case	T_EOF:
		printf("[EOF]\n");
		break;
	  default:
		printf("(%c)\n",token);
		break;
	}
#endif
dbgmsg("<DD_Lex");
	return	(token);
}

