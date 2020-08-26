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
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <json.h>

#include "const.h"
#include "enum.h"
#include "libmondai.h"
#include "directory.h"
#include "wfcdata.h"
#include "dbgroup.h"
#include "monsys.h"
#include "bytea.h"
#include "dbops.h"
#include "msglib.h"
#include "message.h"
#include "debug.h"

typedef enum xml_open_mode {
  MODE_READ = 0,
  MODE_WRITE,
  MODE_WRITE_XML,
  MODE_WRITE_JSON,
  MODE_NONE,
} XMLMode;

static XMLMode PrevMode = MODE_WRITE_XML;

static int _ReadXML(ValueStruct *ret, unsigned char *buff, size_t size) {
  xmlDocPtr doc;
  xmlNodePtr node, root;
  ValueStruct *val, *rname;
  int rc;

  rc = MCP_BAD_ARG;
  doc = xmlReadMemory(buff, size, "http://www.montsuqi.org/", NULL,
                      XML_PARSE_NOBLANKS | XML_PARSE_NOENT);
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
  rname = GetItemLongName(ret, "recordname");
  val = GetRecordItem(ret, CAST_BAD(node->name));
  if (val != NULL) {
    SetValueString(rname, CAST_BAD(node->name), NULL);
    rc = XMLNode2Value(val, node);
  }
  xmlFreeDoc(doc);
  return rc;
}

static int _ReadJSON(ValueStruct *ret, unsigned char *buff, size_t size) {
  unsigned char *jsonstr;
  ValueStruct *val, *rname;
  json_object *obj;
  json_object_iter iter;

  jsonstr = g_malloc0(size + 1);
  memcpy(jsonstr, buff, size);
  obj = json_tokener_parse(jsonstr);
  g_free(jsonstr);
  if (obj == NULL) {
    Warning("_ReadXML_JSON failure");
    return MCP_BAD_ARG;
  }
  if (json_object_get_type(obj) != json_type_object) {
    Warning("invalid json type");
    return MCP_BAD_ARG;
  }
  rname = GetItemLongName(ret, "recordname");
  json_object_object_foreachC(obj, iter) {
    val = GetRecordItem(ret, iter.key);
    if (val != NULL) {
      SetValueString(rname, iter.key, NULL);
      JSON_UnPackValue(NULL,
                       (unsigned char *)json_object_to_json_string_ext(
                           iter.val, JSON_C_TO_STRING_PLAIN),
                       val);
    }
    break;
  }
  json_object_put(obj);
  return MCP_OK;
}

static ValueStruct *_Read(DBG_Struct *dbg, DBCOMM_CTRL *ctrl, RecordStruct *rec,
                          ValueStruct *args) {
  ValueStruct *ret, *val;
  char *buff,*oid;
  size_t size;
  int mode;
  DBG_Struct *mondbg;
  ret = NULL;
  ctrl->rc = MCP_BAD_OTHER;
  if (rec->type != RECORD_DB) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if (rec->type != RECORD_DB) {
    return NULL;
  }
  if ((val = GetItemLongName(args, "object")) == NULL) {
    Warning("no [object] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  oid = ValueObjectId(val);
  if ((val = GetItemLongName(args, "mode")) == NULL) {
    Warning("no [mode] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  mode = ValueInteger(val);
  if (mode != MODE_READ) {
    Warning("invalid mode:%d", mode);
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  mondbg = GetDBG_monsys();
  if (monblob_export_mem(mondbg, oid, &buff, &size)) {
    monblob_delete(mondbg, oid);
    ret = DuplicateValue(args, TRUE);
    if (size > 0) {
      switch (CheckFormat(buff, size)) {
      case MSG_XML:
        PrevMode = MODE_WRITE_XML;
        ctrl->rc = _ReadXML(ret, buff, size);
        break;
      case MSG_JSON:
        PrevMode = MODE_WRITE_JSON;
        ctrl->rc = _ReadJSON(ret, buff, size);
        break;
      default:
        Warning("Invalid fomat(not XML or JSON)");
        break;
      }
    }
    xfree(buff);
  } else {
    Warning("monblob_export_mem failure");
    ctrl->rc = MCP_BAD_OTHER;
    return NULL;
  }
#ifdef TRACE
  DumpValueStruct(ret);
#endif
  return ret;
}

static int _WriteXML(DBG_Struct *dbg, ValueStruct *ret) {
  ValueStruct *rname, *val, *obj;
  xmlDocPtr doc;
  xmlNodePtr root, node;
  unsigned char *buff;
  int rc, size;
  DBG_Struct *mondbg;
  char *id;

  rc = MCP_BAD_OTHER;
  obj = GetItemLongName(ret, "object");
  rname = GetItemLongName(ret, "recordname");
  val = GetRecordItem(ret, ValueToString(rname, NULL));
  doc = xmlNewDoc("1.0");
  root = xmlNewDocNode(doc, NULL, "xmlio2", NULL);
  xmlDocSetRootElement(doc, root);
  node = Value2XMLNode(ValueToString(rname, NULL), val);
  if (node != NULL) {
    xmlAddChildList(root, node);
  } else {
    Warning("node is NULL");
  }
  xmlDocDumpFormatMemoryEnc(doc, &buff, &size, "UTF-8", TRUE);
  if (buff != NULL) {
    mondbg = GetDBG_monsys();
    id = monblob_import_mem(mondbg, NULL, 0, "XMLIO2.xml", "application/xml", 0, buff, size);
    if (id != NULL) {
      SetValueString(obj,id,NULL);
      xfree(id);
      rc = MCP_OK;
    } else {
      Warning("_WriteXML_XML failure");
      rc = MCP_BAD_OTHER;
    }
  }
  xfree(buff);
  xmlFreeDoc(doc);
  return rc;
}

static int _WriteJSON(DBG_Struct *dbg, ValueStruct *ret) {
  ValueStruct *val, *obj;
  char *buff, *rname, *id;
  size_t size;
  int rc;
  json_object *root, *jobj;
  DBG_Struct *mondbg;

  obj = GetItemLongName(ret, "object");
  val = GetItemLongName(ret, "recordname");
  rname = ValueToString(val, NULL);
  val = GetRecordItem(ret, rname);
  size = JSON_SizeValueOmmitString(NULL, val);
  buff = g_malloc(size);
  JSON_PackValueOmmitString(NULL, buff, val);

  jobj = json_tokener_parse(buff);
  g_free(buff);
  if (jobj == NULL) {
    Warning("json_tokener_parse failure");
    return MCP_BAD_OTHER;
  }
  root = json_object_new_object();
  json_object_object_add(root, rname, jobj);
  buff = (char *)json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
  size = strlen(buff);
  mondbg = GetDBG_monsys();
  id = monblob_import_mem(mondbg, NULL, 0, "XMLIO2.json", "application/json", 0, buff, size);
  if (id != NULL) {
    SetValueString(obj,id,NULL);
    xfree(id);
    rc = MCP_OK;
  } else {
    Warning("_WriteXML_JSON failure");
    rc = MCP_BAD_OTHER;
  }
  json_object_put(root);
  return rc;
}

static ValueStruct *_Write(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                           RecordStruct *rec, ValueStruct *args) {
  ValueStruct *rname, *val, *ret;
  int mode;
  ctrl->rc = MCP_BAD_OTHER;
  if (rec->type != RECORD_DB) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if (GetItemLongName(args, "object") == NULL) {
    Warning("no [object] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if ((val = GetItemLongName(args, "mode")) == NULL) {
    Warning("no [mode] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  mode = ValueInteger(val);
  if (mode == MODE_WRITE) {
    mode = PrevMode;
  } else if (mode == MODE_WRITE_XML || mode == MODE_WRITE_JSON) {
  } else {
    Warning("invalid mode:%d", mode);
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if ((rname = GetItemLongName(args, "recordname")) == NULL) {
    Warning("no [recordname] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  val = GetRecordItem(args, ValueToString(rname, NULL));
  if (val == NULL) {
    Warning("no [%s] record", ValueToString(rname, NULL));
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  ret = DuplicateValue(args, TRUE);
  switch (mode) {
  case MODE_WRITE_XML:
    ctrl->rc = _WriteXML(dbg, ret);
    break;
  case MODE_WRITE_JSON:
    ctrl->rc = _WriteJSON(dbg, ret);
    break;
  default:
    Warning("not reach");
    break;
  }
  val = GetRecordItem(ret, ValueToString(rname, NULL));
  InitializeValue(val);
  return ret;
}

extern ValueStruct *XML_BEGIN(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  return (NULL);
}

extern ValueStruct *XML_END(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  return (NULL);
}

static DB_OPS Operations[] = {
    /*	DB operations		*/
    {"DBOPEN", (DB_FUNC)_DBOPEN},
    {"DBDISCONNECT", (DB_FUNC)_DBDISCONNECT},
    {"DBSTART", (DB_FUNC)XML_BEGIN},
    {"DBCOMMIT", (DB_FUNC)XML_END},
    /*	table operations	*/
    {"XMLREAD", _Read},
    {"XMLWRITE", _Write},

    {NULL, NULL}};

static DB_Primitives Core = {
    _EXEC,
    _DBACCESS,
    _QUERY,
    NULL,
};

extern DB_Func *InitXMLIO2(void) {
  return (
      EnterDB_Function("XMLIO2", Operations, DB_PARSER_NULL, &Core, "", ""));
}
