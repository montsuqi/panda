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
#include	"redirect.h"
#include	"debug.h"

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRedirect)
{
	return	(TRUE);
}

static	void
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
dbgmsg(">_DBOPEN");
	dbg->conn = (void *)NULL;
	OpenDB_RedirectPort(dbg);
	dbg->fConnect = TRUE;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
dbgmsg("<_DBOPEN");
}

static	void
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
dbgmsg(">_DBDISCONNECT");
	if		(  dbg->fConnect  ) { 
		CloseDB_RedirectPort(dbg);
		dbg->fConnect = FALSE;
		ctrl->rc = MCP_OK;
	}
dbgmsg("<_DBDISCONNECT");
}

static	void
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
dbgmsg(">_DBSTART");
	BeginDB_Redirect(dbg); 
	ctrl->rc = MCP_OK;
dbgmsg("<_DBSTART");
}

static	void
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
dbgmsg(">_DBCOMMIT");
	CheckDB_Redirect(dbg);
	CommitDB_Redirect(dbg);
	ctrl->rc = MCP_OK;
dbgmsg("<_DBCOMMIT");
}

static	void
_DBSELECT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
dbgmsg(">_DBSELECT");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_OK;
	}
dbgmsg("<_DBSELECT");
}

static	char	*
TermToHost(
	char	*term)
{
	struct	sockaddr_in		in;
	struct	sockaddr_in6	in6;
	char	*p;
	int		i;
	char	buff[NI_MAXHOST+2];
	static	char	host[NI_MAXHOST];
	char	port[NI_MAXSERV];

ENTER_FUNC;
	p = term;
	switch	(*p) {
	  case	'4':
		p ++;
		in.sin_port = HexToInt(p,4);
		p = strchr(p,':') + 1;
		in.sin_addr.s_addr = HexToInt(p,8);
		in.sin_family = AF_INET;
		getnameinfo((struct sockaddr *)&in,sizeof(in),host,NI_MAXHOST,port,NI_MAXSERV,0);
		break;
	  case	'6':
		p ++;
		in6.sin6_port = HexToInt(p,4);
		p = strchr(p,':') + 1;
		for	( i = 0 ; i < 4 ; i ++ ) {
			in6.sin6_addr.s6_addr32[i] = HexToInt(p,8);
			p += 8;
		}
		p ++;
		in6.sin6_scope_id = HexToInt(p,8);
		in6.sin6_family = AF_INET6;
		getnameinfo((struct sockaddr *)&in6,sizeof(in6),buff,NI_MAXHOST,port,NI_MAXSERV,0);
		if		(  strchr(buff,':')  !=  NULL  ) {
			sprintf(host,"[%s]",buff);
		} else {
			strcpy(host,buff);
		}
		break;
	  default:
		*host = 0;
		break;
	}
LEAVE_FUNC;
	return	(host);
}

static	int
SetValues(
	ValueStruct	*value)
{
	ValueStruct	*e;

ENTER_FUNC;
	if		(  ( e = GetItemLongName(value,"host") )  !=  NULL  ) {
		if		(  CurrentProcess  !=  NULL  ) {
			SetValueString(e,TermToHost(CurrentProcess->term),NULL);
		} else {
			SetValueString(e,"",NULL);
		}
	}
LEAVE_FUNC;
	return	(MCP_OK);
}

static	void
_DBFETCH(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
dbgmsg(">_DBFETCH");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = SetValues(rec->value);
	}
dbgmsg("<_DBFETCH");
}

static	void
_DBUPDATE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
dbgmsg(">_DBUPDATE");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
dbgmsg("<_DBUPDATE");
}

static	void
_DBDELETE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
dbgmsg(">_DBDELETE");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
dbgmsg("<_DBDELETE");
}

static	void
_DBINSERT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
dbgmsg(">_DBINSERT");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
	}
dbgmsg("<_DBINSERT");
}

static	Bool
_DBACCESS(
	DBG_Struct		*dbg,
	char			*name,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	Bool	rc;

dbgmsg(">_DBACCESS");
#ifdef	TRACE
	printf("[%s]\n",name); 
#endif
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
		rc = TRUE;
	} else {
		ctrl->rc = MCP_BAD_OTHER;
		rc = FALSE;
	}
dbgmsg("<_DBACCESS");
	return	(rc);
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
	NULL,
	NULL,
	NULL,
	NULL
};

extern	DB_Func	*
InitSystem(void)
{
	return	(EnterDB_Function("System",Operations,&Core,"/*","*/\t"));
}
