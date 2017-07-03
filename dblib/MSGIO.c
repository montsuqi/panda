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
#include	"blobreq.h"
#include	"sysdata.h"
#include	"comm.h"
#include	"comms.h"
#include	"redirect.h"
#include	"message.h"
#include	"debug.h"

#define	NBCONN(dbg)		(NETFILE *)((dbg)->process[PROCESS_UPDATE].conn)
#define CAST_BAD(arg)	(char*)(arg)

typedef enum msg_type {
	MSG_XML = 0,
	MSG_JSON,
	MSG_NONE,
} MSGTYPE;

static gchar*
XMLGetString(
	xmlNodePtr node,
	char *value)
{
	gchar	*buf;
	buf = xmlNodeGetContent(node);
	if (buf == NULL) {
		buf = g_strdup(value);
	}
	return buf;
}

static int
XMLNode2Value(
	ValueStruct	*val,
	xmlNodePtr	root)
{
	int			i;
	xmlNodePtr	node;
    char 		*type;
	char		*buf;

ENTER_FUNC;
	if (val == NULL || root == NULL) {
		Warning("XMLNode2Value val = NULL || root = NULL");
		return MCP_BAD_OTHER;
	} 
	type = xmlGetProp(root,"type");
	if (type == NULL) {
		Warning("XMLNode2Value root type is NULL");
		return MCP_BAD_OTHER;
	}
	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
		if (xmlStrcmp(type, "int") != 0) {
			break;
		}
		buf = XMLGetString(root, "0");
		SetValueStringWithLength(val, buf, strlen(buf), NULL);
		g_free(buf);
		break;
	  case	GL_TYPE_FLOAT:
		if (xmlStrcmp(type, "float") != 0) {
			break;
		}
		buf = XMLGetString(root, "0");
		SetValueStringWithLength(val, buf, strlen(buf), NULL);
		g_free(buf);
		break;
	  case	GL_TYPE_NUMBER:
		if (xmlStrcmp(type, "number") != 0) {
			break;
		}
		buf = XMLGetString(root, "0.0");
		SetValueStringWithLength(val, buf, strlen(buf), NULL);
		g_free(buf);
		break;
	  case	GL_TYPE_BOOL:
		if (xmlStrcmp(type, "bool") != 0) {
			break;
		}
		buf = XMLGetString(root, "FALSE");
		SetValueStringWithLength(val, buf, strlen(buf), NULL);
		g_free(buf);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_SYMBOL:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		if (xmlStrcmp(type, "string") != 0) {
			break;
		}
		buf = XMLGetString(root, "");
		SetValueStringWithLength(val, buf, strlen(buf), NULL);
		g_free(buf);
		break;
	  case	GL_TYPE_ARRAY:
		if (xmlStrcmp(type, "array") != 0) {
			break;
		}
		i = 0;
		for(node=root->children;node!=NULL;node=node->next) {
			if (node->type != XML_ELEMENT_NODE) {
				continue;
			}
			if (i < ValueArraySize(val)) {
				XMLNode2Value(ValueArrayItem(val,i), node);
			} else {
				break;
			}
			i++;
		}
		break;
	  case	GL_TYPE_RECORD:
		if (xmlStrcmp(type, "record") != 0) {
			break;
		}
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			for( node = root->children; node != NULL; node = node->next) {
				if (!xmlStrcmp(node->name, ValueRecordName(val, i))) {
					XMLNode2Value(ValueRecordItem(val,i), node);
					break;
				}
			}
		}
		break;
	  case	GL_TYPE_OBJECT:
		break;
	  default:
		Warning("XMLNode2Value");
		return MCP_BAD_ARG;
		break;
	}
	if (type != NULL) {
		xmlFree(type);
	}
LEAVE_FUNC;
	return MCP_OK;
}

static gboolean
IsEmptyValue(ValueStruct *val)
{
	gboolean ret;
	int i;

	ret = FALSE;
	switch	(ValueType(val)) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_SYMBOL:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		if (strlen(ValueToString(val,NULL))==0) {
			ret = TRUE;
		}
		break;
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
			ret = IsEmptyValue(ValueArrayItem(val,i));
			if (ret == FALSE) {
				break;
			}
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			ret = IsEmptyValue(ValueRecordItem(val,i));
			if (ret == FALSE) {
				break;
			}
		}
		break;
	}
	return ret;
}

static	xmlNodePtr
Value2XMLNode(
	char		*name,
	ValueStruct	*val)
{
	xmlNodePtr	node;
	xmlNodePtr	child;
	char		*type;
	char		*childname;
	int			i;
	gboolean	have_data;

ENTER_FUNC;
	if (val == NULL || name == NULL) {
		Warning("val or name = null,val:%p name:%p",val,name);
		return NULL;
	} 
	node = NULL;
	type = NULL;

	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
		type = "int";
		node =xmlNewNode(NULL, name);
		xmlNodeAddContent(node, ValueToString(val,NULL)); 
		break;
	  case	GL_TYPE_FLOAT:
		type = "float";
		node =xmlNewNode(NULL, name);
		xmlNodeAddContent(node, ValueToString(val,NULL)); 
		break;
	  case	GL_TYPE_NUMBER:
		type = "number";
		node =xmlNewNode(NULL, name);
		xmlNodeAddContent(node, ValueToString(val,NULL)); 
		break;
	  case	GL_TYPE_BOOL:
		type = ("bool");
		node =xmlNewNode(NULL, name);
		xmlNodeAddContent(node, ValueToString(val,NULL)); 
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_SYMBOL:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		type = ("string");
		if (!IsEmptyValue(val)) {
			node =xmlNewNode(NULL, name);
			xmlNodeAddContent(node, ValueToString(val,NULL)); 
		}
		break;
	  case	GL_TYPE_ARRAY:
		have_data = FALSE;
		for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
			if (!IsEmptyValue(ValueArrayItem(val,i))) {
				have_data = TRUE;
			}
		}
		if (have_data) {	
			type = ("array");
			node =xmlNewNode(NULL, name);
        	childname = g_strdup_printf("%s_child",name);
			for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
				if (!IsEmptyValue(ValueArrayItem(val,i))) {
					child = Value2XMLNode(childname, ValueArrayItem(val,i));
					if (child != NULL) {
						xmlAddChildList(node, child);
					}
				}
			}
        	g_free(childname);
		}
		break;
	  case	GL_TYPE_RECORD:
		have_data = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			if (!IsEmptyValue(ValueRecordItem(val,i))) {
				have_data = TRUE;
			}
		}
		if (have_data) {	
			type = ("record");
			node =xmlNewNode(NULL, name);
			for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
				if (!IsEmptyValue(ValueRecordItem(val,i))) {
					child = Value2XMLNode(ValueRecordName(val, i), 
								ValueRecordItem(val,i));
					if (child != NULL) {
						xmlAddChildList(node, child);
					}
				}
			}
		}
		break;
	}
	if (node != NULL && type != NULL) {
		xmlNewProp(node, "type", type);
	}
	return node;
}


static	int
_ReadMSG_XML(
	ValueStruct *ret,
	unsigned char *buff,
	size_t size)
{
	xmlDocPtr	doc;
	xmlNodePtr	node,root;
	ValueStruct *val;
	int rc;

	rc = MCP_BAD_ARG;
	doc = xmlReadMemory(buff,size,"http://www.montsuqi.org/",NULL,XML_PARSE_NOBLANKS|XML_PARSE_NOENT);
	if (doc == NULL) {
		Warning("_ReadXML_XML failure");
		return MCP_BAD_ARG;
	}
	root = xmlDocGetRootElement(doc);
	if (root == NULL || root->children == NULL) {
		return MCP_BAD_ARG;
	}
	node = root->children;
	val = GetRecordItem(ret,"data");
    if (val != NULL) {
		rc = XMLNode2Value(val,node);
	}
	xmlFreeDoc(doc);
	return rc;
}

static	int
_ReadMSG_JSON(
	ValueStruct *ret,
	unsigned char *buff,
	size_t size)
{
	unsigned char *jsonstr;
	ValueStruct *val;
	size_t s;

	jsonstr = g_malloc0(size+1);
	memcpy(jsonstr,buff,size);
	val = GetRecordItem(ret,"data");
	s = JSON_UnPackValue(NULL,jsonstr,val);
	g_free(jsonstr);

	if (s == 0) {
		return MCP_BAD_ARG;
	}
	return MCP_OK;
}

static MSGTYPE
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

static	ValueStruct	*
_ReadMSG(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct		*ret,*obj,*ctype,*data;
	MSGTYPE			type;
	unsigned char	*buff;
	size_t			size;

ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((ctype = GetItemLongName(args,"content_type")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("no [content_type] record");
		return NULL;
	}
	if ((obj = GetItemLongName(args,"object")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("no [object] record");
		return NULL;
	}
	if ((data = GetItemLongName(args,"data")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("no [data] record");
		return NULL;
	}
	type = CheckContentType(ValueToString(ctype,NULL));
	if (type == MSG_NONE) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("invalid content type[%s]",ValueToString(ctype,NULL));
		return NULL;
	}
	if (RequestReadBLOB(NBCONN(dbg),ValueObjectId(obj),&buff,&size) > 0) {
		ret = DuplicateValue(args,TRUE);
		switch(type) {
		case MSG_XML:
			ctrl->rc = _ReadMSG_XML(ret,buff,size);
			break;
		case MSG_JSON:
			ctrl->rc = _ReadMSG_JSON(ret,buff,size);
			break;
		default:
			Warning("not reach");
			break;
		}
		xfree(buff);
	} else {
		Warning("RequestReadBLOB failure");
		return NULL;
	}
LEAVE_FUNC;
	return ret;
}

static	int
_WriteMSG_XML(
	DBG_Struct *dbg,
	ValueStruct *ret)
{
	ValueStruct *val,*obj;
	xmlDocPtr doc;
	xmlNodePtr root,node;
	unsigned char *buff;
	int rc,oid,size;
	size_t wrote;

	rc = MCP_BAD_OTHER;
	obj = GetItemLongName(ret,"object");
	val = GetRecordItem(ret,"data");
	oid = RequestNewBLOB(NBCONN(dbg),BLOB_OPEN_WRITE);
	if (oid == GL_OBJ_NULL) {
		Warning("RequestNewBLOB failure");
		return MCP_BAD_OTHER;
	}
	doc = xmlNewDoc("1.0");
	root = xmlNewDocNode(doc, NULL, "xmlio2", NULL);
	xmlDocSetRootElement(doc,root);
	node = Value2XMLNode("data",val);
	if (node != NULL) {
		xmlAddChildList(root, node);
	} else {
		Warning("node is NULL");
	}
	xmlDocDumpFormatMemoryEnc(doc,&buff,&size,"UTF-8",TRUE);
	if (buff != NULL) {
		wrote = RequestWriteBLOB(NBCONN(dbg),oid,(unsigned char *)buff,size);
		if (wrote == size) {
			ValueObjectId(obj) = oid;
			rc = MCP_OK;
		} else {
			Warning("RequestWriteBLOB failure");
			ValueObjectId(obj) = GL_OBJ_NULL;
			rc = MCP_BAD_OTHER;
		}
	}
	xfree(buff);
	xmlFreeDoc(doc);
	return rc;
}

static	int
_WriteMSG_JSON(
	DBG_Struct *dbg,
	ValueStruct *ret)
{
	ValueStruct *val,*obj;
	char *buff;
	size_t size;
	int rc,oid,wrote;

	obj = GetItemLongName(ret,"object");
	val = GetRecordItem(ret,"data");
	oid = RequestNewBLOB(NBCONN(dbg),BLOB_OPEN_WRITE);
	if (oid == GL_OBJ_NULL) {
		Warning("RequestNewBLOB failure");
		return MCP_BAD_OTHER;
	}
	size = JSON_SizeValueOmmitString(NULL,val);
	buff = g_malloc(size);
	JSON_PackValueOmmitString(NULL,buff,val);
	wrote = RequestWriteBLOB(NBCONN(dbg),oid,buff,size);

	if (wrote == size) {
		ValueObjectId(obj) = oid;
		rc = MCP_OK;
	} else {
		Warning("does not match wrote size %ld:%ld",wrote,size);
		ValueObjectId(obj) = GL_OBJ_NULL;
		rc = MCP_BAD_OTHER;
	}
	return rc;
}

static	ValueStruct	*
_WriteMSG(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ctype,*obj,*data,*ret;
	MSGTYPE type;
ENTER_FUNC;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((ctype = GetItemLongName(args,"content_type")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("no [content_type] record");
		return NULL;
	}
	if ((obj = GetItemLongName(args,"object")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("no [object] record");
		return NULL;
	}
	if ((data = GetItemLongName(args,"data")) == NULL) {
		Warning("no [data] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	type = CheckContentType(ValueToString(ctype,NULL));
	if (type == MSG_NONE) {
		ctrl->rc = MCP_BAD_ARG;
		Warning("invalid content type[%s]",ValueToString(ctype,NULL));
		return NULL;
	}
	ret = DuplicateValue(args,TRUE);
	data = GetItemLongName(ret,"data");
	switch(type) {
	case MSG_XML:
		ctrl->rc = _WriteMSG_XML(dbg,ret);
		break;
	case MSG_JSON:
		ctrl->rc = _WriteMSG_JSON(dbg,ret);
		break;
	default:
		Warning("not reach");
		break;
	}
	InitializeValue(data);
LEAVE_FUNC;
	return ret;
}

extern	ValueStruct	*
MSG_BEGIN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	SYSDATA_DBSTART(dbg,ctrl);
LEAVE_FUNC;
	return	(NULL);
}

extern	ValueStruct	*
MSG_END(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	SYSDATA_DBCOMMIT(dbg,ctrl);
LEAVE_FUNC;
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)SYSDATA_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)SYSDATA_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)MSG_BEGIN },
	{	"DBCOMMIT",		(DB_FUNC)MSG_END },
	/*	table operations	*/
	{	"MSGREAD",		_ReadMSG		},
	{	"MSGWRITE",		_WriteMSG		},

	{	NULL,			NULL }
};

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed,
	int			usage)
{
	return	(MCP_OK);
}

static	ValueStruct	*
_QUERY(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed,
	int			usage)
{
	return NULL;
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

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	_QUERY,
	NULL,
};

extern	DB_Func	*
InitMSGIO(void)
{
	return	(EnterDB_Function("MSGIO",Operations,DB_PARSER_NULL,&Core,"",""));
}

