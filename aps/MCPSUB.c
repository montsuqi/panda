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

#define	DEBUG
#define	TRACE
/*
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
	DBCOMM_CTRL	ctrl;
	RecordStruct	*rec;
	ValueStruct	*mcp;
	char		*mcp_func;

dbgmsg(">MCPSUB");
	mcp = ThisEnv->mcprec->value; 
	OpenCOBOL_UnPackValue(OpenCOBOL_Conv,mcpdata,mcp);
	mcp_func = ValueStringPointer(GetItemLongName(mcp,"func"));

	if		(  !strcmp(mcp_func,"PUTWINDOW")  ) {
		strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.status")),"PUTG");
		ValueInteger(GetItemLongName(mcp,"rc")) = 0;
	} else {
		MakeCTRL(&ctrl,mcp);
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
			OpenCOBOL_UnPackValue(OpenCOBOL_Conv, data, rec->value);
		}
		ExecDB_Process(&ctrl,rec);
		if		(  rec  !=  NULL  ) {
			OpenCOBOL_PackValue(OpenCOBOL_Conv, data, rec->value);
		}
		MakeMCP(mcp,&ctrl);
	}
	OpenCOBOL_PackValue(OpenCOBOL_Conv, mcpdata,mcp);
dbgmsg("<MCPSUB");
}

#endif
