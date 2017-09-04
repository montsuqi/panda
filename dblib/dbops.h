/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_DBOPS_H
#define	_DBOPS_H

extern	ValueStruct	* _DBOPEN(DBG_Struct	*dbg, DBCOMM_CTRL	*ctrl);
extern	ValueStruct	* _DBDISCONNECT( DBG_Struct	*dbg, DBCOMM_CTRL	*ctrl);
extern	ValueStruct	* _DBSTART( DBG_Struct	*dbg, DBCOMM_CTRL	*ctrl);
extern	ValueStruct	* _DBCOMMIT( DBG_Struct	*dbg, DBCOMM_CTRL	*ctrl);
extern	int _EXEC(DBG_Struct *dbg,char *sql,Bool fRed,int usage);
extern	ValueStruct	* _QUERY(DBG_Struct *dbg, char *sql,Bool fRed,int usage);
extern	ValueStruct	* _DBACCESS(DBG_Struct *dbg,DBCOMM_CTRL *ctrl,RecordStruct *rec,ValueStruct *args);

#endif
