/*
*/
#define	DEBUG
#define	TRACE

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"apslib.h"
#include	"message.h"
#include	"debug.h"

static	GHashTable	*StatusTable;

static	void
SetColor(
	ProcessNode	*node,
	char		*color)
{
	RecordStruct	*mainc;
	ValueStruct		*item;

ENTER_FUNC;
	mainc = MCP_GetWindowRecord(node,"main");
	item = GetItemLongName(mainc->value,"area3.color");
	SetValueString(item,color,NULL);
LEAVE_FUNC;
}

static	void
do_LINK(
	ProcessNode	*node)
{
	RecordStruct	*mainc;
	ValueStruct		*item;
ENTER_FUNC;
	mainc = MCP_GetWindowRecord(node,"main");

	item = GetItemLongName(mainc->value,"area1.color");
	SetValueString(item,"white",NULL);
	item = GetItemLongName(mainc->value,"area2.color");
	SetValueString(item,"white",NULL);
	item = GetItemLongName(mainc->value,"area3.color");
	SetValueString(item,"white",NULL);

	item = GetItemLongName(mainc->value,"message");
	SetValueString(item,"漢本\xEE\xC0\xC0漢本",NULL);

	item = GetItemLongName(mainc->value,"user");
	SetValueString(item,"user",NULL);

	MCP_PutWindow(node,"main",MCP_PUT_CURRENT);
LEAVE_FUNC;
}

static	void
do_red(
	ProcessNode	*node)
{
ENTER_FUNC;
	SetColor(node,"red");
	MCP_PutWindow(node,"main",MCP_PUT_CURRENT);
LEAVE_FUNC;
}

static	void
do_blue(
	ProcessNode	*node)
{
ENTER_FUNC;
	SetColor(node,"blue");
	MCP_PutWindow(node,"main",MCP_PUT_CURRENT);
LEAVE_FUNC;
}

static	void
do_yellow(
	ProcessNode	*node)
{
ENTER_FUNC;
	SetColor(node,"yellow");
	MCP_PutWindow(node,"main",MCP_PUT_CURRENT);
LEAVE_FUNC;
}

static	void
do_Quit(
	ProcessNode	*node)
{
ENTER_FUNC;
	MCP_PutWindow(node,"main",MCP_PUT_EXIT);
LEAVE_FUNC;
}

extern	void
mainMain(
	ProcessNode	*node)
{
	void		(*handler)(ProcessNode *);

ENTER_FUNC;
#if	1
	dbgprintf("term   = [%s]",node->term);
	dbgprintf("user   = [%s]",node->user);
	dbgprintf("window = [%s]",node->window);
	dbgprintf("widget = [%s]",node->widget);
	dbgprintf("event  = [%s]",node->event);
#endif
	if		(  ( handler = MCP_GetEventHandler(StatusTable,node) )  !=  NULL  ) {
		(*handler)(node);
	} else {
		printf("not found\n");
		MCP_PutWindow(node,"main",MCP_PUT_CURRENT);
	}
LEAVE_FUNC;
}

extern	void
mainInit(void)
{
ENTER_FUNC;
	StatusTable = NewNameHash(); 
	MCP_RegistHandler(StatusTable,"LINK","",do_LINK);
	MCP_RegistHandler(StatusTable,"PUTG","red",do_red);
	MCP_RegistHandler(StatusTable,"PUTG","blue",do_blue);
	MCP_RegistHandler(StatusTable,"PUTG","yellow",do_yellow);
	MCP_RegistHandler(StatusTable,"PUTG","Quit",do_Quit);
LEAVE_FUNC;
}
