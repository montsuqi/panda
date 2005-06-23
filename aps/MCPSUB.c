/*
PANDA -- a simple transaction monitor
Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifdef	HAVE_OPENCOBOL
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"libmondai.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"handler.h"
#include	"OpenCOBOL.h"
#include	"directory.h"
#include	"debug.h"

static	void
CheckArg(
	char		*func,
	DBCOMM_CTRL	*ctrl)
{
	if		(	(  ctrl->rno  !=  0  )
			||	(  ctrl->pno  !=  0  ) ) {
		fprintf(stderr,"argument invalid on %s\n",func);
	}
}

extern	void
MCPSUB(
	char	*mcpdata,
	char	*data)
{
	DBCOMM_CTRL		ctrl;
	RecordStruct	*rec;
	PathStruct		*path;
	DB_Operation	*op;
	ValueStruct		*mcp
		,			*value;
	char			*mcp_func;
	int				ono;

ENTER_FUNC;
	mcp = ThisEnv->mcprec->value; 
	OpenCOBOL_UnPackValue(OpenCOBOL_Conv,mcpdata,mcp);
	mcp_func = ValueStringPointer(GetItemLongName(mcp,"func"));

	if		(  !strcmp(mcp_func,"PUTWINDOW")  ) {
		strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.status")),"PUTG");
		ValueInteger(GetItemLongName(mcp,"rc")) = 0;
	} else {
		value = NULL;
		MakeCTRL(&ctrl,mcp);
		ctrl.rc = 0;
		if		(  !strcmp(mcp_func,"DBOPEN")  ) {
			CheckArg(mcp_func,&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(mcp_func,"DBCLOSE")  ) {
			CheckArg(mcp_func,&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(mcp_func,"DBSTART")  ) {
			CheckArg(mcp_func,&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(mcp_func,"DBCOMMIT")  ) {
			CheckArg(mcp_func,&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(mcp_func,"DBDISCONNECT")  ) {
			CheckArg(mcp_func,&ctrl);
			rec = NULL;
		} else {
			rec = ThisDB[ctrl.rno];
			value = rec->value;
			path = rec->opt.db->path[ctrl.pno];
			value = ( path->args != NULL ) ? path->args : value;
			if		(  ( ono = (int)g_hash_table_lookup(path->opHash,mcp_func) )  !=  0  ) {
				op = path->ops[ono-1];
				value = ( op->args != NULL ) ? op->args : value;
			}
			OpenCOBOL_UnPackValue(OpenCOBOL_Conv, data, value);
		}
		ExecDB_Process(&ctrl,rec,value);
		if		(	(  rec      !=  NULL    )
				&&	(  ctrl.rc  ==  MCP_OK  ) )	{
			OpenCOBOL_PackValue(OpenCOBOL_Conv, data, value);
		}
		MakeMCP(mcp,&ctrl);
	}
	OpenCOBOL_PackValue(OpenCOBOL_Conv, mcpdata,mcp);
LEAVE_FUNC;
}

#endif
