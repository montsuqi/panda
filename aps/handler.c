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
#define	NEW_SEQUENCT
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	"defaults.h"
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<glib.h>

#include	"defaults.h"
#include	"types.h"
#include	"const.h"
#include	"libmondai.h"
#include	"struct.h"
#include	"DDparser.h"
#include	"enum.h"
#include	"load.h"
#define		_HANDLER
#include	"handler.h"
#include	"apsio.h"
#include	"dblib.h"
#include	"glterm.h"
#include	"BDparser.h"
#include	"debug.h"

static	char	*STATUS[4] = {
	"LINK",
	"PUTG",
	"PUTG",
	"RSND"
};


static	char	*APS_HandlerLoadPath;

static	GHashTable	*HandlerClassTable;
static	GHashTable	*TypeHash;

static	MessageHandlerClass	*
EnterHandlerClass(
	char	*name)
{
	void			*dlhandle;
	MessageHandlerClass	*klass;
	MessageHandlerClass	*(*finit)(void);
	char			filename[SIZE_BUFF];

dbgmsg(">EnterHandlerClass");
	if		(  ( klass = g_hash_table_lookup(HandlerClassTable,name) )  ==  NULL  ) {
		MessagePrintf("%s handlerClass invoke.", name);
		sprintf(filename,"%s.so",name);
		klass = NULL;
		if		(  ( dlhandle = LoadFile(APS_HandlerLoadPath,filename) )  !=  NULL  ) {
			if		(  ( finit = (void *)dlsym(dlhandle,name) )
					   ==  NULL  )	{
				MessagePrintf("[%s] is invalid.",name);
			} else {
				klass = (*finit)();
				if		(  g_hash_table_lookup(HandlerClassTable,name)  ==  NULL  ) {
					g_hash_table_insert(HandlerClassTable,StrDup(name),klass);
				}
			}
		} else {
			fprintf(stderr,"[%s] not found.\n",name);
		}
	}
dbgmsg("<EnterHandlerClass");
	return	(klass);
}

static	void
InitHandler(void)
{
dbgmsg(">InitHandler");

	if		(  ( APS_HandlerLoadPath = getenv("APS_HANDLER_LOAD_PATH") )
			   ==  NULL  ) {
		APS_HandlerLoadPath = MONTSUQI_LIBRARY_PATH;
	}
	HandlerClassTable = NewNameHash();
}

static	void
_InitiateHandler(
	MessageHandler		*handler)
{
	MessageHandlerClass	*klass;

	if		(  ( handler->fInit & INIT_LOAD )  ==  0  ) {
		handler->fInit |= INIT_LOAD;
		if		(  ( klass = EnterHandlerClass((char *)handler->klass) )  ==  NULL  ) {
			MessagePrintf("[%s] is invalid handler class.",(char *)handler->klass);
		} else {
			handler->klass = klass;
		}
		if		(  handler->serialize  !=  NULL  ) {
			handler->serialize = GetConvFunc((char *)handler->serialize);
		}
	}
}

static	void
_OnlineInit(
	char	*name,
	WindowBind	*bind,
	void	*dummy)
{
	_InitiateHandler(bind->handler);
}

extern	void
InitiateHandler(void)
{
dbgmsg(">InitiateHandler");
	InitHandler();
	dbgprintf("LD = [%s]",ThisLD->name);
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_OnlineInit,NULL);

	TypeHash = NewNameiHash();
	g_hash_table_insert(TypeHash,"CURRENT",(gpointer)SCREEN_CURRENT_WINDOW);
	g_hash_table_insert(TypeHash,"NEW",(gpointer)SCREEN_NEW_WINDOW);
	g_hash_table_insert(TypeHash,"CLOSE",(gpointer)SCREEN_CLOSE_WINDOW);
	g_hash_table_insert(TypeHash,"CHANGE",(gpointer)SCREEN_CHANGE_WINDOW);
	g_hash_table_insert(TypeHash,"JOIN",(gpointer)SCREEN_JOIN_WINDOW);
	g_hash_table_insert(TypeHash,"FORK",(gpointer)SCREEN_FORK_WINDOW);
	g_hash_table_insert(TypeHash,"EXIT",(gpointer)SCREEN_END_SESSION);
	g_hash_table_insert(TypeHash,"BACK",(gpointer)SCREEN_BACK_WINDOW);
dbgmsg("<InitiateHandler");
}

static	void
_BatchInit(
	char	*name,
	BatchBind	*bind,
	void	*dummy)
{
	_InitiateHandler(bind->handler);
}

extern	void
InitiateBatchHandler(void)
{
ENTER_FUNC;
	InitHandler();
	g_hash_table_foreach(ThisBD->BatchTable,(GHFunc)_BatchInit,NULL);
LEAVE_FUNC;
}

static	void
_ReadyDC(
	char	*name,
	WindowBind	*bind,
	void	*dummy)
{
	MessageHandler	*handler;

	handler = bind->handler;
ENTER_FUNC;
	if		(  ( handler->fInit & INIT_EXECUTE )  ==  0  ) {
		handler->fInit |= INIT_EXECUTE;
		if		(  handler->klass->ReadyExecute  !=  NULL  ) {
			handler->klass->ReadyExecute(handler);
		}
	}
	if		(  ( handler->fInit & INIT_READYDC )  ==  0  ) {
		handler->fInit |= INIT_READYDC;
		if		(  handler->klass->ReadyDC  !=  NULL  ) {
			handler->klass->ReadyDC(handler);
		}
	}
LEAVE_FUNC;
}

extern	void
ReadyDC(void)
{
ENTER_FUNC;
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_ReadyDC,NULL);
LEAVE_FUNC;
}

extern	void
ReadyHandlerDB(
	MessageHandler	*handler)
{
	if		(  ( handler->fInit & INIT_READYDB )  ==  0  ) {
		handler->fInit |= INIT_READYDB;
		if		(  handler->klass->ReadyDB  !=  NULL  ) {
			handler->klass->ReadyDB(handler);
		}
	}
}

static	void
_ReadyOnlineDB(
	char		*name,
	WindowBind	*bind,
	void		*dummy)
{
	ReadyHandlerDB(bind->handler);
}

extern	void
ReadyOnlineDB(void)
{
ENTER_FUNC;
	InitDB_Process();
	OpenDB(NULL);
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_ReadyOnlineDB,NULL);
LEAVE_FUNC;
}

static	void
CallBefore(
	ProcessNode	*node)
{
	ValueStruct	*mcp;

dbgmsg(">CallBefore");
	mcp = node->mcprec->value; 
	memcpy(ValueStringPointer(GetItemLongName(mcp,"dc.status")),
		   STATUS[*ValueStringPointer(GetItemLongName(mcp,"private.pstatus")) - '1'],
		   SIZE_STATUS);
	ValueIsNonNil(GetItemLongName(mcp,"dc.status"));
	node->w.n = 0;
	CurrentProcess = node; 
dbgmsg("<CallBefore");
}

static	void
SetPutType(
	ProcessNode	*node,
	char		*wname,
	byte		type)
{	
	strcpy(node->w.control[node->w.n].window,wname);
	node->w.control[node->w.n].PutType = type;
	node->w.n ++;
}

static	void
CallAfter(
	ProcessNode	*node)
{
	int		i
		,	winfrom
		,	winend
		,	sindex;
	byte	PutType;

	ValueStruct	*mcp_swindow;
	ValueStruct	*mcp_sindex;
	ValueStruct	*mcp_puttype;
	ValueStruct	*mcp_pputtype;
	ValueStruct	*mcp_dcwindow;
	ValueStruct	*mcp;

dbgmsg(">CallAfter");
	mcp = node->mcprec->value; 
	mcp_sindex 	= GetItemLongName(mcp,"private.count");
	mcp_swindow = GetItemLongName(mcp,"private.swindow");
	mcp_puttype = GetItemLongName(mcp,"dc.puttype");
	mcp_pputtype = GetItemLongName(mcp,"private.pputtype");
	mcp_dcwindow = GetItemLongName(mcp,"dc.window");

	if		(  ( PutType = (byte)(int)g_hash_table_lookup(TypeHash,ValueToString(mcp_puttype,NULL)) )  ==  0  ) {
		PutType = SCREEN_CURRENT_WINDOW;
	}
	if		(  ( sindex = ValueInteger(mcp_sindex) )  ==  0  ) {
		strcpy(ValueStringPointer(GetArrayItem(mcp_swindow,0)),
			   ValueStringPointer(mcp_dcwindow));
		ValueIsNonNil(GetArrayItem(mcp_swindow,0));
		sindex = 1;
	}
	if		(  PutType  ==  SCREEN_BACK_WINDOW  ) {
		sindex --;
		memcpy(ValueStringPointer(GetItemLongName(mcp,"dc.window")),
			   ValueStringPointer(GetArrayItem(mcp_swindow,sindex - 1)),
			   SIZE_NAME);
		ValueIsNonNil(GetItemLongName(mcp,"dc.window"));
		PutType = SCREEN_CHANGE_WINDOW;
	}

	if		(  strcmp(ValueStringPointer(
						  GetArrayItem(mcp_swindow,sindex - 1)),
					  ValueStringPointer(mcp_dcwindow))  !=  0  ) {
		strcpy(ValueStringPointer(GetItemLongName(mcp,"dc.fromwin")),
			   ValueStringPointer(
				   GetArrayItem(mcp_swindow,sindex - 1)));
		ValueIsNonNil(GetItemLongName(mcp,"dc.fromwin"));
		for	( i = 0 ; i < sindex ; i ++ ) {
			if		(  strcmp(ValueStringPointer(
								  GetArrayItem(mcp_swindow,i)),
							  ValueStringPointer(mcp_dcwindow))  ==  0  )
				break;
		}
		if		(  i  <  sindex  ) {
			winfrom = i + 1;
		} else {
			winfrom = 0;
			strcpy(ValueStringPointer(GetArrayItem(mcp_swindow,i)),
				   ValueStringPointer(mcp_dcwindow));
			ValueIsNonNil(GetArrayItem(mcp_swindow,i));
		}
		winend  = sindex;
		sindex = i + 1;
	}

	switch	(PutType) {
	  case	SCREEN_JOIN_WINDOW:
		for	( i = winfrom ; i < winend ; i ++  ) {
			SetPutType(node,
					   ValueStringPointer(GetArrayItem(mcp_swindow,i)),
					   SCREEN_CLOSE_WINDOW);
		}
#ifdef	NEW_SEQUENCE
		SetPutType(node,ValueStringPointer(mcp_dcwindow),SCREEN_CURRENT_WINDOW);
#endif
		break;
	  default:
#ifdef	NEW_SEQUENCE
		SetPutType(node,ValueStringPointer(mcp_dcwindow),PutType);
#endif
		break;
	}
	SetValueInteger(mcp_pputtype,(int)PutType);
	SetValueInteger(mcp_sindex,sindex);
#ifdef	DEBUG
	dbgprintf("mcp_sindex = %d",sindex);
	dbgprintf("mcp->dc.window = [%s]",ValueStringPointer(mcp_dcwindow));
	dbgmsg("**** window stack dump *****************");
	for	( i = 0 ; i < sindex ; i ++ ) {
		dbgprintf("[%d:%s]",i,(ValueStringPointer(
								   GetArrayItem(mcp_swindow,i))));
	}
	dbgmsg("----------------------------------------");
#endif
dbgmsg("<CallAfter");
}

extern	void
ExecuteProcess(
	ProcessNode	*node)
{
	WindowBind	*bind;
	MessageHandler	*handler;
	char		*window;

dbgmsg(">ExecuteProcess");
	window = ValueToString(GetItemLongName(node->mcprec->value,"dc.window"),NULL);
	bind = (WindowBind *)g_hash_table_lookup(ThisLD->whash,window);
	handler = bind->handler;
	if		(  ((MessageHandlerClass *)bind->handler)->ExecuteDC  !=  NULL  ) {
		CallBefore(node);
		if		(  !(handler->klass->ExecuteDC(handler,node))  ) {
			MessageLog("application process illegular execution");
			exit(2);
		}
		CallAfter(node);
	}
dbgmsg("<ExecuteProcess");
}

static	void
StopHandlerDC(
	MessageHandler	*handler)
{
	if		(	(  ( handler->fInit & INIT_READYDC )  !=  0  )
			&&	(  ( handler->fInit & INIT_STOPDC  )  ==  0  ) ) {
		handler->fInit |= INIT_STOPDC;
		if		(  handler->klass->StopDC  !=  NULL  ) {
			handler->klass->StopDC(handler);
		}
	}
}

static	void
_StopDC(
	char		*name,
	WindowBind	*bind,
	void		*dummy)
{
	StopHandlerDC(bind->handler);
}

extern	void
StopDC(void)
{
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_StopDC,NULL);
}

static	void
CleanUpHandlerDC(
	MessageHandler	*handler)
{
	if		(	(  ( handler->fInit & INIT_READYDC  )  !=  0  )
			&&	(  ( handler->fInit & INIT_CLEANDC  )  ==  0  ) ) {
		handler->fInit |= INIT_CLEANDC;
		if		(  handler->klass->CleanUpDC  !=  NULL  ) {
			handler->klass->CleanUpDC(handler);
		}
	}
}

static	void
_CleanUpOnlineDC(
	char		*name,
	WindowBind	*bind,
	void		*dummy)
{
	CleanUpHandlerDC(bind->handler);
}

extern	void
CleanUpOnlineDC(void)
{
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_CleanUpOnlineDC,NULL);
}

extern	void
CleanUpHandlerDB(
	MessageHandler	*handler)
{
	if		(	(  ( handler->fInit & INIT_READYDB  )  !=  0  )
			&&	(  ( handler->fInit & INIT_CLEANDB )  ==  0  ) ) {
		handler->fInit |= INIT_CLEANDB;
		if		(  handler->klass->CleanUpDB  !=  NULL  ) {
			handler->klass->CleanUpDB(handler);
		}
	}
}

static	void
_CleanUpOnlineDB(
	char		*name,
	WindowBind	*bind,
	void		*dummy)
{
	CleanUpHandlerDB(bind->handler);
}

extern	void
CleanUpOnlineDB(void)
{
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_CleanUpOnlineDB,NULL);
}

extern	void
StopHandlerDB(
	MessageHandler	*handler)
{
	if		(	(  ( handler->fInit & INIT_READYDB )  !=  0  )
			&&	(  ( handler->fInit & INIT_STOPDB  )  ==  0  ) ) {
		handler->fInit |= INIT_STOPDB;
		if		(  handler->klass->StopDB  !=  NULL  ) {
			handler->klass->StopDB(handler);
		}
	}
}

static	void
_StopOnlineDB(
	char		*name,
	WindowBind	*bind,
	void		*dummy)
{
	StopHandlerDB(bind->handler);
}

extern	void
StopOnlineDB(void)
{
	CloseDB(NULL);
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_StopOnlineDB,NULL);
}

static	void
CleanUpDC(
	MessageHandler	*handler)
{
	if		(	(  ( handler->fInit & INIT_READYDC  )  !=  0  )
			&&	(  ( handler->fInit & INIT_CLEANDC )  ==  0  ) ) {
		handler->fInit |= INIT_CLEANDC;
		if		(  handler->klass->CleanUpDC  !=  NULL  ) {
			handler->klass->CleanUpDC(handler);
		}
	}
}

extern	int
StartBatch(
	char	*name,
	char	*para)
{
	MessageHandler	*handler;
	BatchBind		*bind;
	int		rc;

dbgmsg(">StartBatch");
	if		(  ( bind = g_hash_table_lookup(ThisBD->BatchTable,name) )  ==  NULL  ) {
		fprintf(stderr,"%s application is not in BD.\n",name);
		exit(1);
	}
	CurrentProcess = NULL;
	handler = bind->handler;
	if		(  handler->klass->ReadyExecute  !=  NULL  ) {
		handler->klass->ReadyExecute(handler);
	}
	if		(  handler->klass->ExecuteBatch  !=  NULL  ) {
		rc = handler->klass->ExecuteBatch(handler,name,para);
	} else {
		rc = -1;
		fprintf(stderr,"%s is handler not support batch.\n",name);
	}
dbgmsg("<StartBatch");
	return	(rc);
}

/*
 *	handler misc functions
 */
extern	void
MakeCTRL(
	DBCOMM_CTRL	*ctrl,
	ValueStruct	*mcp)
{
	strcpy(ctrl->func,ValueStringPointer(GetItemLongName(mcp,"func")));
	ctrl->rc = ValueInteger(GetItemLongName(mcp,"rc"));
	ctrl->blocks = ValueInteger(GetItemLongName(mcp,"db.path.blocks"));
	ctrl->rno = ValueInteger(GetItemLongName(mcp,"db.path.rname"));
	ctrl->pno = ValueInteger(GetItemLongName(mcp,"db.path.pname"));
#ifdef	DEBUG
	DumpDB_Node(ctrl);
#endif
}

extern	void
MakeMCP(
	ValueStruct	*mcp,
	DBCOMM_CTRL	*ctrl)
{
	strcpy(ValueStringPointer(GetItemLongName(mcp,"func")),ctrl->func);
	ValueInteger(GetItemLongName(mcp,"rc")) = ctrl->rc;
	ValueInteger(GetItemLongName(mcp,"db.path.blocks")) = ctrl->blocks;
	ValueInteger(GetItemLongName(mcp,"db.path.rname")) = ctrl->rno;
	ValueInteger(GetItemLongName(mcp,"db.path.pname")) = ctrl->pno;
}

extern	void
DumpDB_Node(
	DBCOMM_CTRL	*ctrl)
{
	printf("func   = [%s]\n",ctrl->func);
	printf("blocks = %d\n",ctrl->blocks);
	printf("rno    = %d\n",ctrl->rno);
	printf("pno    = %d\n",ctrl->pno);
}

extern	RecordStruct	*
BuildDBCTRL(void)
{
	RecordStruct	*rec;
	char			name[SIZE_LONGNAME+1];
	FILE			*fp;

	sprintf(name,"/tmp/dbctrl%d.rec",(int)getpid());
	if		(  ( fp = fopen(name,"w") )  ==  NULL  ) {
		fprintf(stderr,"tempfile can not make.\n");
		exit(1);
	}
	fprintf(fp,	"dbctrl	{");
	fprintf(fp,		"rc int;");
	fprintf(fp,		"func	varchar(%d);",SIZE_FUNC);
	fprintf(fp,		"rname	varchar(%d);",SIZE_NAME);
	fprintf(fp,		"pname	varchar(%d);",SIZE_NAME);
	fprintf(fp,	"};");
	fclose(fp);

	rec = ParseRecordFile(name);
	remove(name);

	return	(rec);
}

extern	void
ExpandStart(
	char	*line,
	char	*start,
	char	*path,
	char	*module,
	char	*param)
{
	char	*p
	,		*q;

	p = start;
	q = line;

	while	(  *p  !=  0  ) {
		if		(  *p  ==  '%'  ) {
			p ++;
			switch	(*p) {
			  case	'm':
				q += sprintf(q,"%s",module);
				break;
			  case	'p':
				q += sprintf(q,"%s",path);
				break;
			  case	'a':
				q += sprintf(q,"%s",param);
				break;
			  default:
				*q ++ = *p;
				break;
			}
		} else {
			*q ++ = *p;
		}
		p ++;
	}
	*q = 0;
}

extern	void
ReadyExecuteCommon(
	MessageHandler	*handler)
{
}
