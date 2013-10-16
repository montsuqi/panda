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
#include	<pthread.h>

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"term.h"
#include	"directory.h"
#include	"redirect.h"
#include	"keyvaluereq.h"
#include	"keyvaluecom.h"
#include	"comm.h"
#include	"comms.h"
#include	"sysdata.h"
#include	"message.h"
#include	"debug.h"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->process[PROCESS_UPDATE].conn)

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
_DBACCESS(
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
	}
LEAVE_FUNC;
	return	(ret);
}

#define DEFFUNC(pklass) \
static	ValueStruct	*											\
_ ## pklass(													\
	DBG_Struct		*dbg,										\
	DBCOMM_CTRL		*ctrl,										\
	RecordStruct	*rec,										\
	ValueStruct		*args)										\
{																\
	ValueStruct *ret;											\
ENTER_FUNC;														\
	ret = NULL;													\
	ctrl->rc = MCP_BAD_OTHER;									\
	if		(  rec->type  !=  RECORD_DB  ) {					\
		ctrl->rc = MCP_BAD_ARG;									\
	} else {													\
		ctrl->rc = KVREQ_Request(NBCONN(dbg), (pklass), args);	\
		ret = DuplicateValue(args, TRUE);						\
	}															\
LEAVE_FUNC;														\
	return	ret;												\
}

DEFFUNC(KV_GETVALUE)
DEFFUNC(KV_SETVALUE)
DEFFUNC(KV_SETVALUEALL)
DEFFUNC(KV_LISTKEY)
DEFFUNC(KV_LISTENTRY)
#if 0
DEFFUNC(KV_NEWENTRY)
DEFFUNC(KV_DELETEENTRY)
#endif

#undef DEFFUNC

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)SYSDATA_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)SYSDATA_DBDISCONNECT },
	{	"DBSTART",		(DB_FUNC)SYSDATA_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)SYSDATA_DBCOMMIT },
	/*	table operations	*/
	{	"GETVALUE",		_KV_GETVALUE },
	{	"SETVALUE",		_KV_SETVALUE },
	{	"SETVALUEALL",	_KV_SETVALUEALL },
	{	"LISTKEY",		_KV_LISTKEY },
	{	"LISTENTRY",	_KV_LISTENTRY },
#if 0
	{	"NEWENTRY",		_KV_NEWENTRY },
	{	"DELETEENTRY",	_KV_DELETEENTRY },
#endif
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
