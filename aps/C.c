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
#include	<dlfcn.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"directory.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"load.h"
#include	"dbgroup.h"
#include	"queue.h"
#include	"driver.h"
#include	"apslib.h"
#include	"debug.h"

static	GHashTable	*ApplicationTable;
static	char	*APS_LoadPath;

static	GHashTable	*
InitLoader(void)
{
	GHashTable	*table;
	char		*path;

dbgmsg(">InitLoader");
	table = NewNameHash();
	if		(  LibPath  ==  NULL  ) { 
		if		(  ( path = getenv("APS_LOAD_PATH") )  ==  NULL  ) {
			APS_LoadPath = MONTSUQI_LOAD_PATH;
		} else {
			APS_LoadPath = path;
		}
	} else {
		APS_LoadPath = LibPath;
	}
dbgmsg("<InitLoader");
	return	(table);
}

static	void	*
LoadModule(
	GHashTable	*table,
	char	*path,
	char	*name)
{
	char		funcname[SIZE_BUFF]
	,			filename[SIZE_BUFF];
	void		*f_main;
	void		(*f_init)(void);
	void		*handle;

dbgmsg(">LoadModule");
	if		(  ( f_main = (void *)g_hash_table_lookup(table,name) )  ==  NULL  ) {
		sprintf(filename,"%s.so",name);
		if		(  ( handle = LoadFile(path,filename) )  !=  NULL  ) {
			sprintf(funcname,"%sInit",name);
			if		(  ( f_init = (void *)dlsym(handle,funcname) )  !=  NULL  ) {
				f_init();
			}
			sprintf(funcname,"%sMain",name);
			f_main = dlsym(handle,funcname);
			g_hash_table_insert(table,StrDup(name),f_main);
		} else {
			fprintf(stderr,"[%s] not found.\n",name);
		}
	}
dbgmsg("<LoadModule");
	return	(f_main);
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
	MessageHandler	*handler,
	ProcessNode	*node)
{
	int		(*apl)(ProcessNode *);
	char	*module;
	Bool	rc;

dbgmsg(">ExecuteProcess");
	module = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.module"));
	if		(  ( apl = LoadModule(ApplicationTable,handler->loadpath,module) )  !=  NULL  ) {
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
_ReadyDC(
	MessageHandler	*handler)
{
	WindowBind	*bind;
	int		i;

dbgmsg(">ReadyDC");
	ApplicationTable = InitLoader(); 
dbgmsg("<ReadyDC");
}

static	void
_StopDC(
	MessageHandler	*handler)
{
}

static	void
_StopDB(
	MessageHandler	*handler)
{
dbgmsg(">StopDB");
dbgmsg("<StopDB");
}

static	void
_ReadyDB(
	MessageHandler	*handler)
{
}

static	int
_StartBatch(
	MessageHandler	*handler,
	char	*name,
	char	*param)
{
	int		(*apl)(char *);
	int		rc;

dbgmsg(">_StartBatch");
	ApplicationTable = InitLoader(); 
	dbgprintf("starting [%s][%s]\n",name,param);
	if		(  ( apl = LoadModule(ApplicationTable,handler->loadpath,name) )  !=  NULL  ) {
		rc = apl(param);
	} else {
		MessagePrintf("%s is not found.",name);
		rc = -1;
	}
dbgmsg("<_StartBatch");
 return	(rc); 
}

static	void
_ReadyExecute(
	MessageHandler	*handler)
{
	if		(  handler->loadpath  ==  NULL  ) {
		handler->loadpath = APS_LoadPath;
	}
}

static	MessageHandlerClass	Handler = {
	"C",
	_ReadyExecute,
	_ExecuteProcess,
	_StartBatch,
	_ReadyDC,
	_StopDC,
	NULL,
	_ReadyDB,
	_StopDB,
	NULL
};

extern	MessageHandlerClass	*
C(void)
{
	GHashTable	*table;
	char		*path;
dbgmsg(">C");
	table = NewNameHash();
	if		(  LibPath  ==  NULL  ) { 
		if		(  ( path = getenv("APS_LOAD_PATH") )  ==  NULL  ) {
			APS_LoadPath = MONTSUQI_LOAD_PATH;
		} else {
			APS_LoadPath = path;
		}
	} else {
		APS_LoadPath = LibPath;
	}
dbgmsg("<C");
	return	(&Handler);
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
	strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.window")),wname);
	strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.puttype")),ptype[type]);
	strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.status")),"PUTG");
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

	status = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.status"));
	event = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.event"));

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
	PathStruct		*path;
	SQL_Operation	*op;
	ValueStruct		*value;
	int			rno
		,		pno
		,		ono;
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

	value = NULL;
	if		(	(  rname  !=  NULL  )
			&&	(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) ) {
		ctrl.rno = rno - 1;
		rec = ThisDB[ctrl.rno];
		value = rec->value;
		if		(	(  pname  !=  NULL  )
				&&	(  ( pno = (int)g_hash_table_lookup(rec->opt.db->paths,
														pname) )  !=  0  ) ) {
			ctrl.pno = pno - 1;
			path = rec->opt.db->path[pno-1];
			value = ( path->args != NULL ) ? path->args : value;
			if		(	(  func  !=  NULL  )
					&&	( ( ono = (int)g_hash_table_lookup(path->opHash,func) )  !=  0  ) ) {
				op = path->ops[ono-1];
				value = ( op->args != NULL ) ? op->args : value;
			}
		} else {
			ctrl.pno = 0;
		}
	} else {
		rec = NULL;
	}

	if		(  rec  !=  NULL  ) {
		size = NativeSizeValue(NULL,rec->value);
		ctrl.blocks = ( ( size + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;
		CopyValue(value,data);
	}
	strcpy(ctrl.func,func);
	ExecDB_Process(&ctrl,rec,value);
	if		(  rec  !=  NULL  ) {
		CopyValue(data,value);
	}
	MakeMCP(node->mcprec->value,&ctrl);
dbgmsg("<MCP_ExecFunction");
	return	(ctrl.rc);
}

extern	ValueStruct	*
MCP_GetDB_Define(
	char	*name)
{
	char			buff[SIZE_LONGNAME+1];
	int				rno
		,			pno
		,			ono;
	RecordStruct	*rec;
	PathStruct		*path;
	SQL_Operation	*op;
	ValueStruct		*val
		,			*ret;
	char			*p
		,			*q
		,			*rname
		,			*pname
		,			*oname;

	strcpy(buff,name);
	rname = buff;
	if		(  ( p = strchr(buff,':') )  !=  NULL  ) {
		*p = 0;
		pname = p + 1;
		if		(  ( p = strchr(pname,':') )  !=  NULL  ) {
			*q = 0;
			oname = p + 1;
		} else {
			oname = NULL;
		}
	} else {
		oname = NULL;
		oname = NULL;
	}		

	val = NULL;
	if		(  ( rno = (int)g_hash_table_lookup(DB_Table,name) )  !=  0  ) {
		rec = ThisDB[rno-1];
		val = rec->value;
		if		(	(  pname  !=  NULL  )
				&&	(  ( pno = (int)g_hash_table_lookup(rec->opt.db->paths,
														pname) )  !=  0  ) )	{
			path = rec->opt.db->path[pno-1];
			val = ( path->args != NULL ) ? path->args : val;
			if		(	(  oname  !=  NULL  )
					&&	(  ( ono = (int)g_hash_table_lookup(path->opHash,oname) )  !=  0  ) )
				{
				op = path->ops[ono-1];
				val = ( op->args != NULL ) ? op->args : val;
			}
		}
	}
	if		(  val  !=  NULL  ) {
		ret = DuplicateValue(val);
	} else {
		ret = NULL;
	}
	return	(ret);		
}
