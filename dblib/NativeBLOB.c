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
#include	"comm.h"
#include	"comms.h"
#include	"redirect.h"
#include	"debug.h"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->update.conn)

static	int
_EXEC(
	DBG_Instance	*dbg,
	char			*sql,
	Bool			fRed,
	int				usage)
{
	return	(MCP_OK);
}

static	ValueStruct	*
_DBOPEN(
	DBG_Instance	*dbg,
	DBCOMM_CTRL		*ctrl)
{
	int		fh
		,	rc;
	Port	*port;

ENTER_FUNC;
	if		(  fpBlob  ==  NULL  ) {
		if		(  ( port = GetDB_Port(dbg->class,DB_UPDATE) )  ==  NULL  ) {
			port = ThisEnv->blob->port;
		}
		if		(	(  port  !=  NULL  )
				&&	(  ( fh = ConnectSocket(port,SOCK_STREAM) )  >=  0  ) ) {
			fpBlob = SocketToNet(fh);
			SendStringDelim(fpBlob,GetDB_User(dbg->class,DB_UPDATE));
			SendStringDelim(fpBlob,"\n");
			SendStringDelim(fpBlob,GetDB_Pass(dbg->class,DB_UPDATE));
			SendStringDelim(fpBlob,"\n");
			if		(  RecvPacketClass(fpBlob)  !=  APS_OK  ) {
				CloseNet(fpBlob);
				fpBlob = NULL;
			}
		}
	}
	OpenDB_RedirectPort(dbg);
	dbg->update.conn = (void *)fpBlob;
	if		(  fpBlob  !=  NULL  ) {
		dbg->update.dbstatus = DB_STATUS_CONNECT;
		dbg->readonly.dbstatus = DB_STATUS_NOCONNECT;
		rc = MCP_OK;
	} else {
		rc = MCP_BAD_OTHER;
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBDISCONNECT(
	DBG_Instance	*dbg,
	DBCOMM_CTRL		*ctrl)
{
ENTER_FUNC;
	if		(  dbg->update.dbstatus == DB_STATUS_CONNECT ) { 
#if	0	/*	dbg->conn already closed by aps_main	*/
		SendPacketClass(NBCONN(dbg),APS_END);	ON_IO_ERROR(NBCONN(dbg),badio);
	  badio:;
		CloseNet(NBCONN(dbg);
#endif
		CloseDB_RedirectPort(dbg);
		dbg->update.dbstatus = DB_STATUS_DISCONNECT;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBSTART(
	DBG_Instance	*dbg,
	DBCOMM_CTRL		*ctrl)
{
	int			rc;

ENTER_FUNC;
	BeginDB_Redirect(dbg); 
	if		(  dbg->update.dbstatus == DB_STATUS_CONNECT ) { 
		rc = MCP_OK;
	} else
	if		(  RequestStartBLOB(NBCONN(dbg),APS_BLOB)  ) {
		dbgmsg("OK");
		rc = MCP_OK;
	} else {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBCOMMIT(
	DBG_Instance	*dbg,
	DBCOMM_CTRL		*ctrl)
{
	int			rc;

ENTER_FUNC;
	CheckDB_Redirect(dbg);
	if		(  dbg->update.dbstatus == DB_STATUS_CONNECT ) { 
		rc = MCP_OK;
	} else
	if		(  RequestStartBLOB(NBCONN(dbg),APS_BLOB)  ) {
		dbgmsg("OK");
		rc = MCP_OK;
	} else {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
	}
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_NewBLOB(
	DBG_Instance	*dbg,
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
			if		(  ( ValueObjectId(e) = RequestNewBLOB(NBCONN(dbg),APS_BLOB,BLOB_OPEN_WRITE) )  !=  GL_OBJ_NULL  ) {
				RequestCloseBLOB(NBCONN(dbg),APS_BLOB,ValueObjectId(e));
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
_OpenBLOB(
	DBG_Instance	*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*m;
	int			mode;
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		if		(  ( m = GetItemLongName(args,"mode") )  !=  NULL  ) {
			mode = ValueToInteger(m);
		} else {
			mode = BLOB_OPEN_READ;
		}
		if		(  ( obj = GetItemLongName(args,"object") )  !=  NULL  ) {
			if		(  RequestOpenBLOB(NBCONN(dbg),APS_BLOB,mode,
									  ValueObjectId(obj))  ) {
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
_CloseBLOB(
	DBG_Instance	*dbg,
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
			if		(  RequestCloseBLOB(NBCONN(dbg),APS_BLOB,
										ValueObjectId(obj))  ) {
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
_WriteBLOB(
	DBG_Instance	*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	int			rc;
	ValueStruct	*obj
		,		*s
		,		*v;
	byte		*value;
	ValueStruct	*ret;
	size_t		size;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		v = GetItemLongName(args,"value");
		if		(  ( s = GetItemLongName(args,"size") )  !=  NULL  ) {
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
		if		(  ( obj = GetItemLongName(args,"object") )  !=  NULL  ) {
			if		(  value  !=  NULL  ) {
				NativePackValue(NULL,value,v);
				if		(  RequestWriteBLOB(NBCONN(dbg),APS_BLOB,
											ValueObjectId(obj),value,size)  ==  size  ) {
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
	return	(ret);
}

static	ValueStruct	*
_ReadBLOB(
	DBG_Instance	*dbg,
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
	ValueStruct	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		rc = MCP_BAD_ARG;
	} else {
		v = GetItemLongName(args,"value");
		if		(  ( s = GetItemLongName(args,"size") )  !=  NULL  ) {
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
		if		(  ( obj = GetItemLongName(args,"object") )  !=  NULL  ) {
			if		(  value  !=  NULL  ) {
				if		(  RequestReadBLOB(NBCONN(dbg),APS_BLOB,
										   ValueObjectId(obj),value,size)  ==  size  ) {
					NativeUnPackValue(NULL,value,v);
					ret = DuplicateValue(args,TRUE);
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
	return	(ret);
}

static	ValueStruct	*
_ExportBLOB(
	DBG_Instance	*dbg,
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
			if		(  RequestExportBLOB(NBCONN(dbg),APS_BLOB,
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
	DBG_Instance	*dbg,
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
			if		(  ( ValueObjectId(obj) = RequestImportBLOB(NBCONN(dbg),APS_BLOB,
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
_SaveBLOB(
	DBG_Instance	*dbg,
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
			if		(  RequestSaveBLOB(NBCONN(dbg),APS_BLOB,
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
_CheckBLOB(
	DBG_Instance	*dbg,
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
			if		(  RequestCheckBLOB(NBCONN(dbg),APS_BLOB,
										ValueObjectId(obj))  ) {
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
	DBG_Instance	*dbg,
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
			if		(  RequestDestroyBLOB(NBCONN(dbg),APS_BLOB,
										  ValueObjectId(obj))  ) {
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
_DBACCESS(
	DBG_Instance	*dbg,
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
	NULL,
};

extern	DB_Func	*
InitNativeBLOB(void)
{
	return	(EnterDB_Function("NativeBLOB",Operations,DB_PARSER_NULL,&Core,"",""));
}

