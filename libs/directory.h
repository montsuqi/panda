/*
 * PANDA -- a simple transaction monitor
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

#ifndef	_INC_DIRECTORY_H
#define	_INC_DIRECTORY_H
#include	<glib.h>

#include	"LDparser.h"
#include	"DIparser.h"
#include	"wfcio.h"

#undef	GLOBAL
#ifdef	_DIRECTORY
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
GLOBAL	DI_Struct	*ThisEnv;
#undef	GLOBAL

extern	void		InitDirectory(void);
extern	void		SetUpDirectory(char *name, char *ld, char *bd, char *db, Bool parse_ld);
extern	LD_Struct	*SearchWindowToLD(char *wname);
extern	LD_Struct	*GetLD(char *name);
extern	BD_Struct	*GetBD(char *name);
extern	DBD_Struct	*GetDBD(char *name);
extern	DBG_Struct	*GetDBG(char *name);
extern	void		RegistDBG(DBG_Struct *dbg);
extern	void		SetDBGPath(char *path);
extern	DB_Func		*EnterDB_Function(char *name, DB_OPS *ops, int type,
									  DB_Primitives *primitive,
									  char *commentStart, char *commentEnd);
extern	RecordStruct	*GetTableDBG(char *gname, char *tname);

#endif
