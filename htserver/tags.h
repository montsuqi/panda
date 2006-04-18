/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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

#ifndef	_INC_TAGS_H
#define	_INC_TAGS_H

typedef	struct _Tag	{
	char	*name;
	void	(*emit)(HTCInfo *, struct _Tag *);
	GHashTable	*args;
}	Tag;

typedef	struct _Js	{
	char	*name;
	char	*body;
	Bool	fUse;
	Bool	fFile;
}	Js;

typedef	struct {
	char	*name;
	Bool	fPara;
	int		nPara;
	char	**Para;
}	TagType;

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	GHashTable	*Tags;
GLOBAL	GHashTable	*Jslib;

#undef	GLOBAL

extern	void	TagsInit(char *);
extern	void	EmitCode(HTCInfo *htc, byte code);
extern	void	ExpandAttributeString(HTCInfo *htc, char *para);

extern	void	JslibInit(void);
extern	void	InvokeJs(char *name);

#endif
