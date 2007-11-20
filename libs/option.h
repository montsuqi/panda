/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1991-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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

extern	char		*GetExt(char *name);
extern	void		ChangeExt(char *,char *,char *);
extern	FILE_LIST	*GetOption(ARG_TABLE *tbl, int argc, char **argv, char *help);
extern	void		PrintUsage(ARG_TABLE *tbl, char *comment, char *help);
