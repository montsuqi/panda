/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2011 NaCl.
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
#include	<libxml/tree.h>
#include	<libxml/parser.h>

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
#include	"monsys.h"
#include	"bytea.h"
#include	"dbops.h"
#include	"msglib.h"
#include	"message.h"
#include	"debug.h"

typedef enum __MODE {
	MODE_READ = 0,
	MODE_WRITE,
	MODE_WRITE_XML,
	MODE_WRITE_JSON,
	MODE_READ_XML,
	MODE_READ_JSON,
	MODE_NONE,
} _MODE;

enum MSGTYPE {
	MSG_XML = 0,
	MSG_JSON,
	MSG_NONE
};

typedef struct {
	MSGMode mode;
	MonObjectType oid;
	int format;
	int	pos;
	/*for XML*/
	xmlDocPtr doc;
	/*for JSON*/
	json_object *obj;
} _CTX;

static _CTX CTX;
static int PrevMode = MODE_WRITE_XML;

static void
_ResetCTX()
{
	CTX.mode = MODE_NONE;
	CTX.oid = 0;
	CTX.format = MSG_NONE;
	CTX.pos = 0;
	if (CTX.xml_doc != NULL) {
		xmlFreeDoc(CTX.doc);
		CTX.xml_doc = NULL;
	}
	if (CTX.obj != NULL) {
		json_object_put(CTX.obj);
		CTX.obj = NULL;
	}
}

static int
CheckContentType(
	const char* ctype)
{
	gchar *upper;
	MSGTYPE ret;

	ret = MSG_NONE;
	upper = g_utf8_strup(ctype,-1);
	if (strstr(upper,"XML") != NULL) {
		ret = MSG_XML;
	}
	if (strstr(upper,"JSON") != NULL) {
		ret = MSG_JSON;
	}
	g_free(upper);
	return ret;
}

static	int
_ReadXML(
	ValueStruct *ret)
{
	xmlNodePtr	node,root;
	ValueStruct *data;
	char *type;
	int i;

	data = GetItemLongName(ret,"data");
	InitializeValue(data);

	if (CTX.doc == NULL) {
		Warning("invalid xml");
		return MCP_BAD_OTHER;
	}
	root = xmlDocGetRootElement(CTX.doc);
	if (root == NULL) {
		Warning("invalid xml root");
		return MCP_BAD_OTHER;
	}
	type = xmlGetProp(root,"type");
	if (type == NULL) {
		Warning("invalid xml root type:%s",type);
		return MCP_BAD_ARG;
	}
	if (xmlStrcmp(type,"array") != 0) {
		Warning("invalid xml root type:%s",type);
		xmlFree(type);
		return MCP_BAD_ARG;
	}
	xmlFree(type);

	i = 0;
	for(node=root->children;node!=NULL;node=node->next) {
		if (node->type != XML_ELEMENT_NODE) {
			continue;
		}
		if (i == CTX.pos) {
			XMLNode2Value(data,node);
			CTX.pos += 1;
			break;
		} 
		i++;
	}
	if (node == NULL) {
		return MCP_EOF;
	}
	return MCP_OK;
}

static	int
_ReadJSON(
	ValueStruct *ret)
{
	ValueStruct *data;
	json_object *obj;

	data = GetItemLongName(ret,"data");
	InitializeValue(data);
	if (!CheckJSONObject(CTX.obj,json_type_array)) {
		Warning("invalid json object");
		return MCP_BAD_OTHER;
	}
	if (CTX.pos >= json_object_array_length(CTX.obj)) {
		return MCP_EOF;
	}
	obj = json_object_array_get_idx(CTX.obj,CTX.pos);
	CTX.pos += 1;
	if (obj != NULL && is_error(obj)) {
		Warning("invalid json object");
		return MCP_BAD_OTHER;
	}
	JSON_UnPackValue(NULL,json_object_to_json_string(obj),data);
	return MCP_OK;
}


static	ValueStruct	*
_Read(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret,*obj,*ctype,*data;
	MSGTYPE		type;
	char		*buff;
	size_t		size;
	DBG_Struct	*mondbg;

ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;

	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((data = GetItemLongName(args,"data")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("no [data] record");
		return NULL;
	}
	if (CTX.mode != MODE_READ) {
		Warning("invalid mode:%d",CTX.mode);
		return NULL;
	}
	ret = DuplicateValue(args,TRUE);
	switch(CTX.format) {
	case MSG_XML:
		ctrl->rc = _ReadXML(ret);
		break;
	case MSG_JSON:
		ctrl->rc = _ReadJSON(ret);
		break;
	default:
		Warning("invalid format:%d",CTX.format);
		return NULL;
		break;
	}
LEAVE_FUNC;
	return ret;
}

static	int
_WriteXML(
	ValueStruct *ret)
{
	ValueStruct *val;
	xmlNodePtr root,node,box;
	char *name;

	if (CTX.doc == NULL) {
		Warning("not reach");
		return MCP_BAD_OTHER;
	}
	root = xmlDocGetRootElement(CTX.doc);

	val = GetRecordItem(ret,"data");
	node = Value2XMLNode("data",val);
	if (node != NULL) {
		name = g_strdup_printf("%s_child",ValueName(ret));
		box = xmlNewNode(NULL, name);
		g_free(name);
		xmlNewProp(box,"type","record");
		xmlAddChildList(box,node);
		xmlAddChildList(root,box);
	} else {
		Warning("node is NULL");
		return MCP_BAD_OTHER;
	}
	return MCP_OK;
}

static	int
_WriteJSON(
	ValueStruct *ret)
{
	ValueStruct *val;
	unsigned char *buf;
	size_t size;
	json_object *obj;

	if (CTX.obj == NULL) {
		Warning("not reach");
		return MCP_BAD_OTHER;
	}
	
	val = GetRecordItem(ret,"data");
	size = JSON_SizeValueOmmitString(NULL,val);
	buf = g_malloc(size);
	JSON_PackValueOmmitString(NULL,buf,val);
	obj = json_tokener_parse(buf);
	g_free(buf);
	if (is_error(obj)) {
		Warning("_WriteJSON failure");
		return MCP_BAD_ARG;
	}
	json_object_array_add(CTX.obj,obj);
	return MCP_OK;
}

static	ValueStruct	*
_Write(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*data,*ret;
ENTER_FUNC;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if (CTX.mode == MODE_WRITE_XML || CTX.mode == MODE_WRITE_JSON) {
    } else {
		Warning("invalid mode:%d",CTX.mode);
		return NULL;
	}
	if ((data = GetItemLongName(args,"data")) == NULL) {
		Warning("no [data] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	ret = DuplicateValue(args,TRUE);
	data = GetItemLongName(ret,"data");
	switch(CTX.mode) {
	case MODE_WRITE_XML:
		ctrl->rc = _WriteXML(ret);
		break;
	case MODE_WRITE_JSON:
		ctrl->rc = _WritJSON(ret);
		break;
	default:
		Warning("not reach");
		break;
	}
	InitializeValue(data);
LEAVE_FUNC;
	return ret;
}

static int
_OpenXML(
	char *buf,
	size_t size)
{
	CTX.doc = xmlReadMemory(buf,size,"http://www.montsuqi.org/",NULL,XML_PARSE_NOBLANKS|XML_PARSE_NOENT);
	if (doc == NULL) {
		Warning("_ReadXML_XML failure");
		return MCP_BAD_ARG;
	}
	return MCP_OK;
}

static int
_OpenJSON(
	char *buf,
	size_t size)
{
	char *tmp;

	tmp = realloc(buf,size+1);
	if (tmp == NULL) {
		Warning("realloc(3) failure");
		return MCP_BAD_OTHER;
	} else {
		buf = tmp;
		memset(tmp+size,0,1);
	}
	CTX.obj = json_tokener_parse(buf);
	if (is_error(obj)) {
		Warning("_ReadXML_JSON failure");
		return MCP_BAD_ARG;
	}
	if (json_object_get_type(obj) != json_type_array) {
		Warning("invalid json type");
		return MCP_BAD_ARG;
	}
	return MCP_OK;
}

static	ValueStruct	*
_Open(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*oid,*mode;
	xmlNodePtr *root;
	char *buf;
	size_t size;
	int format;

	_ResetCTX();
	ctrl->rc = MCP_BAD_ARG;
	ret = NULL;

	if (rec->type  !=  RECORD_DB) {
		return NULL;
	}
	if ((oid = GetItemLongName(args,"object")) == NULL) {
		Warning("no [object] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((mode = GetItemLongName(args,"mode")) == NULL) {
		Warning("no [mode] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	CTX.mode = ValueInteger(mode);

	if (CTX.mode == MODE_WRITE) {
		CTX.mode = PrevMode;
	}
	switch(CTX.mode) {
	case MODE_WRITE_XML:
		CTX.doc = xmlNewDoc("1.0");
		root = xmlNewDocNode(doc, NULL, ValueName(args), NULL);
		xmlNewProp(root,"type","array");
		xmlDocSetRootElement(doc,root);
		ctrl->rc = MCP_OK;
		break;
	case MODE_WRITE_JSON:
		CTX.obj = json_object_new_array();
		ctrl->rc = MCP_OK;
		break;
	case MODE_READ:
        CTX.oid = ValueObjectId(oid);
		mondbg = GetDBG_monsys();
		if (blob_export_mem(mondbg,CTX.oid,&buf,&size)) {
			if (size > 0) {
				CTX.format = CheckFormat(buf,size);
				switch(CTX.format) {
				case MSG_XML:
					PrevMode = MODE_WRITE_XML;
					ctrl->rc = _OpenXML(buf,size);
					break;
				case MSG_JSON:
					PrevMode = MODE_WRITE_JSON;
					ctrl->rc = _OpenJSON(buf,size);
					break;
				default:
					Warning("not reach");
					break;
				}
			}
			xfree(buf);
		} else {
			Warning("RequestReadBLOB failure");
			ctrl->rc = MCP_BAD_OTHER;
			return NULL;
		}
		break;
	default:
		Warning("not reach here");
		break;
	}
	return	NULL;
}

static	ValueStruct	*
_Close(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct *ret,*oid;
	DBG_Struct *mondbg;
	char *buf;
	size_t size;


	ctrl->rc = MCP_BAD_OTHER;
	ret = NULL;

	if (rec->type  !=  RECORD_DB) {
		return NULL;
	}
	if ((oid = GetItemLongName(args,"object")) == NULL) {
		Warning("no [object] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}

	ret = DuplicateValue(args,TRUE);
	oid = GetItemLongName(ret,"object");
	ValueObjectID(oid) = GL_OBJ_NULL;

	switch(CTX.mode) {
	case MODE_WRITE_XML:
		if (CTX.doc == NULL) {
			Warning("invalid xml context");
			return ret;
		}
		xmlDocDumpFormatMemoryEnc(CTX.doc,&buf,&size,"UTF-8",TRUE);
    	mondbg = GetDBG_monsys();
		ValueObjectId(oid) = blob_import_mem(mondbg,0,"MSGARRAY.xml","application/xml",0,buf,size);
		xmlFree(buf);
		break;
	case MODE_WRITE_JSON:
    	mondbg = GetDBG_monsys();
		buf = json_object_to_json_string(CTX.obj);
		ValueObjectId(oid) = blob_import_mem(mondbg,0,"MSGARRAY.json","application/json",0,buf,strlen(buf));
		break;
	}
	_ResetCTX();
	ctrl->rc = MCP_OK;
	return	ret;
}

extern	ValueStruct	*
_START(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	_ResetCTX();
	return	(NULL);
}

extern	ValueStruct	*
_COMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	_ResetCTX();
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_START },
	{	"DBCOMMIT",		(DB_FUNC)_COMMIT },
	/*	table operations	*/
	{	"MSGARRAYOPEN",		_Open		},
	{	"MSGARRAYREAD",		_Read		},
	{	"MSGARRAYWRITE",	_Write		},
	{	"MSGARRAYCLOSE",	_Close		},

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	_QUERY,
	NULL,
};

extern	DB_Func	*
InitMSGIO(void)
{
	return	(EnterDB_Function("MSGARRAY",Operations,DB_PARSER_NULL,&Core,"",""));
}

