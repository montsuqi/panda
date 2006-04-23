/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_DBLIB_H
#define	_INC_DBLIB_H
#include	"struct.h"
#include	<net.h>
#include	<libmondai.h>
#include	"blob.h"

extern	void	InitDB_Process(NETFILE *fp);
extern	void	ExecDBOP(DBG_Struct *dbg, char *sql);
extern	void	ExecDB_Process(DBCOMM_CTRL *ctrl, RecordStruct *rec, ValueStruct *args);
extern	void	TransactionStart(DBG_Struct *dbg);
extern	void	TransactionEnd(DBG_Struct *dbg);
extern	int		GetDBStatus(void);
extern	void	RedirectError(void);
extern	int		OpenDB(DBG_Struct *dbg);
extern	int		CloseDB(DBG_Struct *dbg);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	GHashTable	*DB_Table;
GLOBAL	ProcessNode	*CurrentProcess;	/*	for System handler	*/

#endif
