/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan.
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
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<signal.h>
#include	<unistd.h>

#include	<uuid/uuid.h>

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
#include	"monsys.h"
#include	"bytea.h"
#include	"blobreq.h"
#include	"sysdata.h"
#include	"comm.h"
#include	"comms.h"
#include	"redirect.h"
#include	"debug.h"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->process[PROCESS_UPDATE].conn)

static	ValueStruct	*
_NewBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret, *val;
	char *sql;
	size_t	sql_len = SIZE_SQL;
	DBG_Struct		*mondbg;
	monblob_struct *monblob;
ENTER_FUNC;
	monblob = NewMonblob_struct(NULL);
	mondbg = GetDBG_monsys();
	sql = xmalloc(sql_len);
	snprintf(sql, sql_len, "INSERT INTO %s (id, status) VALUES('%s', '%d');", MONBLOB, monblob->id , 503);
	ExecDBOP(mondbg, sql, FALSE, DB_UPDATE);
	xfree(sql);

	if ((val = GetItemLongName(args, "id")) != NULL) {
		SetValueString(val, monblob->id, dbg->coding);
	}
	FreeMonblob_struct(monblob);
	ret = DuplicateValue(args,TRUE);
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_ImportBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;
	ValueStruct	*val;
	DBG_Struct		*mondbg;
	char *id = NULL;
	char *rid = NULL;
	int	persist = 0;
	char *filename = NULL;
	char *content_type = NULL;

ENTER_FUNC;
	mondbg = GetDBG_monsys();
	if ((val = GetItemLongName(args, "id")) != NULL) {
		id = ValueToString(val,dbg->coding);
	}
	if ((val = GetItemLongName(args, "persist")) != NULL) {
		persist = ValueToInteger(val);
	}
	if ((val = GetItemLongName(args, "filename")) != NULL) {
		filename = ValueToString(val,dbg->coding);
	}
	if ((val = GetItemLongName(args, "content_type")) != NULL) {
		content_type = ValueToString(val,dbg->coding);
	}
	rid = monblob_import(mondbg, id, persist, filename, content_type, 1);
	if ((rid != NULL) && (val = GetItemLongName(args, "id")) != NULL) {
		SetValueString(val, rid, dbg->coding);
		xfree(rid);
	}
	ret = DuplicateValue(args,TRUE);
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_ExportBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;
	ValueStruct	*val;
	DBG_Struct		*mondbg;
	char *id = NULL;
	char *filename = NULL;

ENTER_FUNC;
	mondbg = GetDBG_monsys();
	if ((val = GetItemLongName(args, "id")) != NULL) {
		id = ValueToString(val,dbg->coding);
	}
	if ((val = GetItemLongName(args, "filename")) != NULL) {
		filename = ValueToString(val,dbg->coding);
	}

	monblob_export(mondbg, id, filename);

	ret = DuplicateValue(args,TRUE);
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_PersistBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;
	ValueStruct	*val;
	DBG_Struct		*mondbg;
	char *id = NULL;
	char *filename = NULL;
	char *content_type = NULL;

ENTER_FUNC;
	mondbg = GetDBG_monsys();
	if ((val = GetItemLongName(args, "id")) != NULL) {
		id = ValueToString(val,dbg->coding);
	}
	if ((val = GetItemLongName(args, "filename")) != NULL) {
		filename = ValueToString(val,dbg->coding);
	}
	if ((val = GetItemLongName(args, "content_type")) != NULL) {
		content_type = ValueToString(val,dbg->coding);
	}
	monblob_persist(mondbg, id, filename, content_type, 1);
	ret = DuplicateValue(args,TRUE);
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_GETID(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj, *val;
	ValueStruct	*ret;
	DBG_Struct		*mondbg;
	int blobid;
	char *id = NULL;

ENTER_FUNC;
	mondbg = GetDBG_monsys();
	ret = NULL;
	if (rec->type != RECORD_DB) {
		rc = MCP_BAD_ARG;
	} else {
		if ((obj = GetItemLongName(args,"blobid")) != NULL) {
			blobid = (int)ValueObjectId(obj);
			if ((id = monblob_getid(mondbg, blobid)) != NULL) {
				val = GetItemLongName(args,"id");
				SetValueStringWithLength(val, id, strlen(id), NULL);
				xfree(id);
				ret = DuplicateValue(args,TRUE);
				rc = MCP_OK;
			} else {
				rc = MCP_EOF;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if (ctrl != NULL) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_DestroyBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DBG_Struct 	*mondbg;
	ValueStruct	*ret;
	ValueStruct	*val;
	char *id;

ENTER_FUNC;
	mondbg = GetDBG_monsys();
	if ((val = GetItemLongName(args, "id")) != NULL) {
		id = ValueToString(val,dbg->coding);
		monblob_delete(mondbg, id);
	}
	ret = NULL;
LEAVE_FUNC;
	return	(ret);
}

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed,
	int			usage)
{
	return	(MCP_OK);
}

static	ValueStruct	*
_DBACCESS(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(ret);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)SYSDATA_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)SYSDATA_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)SYSDATA_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)SYSDATA_DBCOMMIT },
	/*	table operations	*/
	{	"MONBLOBNEW",		_NewBLOB		},
	{	"MONBLOBIMPORT",	_ImportBLOB		},
	{	"MONBLOBEXPORT",	_ExportBLOB		},
	{	"MONBLOBPERSIST",	_PersistBLOB	},
	{	"MONBLOBGETID",		_GETID		},
	{	"MONBLOBDESTROY",	_DestroyBLOB	},
	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL,
};

extern	DB_Func	*
InitMONBLOB(void)
{
	return	(EnterDB_Function("MONBLOB",Operations,DB_PARSER_NULL,&Core,"",""));
}

