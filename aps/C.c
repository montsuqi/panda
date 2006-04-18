/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
#include	<dlfcn.h>
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

ENTER_FUNC;
	table = NewNameHash();
LEAVE_FUNC;
	return	(table);
}

static	void	*
LoadModule(
	GHashTable	*table,
	char	*path,
	char	*name)
{
	char		funcname[SIZE_LONGNAME+1]
	,			filename[SIZE_LONGNAME+1];
	void		*f_main;
	void		(*f_init)(void);
	void		*handle;

ENTER_FUNC;
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
LEAVE_FUNC;
	return	(f_main);
}

static	void
PutApplication(
	ProcessNode	*node)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
GetApplication(
	ProcessNode	*node)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	Bool
_ExecuteProcess(
	MessageHandler	*handler,
	ProcessNode	*node)
{
	int		(*apl)(ProcessNode *);
	char	*module;
	Bool	rc;

ENTER_FUNC;
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
		Message("%s is not found.",module);
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc); 
}

static	void
_ReadyDC(
	MessageHandler	*handler)
{
ENTER_FUNC;
	ApplicationTable = InitLoader(); 
LEAVE_FUNC;
}

static	void
_StopDC(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
_StopDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
_ReadyDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	int
_StartBatch(
	MessageHandler	*handler,
	char	*name,
	char	*param)
{
	int		(*apl)(char *);
	int		rc;

ENTER_FUNC;
	ApplicationTable = InitLoader(); 
	dbgprintf("starting [%s][%s]\n",name,param);
	if		(  ( apl = LoadModule(ApplicationTable,handler->loadpath,name) )  !=  NULL  ) {
		rc = apl(param);
	} else {
		Message("%s is not found.",name);
		rc = -1;
	}
LEAVE_FUNC;
	return	(rc); 
}

static	void
_ReadyExecute(
	MessageHandler	*handler,
	char			*loadpath)
{
ENTER_FUNC;
	if		(  LibPath  ==  NULL  ) { 
		if		(  ( APS_LoadPath = getenv("APS_LOAD_PATH") )  ==  NULL  ) {
			if		(  loadpath  !=  NULL  ) {
				APS_LoadPath = loadpath;
			} else {
				APS_LoadPath = MONTSUQI_LOAD_PATH;
			}
		}
	} else {
		APS_LoadPath = LibPath;
	}
	if		(  handler->loadpath  ==  NULL  ) {
		handler->loadpath = APS_LoadPath;
	}
LEAVE_FUNC;
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
ENTER_FUNC;
	table = NewNameHash();
LEAVE_FUNC;
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

ENTER_FUNC;
	mcp = node->mcprec->value;
	strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.window")),wname);
	strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.puttype")),ptype[type]);
	strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.status")),"PUTG");
	ValueInteger(GetItemLongName(mcp,"rc")) = 0;
LEAVE_FUNC;
	return	(0);
}

extern	RecordStruct	*
MCP_GetWindowRecord(
	ProcessNode	*node,
	char		*name)
{
	WindowBind	*bind;
	RecordStruct	*ret;

ENTER_FUNC;
	bind = (WindowBind *)g_hash_table_lookup(node->whash,name);
	ret = bind->rec;
LEAVE_FUNC;
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

ENTER_FUNC;
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

LEAVE_FUNC;
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

ENTER_FUNC;
	if		(  ( EventTable = g_hash_table_lookup(StatusTable,status) )  ==  NULL  ) {
		EventTable = NewNameHash();
		g_hash_table_insert(StatusTable,StrDup(status),EventTable);
	}
	if		(  g_hash_table_lookup(EventTable,event)  ==  NULL  ) {
		g_hash_table_insert(EventTable,StrDup(event),handler);
	}
LEAVE_FUNC;
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
	ValueStruct		*value;

ENTER_FUNC;
#ifdef	DEBUG
	dbgprintf("rname = [%s]",rname); 
	dbgprintf("pname = [%s]",pname); 
	dbgprintf("func  = [%s]",func); 
#endif
	rec = MakeCTRLbyName(&value,&ctrl,rname,pname,func);
	if		(  rec  !=  NULL  ) {
		CopyValue(value,data);
	}
	ExecDB_Process(&ctrl,rec,value);
	if		(  rec  !=  NULL  ) {
		CopyValue(data,value);
	}
	if		(  node  !=  NULL  ) {
		MakeMCP(node->mcprec->value,&ctrl);
	}
LEAVE_FUNC;
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
	DB_Operation	*op;
	ValueStruct		*val
		,			*ret;
	char			*p
		,			*rname
		,			*pname
		,			*oname;

ENTER_FUNC;
	strcpy(buff,name);
	rname = buff;
	if		(  ( p = strchr(buff,':') )  !=  NULL  ) {
		*p = 0;
		pname = p + 1;
		if		(  ( p = strchr(pname,':') )  !=  NULL  ) {
			*p = 0;
			oname = p + 1;
		} else {
			oname = NULL;
		}
	} else {
		oname = NULL;
		pname = NULL;
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
LEAVE_FUNC;
	return	(ret);		
}
