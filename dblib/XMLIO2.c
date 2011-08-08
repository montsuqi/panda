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

typedef enum xml_open_mode {
	MODE_READ = 0,
	MODE_WRITE,
	MODE_NONE,
} XMLMode;

typedef struct {
	xmlDocPtr doc;
	XMLMode mode;
	int	pos;
	int obj;
	int num;
} XMLCtx;

static GHashTable *XMLCtxTable;

static void
FreeXMLCtx(XMLCtx *ctx)
{
	if (ctx == NULL) {
		return;
	}
	if (ctx->doc != NULL) {
		xmlFreeDoc(ctx->doc);
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

static	void
ResetArgValue(ValueStruct *val)
{
	int i;
	gchar *name;

	if (val == NULL || IS_VALUE_RECORD(val)) {
		return ;
	}
	for(i=0;i<ValueRecordSize(val);i++) {
		name = ValueRecordName(val,i);
		if (g_strcmp0(name,"context") &&
			g_strcmp0(name,"object") &&
			g_strcmp0(name,"mode") &&
			g_strcmp0(name,"recordname")) {
			InitializeValue(ValueRecordItem(val,i));
		}
	}
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
		return MCP_BAD_OTHER;
	} 
	type = xmlGetProp(root,"type");
	if (type == NULL) {
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
	int			rc;
	ValueStruct	*obj;
	ValueStruct	*mode;
	ValueStruct	*context;
	ValueStruct	*ret;
	unsigned char		*buff;
	size_t		size;
	xmlNodePtr	root;
	XMLCtx *ctx;

ENTER_FUNC;
	rc = MCP_BAD_OTHER;
	ret = NULL;
	
	ResetArgValue(args);
	if (rec->type  !=  RECORD_DB) {
		rc = MCP_BAD_ARG;
	} else {
		if ((obj = GetItemLongName(args,"object"))  !=  NULL &&
			(mode = GetItemLongName(args,"mode"))  !=  NULL &&
			(context = GetItemLongName(args,"context"))  !=  NULL
		) {
			ctx = NewXMLCtx();
			ValueInteger(context) = ctx->num;
			ret = DuplicateValue(args, TRUE);
			ctx->mode = ValueInteger(mode);
			if ( ctx->mode == MODE_WRITE) {
				ctx->obj = RequestNewBLOB(NBCONN(dbg),BLOB_OPEN_WRITE);
				if (ctx->obj != GL_OBJ_NULL) {
					ValueObjectId(obj) = ctx->obj;
					ctx->doc = xmlNewDoc("1.0");
					root = xmlNewDocNode(ctx->doc, NULL, "xmlio2", NULL);
					xmlDocSetRootElement(ctx->doc, root);
					rc = MCP_OK;
				}
			} else {
				ctx->obj = ValueObjectId(obj);
				if(RequestReadBLOB(NBCONN(dbg),ctx->obj, &buff, &size) > 0) {
					if (size > 0) {
						ctx->doc = xmlReadMemory(buff, size, "http://www.montsuqi.org/", NULL, XML_PARSE_NOBLANKS|XML_PARSE_NOENT);
						if (ctx->doc != NULL) {
							rc = MCP_OK;
						}
					}
					xfree(buff);
				}
			}
		} else {
			rc = MCP_BAD_ARG;
		}
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
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
	ValueStruct	*obj;
	ValueStruct *context;
	ValueStruct	*ret;
	xmlChar 	*buff;
	int			size;
	int			wrote;
	XMLCtx		*ctx;

ENTER_FUNC;
	buff = NULL;
	ret = NULL;
	ResetArgValue(args);
	if (rec->type  !=  RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((obj = GetItemLongName(args,"object")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((context = GetItemLongName(args,"context")) == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	ctx = (XMLCtx*)g_hash_table_lookup(XMLCtxTable,GINT_TO_POINTER(ValueInteger(context)));
	if (ctx == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}

	ctrl->rc = MCP_OK;
	if (ctx->mode == MODE_WRITE && ctx->doc != NULL) {
		xmlDocDumpFormatMemoryEnc(ctx->doc, &buff, &size, "UTF-8", TRUE);
		if (buff != NULL && ctx->obj != GL_OBJ_NULL) {
			wrote = RequestWriteBLOB(NBCONN(dbg),ctx->obj, 
						(unsigned char *)buff, size);
			if (wrote == size) {
				ValueObjectId(obj) = ctx->obj;
				ret = DuplicateValue(args, TRUE);
			} else {
				ctrl->rc = MCP_BAD_OTHER;
			}
		}
		xfree(buff);
	}
	FreeXMLCtx(ctx);
	g_hash_table_remove(XMLCtxTable,GINT_TO_POINTER(ValueInteger(context)));
LEAVE_FUNC;
	return	ret;
}

static	ValueStruct	*
_ReadXML(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	xmlNodePtr	node;
	xmlNodePtr	root;
	ValueStruct	*context;
	ValueStruct	*ret;
	ValueStruct	*recordname;
	ValueStruct	*val;
	XMLCtx		*ctx;
	int			i;

ENTER_FUNC;
	ret = NULL;
	ctrl->rc = MCP_BAD_OTHER;
	ResetArgValue(args);
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((recordname = GetItemLongName(args,"recordname"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((context = GetItemLongName(args,"context"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	ctx = (XMLCtx*)g_hash_table_lookup(XMLCtxTable,
		GINT_TO_POINTER(ValueInteger(context)));
	if (ctx == NULL || ctx->mode != MODE_READ) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	root = xmlDocGetRootElement(ctx->doc);
	if (root == NULL || root->children == NULL) {
		return NULL;
	}
	for (node=root->children, i=0; node != NULL; node=node->next, i++) {
		root = NULL;
		if (i == ctx->pos) {
			root = node;
			break;
		}	
	}
	ctx->pos += 1;
	if (root == NULL) {
		return NULL;
	}
	ret = DuplicateValue(args,FALSE);
	recordname = GetItemLongName(ret,"recordname");
	val = GetRecordItem(ret,CAST_BAD(root->name));
    if (val != NULL) {
		SetValueString(recordname,CAST_BAD(root->name),NULL);
		ctrl->rc = XMLNode2Value(val, root);
    } else {
		SetValueString(recordname,"",NULL);
		ctrl->rc = MCP_OK;
	}
#ifdef TRACE
	DumpValueStruct(ret);
#endif
LEAVE_FUNC;
	return	(ret);
}

static	ValueStruct	*
_WriteXML(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	ValueStruct	*ret;
	ValueStruct *recordname;
    ValueStruct *val;
	ValueStruct	*context;
	xmlNodePtr	root;
	xmlNodePtr	node;
	XMLCtx		*ctx;
ENTER_FUNC;
	ret = NULL;
	if (rec->type != RECORD_DB) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((context = GetItemLongName(args,"context"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	ctx = (XMLCtx*)g_hash_table_lookup(XMLCtxTable,
		GINT_TO_POINTER(ValueInteger(context)));
	if (ctx == NULL || ctx->mode != MODE_WRITE) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	if ((recordname = GetItemLongName(args,"recordname"))  ==  NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
	}
	val = GetRecordItem(args,ValueToString(recordname,NULL));
    if (val == NULL) {
		ctrl->rc = MCP_BAD_ARG;
		return NULL;
    }
	root = xmlDocGetRootElement(ctx->doc);
	node = Value2XMLNode(ValueToString(recordname,NULL), val);
	if (node != NULL) {
		xmlAddChildList(root, node);
	}
	ResetArgValue(args);
	ctrl->rc = MCP_OK;
LEAVE_FUNC;
	return	(ret);
}

extern	ValueStruct	*
XML_BEGIN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	SYSDATA_DBSTART(dbg,ctrl);
	XMLCtxTable = g_hash_table_new(g_direct_hash,g_direct_equal);
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
    g_hash_table_destroy(XMLCtxTable);
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

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL,
};

extern	DB_Func	*
InitXMLIO2(void)
{
	return	(EnterDB_Function("XMLIO2",Operations,DB_PARSER_NULL,&Core,"",""));
}

