/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
#include	"blobreq.h"
#include	"comm.h"
#include	"comms.h"
#include	"redirect.h"
#include	"debug.h"

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed)
{
	return	(MCP_OK);
}

static	void
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int		fh
		,	rc;

ENTER_FUNC;
	if		(  fpBlob  ==  NULL  ) {
		if		(  dbg->port  ==  NULL  ) {
			dbg->port = ThisEnv->blob->port;
		}
		if		(	(  dbg->port  !=  NULL  )
				&&	(  ( fh = ConnectSocket(dbg->port,SOCK_STREAM) )  >=  0  ) ) {
			fpBlob = SocketToNet(fh);
			SendStringDelim(fpBlob,dbg->user);
			SendStringDelim(fpBlob,"\n");
			SendStringDelim(fpBlob,dbg->pass);
			SendStringDelim(fpBlob,"\n");
			if		(  RecvPacketClass(fpBlob)  !=  APS_OK  ) {
				CloseNet(fpBlob);
				fpBlob = NULL;
			}
		}
	}
	if		(  fpBlob  !=  NULL  ) {
		OpenDB_RedirectPort(dbg);
		dbg->conn = (void *)fpBlob;
		dbg->fConnect = TRUE;
		rc = MCP_OK;
	} else {
		rc = MCP_BAD_OTHER;
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
}

static	void
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  dbg->fConnect  ) { 
		SendPacketClass(fpBlob,APS_END);
		CloseNet((NETFILE *)dbg->conn);
		CloseDB_RedirectPort(dbg);
		dbg->fConnect = FALSE;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
}

static	void
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int			rc;

ENTER_FUNC;
	BeginDB_Redirect(dbg); 
	dbgmsg("OK");
	rc = MCP_OK;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
}

static	void
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int			rc;

ENTER_FUNC;
	CheckDB_Redirect(dbg);
	dbgmsg("OK");
	rc = MCP_OK;
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
}

static	void
_NewBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*e;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( e = GetItemLongName(rec->value,"object") )  !=  NULL  ) {
			if		(  ( ValueObject(e) = RequestNewBLOB((NETFILE*)dbg->conn,APS_BLOB,BLOB_OPEN_WRITE) )  !=  GL_OBJ_NULL  ) {
				RequestCloseBLOB((NETFILE*)dbg->conn,APS_BLOB,ValueObject(e));
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
}

static	void
_OpenBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*m;
	int			mode;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( m = GetItemLongName(rec->value,"mode") )  !=  NULL  ) {
			mode = ValueToInteger(m);
		} else {
			mode = BLOB_OPEN_READ;
		}
		if		(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  ) {
			if		(  RequestOpenBLOB((NETFILE*)dbg->conn,APS_BLOB,mode,
									  ValueObject(obj))  ) {
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
}

static	void
_CloseBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  ) {
			if		(  RequestCloseBLOB((NETFILE*)dbg->conn,APS_BLOB,
									  ValueObject(obj))  ) {
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
}

static	void
_WriteBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*s
		,		*v;
	byte		*value;
	size_t		size;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		v = GetItemLongName(rec->value,"value");
		if		(  ( s = GetItemLongName(rec->value,"size") )  !=  NULL  ) {
			size = ValueToInteger(s);
		} else
		if		(  v  !=  NULL  ) {
			size = NativeSizeValue(NULL,v);
		} else {
			size = 0;
		}
		if		(  size  >  0  ) {
			value = (byte *)xmalloc(size);
		} else {
			value = NULL;
		}
		if		(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  ) {
			if		(  value  !=  NULL  ) {
				NativePackValue(NULL,value,v);
				if		(  RequestWriteBLOB((NETFILE*)dbg->conn,APS_BLOB,
											ValueObject(obj),value,size)  ==  size  ) {
					rc = MCP_OK;
				} else {
					rc = MCP_BAD_OTHER;
				}
			} else {
				rc = MCP_BAD_ARG;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
		if		(  value  !=  NULL  ) {
			xfree(value);
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
}

static	void
_ReadBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*s
		,		*v;
	byte		*value;
	size_t		size;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		v = GetItemLongName(rec->value,"value");
		if		(  ( s = GetItemLongName(rec->value,"size") )  !=  NULL  ) {
			size = ValueToInteger(s);
		} else
		if		(  v  !=  NULL  ) {
			size = NativeSizeValue(NULL,v);
		} else {
			size = 0;
		}
		if		(  size  >  0  ) {
			value = (byte *)xmalloc(size);
		} else {
			value = NULL;
		}
		if		(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  ) {
			if		(  value  !=  NULL  ) {
				if		(  RequestReadBLOB((NETFILE*)dbg->conn,APS_BLOB,
											ValueObject(obj),value,size)  ==  size  ) {
					NativeUnPackValue(NULL,value,v);
					rc = MCP_OK;
				} else {
					rc = MCP_BAD_OTHER;
				}
			} else {
				rc = MCP_BAD_ARG;
			}
		} else {
			rc = MCP_BAD_ARG;
		}
		if		(  value  !=  NULL  ) {
			xfree(value);
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
}

static	void
_ExportBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*f;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(	(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  )
				&&	(  ( f   = GetItemLongName(rec->value,"file") )    !=  NULL  ) ) {
			if		(  RequestExportBLOB((NETFILE*)dbg->conn,APS_BLOB,
										 ValueObject(obj),ValueToString(f,NULL))  ) {
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
}

static	void
_ImportBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*f;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(	(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  )
				&&	(  ( f   = GetItemLongName(rec->value,"file") )    !=  NULL  ) ) {
			if		(  ( ValueObject(obj) = RequestImportBLOB((NETFILE*)dbg->conn,APS_BLOB,
															  ValueToString(f,NULL)) )  !=  GL_OBJ_NULL  ) {
                ValueIsNonNil(obj);
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
}

static	void
_SaveBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*f;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(	(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  )
				&&	(  ( f   = GetItemLongName(rec->value,"file") )    !=  NULL  ) ) {
			if		(  RequestSaveBLOB((NETFILE*)dbg->conn,APS_BLOB,
									   ValueObject(obj),ValueToString(f,NULL))  ) {
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
}

static	void
_CheckBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  ) {
			if		(  RequestCheckBLOB((NETFILE*)dbg->conn,APS_BLOB,
										ValueObject(obj))  ) {
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
}

static	void
_DestroyBLOB(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( obj = GetItemLongName(rec->value,"object") )  !=  NULL  ) {
			if		(  RequestDestroyBLOB((NETFILE*)dbg->conn,APS_BLOB,
									   ValueObject(obj))  ) {
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
}

static	Bool
_DBACCESS(
	DBG_Struct		*dbg,
	char			*name,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	Bool	rc;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
		rc = FALSE;
	} else {
		ctrl->rc = MCP_OK;
		rc = TRUE;
	}
LEAVE_FUNC;
	return	(rc);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)_DBCOMMIT },
	/*	table operations	*/
	{	"BLOBNEW",		_NewBLOB	},
	{	"BLOBOPEN",		_OpenBLOB	},
	{	"BLOBWRITE",	_WriteBLOB	},
	{	"BLOBREAD",		_ReadBLOB	},
	{	"BLOBCLOSE",	_CloseBLOB	},
	{	"BLOBEXPORT",	_ExportBLOB	},
	{	"BLOBIMPORT",	_ImportBLOB	},
	{	"BLOBSAVE",		_SaveBLOB	},
	{	"BLOBCHECK",	_CheckBLOB	},
	{	"BLOBDESTROY",	_DestroyBLOB},

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
};

extern	DB_Func	*
InitNativeBLOB(void)
{
	return	(EnterDB_Function("NativeBLOB",Operations,&Core,"",""));
}

