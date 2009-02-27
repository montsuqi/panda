/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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
#include	<ctype.h>
#include	<glib.h>
#include	<numeric.h>
#include	<netdb.h>

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"SQLparser.h"
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"term.h"
#include	"directory.h"
#include	"redirect.h"
#include	"debug.h"

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRedirect,
	int			usage)
{
	return	(MCP_OK);
}

static	ValueStruct	*
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	OpenDB_RedirectPort(dbg);
	dbg->process[PROCESS_UPDATE].conn = NULL;
	dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_CONNECT;
	dbg->process[PROCESS_READONLY].dbstatus = DB_STATUS_NOCONNECT;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  dbg->process[PROCESS_UPDATE].dbstatus == DB_STATUS_CONNECT ) { 
		CloseDB_RedirectPort(dbg);
		dbg->process[PROCESS_UPDATE].dbstatus = DB_STATUS_DISCONNECT;
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	BeginDB_Redirect(dbg); 
	ctrl->rc = MCP_OK;
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	CheckDB_Redirect(dbg);
	CommitDB_Redirect(dbg);
	ctrl->rc = MCP_OK;
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBSELECT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;
ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_OK;
		ret = DuplicateValue(args,TRUE);
	}
LEAVE_FUNC;
	return	(ret);
}

static	int
SetValues(
	ValueStruct	*value)
{
	ValueStruct	*e;

ENTER_FUNC;
	if		(  ( e = GetItemLongName(value,"host") )  !=  NULL  ) {
		if		(  CurrentProcess  !=  NULL  ) {
			dbgprintf("term = [%s]",CurrentProcess->term);
			dbgprintf("host = [%s]",TermToHost(CurrentProcess->term));
			SetValueString(e,TermToHost(CurrentProcess->term),NULL);
		} else {
			SetValueString(e,"",NULL);
		}
	}
LEAVE_FUNC;
	return	(MCP_OK);
}

static	ValueStruct	*
_DBFETCH(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;
ENTER_FUNC;
	ret = NULL;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = SetValues(args);
		ret = DuplicateValue(args,TRUE);
	}
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_DBUPDATE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBDELETE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBINSERT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_DBACCESS(
	DBG_Struct		*dbg,
	char			*name,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
ENTER_FUNC;
#ifdef	TRACE
	printf("[%s]\n",name); 
#endif
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)_DBCOMMIT },
	/*	table operations	*/
	{	"DBSELECT",		_DBSELECT },
	{	"DBFETCH",		_DBFETCH },
	{	"DBUPDATE",		_DBUPDATE },
	{	"DBDELETE",		_DBDELETE },
	{	"DBINSERT",		_DBINSERT },

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL
};

extern	DB_Func	*
InitSystem(void)
{
	return	(EnterDB_Function("System",Operations,DB_PARSER_NULL,&Core,"/*","*/\t"));
}
