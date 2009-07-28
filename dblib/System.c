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
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"term.h"
#include	"directory.h"
#include	"redirect.h"
#include	"debug.h"

#define	LockDB(db)		dbgmsg("LockDB");pthread_mutex_lock(&(db->mutex))
#define	UnLockDB(db)	dbgmsg("UnLockDB");pthread_mutex_unlock(&(db->mutex))

typedef struct {
	GHashTable		*table;
	pthread_mutex_t	mutex;
}	_SystemDB;

static _SystemDB *DBctx = NULL;

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
	char			*name,
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
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if (DBctx == NULL) {
		DBctx = New(_SystemDB);
		DBctx->table = NewNameHash();
		pthread_mutex_init(&(DBctx->mutex),NULL);
	}

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
_GETVALUE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*id;
	ValueStruct	*num;
	ValueStruct	*query;
	ValueStruct	*key;
	ValueStruct	*value;
	ValueStruct	*ret;
	GHashTable	*record;
	int i;
	int max;
	char *val;
ENTER_FUNC;
	LockDB(DBctx);
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if ((id = GetItemLongName(args, "id")) == NULL ||
				(num = GetItemLongName(args, "num")) == NULL ||
				(query = GetItemLongName(args, "query")) == NULL ) {
			ctrl->rc = MCP_BAD_ARG;
			Warning("does not found id or num or query");
		} else {
			if ((record = g_hash_table_lookup(DBctx->table, ValueToString(id,NULL))) == NULL) {
				ctrl->rc = MCP_BAD_OTHER;
				Warning("does not found id[%s]", ValueToString(id,NULL));
			} else {
				max = ValueArraySize(query) < ValueInteger(num) ? ValueArraySize(query) : ValueInteger(num);
				for (i = 0; i < max; i++) {
					if ((key = GetItemLongName(ValueArrayItem(query, i), "key")) != NULL &&
							(value = GetItemLongName(ValueArrayItem(query, i), "value")) != NULL) {
						if ((val = g_hash_table_lookup(record, ValueToString(key, NULL))) != NULL) {
							SetValueStringWithLength(value, val, strlen(val), NULL);
						} else {
							SetValueStringWithLength(value, "", strlen(""), NULL);
						}
					}
				}
				ret = DuplicateValue(args, TRUE);
				ctrl->rc = MCP_OK;
			}
		}
	}
	UnLockDB(DBctx);
LEAVE_FUNC;
	return	ret;
}


static	void
SetValue(
	char		*id,
	GHashTable	*record,
	ValueStruct	*args)
{
	ValueStruct	*query;
	ValueStruct	*num;
	ValueStruct	*key;
	ValueStruct	*value;
	gpointer	okey;
	gpointer	oval;
	int i;
	int max;
ENTER_FUNC;
	okey = oval = NULL;
	num = GetItemLongName(args, "num");
	query = GetItemLongName(args, "query");
	max = ValueArraySize(query) < ValueInteger(num) ? ValueArraySize(query) : ValueInteger(num);
	for (i = 0; i < max; i++) {
		key = GetItemLongName(ValueArrayItem(query, i), "key");
		value = GetItemLongName(ValueArrayItem(query, i), "value");
		if ( key != NULL && value != NULL && strlen(ValueToString(key,NULL)) > 0 ) {
			if (g_hash_table_lookup_extended(record, ValueToString(key, NULL), &okey, &oval)) {
				g_hash_table_remove(record, ValueToString(key, NULL));
				xfree(okey);
				xfree(oval);
			}
			g_hash_table_insert(record, 
				StrDup(ValueToString(key,NULL)), 
				StrDup(ValueToString(value, NULL)));
		}
	}
LEAVE_FUNC;
}

static	ValueStruct	*
_SETVALUE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*id;
	ValueStruct	*query;
	ValueStruct	*num;
	GHashTable	*record;
ENTER_FUNC;
	LockDB(DBctx);
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if ((id = GetItemLongName(args, "id")) == NULL ||
				(num = GetItemLongName(args, "num")) == NULL ||
				(query = GetItemLongName(args, "query")) == NULL) {
			ctrl->rc = MCP_BAD_ARG;
			Warning("does not found id or num or query");
		} else {
			if ((record = g_hash_table_lookup(DBctx->table, ValueToString(id,NULL))) == NULL) {
				ctrl->rc = MCP_BAD_OTHER;
				Warning("does not found id[%s]", ValueToString(id,NULL));
			} else {
				SetValue(ValueToString(id,NULL), record, args);
				ctrl->rc = MCP_OK;
			}
		}
	}
	UnLockDB(DBctx);
LEAVE_FUNC;
	return	(NULL);
}

static	ValueStruct	*
_SETVALUEALL(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*query;
	ValueStruct	*num;
ENTER_FUNC;
	LockDB(DBctx);
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if ((num = GetItemLongName(args, "num")) == NULL ||
				(query = GetItemLongName(args, "query")) == NULL) {
			ctrl->rc = MCP_BAD_ARG;
			Warning("does not found num or query");
		} else {
			g_hash_table_foreach(DBctx->table, (GHFunc)SetValue, args);
			ctrl->rc = MCP_OK;
		}
	}
	UnLockDB(DBctx);
LEAVE_FUNC;
	return	(NULL);
}

static	void
KeyToKey(
	char		 	*id,
	char			*value,
	ValueStruct		*args)
{
	ValueStruct	*key;
	ValueStruct	*num;
	ValueStruct	*query;
	int	n;
ENTER_FUNC;
	num = GetItemLongName(args, "num");
	query = GetItemLongName(args, "query");
	n = ValueInteger(num);
	if (n < ValueArraySize(query)) {
		key = GetItemLongName(ValueArrayItem(query, n), "key");
		SetValueStringWithLength(key, id, strlen(id), NULL);
		ValueInteger(num) += 1;
	}
LEAVE_FUNC;
}

static	ValueStruct	*
_LISTKEY(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*id;
	ValueStruct	*num;
	ValueStruct	*query;
	ValueStruct	*ret;
	GHashTable	*record;
ENTER_FUNC;
	LockDB(DBctx);
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if ((id = GetItemLongName(args, "id")) == NULL ||
				(num = GetItemLongName(args, "num")) == NULL ||
				(query = GetItemLongName(args, "query")) == NULL) {
			ctrl->rc = MCP_BAD_ARG;
			Warning("does not found id or num or query");
		} else {
			if ((record = g_hash_table_lookup(DBctx->table, ValueToString(id,NULL))) == NULL) {
				ctrl->rc = MCP_BAD_OTHER;
				Warning("does not found id[%s]", ValueToString(id,NULL));
			} else {
				ValueInteger(num) = 0;
				ret = DuplicateValue(args, FALSE);
				g_hash_table_foreach(record, (GHFunc)KeyToKey, ret);
				ctrl->rc = MCP_OK;
			}
		}
	}
	UnLockDB(DBctx);
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
_LISTID(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*num;
	ValueStruct	*query;
	ValueStruct	*ret;
ENTER_FUNC;
	LockDB(DBctx);
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if ((num = GetItemLongName(args, "num")) == NULL ||
				(query = GetItemLongName(args, "query")) == NULL) {
			ctrl->rc = MCP_BAD_ARG;
			Warning("does not found id or num or query");
		} else {
			ValueInteger(num) = 0;
			ret = DuplicateValue(args, FALSE);
			g_hash_table_foreach(DBctx->table, (GHFunc)KeyToKey, ret);
			ctrl->rc = MCP_OK;
		}
	}
	UnLockDB(DBctx);
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
_NEWRECORD(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*id;
ENTER_FUNC;
	LockDB(DBctx);
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if ((id = GetItemLongName(args, "id")) == NULL) {
			ctrl->rc = MCP_BAD_ARG;
			Warning("does not found id");
		} else {
			if (g_hash_table_lookup(DBctx->table, ValueToString(id,NULL)) == NULL) {
				g_hash_table_insert(DBctx->table, StrDup(ValueToString(id,NULL)), NewNameHash());
			}
			ctrl->rc = MCP_OK;
		}
	}
	UnLockDB(DBctx);
LEAVE_FUNC;
	return	(NULL);
}

static	gboolean
RemoveValue(
	gpointer 	key,
	gpointer	value,
	gpointer	data)
{
	xfree(key);
	xfree(value);
	return TRUE;
}

static	ValueStruct	*
_DESTROYRECORD(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*id;
	gpointer	okey;
	gpointer	oval;
ENTER_FUNC;
	LockDB(DBctx);
	ctrl->rc = MCP_BAD_OTHER;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		if ((id = GetItemLongName(args, "id")) == NULL) {
			ctrl->rc = MCP_BAD_ARG;
			Warning("does not found id");
		} else {
			if (g_hash_table_lookup_extended(DBctx->table, ValueToString(id,NULL), &okey, &oval)) {
				g_hash_table_remove(DBctx->table, okey);
				g_hash_table_foreach_remove((GHashTable*)oval, RemoveValue, NULL);
				xfree(okey);
				xfree(oval);
				ctrl->rc = MCP_OK;
			}
		}
	}
	UnLockDB(DBctx);
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
	{	"GETVALUE",		_GETVALUE },
	{	"SETVALUE",		_SETVALUE },
	{	"SETVALUEALL",	_SETVALUEALL },
	{	"LISTKEY",		_LISTKEY },
	{	"LISTID",		_LISTID },
	{	"NEWRECORD",	_NEWRECORD },
	{	"DESTROYRECORD",_DESTROYRECORD },
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
