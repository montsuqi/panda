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
#include	"defaults.h"
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<glib.h>

#include	"defaults.h"
#include	"types.h"
#include	"misc.h"
#include	"const.h"
#include	"libmondai.h"
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

static	GHashTable	*HandlerTable;
static	MessageHandler	*
EnterHandler(
	char	*name)
{
	void			*dlhandle;
	MessageHandler	*handler;
	MessageHandler	*(*finit)(void);
	char			filename[SIZE_BUFF];

dbgmsg(">EnterHandler");
	if		(  ( handler = g_hash_table_lookup(HandlerTable,name) )  ==  NULL  ) {
		MessagePrintf("%s handler invoke.", name);
		sprintf(filename,"%s.so",name);
		handler = NULL;
		if		(  ( dlhandle = LoadFile(APS_HandlerLoadPath,filename) )  !=  NULL  ) {
			if		(  ( finit = (void *)dlsym(dlhandle,name) )
					   ==  NULL  )	{
				MessagePrintf("[%s] is invalid.",name);
			} else {
				handler = (*finit)();
				if		(  g_hash_table_lookup(HandlerTable,name)  ==  NULL  ) {
					g_hash_table_insert(HandlerTable,name,handler);
				}
			}
		} else {
			fprintf(stderr,"[%s] not found.\n",name);
		}
	}
dbgmsg("<EnterHandler");
	return	(handler); 
}

static	void
InitHandler(void)
{
dbgmsg(">InitHandler");

	if		(  ( APS_HandlerLoadPath = getenv("APS_HANDLER_LOAD_PATH") )
			   ==  NULL  ) {
		APS_HandlerLoadPath = MONTSUQI_LIBRARY_PATH;
	}
	HandlerTable = NewNameHash();
}

static	void
_InitiateHandler(
	char	*name,
	WindowBind	*bind,
	void		*dummy)
{
	MessageHandler	*handler;

	handler = g_hash_table_lookup(HandlerTable,
								  (char *)bind->handler);
	if		(  handler  ==  NULL  ) {
		if		(  ( handler = EnterHandler((char *)bind->handler) )
				   ==  NULL  ) {
			MessagePrintf("[%s] is invalid handler.",
						  (char *)bind->handler);
		} else {
			handler->fUse = TRUE;
			bind->handler = handler;
		}
	} else {
		handler->fUse = TRUE;
		bind->handler = handler;
	}
}

extern	void
InitiateHandler(void)
{
dbgmsg(">InitiateHandler");
	InitHandler();
	dbgprintf("LD = [%s]",ThisLD->name);
	g_hash_table_foreach(ThisLD->whash,(GHFunc)_InitiateHandler,NULL);
dbgmsg("<InitiateHandler");
}

static	void
_Use(
	char	*name,
	BatchBind	*bind,
	void	*dummy)
{
	MessageHandler	*handler;

	handler = EnterHandler(bind->handler);
	handler->fUse = TRUE;
}

extern	void
InitiateBatchHandler(void)
{
dbgmsg(">InitiateBatchHandler");
	InitHandler();
	g_hash_table_foreach(ThisBD->BatchTable,(GHFunc)_Use,NULL);
dbgmsg("<InitiateBatchHandler");
}

static	void
_ReadyDC(
	char	*name,
	MessageHandler	*handler,
	void	*dummy)
{
	if		(  handler->fUse  ) {
		if		(  handler->ReadyDC  !=  NULL  ) {
			handler->ReadyDC();
		}
	}
}

extern	void
ReadyDC(void)
{
	g_hash_table_foreach(HandlerTable,(GHFunc)_ReadyDC,NULL);
}

static	void
_ReadyDB(
	char	*name,
	MessageHandler	*handler,
	void	*dummy)
{
	if		(  handler->fUse  ) {
		if		(  handler->ReadyDB  !=  NULL  ) {
			handler->ReadyDB();
		}
	}
}

extern	void
ReadyDB(void)
{
dbgmsg(">ReadyDB");
	InitDB_Process();
	g_hash_table_foreach(HandlerTable,(GHFunc)_ReadyDB,NULL);
dbgmsg("<ReadyDB");
}

static	void
CallBefore(
	ProcessNode	*node)
{
	ValueStruct	*mcp;

dbgmsg(">CallBefore");
	mcp = node->mcprec->value; 
	memcpy(ValueString(GetItemLongName(mcp,"dc.status")),
		   STATUS[*ValueString(GetItemLongName(mcp,"private.pstatus")) - '1'],
		   SIZE_STATUS);
dbgmsg("<CallBefore");
}
static	void
CallAfter(
	ProcessNode	*node)
{
	int		i
	,		winfrom
	,		winend;

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
	if		(	(  *ValueString(mcp_puttype)          ==  0  )
			||	(  !strcmp(ValueString(mcp_puttype),"NULL")  ) ) {
		*ValueString(mcp_pputtype) = SCREEN_NULL + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"CURRENT")  ) {
		*ValueString(mcp_pputtype) = SCREEN_CURRENT_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"NEW")  ) {
		*ValueString(mcp_pputtype) = SCREEN_NEW_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"CLOSE")  ) {
		*ValueString(mcp_pputtype) = SCREEN_CLOSE_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"CHANGE")  ) {
		*ValueString(mcp_pputtype) = SCREEN_CHANGE_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"BACK")  ) {
		SetValueInteger(mcp_sindex,ValueInteger(mcp_sindex)-1);
		memcpy(ValueString(GetItemLongName(mcp,"dc.window")),
			   ValueString(GetArrayItem(mcp_swindow,ValueInteger(mcp_sindex))),
			   SIZE_NAME);
		*ValueString(mcp_pputtype) = SCREEN_CHANGE_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"JOIN")  ) {
		*ValueString(mcp_pputtype) = SCREEN_JOIN_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"FORK")  ) {
		*ValueString(mcp_pputtype) = SCREEN_FORK_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"EXIT")  ) {
		*ValueString(mcp_pputtype) = SCREEN_END_SESSION + '0';
	} else {
		*ValueString(mcp_pputtype) = SCREEN_CURRENT_WINDOW + '0';
	}
	node->w.n = 0;
	if		(  ValueInteger(mcp_sindex)  ==  0  ) {
		strcpy(ValueString(GetArrayItem(mcp_swindow,0)),
			   ValueString(mcp_dcwindow));
		ValueInteger(mcp_sindex) = 1;
	} else {
#ifdef	DEBUG
		dbgprintf("mcp_sindex = %d",ValueInteger(mcp_sindex));
		dbgprintf("mcp->dc.window = [%s]",ValueString(mcp_dcwindow));
		dbgmsg("**** window stack dump *****************");
		for	( i = 0 ; i < ValueInteger(mcp_sindex) ; i ++ ) {
			dbgprintf("[%d:%s]",i,(ValueString(
									  GetArrayItem(mcp_swindow,i))));
		}
		dbgmsg("----------------------------------------");
#endif
		if		(  strcmp(ValueString(
							  GetArrayItem(mcp_swindow,ValueInteger(mcp_sindex) - 1)),
						  ValueString(mcp_dcwindow))  !=  0  ) {
			strcpy(ValueString(GetItemLongName(mcp,"dc.fromwin")),
				   ValueString(
					   GetArrayItem(mcp_swindow,ValueInteger(mcp_sindex) - 1)));
			for	( i = 0 ; i < ValueInteger(mcp_sindex) ; i ++ ) {
				if		(  strcmp(ValueString(
									  GetArrayItem(mcp_swindow,i)),
								  ValueString(mcp_dcwindow))  ==  0  )
					break;
			}
			if		(  strcmp(ValueString(
								  GetArrayItem(mcp_swindow,i)),
							  ValueString(mcp_dcwindow))  ==  0  ) {
				winfrom = i + 1;
				winend  = ValueInteger(mcp_sindex);
				ValueInteger(mcp_sindex) = i + 1;
			} else {
				winfrom = 0;
				winend  = ValueInteger(mcp_sindex);
				ValueInteger(mcp_sindex) = i + 1;
				strcpy(ValueString(
						   GetArrayItem(mcp_swindow,i)),
					   ValueString(mcp_dcwindow));
			}
			if		(  *ValueString(mcp_pputtype)  ==  SCREEN_JOIN_WINDOW + '0'  ) {
				for	( i = winfrom ; i < winend ; i ++  ) {
					strcpy(node->w.close[node->w.n].window,
						   ValueString(GetArrayItem(mcp_swindow,i)));
					node->w.n ++;
				}
			}
		}
	}
dbgmsg("<CallAfter");
}

extern	void
ExecuteProcess(
	ProcessNode	*node)
{
	WindowBind	*bind;
	char		*window;

dbgmsg(">ExecuteProcess");
	window = ValueString(GetItemLongName(node->mcprec->value,"dc.window"));
	bind = (WindowBind *)g_hash_table_lookup(ThisLD->whash,window);
	if		(  ((MessageHandler *)bind->handler)->ExecuteProcess  !=  NULL  ) {
		CallBefore(node);
		((MessageHandler *)bind->handler)->ExecuteProcess(node);
		CallAfter(node);
	}
dbgmsg("<ExecuteProcess");
}

static	void
_StopDC(
	char	*name,
	MessageHandler	*handler,
	void	*dummy)
{
	if		(  handler->fUse  ) {
		if		(  handler->StopDC  !=  NULL  ) {
			handler->StopDC();
		}
	}
}

extern	void
StopDC(void)
{
	g_hash_table_foreach(HandlerTable,(GHFunc)_StopDC,NULL);
}

static	void
_CleanUp(
	char	*name,
	MessageHandler	*handler,
	void	*dummy)
{
	if		(  handler->fUse  ) {
		if		(  handler->CleanUpDC  !=  NULL  ) {
			handler->CleanUpDC();
		}
		if		(  ThisLD->cDB  >  0  ) {
			if		(  handler->CleanUpDB  !=  NULL  ) {
				handler->CleanUpDB();
			}
		}
	}
}

extern	void
CleanUp(void)
{
	g_hash_table_foreach(HandlerTable,(GHFunc)_CleanUp,NULL);
}

static	void
_CleanUpDB(
	char	*name,
	MessageHandler	*handler,
	void	*dummy)
{
	if		(  handler->fUse  ) {
		if		(  handler->CleanUpDB  !=  NULL  ) {
			handler->CleanUpDB();
		}
	}
}

extern	void
CleanUpDB(void)
{
	g_hash_table_foreach(HandlerTable,(GHFunc)_CleanUpDB,NULL);
}

static	void
_StopDB(
	char	*name,
	MessageHandler	*handler,
	void	*dummy)
{
	if		(  handler->fUse  ) {
		if		(  handler->StopDB  !=  NULL  ) {
			handler->StopDB();
		}
	}
}

extern	void
StopDB(void)
{
	g_hash_table_foreach(HandlerTable,(GHFunc)_StopDB,NULL);
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
	if		(  ( handler = g_hash_table_lookup(HandlerTable,bind->handler) )  !=  NULL  ) {
		if		(  handler->StartBatch  !=  NULL  ) {
			rc = handler->StartBatch(name,para);
		} else {
			rc = -1;
			fprintf(stderr,"%s is handler not support batch.\n",name);
		}
	} else {
		fprintf(stderr,"%s application is not in BD.\n",name);
		exit(1);
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
	strcpy(ctrl->func,ValueString(GetItemLongName(mcp,"func")));
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
	strcpy(ValueString(GetItemLongName(mcp,"func")),ctrl->func);
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
