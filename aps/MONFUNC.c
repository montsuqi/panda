/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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
		Warning("argument invalid on %s\n",func);
	}
}

extern	int
MONFUNC(
	char	*mcpdata,
	char	*data)
{
	DBCOMM_CTRL		ctrl;
	RecordStruct	*rec;
	ValueStruct		*mcp
		,			*value
		,			*ret;
	char			*func
		,			*rname
		,			*pname;

ENTER_FUNC;
	mcp = ThisEnv->mcprec->value; 
	OpenCOBOL_UnPackValue(OpenCOBOL_Conv,mcpdata,mcp);
	func  = ValueStringPointer(GetItemLongName(mcp,"func"));

	if		(  !strcmp(func,"PUTWINDOW")  ) {
		strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.status")),"PUTG");
		ValueInteger(GetItemLongName(mcp,"rc")) = 0;
	} else {
		rname = ValueStringPointer(GetItemLongName(mcp,"db.table"));
		pname = ValueStringPointer(GetItemLongName(mcp,"db.pathname"));
		dbgprintf("%s:%s:%s",rname,pname,func);
#if	0
		ctrl.limit = ValueInteger(GetItemLongName(mcp,"db.limit"));
#else
		if		(  ValueInteger(GetItemLongName(mcp,"version"))  ==  2  ) {
			ctrl.limit = ValueInteger(GetItemLongName(mcp,"db.limit"));
			ctrl.redirect = ValueInteger(GetItemLongName(mcp,"db.redirect"));
		} else {
			ctrl.limit = 1;
			ctrl.redirect = 1;
		}
#endif
		ctrl.rcount = 0;
		ctrl.rc = 0;
		strcpy(ctrl.func,func);
		if		(  !strcmp(func,"DBOPEN") 		||
				   !strcmp(func,"DBCLOSE")		||
				   !strcmp(func,"DBSTART") 		||
				   !strcmp(func,"DBCOMMIT")		||
				   !strcmp(func,"DBDISCONNECT")  ) {
			ctrl.limit = 1;
			ctrl.fDBOperation = TRUE;
			rec = NULL;
			value = NULL;
			CheckArg(func,&ctrl);
		} else {
			if (  !GetTableFuncData(&rec,&value,&ctrl,rname,pname,func)  ) {
				Error("record[%s,%s,%s] not found",rname,pname,func);
			}
			OpenCOBOL_UnPackValue(OpenCOBOL_Conv, data, value);
		}
		ret = ExecDB_Process(&ctrl,rec,value);
		if		(	(  ret      !=  NULL    )
				&&	(  ctrl.rc  ==  MCP_OK  ) )	{
			OpenCOBOL_PackValue(OpenCOBOL_Conv, data, ret);
		}
		if		(  ret  !=  NULL  ) {
			FreeValueStruct(ret);
		}
		MakeMCP(mcp,&ctrl);
	}
#ifdef	DEBUG
	DumpValueStruct(mcp);
#endif
	OpenCOBOL_PackValue(OpenCOBOL_Conv, mcpdata, mcp);
LEAVE_FUNC;
	return ctrl.rc;
}

#endif
