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
#include	"libmondai.h"
#include	"DIlex.h"
#include	"dirs.h"
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
	{	"ld"				,T_LD		},
	{	"bd"				,T_BD		},
	{	"db"				,T_DB		},
	{	"name"				,T_NAME 	},
	{	"linksize"			,T_LINKSIZE	},
	{	"multiplex_level"	,T_MULTI	},
	{	"db_group"			,T_DBGROUP	},
	{	"port"				,T_PORT		},
	{	"user"				,T_USER		},
	{	"password"			,T_PASS		},
	{	"type"				,T_TYPE		},
	{	"file"				,T_FILE		},
	{	"redirect"			,T_REDIRECT	},
	{	"redirect_port"		,T_REDIRECTPORT	},
	{	"lddir"				,T_LDDIR	},
	{	"bddir"				,T_BDDIR	},
	{	"dbddir"			,T_DBDDIR	},
	{	"record"			,T_RECDIR	},
	{	"base"				,T_BASEDIR	},
	{	"priority"			,T_PRIORITY	},
	{	"linkage"			,T_LINKAGE	},
	{	"stacksize"			,T_STACKSIZE	},
	{	"wfc"				,T_WFC		},
	{	"exit"				,T_EXIT		},
	{	""					,0			}
};

static	GHashTable	*Reserved;

extern	void
DI_LexInit(void)
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

#define	SKIP_SPACE	while	(  isspace( c = GetChar(DI_File) ) ) {	\
						if		(  c  ==  '\n'  ) {	\
							c = ' ';\
							DI_cLine ++;\
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
	back->fn =  DI_FileName;
	if		(  DirectoryDir  !=  NULL  ) {
		sprintf(name,"%s/%s",DirectoryDir,fn);
	} else {
		sprintf(name,"%s",fn);
	}
	fgetpos(DI_File,&back->pos);
	back->cLine = DI_cLine;
	ftop = back;
	fclose(DI_File);
	strcpy(buff,DirectoryDir);
	p = buff;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s",p,fn);
		if		(  ( DI_File = fopen(name,"r") )  !=  NULL  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );
	if		(  DI_File  ==  NULL  ) {
		Error("include not found");
	}
	DI_cLine = 1;
	DI_FileName = StrDup(name);
dbgmsg("<DoInclude");
}

static	void
ExitInclude(void)
{
	INCFILE	*back;

dbgmsg(">ExitInclude");
	fclose(DI_File);
	back = ftop;
	if		(  ( DI_File = fopen(back->fn,"r") )  ==  NULL  ) {
		Error("original file not found.");
	}
	xfree(DI_FileName);
	DI_FileName = back->fn;
	fsetpos(DI_File,&back->pos);
	DI_cLine = back->cLine;
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
	}	while	(  !isspace( c = GetChar(DI_File) )  );
	*s = 0;
	if		(  !stricmp(buff,"include")  ) {
		SKIP_SPACE;
		s = fn;
		switch	(c) {
		  case	'"':
			while	(  ( c = GetChar(DI_File) )  !=  '"'  ) {
				*s ++ = c;
			}
			*s = 0;
			break;
		  case	'<':
			while	(  ( c = GetChar(DI_File) )  !=  '>'  ) {
				*s ++ = c;
			}
			*s = 0;
			break;
		  default:
			*s = 0;
			break;
		}
		while	(  ( c = GetChar(DI_File) )  !=  '\n'  );
		DI_cLine ++;
		if		(  *fn  !=  0  ) {
			DoInclude(fn);
		}
	} else {
		UnGetChar(DI_File,c);
		while	(  ( c = GetChar(DI_File) )  !=  '\n'  );
		DI_cLine ++;
	}
dbgmsg("<ReadyDirective");
}

extern	int
DI_Lex(
	Bool	fSymbol)
{	int		c
	,		len;
	int		token;
	char	*s;

dbgmsg(">DI_Lex");
  retry: 
	SKIP_SPACE; 
	if		(  c  ==  '#'  ) {
		ReadyDirective();
		goto	retry;
	}
	if		(  c  ==  '!'  ) {
		while	(  ( c = GetChar(DI_File) )  !=  '\n'  );
		DI_cLine ++;
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = DI_ComSymbol;
		len = 0;
		while	(  ( c = GetChar(DI_File) )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(DI_File);
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
		s = DI_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(DI_File);
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '.' )
					 ||	(  c  ==  '_' ) );
		*s = 0;
		UnGetChar(DI_File,c);
		if		(  fSymbol  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(DI_ComSymbol);
		}
	} else
	if		(  isdigit(c)  )	{
		s = DI_ComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(DI_File);
		}	while	(  isdigit(c)  );
		*s = 0;
		UnGetChar(DI_File,c);
		token = T_ICONST;
		DI_ComInt = atoi(DI_ComSymbol);
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
dbgmsg("<DI_Lex");
	return	(token);
}

