/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 NaCl.
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
#include	<errno.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<signal.h>
#include	<time.h>
#include	<sys/time.h>
#include	<json.h>

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
#include	"dbops.h"
#include	"monsys.h"
#include	"monpushevent.h"
#include	"message.h"
#include	"debug.h"

static json_object *PushData = NULL;

static	ValueStruct	*
_PushEvent(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	json_object *obj;

	ctrl->rc = MCP_BAD_OTHER;
	if (!CheckJSONObject(PushData,json_type_array)) {
		Warning("PushData is wrong");
		return NULL;
	}

	obj = push_event_conv_value(args);
	if (CheckJSONObject(obj,json_type_object)) {
		json_object_array_add(PushData,obj);
	}

	ctrl->rc = MCP_OK;
	return	NULL;
}

extern	ValueStruct	*
_PushEventStart(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if (PushData != NULL) {
		json_object_put(PushData);
		PushData = NULL;
	}
	PushData = json_object_new_array();
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
_PushEventCommit(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	DBG_Struct *mondbg;
	json_object *obj,*body,*event;
	int i,len;

	if (ctrl == NULL) {
		return NULL;
	}
	if (PushData == NULL) {
		Warning("invalid COMMIT.BEGIN forget?");
		return NULL;
	}
	ctrl->rc = MCP_OK;
	mondbg = GetDBG_monsys();
	len = json_object_array_length(PushData);
	for (i=0;i<len;i++) {
		obj = json_object_array_get_idx(PushData,i);
		if (!json_object_object_get_ex(obj,"body",&body)) {
			continue;
		}
		if (!json_object_object_get_ex(obj,"event",&event)) {
			continue;
		}
		if (!push_event_via_json(mondbg,(const char*)json_object_get_string(event),body)) {
			ctrl->rc = MCP_BAD_OTHER;
		}
	}

	json_object_put(PushData);
	PushData = NULL;
LEAVE_FUNC;
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_PushEventStart },
	{	"DBCOMMIT",		(DB_FUNC)_PushEventCommit },
	/*	table operations	*/
	{	"PUSHEVENT",	_PushEvent		},

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	_QUERY,
	NULL,
};

extern	DB_Func	*
InitPushEvent(void)
{
	return	(EnterDB_Function("PushEvent",Operations,DB_PARSER_NULL,&Core,"",""));
}

