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
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"
#include	"const.h"
#include	"value.h"
#include	"enum.h"
#include	"aps_main.h"
#include	"apsio.h"
#include	"dbgroup.h"
#define		_HANDLER
#include	"handler.h"
#include	"glterm.h"
#include	"BDparser.h"
#include	"debug.h"

#ifdef	HAVE_POSTGRES
#include	"Postgres.h"
#endif
#ifdef	USE_SHELL
#include	"Shell.h"
#endif

#ifdef	HAVE_DOTCOBOL
#include	"dotCOBOL.h"
#endif
#ifdef	HAVE_OPENCOBOL
#include	"OpenCOBOL.h"
#endif

static	char	*STATUS[4] = {
	"LINK",
	"PUTG",
	"PUTG",
	"RSND"
};

static	GHashTable	*HandlerTable;

extern	void
InitHandler(void)
{
	HandlerTable = NewNameHash();
}

extern	void
EnterHandler(
	MessageHandler	*handler)
{
	printf("%s handler invoke\n", handler->name);
	if		(  g_hash_table_lookup(HandlerTable,handler->name)  ==  NULL  ) {
		g_hash_table_insert(HandlerTable,handler->name,handler);
	}
}

static	void
_Use(
	char	*name,
	MessageHandler	*handler,
	void	*dummy)
{
	handler->fUse = TRUE;
}

extern	void
InitiateHandler(void)
{
	int		i;
	MessageHandler	*handler;

dbgmsg(">InitiateHandler");
	InitHandler();
#ifdef	HAVE_DOTCOBOL
	Init_dotCOBOL_Handler();
#endif
#ifdef	HAVE_OPENCOBOL
	Init_OpenCOBOL_Handler();
#endif
	if		(  ThisLD  !=  NULL  ) {
#ifdef	DEBUG
		printf("LD = [%s]\n",ThisLD->name);
#endif
		for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
			handler = g_hash_table_lookup(HandlerTable,
										  (char *)ThisLD->window[i]->handler);
			if		(  handler  ==  NULL  ) {
				printf("[%s] is invalid handler.\n",
					   (char *)ThisLD->window[i]->handler);
			} else {
				handler->fUse = TRUE;
				ThisLD->window[i]->handler = handler;
			}
		}
	} else {
		g_hash_table_foreach(HandlerTable,(GHFunc)_Use,NULL);
	}
dbgmsg("<InitiateHandler");
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
InitDB_Process(void)
{
	InitDBG();
#ifdef	HAVE_POSTGRES
	InitPostgres();
#endif
#ifdef	USE_SHELL
	InitShellOperation();
#endif
	SetUpDBG();
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
	mcp = node->mcprec; 
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

dbgmsg(">CallAfter");
	mcp_sindex 	= GetItemLongName(node->mcprec,"private.count");
	mcp_swindow = GetItemLongName(node->mcprec,"private.swindow");
	mcp_puttype = GetItemLongName(node->mcprec,"dc.puttype");
	mcp_pputtype = GetItemLongName(node->mcprec,"private.pputtype");
	mcp_dcwindow = GetItemLongName(node->mcprec,"dc.window");
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
		memcpy(ValueString(GetItemLongName(node->mcprec,"dc.window")),
			   ValueString(GetArrayItem(mcp_swindow,ValueInteger(mcp_sindex))),
			   SIZE_NAME);
		*ValueString(mcp_pputtype) = SCREEN_CHANGE_WINDOW + '0';
	} else
	if		(  !strcmp(ValueString(mcp_puttype),"JOIN")  ) {
		*ValueString(mcp_pputtype) = SCREEN_JOIN_WINDOW + '0';
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
		printf("mcp_sindex = %d\n",ValueInteger(mcp_sindex));
		printf("mcp->dc.window = [%s]\n",ValueString(mcp_dcwindow));
		printf("**** window stack dump *****************\n");
		for	( i = 0 ; i < ValueInteger(mcp_sindex) ; i ++ ) {
			printf("[%d:%s]\n",i,(ValueString(
									  GetArrayItem(mcp_swindow,i))));
		}
		printf("----------------------------------------\n");
#endif
		if		(  strcmp(ValueString(
							  GetArrayItem(mcp_swindow,ValueInteger(mcp_sindex) - 1)),
						  ValueString(mcp_dcwindow))  !=  0  ) {
			strcpy(ValueString(GetItemLongName(node->mcprec,"dc.fromwin")),
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
	int			ix;
	char		*window;
dbgmsg(">ExecuteProcess");
	window = ValueString(GetItemLongName(node->mcprec,"dc.window"));
	ix = (int)g_hash_table_lookup(ThisLD->whash,window);
	bind = ThisLD->window[ix-1];
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

