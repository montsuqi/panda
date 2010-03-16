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

/*
#define	NEW_SEQUENCT
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<glib.h>
#include	<unistd.h>
#include	<sys/time.h>

#include	"defaults.h"
#include	"const.h"
#include	"libmondai.h"
#include	"struct.h"
#include	"DDparser.h"
#include	"enum.h"
#include	"load.h"
#define		_HANDLER
#include	"dbgroup.h"
#include	"driver.h"
#include	"handler.h"
#include	"aps_main.h"
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

static	char	*DBSTATUS[8] = {
	"NORE",
	"CONN",
	"WAIT",
	"FAIL",
	"DISC",
	"RERR",
	"REDL",
	"SYNC"
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
	char			filename[SIZE_LONGNAME+1];

ENTER_FUNC;
	if		(  ( klass = g_hash_table_lookup(HandlerClassTable,name) )  ==  NULL  ) {
		dbgprintf("%s handlerClass invoke.", name);
		sprintf(filename,"%s." SO_SUFFIX,name);
		klass = NULL;
		if		(  ( dlhandle = LoadFile(APS_HandlerLoadPath,filename) )  !=  NULL  ) {
			if		(  ( finit = (void *)dlsym(dlhandle,name) )
					   ==  NULL  )	{
				Warning("[%s] is invalid.",name);
			} else {
				klass = (*finit)();
				if		(  g_hash_table_lookup(HandlerClassTable,name)  ==  NULL  ) {
					g_hash_table_insert(HandlerClassTable,StrDup(name),klass);
				}
			}
		} else {
			Error("[%s] not found.",name);
		}
	}
LEAVE_FUNC;
	return	(klass);
}

static	void
InitHandler(void)
{
ENTER_FUNC;
	HandlerClassTable = NewNameHash();
LEAVE_FUNC;
}

static	void
_InitiateHandler(
	MessageHandler		*handler)
{
	MessageHandlerClass	*klass;

	if		(  ( handler->fInit & INIT_LOAD )  ==  0  ) {
		handler->fInit |= INIT_LOAD;
		if		(  ( klass = EnterHandlerClass((char *)handler->klass) )  ==  NULL  ) {
			Message("[%s] is invalid handler class.",(char *)handler->klass);
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
ENTER_FUNC;
	if		(  ( APS_HandlerLoadPath = getenv("APS_HANDLER_LOAD_PATH") )
			   ==  NULL  ) {
		if		(  ThisLD->handlerpath  !=  NULL  ) {
			APS_HandlerLoadPath = ThisLD->handlerpath;
		} else {
			APS_HandlerLoadPath = MONTSUQI_LIBRARY_PATH;
		}
	}
	InitHandler();
	dbgprintf("LD = [%s]",ThisLD->name);
	g_hash_table_foreach(ThisLD->bhash,(GHFunc)_OnlineInit,NULL);

	TypeHash = NewNameiHash();
	g_hash_table_insert(TypeHash,"CURRENT",(gpointer)SCREEN_CURRENT_WINDOW);
	g_hash_table_insert(TypeHash,"NEW",(gpointer)SCREEN_NEW_WINDOW);
	g_hash_table_insert(TypeHash,"CLOSE",(gpointer)SCREEN_CLOSE_WINDOW);
	g_hash_table_insert(TypeHash,"CHANGE",(gpointer)SCREEN_CHANGE_WINDOW);
	g_hash_table_insert(TypeHash,"JOIN",(gpointer)SCREEN_JOIN_WINDOW);
	g_hash_table_insert(TypeHash,"FORK",(gpointer)SCREEN_FORK_WINDOW);
	g_hash_table_insert(TypeHash,"EXIT",(gpointer)SCREEN_END_SESSION);
	g_hash_table_insert(TypeHash,"BACK",(gpointer)SCREEN_BACK_WINDOW);

	g_hash_table_insert(TypeHash,"CALL",(gpointer)SCREEN_CALL_COMPONENT);
	g_hash_table_insert(TypeHash,"RETURN",(gpointer)SCREEN_RETURN_COMPONENT);
LEAVE_FUNC;
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
	if		(  ( APS_HandlerLoadPath = getenv("APS_HANDLER_LOAD_PATH") )
			   ==  NULL  ) {
		if		(  ThisBD->handlerpath  !=  NULL  ) {
			APS_HandlerLoadPath = ThisBD->handlerpath;
		} else {
			APS_HandlerLoadPath = MONTSUQI_LIBRARY_PATH;
		}
	}
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
			handler->klass->ReadyExecute(handler,ThisLD->loadpath);
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
	g_hash_table_foreach(ThisLD->bhash,(GHFunc)_ReadyDC,NULL);
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

extern	int
ReadyOnlineDB(
	NETFILE	*fp)
{
	int	rc;
ENTER_FUNC;
	InitDB_Process(fp);
	rc = OpenDB(NULL);
	g_hash_table_foreach(ThisLD->bhash,(GHFunc)_ReadyOnlineDB,NULL);
LEAVE_FUNC;
	return	rc;
}

static	void
CallBefore(
	WindowBind		*bind,
	ProcessNode		*node)
{
	ValueStruct	*mcp;

ENTER_FUNC;
	mcp = node->mcprec->value; 
	SetValueString(GetItemLongName(mcp,"dc.status"),
				   STATUS[(*ValueStringPointer(GetItemLongName(mcp,"private.pstatus")) - '1')%sizeof(STATUS)],
				   NULL);
	dbgprintf("node->dbstatus = %d",node->dbstatus);
	SetValueString(GetItemLongName(mcp,"dc.dbstatus"),
				   DBSTATUS[(int)node->dbstatus],NULL);
	SetValueInteger(GetItemLongName(mcp,"db.rcount"),0);
	SetValueInteger(GetItemLongName(mcp,"db.limit"),1);
	node->w.n = 0;
	node->thisscrrec = bind->rec;
	CurrentProcess = node;
LEAVE_FUNC;
}

static	void
SetPutType(
	ProcessNode	*node,
	char		*wname,
	unsigned char		type)
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
		,	sindex;	/*	sindex orgine is 1	*/
	char	buff[SIZE_LONGNAME+1]
		,	*p;
	int		PutType;

	ValueStruct	*mcp_swindow;
	ValueStruct	*mcp_sindex;
	ValueStruct	*mcp_puttype;
	ValueStruct	*mcp_pputtype;
	ValueStruct	*mcp_dcwindow;
	ValueStruct	*mcp;

ENTER_FUNC;
	mcp = node->mcprec->value; 
	mcp_sindex 	= GetItemLongName(mcp,"private.count");
	mcp_swindow = GetItemLongName(mcp,"private.swindow");
	mcp_puttype = GetItemLongName(mcp,"dc.puttype");
	mcp_pputtype = GetItemLongName(mcp,"private.pputtype");
	mcp_dcwindow = GetItemLongName(mcp,"dc.window");

	winfrom = 0;
	winend = 0;
	if		(  ( PutType = (int)(long)g_hash_table_lookup(TypeHash,ValueToString(mcp_puttype,NULL)) )
			   ==  0  ) {
		PutType = SCREEN_CURRENT_WINDOW;
	}
	if		(  ( sindex = ValueInteger(mcp_sindex) )  ==  0  ) {
		SetValueString(GetArrayItem(mcp_swindow,0),ValueStringPointer(mcp_dcwindow),NULL);
		sindex = 1;
	}

	if		(  sindex  >  1  ) {
		SetValueString(GetItemLongName(mcp,"dc.fromwin"),
					   ValueStringPointer(GetArrayItem(mcp_swindow,sindex - 2)),NULL);
	} else {
		SetValueString(GetItemLongName(mcp,"dc.fromwin"),"",NULL);
	}

	switch	(PutType) {
	  case	SCREEN_BACK_WINDOW:
		dbgmsg("back");
		sindex --;
		SetValueString(GetItemLongName(mcp,"dc.window"),
					   ValueStringPointer(GetArrayItem(mcp_swindow,sindex - 1)),NULL);
		PutType = SCREEN_CHANGE_WINDOW;
		SetValueString(GetItemLongName(mcp,"dc.puttype"),"CHANGE",NULL);
		break;
	  case	SCREEN_RETURN_COMPONENT:
		dbgmsg("return");
		sindex --;
		strcpy(buff,ValueStringPointer(GetItemLongName(mcp,"dc.window")));
		if		(  ( p = strrchr(buff,'.') )  !=  NULL  ) {
			*p = 0;
		}
		SetValueString(GetItemLongName(mcp,"dc.window"),buff,NULL);
		break;
	  default:
		dbgmsg("other");
		if		(  strcmp(ValueStringPointer(
							  GetArrayItem(mcp_swindow,sindex - 1)),
						  ValueStringPointer(mcp_dcwindow))  !=  0  ) {
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
				SetValueString(GetArrayItem(mcp_swindow,i),
							   ValueStringPointer(mcp_dcwindow),NULL);
			}
			winend  = sindex;
			sindex = i + 1;
		}
		break;
	}

	switch	(PutType) {
	  case	SCREEN_JOIN_WINDOW:
		for	( i = winfrom ; i < winend ; i ++  ) {
			SetPutType(node,
					   ValueStringPointer(GetArrayItem(mcp_swindow,i)),
					   SCREEN_CLOSE_WINDOW);
		}
		break;
	  default:
		break;
	}
	dbgprintf("PutType = %d",PutType);
	SetValueInteger(mcp_pputtype,PutType);
	if		(  strcmp(ValueStringPointer(mcp_dcwindow), node->window)  !=  0  )	{
		SetValueInteger(mcp_sindex,sindex);
	}
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
LEAVE_FUNC;
}

#ifdef	DEBUG
static	void
DumpProcessNode(
	ProcessNode	*node)
{
	int		i;
	int		sindex;
	ValueStruct	*mcp;
	ValueStruct	*mcp_dcwindow;
	ValueStruct	*mcp_swindow;
	ValueStruct	*mcp_sindex;

	mcp = node->mcprec->value; 
	mcp_dcwindow = GetItemLongName(mcp,"dc.window");
	mcp_swindow = GetItemLongName(mcp,"private.swindow");
	mcp_sindex 	= GetItemLongName(mcp,"private.count");
	sindex = ValueInteger(mcp_sindex);

	printf("mcp_sindex = %d\n",sindex);
	printf("mcp->dc.window = [%s]\n",ValueStringPointer(mcp_dcwindow));
	printf("**** window stack dump *****************\n");
	for	( i = 0 ; i < sindex ; i ++ ) {
		printf("[%d:%s]\n",i,(ValueStringPointer(
								   GetArrayItem(mcp_swindow,i))));
	}
#if	0
	printf("term   = [%s]\n",node->term);
	printf("user   = [%s]\n",node->user);
	printf("window = [%s]\n",node->window);
	printf("widget = [%s]\n",node->widget);
	printf("event  = [%s]\n",node->event);
	printf("*** mcprec ***\n");
	DumpValueStruct(node->mcprec->value);
	printf("*** sparec ***\n");
	DumpValueStruct(node->sparec->value);
	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		printf(" *** [%s] ***\n",node->scrrec[i]->name);
		DumpValueStruct(node->scrrec[i]->value);
	}
#endif
}
#endif

extern	void
ExecuteProcess(
	ProcessNode	*node)
{
	WindowBind	*bind;
	MessageHandler	*handler;
	char		compo[SIZE_LONGNAME+1];
	struct	timeval	tv;
	long	ever
		,	now;

ENTER_FUNC;
	ever = 0;
	PureComponentName(ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.window")),
					  compo);
	dbgprintf("component [%s]",compo);
	if		(  ( bind = (WindowBind *)g_hash_table_lookup(ThisLD->bhash,compo) )
			   !=  NULL  ) {
		dbgprintf("calling [%s] widget [%s] event [%s]",bind->module, node->widget, node->event);
		handler = bind->handler;
		if		(  ((MessageHandlerClass *)bind->handler)->ExecuteDC  !=  NULL  ) {
			CallBefore(bind,node);
			if		(  fTimer  ) {
				gettimeofday(&tv,NULL);
				ever = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
			}
			if		(  !(handler->klass->ExecuteDC(handler,node))  ) {
				MessageLog("application process illegular execution");
				exit(2);
			}
			if		(  fTimer  ) {
				gettimeofday(&tv,NULL);
				now = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
				printf("aps %s@%s:%s process time %6ld(ms)\n",
					   node->user,node->term,compo,(now - ever));
				dbgprintf("aps %s@%s:%s process time %6ld(ms)\n",
						  node->user,node->term,compo,(now - ever));
			}
			CallAfter(node);
		}
	} else {
		Error("invalid event request [%s]",compo);
	}
LEAVE_FUNC;
}

static	void
StopHandlerDC(
	MessageHandler	*handler)
{
ENTER_FUNC;
	if		(	(  ( handler->fInit & INIT_READYDC )  !=  0  )
			&&	(  ( handler->fInit & INIT_STOPDC  )  ==  0  ) ) {
		handler->fInit |= INIT_STOPDC;
		if		(  handler->klass->StopDC  !=  NULL  ) {
			handler->klass->StopDC(handler);
		}
	}
LEAVE_FUNC;
}

static	void
_StopDC(
	char		*name,
	WindowBind	*bind,
	void		*dummy)
{
ENTER_FUNC;
	dbgprintf("name = [%s]",name);
	StopHandlerDC(bind->handler);
LEAVE_FUNC;
}

extern	void
StopDC(void)
{
ENTER_FUNC;
	g_hash_table_foreach(ThisLD->bhash,(GHFunc)_StopDC,NULL);
LEAVE_FUNC;
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
	g_hash_table_foreach(ThisLD->bhash,(GHFunc)_CleanUpOnlineDC,NULL);
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
	g_hash_table_foreach(ThisLD->bhash,(GHFunc)_CleanUpOnlineDB,NULL);
}

extern	void
StopHandlerDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
	if		(	(  ( handler->fInit & INIT_READYDB )  !=  0  )
			&&	(  ( handler->fInit & INIT_STOPDB  )  ==  0  ) ) {
		handler->fInit |= INIT_STOPDB;
		if		(  handler->klass->StopDB  !=  NULL  ) {
			handler->klass->StopDB(handler);
		}
	}
LEAVE_FUNC;
}

static	void
_StopOnlineDB(
	char		*name,
	WindowBind	*bind,
	void		*dummy)
{
ENTER_FUNC;
	StopHandlerDB(bind->handler);
LEAVE_FUNC;
}

extern	void
StopOnlineDB(void)
{
ENTER_FUNC;
	CloseDB(NULL);
	g_hash_table_foreach(ThisLD->bhash,(GHFunc)_StopOnlineDB,NULL);
LEAVE_FUNC;
}

#if	0
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
#endif

extern	int
StartBatch(
	char	*name,
	char	*para)
{
	MessageHandler	*handler;
	BatchBind		*bind;
	int		rc;

ENTER_FUNC;
	dbgprintf("calling [%s]",name);
	if		(  ( bind = g_hash_table_lookup(ThisBD->BatchTable,name) )  ==  NULL  ) {
		Error("%s application is not in BD.\n",name);
	}
	CurrentProcess = NULL;
	handler = bind->handler;
	if		(  handler->klass->ReadyExecute  !=  NULL  ) {
		handler->klass->ReadyExecute(handler,ThisBD->loadpath);
	}
	if		(  handler->klass->ExecuteBatch  !=  NULL  ) {
		rc = handler->klass->ExecuteBatch(handler,name,para);
	} else {
		rc = -1;
		Warning("%s is handler not support batch.\n",name);
	}
LEAVE_FUNC;
	return	(rc);
}

/*
 *	handler misc functions
 */
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
