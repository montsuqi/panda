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
	mainc = node->thisscrrec;
	item = GetItemLongName(mainc->value,"color");
	SetValueString(item,color,NULL);
LEAVE_FUNC;
}

static	void
do_LINK(
	ProcessNode	*node)
{
ENTER_FUNC;
	MCP_PutWindow(node,NULL,MCP_PUT_CURRENT);
LEAVE_FUNC;
}

static	void
do_red(
	ProcessNode	*node)
{
ENTER_FUNC;
	SetColor(node,"red");
	MCP_ReturnComponent(node,"blue");
LEAVE_FUNC;
}

static	void
do_blue(
	ProcessNode	*node)
{
ENTER_FUNC;
	SetColor(node,"blue");
	MCP_ReturnComponent(node,"yellow");
LEAVE_FUNC;
}

static	void
do_yellow(
	ProcessNode	*node)
{
ENTER_FUNC;
	SetColor(node,"yellow");
	MCP_ReturnComponent(node,"red");
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
mainColorMain(
	ProcessNode	*node)
{
	void		(*handler)(ProcessNode *);

ENTER_FUNC;
#if	0
	printf("term   = [%s]\n",node->term);
	printf("user   = [%s]\n",node->user);
	printf("window = [%s]\n",node->window);
	printf("widget = [%s]\n",node->widget);
	printf("event  = [%s]\n",node->event);
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
mainColorInit(void)
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
