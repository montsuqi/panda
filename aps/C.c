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

#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"misc.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"directory.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"queue.h"
#include	"driver.h"
#include	"loader.h"
#include	"apslib.h"
#include	"debug.h"

static	GHashTable	*ApplicationTable;

static	void	_ReadyDC(void);
static	void	_StopDC(void);
static	Bool	_ExecuteProcess(ProcessNode *node);
static	void	_ReadyDB(void);
static	void	_StopDB(void);
static	int		_StartBatch(char *name, char *param);

static	MessageHandler	Handler = {
	"C",
	FALSE,
	_ExecuteProcess,
	_StartBatch,
	_ReadyDC,
	_StopDC,
	NULL,
	_ReadyDB,
	_StopDB,
	NULL
};

extern	MessageHandler	*
C(void)
{
dbgmsg(">C");
dbgmsg("<C");
	return	(&Handler);
}

static	void
PutApplication(
	ProcessNode	*node)
{

dbgmsg(">PutApplication");
dbgmsg("<PutApplication");
}

static	void
GetApplication(
	ProcessNode	*node)
{
dbgmsg(">GetApplication");
dbgmsg("<GetApplication");
}

static	Bool
_ExecuteProcess(
	ProcessNode	*node)
{
	int		(*apl)(ProcessNode *);
	char	*module;
	Bool	rc;

dbgmsg(">ExecuteProcess");
	module = ValueString(GetItemLongName(node->mcprec->value,"dc.module"));
	if		(  ( apl = LoadModule(ApplicationTable,module) )  !=  NULL  ) {
		PutApplication(node);
		dbgmsg(">C application");
		(void)apl(node);
		dbgmsg("<C application");
		GetApplication(node);
		if		(  ValueInteger(GetItemLongName(node->mcprec->value,"rc"))  <  0  ) {
			rc = FALSE;
		} else {
			rc = TRUE;
		}
	} else {
		MessagePrintf("%s is not found.",module);
		rc = FALSE;
	}
dbgmsg("<ExecuteProcess");
	return	(rc); 
}

static	void
_ReadyDC(void)
{
	WindowBind	*bind;
	int		i;

dbgmsg(">ReadyDC");
	ApplicationTable = InitLoader(); 

	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		bind = ThisLD->window[i];
		if		(  bind->handler  ==  (void *)&Handler  ) {
			printf("preload [%s][%s]\n",bind->name,bind->module);
			(void)LoadModule(ApplicationTable,bind->module);
		}
	}
dbgmsg("<ReadyDC");
}

static	void
_StopDC(void)
{
}

static	void
_StopDB(void)
{
dbgmsg(">StopDB");
	ExecDB_Function("DBDISCONNECT",NULL,NULL);
dbgmsg("<StopDB");
}

static	void
_ReadyDB(void)
{
	ExecDB_Function("DBOPEN",NULL,NULL);
}

static	int
_StartBatch(
	char	*name,
	char	*param)
{
	int		(*apl)(char *);
	int		rc;

dbgmsg(">_StartBatch");
	ApplicationTable = InitLoader(); 
	dbgprintf("starting [%s][%s]\n",name,param);
	if		(  ( apl = LoadModule(ApplicationTable,name) )  !=  NULL  ) {
		rc = apl(param);
	} else {
		MessagePrintf("%s is not found.",name);
		rc = -1;
	}
dbgmsg("<_StartBatch");
 return	(rc); 
}

/*
 *	MCP functions
 */

static	char	*ptype[] = {
	"NULL",
	"CURRENT",
	"NEW",
	"CLOSE",
	"CHANGE",
	"BACK",
	"JOIN",
	"FORK",
	"EXIT"
};

extern	int
MCP_PutWindow(
	ProcessNode	*node,
	char		*wname,
	int			type)
{
	ValueStruct	*mcp;

	mcp = node->mcprec->value;
	memcpy(ValueString(GetItemLongName(mcp,"dc.window")),
		   wname,SIZE_NAME);
	memcpy(ValueString(GetItemLongName(mcp,"dc.puttype")),
		   ptype[type],SIZE_PUTTYPE);
	memcpy(ValueString(GetItemLongName(mcp,"dc.status")),
		   "PUTG",SIZE_STATUS);
	ValueInteger(GetItemLongName(mcp,"rc")) = 0;
	return	(0);
}

extern	RecordStruct	*
MCP_GetWindowRecord(
	ProcessNode	*node,
	char		*name)
{
	WindowBind	*bind;
	RecordStruct	*ret;

dbgmsg(">MCP_GetWindowRecord");
	bind = (WindowBind *)g_hash_table_lookup(node->whash,name);
	ret = bind->rec;
dbgmsg("<MCP_GetWindowRecord");
	return	(ret);
}

extern	void	*
MCP_GetEventHandler(
	GHashTable	*StatusTable,
	ProcessNode	*node)
{
	char		*status
	,			*event;
	GHashTable	*EventTable;
	void		(*handler)(ProcessNode *);

	status = ValueString(GetItemLongName(node->mcprec->value,"dc.status"));
	event  = ValueString(GetItemLongName(node->mcprec->value,"dc.event"));

	dbgprintf("status = [%s]",status);
	dbgprintf("event  = [%s]",status,event);
	if		(  ( EventTable = g_hash_table_lookup(StatusTable,status) )  ==  NULL  ) {
		EventTable = g_hash_table_lookup(StatusTable,"");
	}
	if		(  EventTable  !=  NULL  ) {
		if		(  ( handler = g_hash_table_lookup(EventTable,event) )  ==  NULL  ) {
			handler = g_hash_table_lookup(EventTable,"");
		}
	} else {
		handler = NULL;
	}

	return	(handler);		
}

extern	void
MCP_RegistHandler(
	GHashTable	*StatusTable,
	char	*status,
	char	*event,
	void	(*handler)(ProcessNode *))
{
	GHashTable	*EventTable;

	if		(  ( EventTable = g_hash_table_lookup(StatusTable,status) )  ==  NULL  ) {
		EventTable = NewNameHash();
		g_hash_table_insert(StatusTable,StrDup(status),EventTable);
	}
	if		(  g_hash_table_lookup(EventTable,event)  ==  NULL  ) {
		g_hash_table_insert(EventTable,StrDup(event),handler);
	}
}

extern	int
MCP_ExecFunction(
	ProcessNode	*node,
	char	*rname,
	char	*pname,
	char	*func,
	ValueStruct	*data)
{
	DBCOMM_CTRL		ctrl;
	RecordStruct	*rec;
	int			rno
	,			pno;
	size_t		size;

dbgmsg(">MCP_ExecFunction");
#ifdef	DEBUG
	dbgprintf("rname = [%s]",rname); 
	dbgprintf("pname = [%s]",pname); 
	dbgprintf("func  = [%s]",func); 
#endif
	ctrl.rno = 0;
	ctrl.pno = 0;
	ctrl.blocks = 0;
	if		(  rname  ==  NULL  ) {
		rec = NULL;
	} else
	if		(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) {
		ctrl.rno = rno - 1;
		rec = ThisDB[ctrl.rno];
		if		(  ( pno = (int)g_hash_table_lookup(rec->opt.db->paths,
													pname) )  !=  0  ) {
			ctrl.pno = pno - 1;
		} else {
			ctrl.pno = 0;
		}
	} else {
		rec = NULL;
	}
	if		(  rec  !=  NULL  ) {
		size = NativeSizeValue(NULL,rec->value);
		ctrl.blocks = ( ( size + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;
		CopyValue(rec->value,data);
	}
	strcpy(ctrl.func,func);
	ExecDB_Process(&ctrl,rec);
	if		(  rec  !=  NULL  ) {
		CopyValue(data,rec->value);
	}
	MakeMCP(node->mcprec->value,&ctrl);
dbgmsg("<MCP_ExecFunction");
	return	(ctrl.rc);
}

extern	ValueStruct	*
MCP_GetDB_Define(
	char	*name)
{
	int				rno;
	RecordStruct	*rec;
	ValueStruct		*val;

	if		(  ( rno = (int)g_hash_table_lookup(DB_Table,name) )  !=  0  ) {
		rec = ThisDB[rno-1];
		val = DuplicateValue(rec->value);
	} else {
		val = NULL;
	}
	return	(val);		
}
