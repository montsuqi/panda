/*	PANDA -- a simple transaction monitor

Copyright (C) 1996-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

/*
#define	TRACE
#define	DEBUG
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	"types.h"
#include	"i18n_struct.h"
#include	"misc.h"
#include	"i18n_code.h"
#include	"i18n_codeset.h"
#include	"i18n_res.h"
#include	"debug.h"

#define	HASH_MOD		31
#define	MAX_CODE_SET	32
#define	CODE_HASH_MOD	HASH_MOD
#define	ESC_HASH_MOD	HASH_MOD
#define	NO_HASH_MOD		HASH_MOD

word		SET_ASCII
,			SET_JIS83;

static	CODE_SET		*tCd[CODE_HASH_MOD];
static	CODE_SET		*tCde[NO_HASH_MOD];
static	ESCAPE_TABLE	*tEsc[ESC_HASH_MOD];
static	CODE_SET		*tSet[MAX_CODE_SET];

static	int		CodeNumber;
static	void
InitTable(void)
{	int		i;

	CodeNumber = 0;
	for	( i = 0 ; i < CODE_HASH_MOD ; i ++ )	{
		tCd[i] = NULL;
	}
	for	( i = 0 ; i < NO_HASH_MOD ; i ++ )	{
		tCde[i] = NULL;
	}
	for	( i = 0 ; i < ESC_HASH_MOD ; i ++ )	{
		tEsc[i] = NULL;
	}
	for	( i = 0 ; i < MAX_CODE_SET ; i ++ )	{
		tSet[i] = NULL;
	}
}

static	int
HashCodeTable(
	char	*name)
{	int		h;

	for	( h = 0 ; *name  !=  0  ; name ++ )	{
		h += *name;
	}
	return	(h % CODE_HASH_MOD);
}

static	int
HashNoTable(
	int		no)
{
	return	(no % NO_HASH_MOD);
}

extern	CODE_SET	*
AddCodeSet(
	char	*name,
	int		set,
	int		class)
{	CODE_SET	*el;
	int			h;

dbgmsg(">AddCodeSet");
	el = (CODE_SET *)xmalloc(sizeof(CODE_SET));
	el->name = (char *)xmalloc(strlen(name)+1);
	strcpy(el->name,name);
	h = HashCodeTable(name);
	el->next = tCd[h];
	el->no = (set | class);
	tCd[h] = el;
	h = HashNoTable(el->no);
	el->nnext = tCde[h];
	tCde[h] = el;
	tSet[CodeNumber] = el;
	CodeNumber ++;
dbgmsg("<AddCodeSet");
	return	(el);
}

extern	word
GetCodeSet(
	char	*name)
{	CODE_SET	*e;

dbgmsg(">GetCodeSet");
	e = tCd[HashCodeTable(name)];
	while	(	(  e  !=  NULL  )
			 &&	(  strcmp(e->name,name)  ) )	{
		e = e->next;
	}
dbgmsg("<GetCodeSet");
	return	(e->no);
}

extern	CODE_SET	*
GetEscape(
	int		no)
{	CODE_SET	*e;

dbgmsg(">GetEscape");
	e = tCde[HashNoTable(no)];
	while	(	(  e  !=  NULL  )
			 &&	(  e->no  !=  no  ) )	{
		e = e->nnext;
	}
dbgmsg("<GetEscape");
	return	(e);
}

static	int
HashEscape(
	char	*esc)
{	int		h;

	for	( h = 0 ; *esc  !=  0  ; esc ++ )	{
		h += *esc;
	}
	return	(h % ESC_HASH_MOD);
}

extern	void
AddEscape(
	char		*esc,
	CODE_SET	*set,
	int			dig)
{	ESCAPE_TABLE	*el;
	int			h;

dbgmsg(">AddEscape");
	el = (ESCAPE_TABLE *)xmalloc(sizeof(ESCAPE_TABLE));
	el->esc = (char *)xmalloc(strlen(esc)+1);
	strcpy(el->esc,esc);
	h = HashEscape(esc);
	el->next = tEsc[h];
	el->set = set;
	el->dig = dig;
	tEsc[h] = el;
	set->G[dig] = el;
dbgmsg("<AddEscape");
}

extern	ESCAPE_TABLE	*
CheckESC(
	char	*s,
	size_t	len)
{
	ESCAPE_TABLE	*e;

	s[len] = 0;
	e = tEsc[HashEscape(s)];
	while	(	(  e  !=  NULL  )
			 &&	(  strcmp(e->esc,s)  ) )	{
		e = e->next;
	}
	return	(e);
}

static	CODE_SET	*
AddCodeSetEntry(
	char	*name,
	int		set,
	int		class,
	Bool	fGR,
	char	*esc0,
	char	*esc1,
	char	*esc2,
	char	*esc3)
{	CODE_SET	*codeset;

	codeset = AddCodeSet(name,set,class);
	codeset->fGR = fGR;
	if		(  esc0  !=  NULL  )	AddEscape(esc0,codeset,0);
	if		(  esc1  !=  NULL  )	AddEscape(esc1,codeset,1);
	if		(  esc2  !=  NULL  )	AddEscape(esc2,codeset,2);
	if		(  esc3  !=  NULL  )	AddEscape(esc3,codeset,3);

	return	(codeset);
}

static	void
CodeSetFallBack(void)
{
dbgmsg(">CodeSetFallBack");
	AddCodeSetEntry("ASCII",0x0000,SET_SBCS,FALSE,
					"\x1B\x28\x42","\x1B\x29\x42","\x1B\x2A\x42","\x1B\x2B\x42");
	AddCodeSetEntry("JISX0208-1983",0x3001,SET_DBCS,FALSE,
					"\x1B\x24\x42","\x1B\x24\x29\x42","\x1B\x24\x2A\x42","\x1B\x24\x2B\x42");
	AddCodeSetEntry("JISX0201-1976-kana",0x1000,SET_SBCS,TRUE,
					"\x1B\x28\x49","\x1B\x29\x49","\x1B\x2A\x49","\x1B\x2B\x49");
	AddCodeSetEntry("JISX0201-1976-alpha",0x0000,SET_SBCS,FALSE,
					"\x1B\x28\x4A","\x1B\x29\x4A","\x1B\x2A\x4A","\x01B\x2B\x4A");
	AddCodeSetEntry("JISC6226-1978",0x3002,SET_DBCS,FALSE,
					"\x1B\x24\x40","\x1B\x24\x29\x40","\x1B\x24\x2A\x40","\x1B\x24\x2B\x40");
	AddCodeSetEntry("JISX0208-1990",0x3003,SET_DBCS,FALSE,
					"\x1B\x26\x40\x1B\x24\x42",NULL,NULL,NULL);
	AddCodeSetEntry("JISX0212-1990",0x2000,SET_DBCS,FALSE,
					"\x1B\x24\x82\x44",NULL,NULL,NULL);
	AddCodeSetEntry("JISX0201-1976-miss",0x1000,SET_SBCS,TRUE,
					"\x1B\x28\x48","\x1B\x29\x48","\x1B\x2A\x48","\x1B\x2B\x48");
	
dbgmsg("<CodeSetFallBack");
}

extern	void
InitCodeSet(void)
{	char		*e;

dbgmsg(">InitCodeSet");
	InitTable();
	AddCodeSet("NULL",0,SET_SBCS);
	CodeSetFallBack();
	if		(	(  ( e = getenv("CODESET_FILE") )  !=  NULL  )
			 &&	(  strlen(e)                       >   0     ) )	{
		ResParse(e);
	}

	SET_ASCII = GetCodeSet("ASCII");
	SET_JIS83 = GetCodeSet("JISX0208-1983");
dbgmsg("<InitCodeSet");
}
