/*	PANDA -- a simple transaction monitor

Copyright (C) 1996-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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
*	91.09.30.	Ogochan	load from dviprt
*	91.10.01.	Ogochan	modify to my style
*	91.12.04.	Ogochan	add file name list
*	99.01.19	Ogochan unix type configuration file
*/

#ifdef	MSDOS
#define	CONFIG_TRAILER			".CFG"
#else
#define	CONFIG_TRAILER			".conf"
#endif
#define	COMMAND_SWITCH			'-'
#define	RESPONSE_FILE_SWITCH	'@'

typedef enum ARG_TYPE {
    BOOLEAN, INTEGER, LONGINT, STRING, PROCEDURE
} ARG_TYPE;

typedef struct {
	char		*option;
	ARG_TYPE	type;
	Bool		defval;
	void		*var;
	char		*message;
}	ARG_TABLE;

typedef	struct	FILE_LIST_S	{
	struct	FILE_LIST_S	*next;
	char	*name;
}	FILE_LIST;

extern	FILE_LIST	*GetOption(ARG_TABLE *,int,char**);
extern	char		*GetExt(char *name);
extern	void		ChangeExt(char *,char *,char *);
extern	void		PrintUsage(ARG_TABLE *,char *);
