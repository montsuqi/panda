/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2011 NaCl.
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
#include	<errno.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<signal.h>
#include	<libxml/tree.h>
#include	<libxml/parser.h>

#include	"const.h"
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
#include	"message.h"
#include	"debug.h"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->process[PROCESS_UPDATE].conn)
#define CAST_BAD(arg)	(char*)(arg)

static	void
ResetArgValue(
	ValueStruct *args)
{
	ValueStruct *v;
	int obj;

	if (args == NULL || IS_VALUE_RECORD(args)) {
		return;
	} 
	v = GetItemLongName(args,"object");
	if (v == NULL) {
		return;
	}
	obj = ValueObjectId(v);
	InitializeValue(args);
	ValueObjectId(v) = obj;
}

static	ValueStruct	*
_ReadJSON2(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret,*obj,*v;
	size_t size;
	unsigned char *buf,*jsonstr;

ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	ResetArgValue(args);
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((obj = GetItemLongName(args,"data"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((obj = GetItemLongName(args,"object"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if(RequestReadBLOB(NBCONN(dbg),ValueObjectId(obj),&buf,&size) > 0) {
		if (size > 0) {
			ret = DuplicateValue(args,FALSE);
			v = GetItemLongName(ret,"data");
			jsonstr = g_malloc0(size+1);
			memcpy(jsonstr,buf,size);
			xfree(buf);
			JSON_UnPackValueOmmit(NULL,jsonstr,v);
			g_free(jsonstr);
			ctrl->rc = MCP_OK;
		}
	}
#ifdef TRACE
	DumpValueStruct(ret);
#endif
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_WriteJSON2(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret,*v;
	int oid;
	size_t size,wrote;
	char *buf;
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((v = GetItemLongName(args,"data"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((v = GetItemLongName(args,"object"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	oid = RequestNewBLOB(NBCONN(dbg),BLOB_OPEN_WRITE);
	if (oid == GL_OBJ_NULL) {
		return NULL;
	}
	v = GetItemLongName(args,"data");
	size = JSON_SizeValueOmmit(NULL,v);
	buf = malloc(size);
	JSON_PackValueOmmit(NULL,buf,v);
	wrote = RequestWriteBLOB(NBCONN(dbg),oid,buf,size);
	free(buf);
	if (wrote == size) {
		ret = DuplicateValue(args,TRUE);
		v = GetItemLongName(ret,"object");
		ValueObjectId(v) = oid;
		ctrl->rc = MCP_OK;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	ValueStruct	*
JSON2_BEGIN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	SYSDATA_DBSTART(dbg,ctrl);
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
JSON2_END(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	SYSDATA_DBCOMMIT(dbg,ctrl);
LEAVE_FUNC;
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)SYSDATA_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)SYSDATA_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)JSON2_BEGIN },
	{	"DBCOMMIT",		(DB_FUNC)JSON2_END },
	/*	table operations	*/
	{	"JSON2READ",	_ReadJSON2		},
	{	"JSON2WRITE",	_WriteJSON2		},

	{	NULL,			NULL }
};

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
_QUERY(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed,
	int			usage)
{
	return NULL;
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

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	_QUERY,
	NULL,
};

extern	DB_Func	*
InitJSONIO2(void)
{
	return	(EnterDB_Function("JSONIO2",Operations,DB_PARSER_NULL,&Core,"",""));
}

