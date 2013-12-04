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
#include	"RecParser.h"
#include	"dbgroup.h"
#include	"term.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"redirect.h"
#include	"comm.h"
#include	"comms.h"
#include	"sysdata.h"
#include	"sysdbreq.h"
#include	"message.h"
#include	"debug.h"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->process[PROCESS_UPDATE].conn)

#define SYSDB_MAX_SIZE (100)

static ValueStruct *sysdbval = NULL;

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

static	ValueStruct	*
GETDATA(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret;
	PacketClass rc;
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		rc = SYSDB_GetData(NBCONN(dbg), args);
		if (rc == SESSION_CONTROL_OK) {
			ctrl->rc = MCP_OK;
			ret = DuplicateValue(args,TRUE);
		} else {
			ctrl->rc = MCP_BAD_OTHER;
		}
	}
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
SETMESSAGE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret;
	PacketClass rc;
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		rc = SYSDB_SetMessage(NBCONN(dbg), args);
		ctrl->rc = rc == SESSION_CONTROL_OK ? MCP_OK : MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
SETMESSAGEALL(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret;
	PacketClass rc;
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		rc = SYSDB_SetMessageAll(NBCONN(dbg), args);
		ctrl->rc = rc == SESSION_CONTROL_OK ? MCP_OK : MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	ret;
}

static	int				idx = 0;
static	int				numData = 0;
static	Bool			hasData = FALSE;
static	ValueStruct *	sysdbvals = NULL;

static	ValueStruct	*
SELECTALL(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret;
	PacketClass rc;
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if (sysdbvals == NULL) {
			sysdbvals = RecParseValueMem(SYSDBVALS_DEF,NULL);
			InitializeValue(sysdbvals);
		}
		rc = SYSDB_GetDataAll(NBCONN(dbg),&numData, sysdbvals);
		if (rc == SESSION_CONTROL_OK) {
			hasData = TRUE;
			idx = 0;
			ctrl->rc = MCP_OK;
		} else {
			hasData = FALSE;
			ctrl->rc = MCP_BAD_OTHER;
		}
	}
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
FETCH(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*v;
	char vname[256];
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB || !hasData){
		ctrl->rc = MCP_BAD_ARG;
		return ret;
	}
	if (idx >= numData) {
		ctrl->rc = MCP_EOF;
		return ret;
	}
	if (sysdbvals == NULL) {
		ctrl->rc = MCP_EOF;
		return ret;
	}
	snprintf(vname,sizeof(vname),"values[%d]",idx);
	v = GetItemLongName(sysdbvals,vname); 
	if (v != NULL) {
		ret = DuplicateValue(v,TRUE); 
	}
	idx += 1;
	ctrl->rc = MCP_OK;
LEAVE_FUNC;
	return	ret;
}

static const char *SYSDB_KEYS[] = {
	"user",
	"host",
	"agent",
	"window",
	"widget",
	"event",
	"in_process",
	"create_time",
	"access_time",
	"process_time",
	"total_process_time",
	"count",
	"popup",
	"dialog",
	"abort",
	NULL
};

static	Bool
IsValidKey(
	const char *key)
{
	int i;

	if (key == NULL) {
		return FALSE;
	}

	for(i=0;SYSDB_KEYS[i] != NULL;i++) {
		if (!strcmp(key,SYSDB_KEYS[i])) {
			return TRUE;
		}
	}
	return FALSE;
}

static	Bool
IsMessageKey(
	const char *key)
{
	if (key == NULL) {
		return FALSE;
	}
	if (
		!strcmp(key,"popup") ||
		!strcmp(key,"dialog") ||
		!strcmp(key,"abort")
	) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static	ValueStruct	*
GETVALUE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*v,*k;
	PacketClass rc;
	int i,n;
	char *key,lname[256];
ENTER_FUNC;
	ret = NULL;
	InitializeValue(sysdbval);
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		SetValueString(
			GetItemLongName(sysdbval,"id"),
			ValueToString(GetItemLongName(args,"id"),NULL),
			NULL);
		rc = SYSDB_GetData(NBCONN(dbg), sysdbval);
		if (rc == SESSION_CONTROL_OK) {
			ctrl->rc = MCP_OK;
			ret = DuplicateValue(args,TRUE);
			n = ValueToInteger(GetItemLongName(args,"num"));
			n = n > SYSDB_MAX_SIZE ? SYSDB_MAX_SIZE : n;
			for (i = 0; i < n; i++) {
				snprintf(lname,sizeof(lname),"query[%d].key",i);
				k = GetItemLongName(ret,lname);
				key = ValueToString(k,NULL);
				snprintf(lname,sizeof(lname),"query[%d].value",i);
				v = GetItemLongName(ret,lname);

				if (IsValidKey(key)) {
					SetValueString(v,
						ValueToString(GetItemLongName(sysdbval,key),NULL),NULL);
				}
			}
		} else {
			ctrl->rc = MCP_BAD_OTHER;
		}
	}
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
SETVALUE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*v,*k;
	PacketClass rc;
	int i,n;
	char *key,lname[256];
ENTER_FUNC;
	ret = NULL;
	InitializeValue(sysdbval);
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		SetValueString(
			GetItemLongName(sysdbval,"id"),
			ValueToString(GetItemLongName(args,"id"),NULL),
			NULL);
		n = ValueToInteger(GetItemLongName(args,"num"));
		n = n > SYSDB_MAX_SIZE ? SYSDB_MAX_SIZE : n;
		for (i = 0; i < n; i++) {
			snprintf(lname,sizeof(lname),"query[%d].key",i);
			k = GetItemLongName(args,lname);
			key = ValueToString(k,NULL);
			snprintf(lname,sizeof(lname),"query[%d].value",i);
			v = GetItemLongName(args,lname);
			if (IsMessageKey(key)) {
				SetValueString(
					GetItemLongName(sysdbval,key),
					ValueToString(v,NULL),
					NULL);
				rc = SYSDB_SetMessage(NBCONN(dbg), sysdbval);
				if (rc == SESSION_CONTROL_OK) {
					ctrl->rc = MCP_OK;
				}
			}
		}
	}
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
SETVALUEALL(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*v,*k;
	PacketClass rc;
	int i,n;
	char *key,lname[256];
ENTER_FUNC;
	ret = NULL;
	InitializeValue(sysdbval);
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		n = ValueToInteger(GetItemLongName(args,"num"));
		n = n > SYSDB_MAX_SIZE ? SYSDB_MAX_SIZE : n;
		for (i = 0; i < n; i++) {
			snprintf(lname,sizeof(lname),"query[%d].key",i);
			k = GetItemLongName(args,lname);
			key = ValueToString(k,NULL);
			snprintf(lname,sizeof(lname),"query[%d].value",i);
			v = GetItemLongName(args,lname);
			if (IsMessageKey(key)) {
				SetValueString(
					GetItemLongName(sysdbval,key),
					ValueToString(v,NULL),
					NULL);
				rc = SYSDB_SetMessageAll(NBCONN(dbg), sysdbval);
				if (rc == SESSION_CONTROL_OK) {
					ctrl->rc = MCP_OK;
				}
			}
		}
	}
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
LISTKEY(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*k;
	int i;
	char lname[256];
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_OK;
		ret = DuplicateValue(args,FALSE);
		for(i=0;SYSDB_KEYS[i] != NULL;i++) {
			snprintf(lname,sizeof(lname),"query[%d].key",i);
			k = GetItemLongName(ret,lname);
			SetValueString(k,SYSDB_KEYS[i],NULL);
		}
		SetValueInteger(GetItemLongName(ret,"num"),i);
	}
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
LISTENTRY(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*v,*k;
	PacketClass rc;
	int i,n;
	char vname[256];
ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		/* SELECTALL */
		if (sysdbvals == NULL) {
			sysdbvals = RecParseValueMem(SYSDBVALS_DEF,NULL);
			InitializeValue(sysdbvals);
		}
		rc = SYSDB_GetDataAll(NBCONN(dbg),&numData, sysdbvals);
		if (rc != SESSION_CONTROL_OK) {
			ctrl->rc = MCP_BAD_OTHER;
			return ret;
		}
		ret = DuplicateValue(args,FALSE);
		if (numData > SYSDB_MAX_SIZE) {
			n = SYSDB_MAX_SIZE;
		} else {
			n = numData;
		}
		SetValueInteger(GetItemLongName(ret,"num"),n);
		for(i=0;i<n;i++){
			snprintf(vname,sizeof(vname),"values[%d]",i);
			v = GetItemLongName(sysdbvals,vname); 
			if (v!=NULL) {
				snprintf(vname,sizeof(vname),"query[%d].key",i);
				k = GetItemLongName(ret,vname);
				SetValueString(k,
					ValueToString(GetItemLongName(v,"id"),NULL),
					NULL);
			}
		}
		ctrl->rc = MCP_OK;
	}
LEAVE_FUNC;
	return	ret;
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",			(DB_FUNC)SYSDATA_DBOPEN },
	{	"DBDISCONNECT",		(DB_FUNC)SYSDATA_DBDISCONNECT },
	{	"DBSTART",			(DB_FUNC)SYSDATA_DBSTART },
	{	"DBCOMMIT",			(DB_FUNC)SYSDATA_DBCOMMIT },
	/*	table operations	*/
	{	"GETVALUE",			GETVALUE },
	{	"SETVALUE",			SETVALUE },
	{	"SETVALUEALL",		SETVALUEALL },
	{	"LISTKEY",			LISTKEY },
	{	"LISTENTRY",		LISTENTRY },
	{	NULL,				NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL
};

extern	DB_Func	*
InitSystem(void)
{
	sysdbval = RecParseValueMem(SYSDBVAL_DEF,NULL);
	return	(EnterDB_Function("System",Operations,DB_PARSER_NULL,&Core,"/*","*/\t"));
}
