/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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
#ifdef	HAVE_OPENCOBOL
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"
#include	"const.h"
#include	"value.h"
#include	"comm.h"
#include	"cobolvalue.h"
#include	"enum.h"
#include	"driver.h"
#include	"dbgroup.h"
#include	"OpenCOBOL.h"
#include	"directory.h"
#include	"debug.h"

#ifdef	DEBUG
static	void
DumpDB_Node(
	DBCOMM_CTRL	*ctrl)
{
	printf("func   = [%s]\n",ctrl->func);
	printf("blocks = %d\n",ctrl->blocks);
	printf("rno    = %d\n",ctrl->rno);
	printf("pno    = %d\n",ctrl->pno);
}
#endif

static	void
MakeCTRL(
	DBCOMM_CTRL	*ctrl,
	ValueStruct	*mcp)
{
	strcpy(ctrl->func,ValueString(GetItemLongName(mcp,"func")));
	ctrl->rc = ValueInteger(GetItemLongName(mcp,"rc"));
	ctrl->blocks = ValueInteger(GetItemLongName(mcp,"db.path.blocks"));
	ctrl->rno = ValueInteger(GetItemLongName(mcp,"db.path.rname"));
	ctrl->pno = ValueInteger(GetItemLongName(mcp,"db.path.pname"));
#ifdef	DEBUG
	DumpDB_Node(ctrl);
#endif
}

static	void
MakeMCP(
	ValueStruct	*mcp,
	DBCOMM_CTRL	*ctrl)
{
	strcpy(ValueString(GetItemLongName(mcp,"func")),ctrl->func);
	ValueInteger(GetItemLongName(mcp,"rc")) = ctrl->rc;
	ValueInteger(GetItemLongName(mcp,"db.path.blocks")) = ctrl->blocks;
	ValueInteger(GetItemLongName(mcp,"db.path.rname")) = ctrl->rno;
	ValueInteger(GetItemLongName(mcp,"db.path.pname")) = ctrl->pno;
}

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
	ValueStruct	*mcp_func;

dbgmsg(">MCPSUB");
	mcp = ThisEnv->mcprec; 
	OpenCOBOL_UnPackValue(mcpdata,mcp);
	mcp_func = GetItemLongName(mcp,"func");

	if		(  !strcmp(ValueString(mcp_func),"PUTWINDOW")  ) {
		memcpy(ValueString(GetItemLongName(mcp,"dc.status")),
			   "PUTG",SIZE_STATUS);
		ValueInteger(GetItemLongName(mcp,"rc")) = 0;
	} else {
		MakeCTRL(&ctrl,mcp);
		if		(  !strcmp(ValueString(mcp_func),"DBOPEN")  ) {
			CheckArg(ValueString(mcp_func),&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(ValueString(mcp_func),"DBCLOSE")  ) {
			CheckArg(ValueString(mcp_func),&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(ValueString(mcp_func),"DBSTART")  ) {
			CheckArg(ValueString(mcp_func),&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(ValueString(mcp_func),"DBCOMMIT")  ) {
			CheckArg(ValueString(mcp_func),&ctrl);
			rec = NULL;
		} else
		if		(  !strcmp(ValueString(mcp_func),"DBDISCONNECT")  ) {
			CheckArg(ValueString(mcp_func),&ctrl);
			rec = NULL;
		} else {
			rec = ThisDB[ctrl.rno];
			OpenCOBOL_UnPackValue(data, rec->rec);
		}
		ExecDB_Process(&ctrl,rec);
		if		(  rec  !=  NULL  ) {
			OpenCOBOL_PackValue(data, rec->rec);
		}
		MakeMCP(mcp,&ctrl);
	}
	OpenCOBOL_PackValue(mcpdata,mcp);
dbgmsg("<MCPSUB");
}

#endif
