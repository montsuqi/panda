/*	Panda -- a simple transaction monitor

Copyright (C) 2003 Ogochan.

This module is part of Panda.

	Panda is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
Panda, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with Panda so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/
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

#define	SRC_CODING	"euc-jp"

static	GHashTable	*StatusTable;
static	ValueStruct	*DB_BlogEntry;
static	ValueStruct	*DB_BlogBody;

static	void
do_LINK(
	ProcessNode	*node)
{
	RecordStruct	*entry;

dbgmsg(">do_LINK");
	entry = MCP_GetWindowRecord(node,"entry");							  
	if		(  strlen(node->window)  ==  0  )	{
		SetValueString(GetItemLongName(entry->value,"title.value"),"",NULL);
		SetValueString(GetItemLongName(entry->value,"body.value"),"0_0",NULL);
	}
	MCP_PutWindow(node,"entry",MCP_PUT_CURRENT);
dbgmsg("<do_LINK");
}

static	void
do_OK(
	ProcessNode	*node)
{
	RecordStruct	*entry;
	time_t	nowtime;
	int		rc;

ENTER_FUNC;
	time(&nowtime);

	entry = MCP_GetWindowRecord(node,"entry");
	SetValueInteger(GetItemLongName(DB_BlogEntry,"id"),(int)nowtime);
	CopyValue(GetItemLongName(DB_BlogEntry,"date.yaer"),
			  GetItemLongName(entry->value,"date.year.value"));
	CopyValue(GetItemLongName(DB_BlogEntry,"date.month"),
			  GetItemLongName(entry->value,"date.month.value"));
	CopyValue(GetItemLongName(DB_BlogEntry,"date.day"),
			  GetItemLongName(entry->value,"date.day.value"));
	CopyValue(GetItemLongName(DB_BlogEntry,"title"),
			  GetItemLongName(entry->value,"title.value"));
	CopyValue(GetItemLongName(DB_BlogEntry,"entry"),
			  GetItemLongName(entry->value,"body.value"));
	(void)MCP_ExecFunction(node,"blog_entry","main","DBINSERT",DB_BlogEntry);

	CopyValue(GetItemLongName(DB_BlogBody,"object"),
			  GetItemLongName(DB_BlogEntry,"entry"));
	SetValueString(GetItemLongName(DB_BlogBody,"file"),"./export",NULL);

	rc = MCP_ExecFunction(node,"blog_body","","BLOBEXPORT",DB_BlogBody);
	printf("rc = [%d]\n",rc);


	MCP_PutWindow(node,"entry",MCP_PUT_CURRENT);
LEAVE_FUNC;
}

static	void
do_Cancel(
	ProcessNode	*node)
{
ENTER_FUNC;
	MCP_PutWindow(node,"entry",MCP_PUT_EXIT);
LEAVE_FUNC;
}

static	void
do_Quit(
	ProcessNode	*node)
{
ENTER_FUNC;
	MCP_PutWindow(node,"entry",MCP_PUT_EXIT);
LEAVE_FUNC;
}

extern	void
entryMain(
	ProcessNode	*node)
{
	void		(*handler)(ProcessNode *);

ENTER_FUNC;
#ifdef	DEBUG
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
		MCP_PutWindow(node,"entry",MCP_PUT_CURRENT);
	}
LEAVE_FUNC;
}

extern	void
entryInit(void)
{
ENTER_FUNC;
	StatusTable = NewNameHash(); 
	MCP_RegistHandler(StatusTable,"LINK","",do_LINK);
	MCP_RegistHandler(StatusTable,"","",do_LINK);

	MCP_RegistHandler(StatusTable,"PUTG","OK",do_OK);
	MCP_RegistHandler(StatusTable,"PUTG","Cancel",do_Cancel);
	MCP_RegistHandler(StatusTable,"PUTG","Quit",do_Quit);

	DB_BlogEntry = MCP_GetDB_Define("blog_entry");
	DB_BlogBody = MCP_GetDB_Define("blog_body");
LEAVE_FUNC;
}
