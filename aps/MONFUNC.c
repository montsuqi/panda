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
InitializeCTRL(
	DBCOMM_CTRL	*ctrl)
{
	ctrl->rno = 0;
	ctrl.pno = 0;
	ctrl->rcount = 0;
	ctrl.blocks = 0;
	ctrl->rc = 0;
	ctrl->limit = 1;
	ctrl->redirect = 1;
}

static	void
ResetUser(
	ValueStruct		*mcp)
{
	if ( CurrentTerm ){
		SetValueString(GetItemLongName(mcp,"dc.term"),CurrentTerm,NULL);
	}
	if ( CurrentUser ){	
		SetValueString(GetItemLongName(mcp,"dc.user"),CurrentUser,NULL);
	}
}

static int
MonDBFunc(
	ValueStruct		*mcp,
	char			*func,	
	char			*data)
{
	RecordStruct	*rec;
	DBCOMM_CTRL	ctrl;
	ValueStruct	*value
		,			*ret;
	char			*rname
		,			*pname;
ENTER_FUNC;
	ResetUser(mcp);
	InitializeCTRL(&ctrl);
	strcpy(ctrl.func,func);
	if		(  !strcmp(func,"DBOPEN") 		||
				   !strcmp(func,"DBCLOSE")		||
				   !strcmp(func,"DBSTART") 		||
				   !strcmp(func,"DBCOMMIT")		||
				   !strcmp(func,"DBDISCONNECT")  ) {
		ctrl.fDBOperation = TRUE;
		rec = NULL;
		value = NULL;
	} else {
#if	0
		ctrl.limit = ValueInteger(GetItemLongName(mcp,"db.limit"));
#else
		if		(  ValueInteger(GetItemLongName(mcp,"version"))  ==  2  ) {
			ctrl.limit = ValueInteger(GetItemLongName(mcp,"db.limit"));
			ctrl.redirect = ValueInteger(GetItemLongName(mcp,"db.redirect"));
		}
#endif
		rname = ValueStringPointer(GetItemLongName(mcp,"db.table"));
		pname = ValueStringPointer(GetItemLongName(mcp,"db.pathname"));
		dbgprintf("%s:%s:%s",rname,pname,func);
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
LEAVE_FUNC;
	return ctrl.rc;
}

static	int
MonGLFunc(
	ValueStruct		*mcp)
{
ENTER_FUNC;
	strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.status")),"PUTG");
	ValueInteger(GetItemLongName(mcp,"rc")) = 0;
	return 0;
LEAVE_FUNC;
}

extern	int
MONFUNC(
	char	*mcpdata,
	char	*data)
{
	ValueStruct		*mcp;
	char			*func;
	int			ret;

ENTER_FUNC;
	mcp = ThisEnv->mcprec->value;
	OpenCOBOL_UnPackValue(OpenCOBOL_Conv, mcpdata, mcp);
	func  = ValueStringPointer(GetItemLongName(mcp,"func"));
	if		(  !strcmp(func,"PUTWINDOW")  ) {
		ret = MonGLFunc(mcp);
	} else {
		ret = MonDBFunc(mcp, func, data);
	}
#ifdef	DEBUG
	DumpValueStruct(mcp);
#endif
	OpenCOBOL_PackValue(OpenCOBOL_Conv, mcpdata, mcp);
LEAVE_FUNC;
	return ret;
}

#endif
