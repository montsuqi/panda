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

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
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
	int			rc;
	ValueStruct	*e;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( e = GetItemLongName(args,"object") )  !=  NULL  ) {
			if		(  ( ValueObjectId(e) = RequestNewBLOB(NBCONN(dbg),BLOB_OPEN_WRITE) )  !=  GL_OBJ_NULL  ) {
				ret = DuplicateValue(args,TRUE);
				rc = MCP_OK;
			} else {
				rc = MCP_BAD_OTHER;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
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
	int			rc;
	ValueStruct	*obj
		,		*f;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(	(  ( obj = GetItemLongName(args,"object") )  !=  NULL  )
				&&	(  ( f   = GetItemLongName(args,"file") )    !=  NULL  ) ) {
			if		(  RequestExportBLOB(NBCONN(dbg),
										 ValueObjectId(obj),ValueToString(f,NULL))  ) {
				rc = MCP_OK;
			} else {
				rc = MCP_BAD_OTHER;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
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
	int			rc;
	ValueStruct	*obj
		,		*f;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(	(  ( obj = GetItemLongName(args,"object") )  !=  NULL  )
				&&	(  ( f   = GetItemLongName(args,"file") )    !=  NULL  ) ) {
			if		(  ( ValueObjectId(obj) = RequestImportBLOB(NBCONN(dbg),
																ValueToString(f,NULL)) )  !=  GL_OBJ_NULL  ) {
                ValueIsNonNil(obj);
				ret = DuplicateValue(args,TRUE);
				rc = MCP_OK;
			} else {
				rc = MCP_BAD_OTHER;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_CheckBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( obj = GetItemLongName(args,"object") )  !=  NULL  ) {
			if		(  RequestCheckBLOB(NBCONN(dbg),ValueObjectId(obj))  ) {
				rc = MCP_OK;
			} else {
				rc = MCP_EOF;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
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
	int			rc;
	ValueStruct	*obj;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( obj = GetItemLongName(args,"object") )  !=  NULL  ) {
			if		(  RequestDestroyBLOB(NBCONN(dbg),ValueObjectId(obj))  ) {
				rc = MCP_OK;
			} else {
				rc = MCP_EOF;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(ret);
}

#if BLOB_VERSION == 1
static	ValueStruct	*
_RegisterBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*f;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(	(  ( obj = GetItemLongName(args,"object") )  !=  NULL  )
				&&	(  ( f   = GetItemLongName(args,"file") )    !=  NULL  ) ) {
			if		(  RequestRegisterBLOB(NBCONN(dbg),ValueObjectId(obj), ValueToString(f,NULL))  ) {
				ret = DuplicateValue(args,TRUE);
				rc = MCP_OK;
			} else {
				rc = MCP_BAD_OTHER;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_LookupBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*f;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(	(  ( obj = GetItemLongName(args,"object") )  !=  NULL  )
				&&	(  ( f   = GetItemLongName(args,"file") )    !=  NULL  ) ) {
			ValueObjectId(obj) = RequestLookupBLOB(NBCONN(dbg),ValueToString(f,NULL));
			ret = DuplicateValue(args,TRUE);
			rc = MCP_OK;
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(ret);
}
#endif

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
	char			*name,
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
	{	"BLOBNEW",		_NewBLOB		},
	{	"BLOBEXPORT",	_ExportBLOB		},
	{	"BLOBIMPORT",	_ImportBLOB		},
	{	"BLOBCHECK",	_CheckBLOB		},
	{	"BLOBDESTROY",	_DestroyBLOB	},
#if BLOB_VERSION == 1
	{	"BLOBREGISTER",	_RegisterBLOB	},
	{	"BLOBLOOKUP",	_LookupBLOB  	},
#endif

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL,
};

extern	DB_Func	*
InitNativeBLOB(void)
{
	return	(EnterDB_Function("NativeBLOB",Operations,DB_PARSER_NULL,&Core,"",""));
}

