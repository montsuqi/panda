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
static	void	_ExecuteProcess(ProcessNode *node);
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

static	void
_ExecuteProcess(
	ProcessNode	*node)
{
	int		(*apl)(ProcessNode *);
	char	*module;

dbgmsg(">ExecuteProcess");
	module = ValueString(GetItemLongName(node->mcprec,"dc.module"));
	if		(  ( apl = LoadModule(ApplicationTable,module) )  !=  NULL  ) {
		PutApplication(node);
		dbgmsg(">C application");
		(void)apl(node);
		dbgmsg("<C application");
		GetApplication(node);
	} else {
		printf("%s is not found.\n",module);
	}
dbgmsg("<ExecuteProcess");
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
#ifdef	DEBUG
	printf("starting [%s][%s]\n",name,param);
#endif
	if		(  ( apl = LoadModule(ApplicationTable,name) )  !=  NULL  ) {
		rc = apl(param);
	} else {
		fprintf(stderr,"%s is not found.\n",name);
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
	"EXIT"
};

extern	int
MCP_PutWindow(
	ProcessNode	*node,
	char		*wname,
	int			type)
{
	ValueStruct	*mcp;

	mcp = node->mcprec;
	memcpy(ValueString(GetItemLongName(mcp,"dc.window")),
		   wname,SIZE_NAME);
	memcpy(ValueString(GetItemLongName(mcp,"dc.puttype")),
		   ptype[type],SIZE_PUTTYPE);
	memcpy(ValueString(GetItemLongName(mcp,"dc.status")),
		   "PUTG",SIZE_STATUS);
	ValueInteger(GetItemLongName(mcp,"rc")) = 0;
	return	(0);
}

extern	ValueStruct	*
MCP_GetWindowRecord(
	ProcessNode	*node,
	char		*name)
{
	int		ix;
	ValueStruct	*ret;

	if		(  ( ix = (int)g_hash_table_lookup(node->whash,name) )  !=  0  ) {
		ret = node->scrrec[ix-1];
	} else {
		ret = NULL;
	}
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

	status = ValueString(GetItemLongName(node->mcprec,"dc.status"));
	event  = ValueString(GetItemLongName(node->mcprec,"dc.event"));

	printf("status = [%s]\nevent  = [%s]\n",status,event);
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
HAKAMA_Function(
	ValueStruct	*mcp,
	ValueStruct	*data)
{
	DBCOMM_CTRL	ctrl;
	RecordStruct	*rec;
	char		*mcp_func;

dbgmsg(">HAKAMA_Function");
	mcp_func = ValueString(GetItemLongName(mcp,"func"));

	MakeCTRL(&ctrl,mcp);
	if		(  !strcmp(mcp_func,"DBOPEN")  ) {
		rec = NULL;
	} else
	if		(  !strcmp(mcp_func,"DBCLOSE")  ) {
		rec = NULL;
	} else
	if		(  !strcmp(mcp_func,"DBSTART")  ) {
		rec = NULL;
	} else
	if		(  !strcmp(mcp_func,"DBCOMMIT")  ) {
		rec = NULL;
	} else
	if		(  !strcmp(mcp_func,"DBDISCONNECT")  ) {
		rec = NULL;
	} else {
		rec = ThisDB[ctrl.rno];
		CopyValue(rec->rec,data);
	}
	ExecDB_Process(&ctrl,rec);
	if		(  rec  !=  NULL  ) {
		CopyValue(data,rec->rec);
	}
	MakeMCP(mcp,&ctrl);
dbgmsg("<HAKAMA_Function");
	return	(ValueInteger(GetItemLongName(mcp,"rc")));
}
