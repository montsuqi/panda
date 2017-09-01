/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_DBUTILS_H
#define	_INC_DBUTILS_H
#include	"libmondai.h"
#include	"dblib.h"
#include	"dbgroup.h"

extern	Bool	table_exist(DBG_Struct	*dbg, char *table_name);
extern	Bool	index_exist(DBG_Struct	*dbg, char *index_name);
extern	Bool	sequence_exist(DBG_Struct *dbg,	char *sequence_name);

extern	Bool	column_exist(DBG_Struct *dbg, char *table_name, char *column_name);
extern	void	timestamp(char *daytime, size_t size);

#endif
