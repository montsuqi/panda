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

#ifndef	_DBGROUP_H
#define	_DBGROUP_H

#include	"libmondai.h"
#include	"net.h"
#include	"dblib.h"

#define	SIZE_SQL		1024

#define REDIRECT_LOCK_TABLE "montsuqi_redirector_lock_table"
#define MONDB	"mondb_"

extern	void	InitializeCTRL(DBCOMM_CTRL*ctrl);

extern	int		OpenRedirectDB(DBG_Struct *dbg);
extern	int		CloseRedirectDB(DBG_Struct *dbg);
extern	int		ExecRedirectDBOP(DBG_Struct *dbg, char *sql, Bool fRed, int usage);
extern	int		TransactionRedirectStart(DBG_Struct *dbg);
extern	int		TransactionRedirectEnd(DBG_Struct *dbg);

extern	void	CloseDB_RedirectPort(DBG_Struct *dbg);

extern	Port	*GetDB_Port(DBG_Struct *dbg, int usage);
extern	char	*GetDB_Host(DBG_Struct *dbg, int usage);
extern	char	*GetDB_PortName(DBG_Struct *dbg, int usage);
extern	char	*GetDB_DBname(DBG_Struct *dbg, int usage);
extern	char	*GetDB_User(DBG_Struct *dbg, int usage);
extern	char	*GetDB_Pass(DBG_Struct *dbg, int usage);
extern	char	*GetDB_Sslmode(DBG_Struct *dbg, int usage);
extern	char	*GetDB_Sslcert(DBG_Struct *dbg, int usage);
extern	char	*GetDB_Sslkey(DBG_Struct *dbg, int usage);
extern	char	*GetDB_Sslrootcert(DBG_Struct *dbg, int usage);
extern	char	*GetDB_Sslcrl(DBG_Struct *dbg, int usage);
extern	int	GetDB_PortMode(DBG_Struct *dbg, int usage);
extern	int	GetTableFuncData(RecordStruct **rec, ValueStruct **value,
														 DBCOMM_CTRL *ctrl);
extern	Bool	SetDBCTRLRecord(DBCOMM_CTRL *ctrl, char *rname);
extern	Bool	SetDBCTRLValue(DBCOMM_CTRL *ctrl, char *pname);

extern	RecordStruct	*BuildDBCTRL(void);
extern	void	DumpDB_Node(DBCOMM_CTRL *ctrl);
extern	void	AuditLog(ValueStruct *mcp);

extern	Bool	IsDBOperation(char *func);
extern	Bool	IsDBUpdateFunc(char *func);

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
GLOBAL	Bool	fNoAudit;
GLOBAL	int		MaxSendRetry;
GLOBAL	int		RetryInterval;
GLOBAL	char	*AuditLogFile;

#endif
