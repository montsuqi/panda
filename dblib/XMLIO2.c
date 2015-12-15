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
#include	<json.h>
#include	<json_object_private.h> /*for json_object_object_foreachC()*/

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

typedef enum xml_open_mode {
	MODE_READ = 0,
	MODE_WRITE,
	MODE_WRITE_XML,
	MODE_WRITE_JSON,
	MODE_NONE,
} XMLMode;

typedef struct {
	XMLMode mode;
	int	pos;
	int obj;
	int num;
} XMLCtx;

static GHashTable *XMLCtxTable = NULL;
static XMLMode PrevMode = MODE_WRITE_XML;

static void
FreeXMLCtx(XMLCtx *ctx)
{
	if (ctx == NULL) {
		return;
	}
	g_free(ctx);
}

static XMLCtx*
NewXMLCtx()
{
	XMLCtx *ctx;
	int i;

	if (XMLCtxTable == NULL) {
		Error("CtxTable = NULL...something wrong");
	}
	ctx = g_new0(XMLCtx,1);
	for(i=0;g_hash_table_lookup(XMLCtxTable,GINT_TO_POINTER(i))!=NULL;i++);
	g_hash_table_insert(XMLCtxTable,GINT_TO_POINTER(i),ctx);
	ctx->num = i;
	return ctx;
}

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

static	ValueStruct	*
_OpenXML(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct		*obj,*mode,*context,*ret;
	XMLCtx			*ctx;

ENTER_FUNC;
	ctrl->rc = MCP_BAD_ARG;
	ret = NULL;
	
	if (rec->type  !=  RECORD_DB) {
		return NULL;
	}
	if ((obj = GetItemLongName(args,"object")) == NULL) {
		Warning("no [object] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((mode = GetItemLongName(args,"mode")) == NULL) {
		Warning("no [mode] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((context = GetItemLongName(args,"context")) == NULL) {
		Warning("no [context] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}

	ctx = NewXMLCtx();
	ValueInteger(context) = ctx->num;
	ret = DuplicateValue(args,TRUE);
	ctx->mode = ValueInteger(mode);
	switch(ctx->mode) {
	case MODE_WRITE:
	case MODE_WRITE_XML:
	case MODE_WRITE_JSON:
		break;
	case MODE_READ:
		ctx->obj = ValueObjectId(obj);
		break;
	default:
		Warning("not reach here");
		break;
	}
	ctrl->rc = MCP_OK;
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
_CloseXML(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
ENTER_FUNC;
	ctrl->rc = MCP_OK;
LEAVE_FUNC;
	return	NULL;
}

static	XMLMode
CheckFormat(
	char	*buff,
	size_t	size)
{
	int i;
	char *p;

	p = buff;
	for(i=0;i<size;i++) {
		if (*p == '<') {
			return MODE_WRITE_XML;
		}
		if (*p == '{') {
			return MODE_WRITE_JSON;
		}
		p++;
	}
	return MODE_NONE;
}

static	int
_ReadXML_XML(
	ValueStruct *ret,
	unsigned char *buff,
	size_t size)
{
	xmlDocPtr	doc;
	xmlNodePtr	node,root;
	ValueStruct *val,*rname;
	int rc;

	rc = MCP_BAD_ARG;
	doc = xmlReadMemory(buff,size,"http://www.montsuqi.org/",NULL,XML_PARSE_NOBLANKS|XML_PARSE_NOENT);
	if (doc == NULL) {
		Warning("_ReadXML_XML failure");
		return MCP_BAD_ARG;
	}
	root = xmlDocGetRootElement(doc);
	if (root == NULL || root->children == NULL) {
		Warning("root or root children is null");
		return MCP_BAD_ARG;
	}
	node = root->children;
	rname = GetItemLongName(ret,"recordname");
	val = GetRecordItem(ret,CAST_BAD(node->name));
    if (val != NULL) {
		SetValueString(rname,CAST_BAD(node->name),NULL);
		rc = XMLNode2Value(val,node);
	}
	xmlFreeDoc(doc);
	return rc;
}

static	int
_ReadXML_JSON(
	ValueStruct *ret,
	unsigned char *buff,
	size_t size)
{
	unsigned char *jsonstr;
	ValueStruct *val,*rname;
	json_object *obj;
	json_object_iter iter;

	jsonstr = g_malloc0(size+1);
	memcpy(jsonstr,buff,size);
	obj = json_tokener_parse(jsonstr);
	g_free(jsonstr);
	if (is_error(obj)) {
		Warning("_ReadXML_JSON failure");
		return MCP_BAD_ARG;
	}
	if (json_object_get_type(obj) != json_type_object) {
		Warning("invalid json type");
		return MCP_BAD_ARG;
	}
	rname = GetItemLongName(ret,"recordname");
	json_object_object_foreachC(obj,iter) {
		val = GetRecordItem(ret,iter.key);
		if (val != NULL) {
			SetValueString(rname,iter.key,NULL);
			JSON_UnPackValueOmmit(NULL,(unsigned char*)json_object_to_json_string(iter.val),val);
		}
		break;
	}
	json_object_put(obj);
	return MCP_OK;
}

static	ValueStruct	*
_ReadXML(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct		*ret,*context;
	XMLCtx			*ctx;
	unsigned char	*buff;
	size_t			size;

ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((context = GetItemLongName(args,"context"))  ==  NULL) {
		Warning("no [context] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	ctx = (XMLCtx*)g_hash_table_lookup(XMLCtxTable,GINT_TO_POINTER(ValueInteger(context)));
	if (ctx == NULL || ctx->mode != MODE_READ) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if (RequestReadBLOB(NBCONN(dbg),ctx->obj,&buff,&size) > 0) {
		ret = DuplicateValue(args,TRUE);
		if (size > 0) {
			PrevMode = CheckFormat(buff,size);
			switch(PrevMode) {
			case MODE_WRITE_XML:
				ctrl->rc = _ReadXML_XML(ret,buff,size);
				break;
			case MODE_WRITE_JSON:
				ctrl->rc = _ReadXML_JSON(ret,buff,size);
				break;
			default:
				Warning("not reach");
				break;
			}
		}
		xfree(buff);
	} else {
		Warning("RequestReadBLOB failure");
		return NULL;
	}
#ifdef TRACE
	DumpValueStruct(ret);
#endif
LEAVE_FUNC;
	return ret;
}

static	int
_WriteXML_XML(
	DBG_Struct *dbg,
	XMLCtx *ctx,
	ValueStruct *ret)
{
	ValueStruct *rname,*val,*obj;
	xmlDocPtr doc;
	xmlNodePtr root,node;
	unsigned char *buff;
	int rc,oid,size;
	size_t wrote;

	rc = MCP_BAD_OTHER;
	obj = GetItemLongName(ret,"object");
	rname = GetItemLongName(ret,"recordname");
	val = GetRecordItem(ret,ValueToString(rname,NULL));
	oid = RequestNewBLOB(NBCONN(dbg),BLOB_OPEN_WRITE);
	if (oid == GL_OBJ_NULL) {
		Warning("RequestNewBLOB failure");
		return MCP_BAD_OTHER;
	}
	doc = xmlNewDoc("1.0");
	root = xmlNewDocNode(doc, NULL, "xmlio2", NULL);
	xmlDocSetRootElement(doc,root);
	node = Value2XMLNode(ValueToString(rname,NULL),val);
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
_WriteXML_JSON(
	DBG_Struct *dbg,
	XMLCtx *ctx,
	ValueStruct *ret)
{
	ValueStruct *val,*obj;
	char *buff,*rname;
	size_t size;
	int rc,oid,wrote;
	json_object *root,*jobj;

	obj = GetItemLongName(ret,"object");
	val = GetItemLongName(ret,"recordname");
	rname = ValueToString(val,NULL);
	val = GetRecordItem(ret,rname);
	oid = RequestNewBLOB(NBCONN(dbg),BLOB_OPEN_WRITE);
	if (oid == GL_OBJ_NULL) {
		Warning("RequestNewBLOB failure");
		return MCP_BAD_OTHER;
	}
	size = JSON_SizeValueOmmit(NULL,val);
	buff = g_malloc(size);
	JSON_PackValueOmmit(NULL,buff,val);

	jobj = json_tokener_parse(buff);
	g_free(buff);
	if (is_error(jobj)) {
		Warning("json_tokener_parse failure");
		return MCP_BAD_OTHER;
	}
	root = json_object_new_object();
	json_object_object_add(root,rname,jobj);
	buff = (char*)json_object_to_json_string(root);
	size = strlen(buff);

	wrote = RequestWriteBLOB(NBCONN(dbg),oid,buff,size);
	json_object_put(root);
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
_WriteXML(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*rname,*val,*context,*obj,*ret;
	XMLCtx		*ctx;
	int 		ctxnum;
ENTER_FUNC;
	ctrl->rc = MCP_BAD_OTHER;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((context = GetItemLongName(args,"context"))  ==  NULL) {
		Warning("no [context] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	ctxnum = ValueInteger(context);
	ctx = (XMLCtx*)g_hash_table_lookup(XMLCtxTable,GINT_TO_POINTER(ctxnum));
	if (ctx == NULL || ctx->mode == MODE_READ) {
		Warning("ctx is null or invalid READ mode");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((obj = GetItemLongName(args,"object"))  ==  NULL) {
		Warning("no [object] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((rname = GetItemLongName(args,"recordname"))  ==  NULL) {
		Warning("no [recordname] record");
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	val = GetRecordItem(args,ValueToString(rname,NULL));
	if (val == NULL) {
		Warning("no [%s] record",ValueToString(rname,NULL));
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	ret = DuplicateValue(args,TRUE);
	if (ctx->mode == MODE_WRITE) {
		ctx->mode = PrevMode;
	}
	switch(ctx->mode) {
	case MODE_WRITE_XML:
		ctrl->rc = _WriteXML_XML(dbg,ctx,ret);
		break;
	case MODE_WRITE_JSON:
		ctrl->rc = _WriteXML_JSON(dbg,ctx,ret);
		break;
	default:
		Warning("not reach");
		break;
	}
	FreeXMLCtx(ctx);
	g_hash_table_remove(XMLCtxTable,GINT_TO_POINTER(ctxnum));
	val = GetRecordItem(ret,ValueToString(rname,NULL));
	InitializeValue(val);
LEAVE_FUNC;
	return ret;
}

extern	ValueStruct	*
XML_BEGIN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	SYSDATA_DBSTART(dbg,ctrl);
    if (XMLCtxTable == NULL){
		XMLCtxTable = g_hash_table_new(g_direct_hash,g_direct_equal);
	}
LEAVE_FUNC;
	return	(NULL);
}

static gboolean
EachRemoveCtx(gpointer key,
	gpointer value,
	gpointer data)
{
	FreeXMLCtx((XMLCtx*)value);
	return TRUE;
}

extern	ValueStruct	*
XML_END(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	SYSDATA_DBCOMMIT(dbg,ctrl);
	g_hash_table_foreach_remove(XMLCtxTable,EachRemoveCtx,NULL);
LEAVE_FUNC;
	return	(NULL);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
	{	"DBOPEN",		(DB_FUNC)SYSDATA_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)SYSDATA_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)XML_BEGIN },
	{	"DBCOMMIT",		(DB_FUNC)XML_END },
	/*	table operations	*/
	{	"XMLOPEN",		_OpenXML		},
	{	"XMLREAD",		_ReadXML		},
	{	"XMLWRITE",		_WriteXML		},
	{	"XMLCLOSE",		_CloseXML		},

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
InitXMLIO2(void)
{
	return	(EnterDB_Function("XMLIO2",Operations,DB_PARSER_NULL,&Core,"",""));
}

