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

#ifndef	_DBGROUP_H
#define	_DBGROUP_H

#include	"libmondai.h"
#include	"net.h"
#include	"dblib.h"

extern	int		OpenRedirectDB(DBG_Struct *dbg);
extern	int		CloseRedirectDB(DBG_Struct *dbg);
extern	int		ExecRedirectDBOP(DBG_Struct *dbg, char *sql);
extern	int		TransactionRedirectStart(DBG_Struct *dbg);
extern	int		TransactionRedirectEnd(DBG_Struct *dbg);

extern	void	CloseDB_RedirectPort(DBG_Struct *dbg);

extern	char	*GetDB_Host(DBG_Struct *dbg);
extern	char	*GetDB_Port(DBG_Struct *dbg);
extern	char	*GetDB_DBname(DBG_Struct *dbg);
extern	char	*GetDB_User(DBG_Struct *dbg);
extern	char	*GetDB_Pass(DBG_Struct *dbg);

extern	ValueStruct	*_GetDB_Argument(RecordStruct *rec, char *pname, char *func, int *apno);
extern	void	MakeCTRL(DBCOMM_CTRL *ctrl, ValueStruct *mcp);
extern	RecordStruct	*MakeCTRLbyName(ValueStruct **value, DBCOMM_CTRL *ctrl,
										char *rname, char *pname, char *func);
extern	void	MakeMCP(ValueStruct *mcp, DBCOMM_CTRL *ctrl);
extern	RecordStruct	*BuildDBCTRL(void);
extern	void	DumpDB_Node(DBCOMM_CTRL *ctrl);

#define	GetDB_Argument(rec,pname,func)	_GetDB_Argument((rec),(pname),(func),NULL)

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	RecordStruct	**ThisDB;

GLOBAL	char	*DB_Host;
GLOBAL	char	*DB_Port;
GLOBAL	char	*DB_Name;
GLOBAL	char	*DB_User;
GLOBAL	char	*DB_Pass;

GLOBAL	Bool	fNoCheck;
GLOBAL	Bool	fNoRedirect;
GLOBAL	Bool	fNoSumCheck;
GLOBAL	int		MaxSendRetry;
GLOBAL	int		RetryInterval;
GLOBAL	NETFILE	*fpBlob;

#endif
