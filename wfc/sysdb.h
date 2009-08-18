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

#ifndef	_INC_SYSDB_H
#define	_INC_SYSDB_H

extern	void		SYSDB_Init();
extern	ValueStruct	*SYSDB_TERM_New(char *termid);
extern	void		SYSDB_TERM_Delete(ValueStruct *value);
extern	void		SYSDB_TERM_Update(ValueStruct *value);
extern	void		SYSDB_TERM_SetValue(ValueStruct *value, int kid, char *data);

enum {
	SYSDB_TERM_USER	= 0,
	SYSDB_TERM_HOST,
	SYSDB_TERM_WINDOW,
	SYSDB_TERM_WIDGET,
	SYSDB_TERM_EVENT,
	SYSDB_TERM_INPROCESS,
	SYSDB_TERM_CTIME,
	SYSDB_TERM_ATIME,
	SYSDB_TERM_PTIME,
	SYSDB_TERM_TPTIME,
	SYSDB_TERM_COUNT,
	SYSDB_TERM_LAST
};	

#endif
