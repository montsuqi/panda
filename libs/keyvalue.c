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
#include	<unistd.h>
#include	<ctype.h>
#include	<glib.h>
#include	<numeric.h>
#include	<netdb.h>
#include	<pthread.h>
#include	<sys/types.h>
#include	<sys/stat.h>

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"keyvalue.h"
#include	"message.h"
#include	"debug.h"

#define	KV_LockRead(db)		dbgmsg("KV_LockRead");LockRead(db)
#define	KV_LockWrite(db)	dbgmsg("KV_LockWrite");LockWrite(db)
#define	KV_UnLock(db)		dbgmsg("KV_UnLock");UnLock(db)

extern	KV_State *
InitKV(void)
{
	KV_State *state;
ENTER_FUNC;
	state = New(KV_State);
	state->table = NewNameHash();
	InitLock(state);
LEAVE_FUNC;
	return state;
}

static	gboolean
RemoveValue(
	gpointer	key,
	gpointer	value,
	gpointer	data)
{
	xfree(key);
	xfree(value);
	return TRUE;
}

static	gboolean
RemoveEntry(
	gpointer	key,
	gpointer	value,
	gpointer	data)
{
	g_hash_table_foreach_remove((GHashTable*)value, RemoveValue, NULL);
	g_hash_table_destroy(value);
	xfree(key);
	return TRUE;
}

extern	void
FinishKV(
	KV_State *state)
{
	g_hash_table_foreach_remove(state->table, RemoveEntry, NULL);
	g_hash_table_destroy(state->table);
	DestroyLock(state);
	xfree(state);
}

extern	int	
KV_GetValue(
	KV_State	*state,
	ValueStruct		*args)
{
	ValueStruct	*id;
	ValueStruct	*num;
	ValueStruct	*query;
	ValueStruct	*key;
	ValueStruct	*value;
	GHashTable	*record;
	int rc;
	int i;
	int max;
	char *val;
ENTER_FUNC;
	KV_LockRead(state);
	rc = MCP_BAD_OTHER;
	if ((id = GetItemLongName(args, "id")) == NULL ||
			(num = GetItemLongName(args, "num")) == NULL ||
			(query = GetItemLongName(args, "query")) == NULL ) {
		rc = MCP_BAD_ARG;
		Warning("does not found id or num or query");
	} else {
		if ((record = g_hash_table_lookup(state->table, ValueToString(id,NULL))) == NULL) {
			rc = MCP_BAD_OTHER;
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
			rc = MCP_OK;
		}
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return	rc;
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

extern	int
KV_SetValue(
	KV_State	*state,
	ValueStruct		*args)
{
	ValueStruct	*id;
	ValueStruct	*query;
	ValueStruct	*num;
	GHashTable	*record;
	int rc;
ENTER_FUNC;
	KV_LockWrite(state);
	rc = MCP_BAD_OTHER;
	if ((id = GetItemLongName(args, "id")) == NULL ||
			(num = GetItemLongName(args, "num")) == NULL ||
			(query = GetItemLongName(args, "query")) == NULL) {
		rc = MCP_BAD_ARG;
		Warning("does not found id or num or query");
	} else {
		if ((record = g_hash_table_lookup(state->table, ValueToString(id,NULL))) == NULL) {
			rc = MCP_BAD_OTHER;
			Warning("does not found id[%s]", ValueToString(id,NULL));
		} else {
			SetValue(ValueToString(id,NULL), record, args);
			rc = MCP_OK;
		}
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return rc;
}

extern	int
KV_SetValueALL(
	KV_State	*state,
	ValueStruct		*args)
{
	ValueStruct	*query;
	ValueStruct	*num;
	int rc;
ENTER_FUNC;
	KV_LockWrite(state);
	rc = MCP_BAD_OTHER;
	if ((num = GetItemLongName(args, "num")) == NULL ||
			(query = GetItemLongName(args, "query")) == NULL) {
		rc = MCP_BAD_ARG;
		Warning("does not found num or query");
	} else {
		g_hash_table_foreach(state->table, (GHFunc)SetValue, args);
		rc = MCP_OK;
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return	rc;
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

extern	int
KV_ListKey(
	KV_State	*state,
	ValueStruct		*args)
{
	ValueStruct	*id;
	ValueStruct	*num;
	ValueStruct	*query;
	GHashTable	*record;
	int rc;
ENTER_FUNC;
	KV_LockRead(state);
	rc = MCP_BAD_OTHER;
	if ((id = GetItemLongName(args, "id")) == NULL ||
			(num = GetItemLongName(args, "num")) == NULL ||
			(query = GetItemLongName(args, "query")) == NULL) {
		rc = MCP_BAD_ARG;
		Warning("does not found id or num or query");
	} else {
		if ((record = g_hash_table_lookup(state->table, ValueToString(id,NULL))) == NULL) {
			rc = MCP_BAD_OTHER;
			Warning("does not found id[%s]", ValueToString(id,NULL));
		} else {
			ValueInteger(num) = 0;
			g_hash_table_foreach(record, (GHFunc)KeyToKey, args);
			rc = MCP_OK;
		}
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return	rc;
}

extern	int
KV_ListEntry(
	KV_State	*state,
	ValueStruct		*args)
{
	ValueStruct	*num;
	ValueStruct	*query;
	int rc;
ENTER_FUNC;
	KV_LockRead(state);
	rc = MCP_BAD_OTHER;
	if ((num = GetItemLongName(args, "num")) == NULL ||
			(query = GetItemLongName(args, "query")) == NULL) {
		rc = MCP_BAD_ARG;
		Warning("does not found id or num or query");
	} else {
		ValueInteger(num) = 0;
		g_hash_table_foreach(state->table, (GHFunc)KeyToKey, args);
		rc = MCP_OK;
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return rc;
}

extern	int
KV_NewEntry(
	KV_State	*state,
	ValueStruct		*args)
{
	ValueStruct	*id;
	int rc;
ENTER_FUNC;
	KV_LockWrite(state);
	rc = MCP_BAD_OTHER;
	if ((id = GetItemLongName(args, "id")) == NULL) {
		rc = MCP_BAD_ARG;
		Warning("does not found id");
	} else {
		if (g_hash_table_lookup(state->table, ValueToString(id,NULL)) == NULL) {
			g_hash_table_insert(state->table, StrDup(ValueToString(id,NULL)), NewNameHash());
		}
		rc = MCP_OK;
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return rc;
}

extern	int
KV_DeleteEntry(
	KV_State	*state,
	ValueStruct	*args)
{
	ValueStruct	*id;
	gpointer	okey;
	gpointer	oval;
	int	rc;
ENTER_FUNC;
	KV_LockWrite(state);
	rc = MCP_BAD_OTHER;
	if ((id = GetItemLongName(args, "id")) == NULL) {
		rc = MCP_BAD_ARG;
		Warning("does not found id");
	} else {
		if (g_hash_table_lookup_extended(state->table, ValueToString(id,NULL), &okey, &oval)) {
			g_hash_table_remove(state->table, okey);
			g_hash_table_foreach_remove((GHashTable*)oval, RemoveValue, NULL);
			xfree(okey);
			g_hash_table_destroy(oval);
			rc = MCP_OK;
		}
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return rc;
}

static	void
DumpValue(
	gpointer key,
	gpointer value,
	gpointer data)
{
	fprintf((FILE*)data,"  - - \"%s\"\n", (char*)key);
	fprintf((FILE*)data,"    - \"%s\"\n", (char*)value);
}

static	void
DumpEntry(
	gpointer key,
	gpointer value,
	gpointer data)
{
	fprintf((FILE*)data,"\"%s\":\n",(char*)key);
	g_hash_table_foreach((GHashTable*)value,DumpValue,data);
}

extern	int
KV_Dump(
	KV_State	*state,
	ValueStruct	*args)
{
	FILE *fp;
	char buff[256];
	int rc;
	int fd;
	ValueStruct *id;
ENTER_FUNC;
	KV_LockRead(state);
	rc = MCP_BAD_OTHER;
	if ((id = GetItemLongName(args, "id")) == NULL) {
		rc = MCP_BAD_ARG;
		Warning("does not found id");
	} else {
		sprintf(buff, "/tmp/sysdata_dump_XXXXXX");
		if ((fd = mkstemp(buff)) != -1) {
			if ((fp = fdopen(fd, "w")) != NULL) {
				g_hash_table_foreach(state->table, DumpEntry, fp);
				fclose(fp);
				SetValueStringWithLength(id, buff, strlen(buff), NULL);
				rc = MCP_OK;
			} else {
				Warning("fdopne failure");
			}
		} else {
			Warning("mkstemp failure");
		}
	}
	KV_UnLock(state);
LEAVE_FUNC;
	return rc;
}
