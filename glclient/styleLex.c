/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	"libmondai.h"
#include	"styleLex.h"
#include	"debug.h"


#define	GetChar(fp)		fgetc(fp)
#define	UnGetChar(fp,c)	ungetc((c),(fp))

static	struct	{
	char	*str;
	int		token;
}	tokentable[] = {
	{	"style"		,T_STYLE },
	{	"fontset"	,T_FONTSET },
	{	"fg"		,T_FG },
	{	"bg"		,T_BG },
	{	"bg_pixmap"	,T_BG_PIXMAP },
	{	"normal"	,T_NORMAL },
	{	"NORMAL"	,T_NORMAL },
	{	"prelight"	,T_PRELIGHT },
	{	"PRELIGHT"	,T_PRELIGHT },
	{	"insensitive",T_INSENSITIVE },
	{	"INSENSITIVE",T_INSENSITIVE },
	{	"selected"	,T_SELECTED },
	{	"SELECTED"	,T_SELECTED },
	{	"active"	,T_ACTIVE },
	{	"ACTIVE"	,T_ACTIVE },
	{	""			,0	}
};

static	GHashTable	*Reserved;

extern	void
StyleLexInit(void)
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
StyleLex(
	Bool	fSymbol)
{	int		c
	,		len;
	int		token;
	char	*s;
	Bool	fFloat;

dbgmsg(">StyleLex");
  retry: 
	while	(  isspace( c = GetChar(StyleFile) ) ) {
		if		(  c  ==  '\n'  ) {
			c = ' ';
			StylecLine ++;
		}
	}
	if		(  c  ==  '#'  ) {
		while	(  ( c = GetChar(StyleFile) )  !=  '\n'  );
		StylecLine ++;
		goto	retry;
	}
	if		(  c  ==  '"'  ) {
		s = StyleComSymbol;
		len = 0;
		while	(  ( c = GetChar(StyleFile) )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar(StyleFile);
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
		s = StyleComSymbol;
		len = 0;
		do {
			*s = c;
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(StyleFile);
		}	while	(	(  isalpha(c) )
					 ||	(  isdigit(c) )
					 ||	(  c  ==  '.' )
					 ||	(  c  ==  '_' ) );
		*s = 0;
		UnGetChar(StyleFile,c);
		if		(  fSymbol  ) {
			token = T_SYMBOL;
		} else {
			token = CheckReserved(StyleComSymbol);
		}
	} else
	if		(  isdigit(c)  )	{
		s = StyleComSymbol;
		len = 0;
		fFloat = FALSE;
		do {
			*s = c;
			if		(  c  ==  '.'  ) {
				if		(  fFloat  ) {
					printf(". duplicate\n");
				}
				fFloat = TRUE;
			}
			if		(  len  <  SIZE_SYMBOL  ) {
				s ++;
				len ++;
			}
			c = GetChar(StyleFile);
		}	while	(	(  isdigit(c)  )
					||	(  c  ==  '.'  ) );
		*s = 0;
		UnGetChar(StyleFile,c);
		if		(  fFloat  ) {
			StyleComInt = (int)(atof(StyleComSymbol) * 65535.0);
		} else {
			StyleComInt = atoi(StyleComSymbol);
		}
		token = T_ICONST;
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
dbgmsg("<StyleLex");
	return	(token);
}

