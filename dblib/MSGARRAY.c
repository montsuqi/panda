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

typedef enum __MODE {
  MODE_READ = 0,
  MODE_WRITE,
  MODE_WRITE_XML,
  MODE_WRITE_JSON,
  MODE_READ_XML,
  MODE_READ_JSON,
  MODE_NONE,
} _MODE;

typedef struct {
  int mode;
  int format;
  int pos;
  /*for XML*/
  xmlDocPtr doc;
  /*for JSON*/
  json_object *obj;
} _CTX;

static _CTX CTX;
static int PrevMode = MODE_WRITE_XML;

static void _ResetCTX() {
  CTX.mode = MODE_NONE;
  CTX.format = MSG_NONE;
  CTX.pos = 0;
  if (CTX.doc != NULL) {
    xmlFreeDoc(CTX.doc);
    CTX.doc = NULL;
  }
  if (CTX.obj != NULL) {
    json_object_put(CTX.obj);
    CTX.obj = NULL;
  }
}

static int _ReadXML(ValueStruct *ret) {
  xmlNodePtr node, root;
  ValueStruct *data;
  char *type;
  int i;

  data = GetItemLongName(ret, "data");
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
  type = xmlGetProp(root, "type");
  if (type == NULL) {
    Warning("invalid xml root type:%s", type);
    return MCP_BAD_ARG;
  }
  if (xmlStrcmp(type, "array") != 0) {
    Warning("invalid xml root type:%s", type);
    xmlFree(type);
    return MCP_BAD_ARG;
  }
  xmlFree(type);

  i = 0;
  for (node = root->children; node != NULL; node = node->next) {
    if (node->type != XML_ELEMENT_NODE) {
      continue;
    }
    if (i == CTX.pos) {
      XMLNode2Value(data, node);
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

static int _ReadJSON(ValueStruct *ret) {
  ValueStruct *data;
  json_object *obj;

  data = GetItemLongName(ret, "data");
  InitializeValue(data);
  if (!CheckJSONObject(CTX.obj, json_type_array)) {
    Warning("invalid json object");
    return MCP_BAD_OTHER;
  }
  if (CTX.pos >= json_object_array_length(CTX.obj)) {
    return MCP_EOF;
  }
  obj = json_object_array_get_idx(CTX.obj, CTX.pos);
  CTX.pos += 1;
  if (obj == NULL) {
    Warning("invalid json object");
    return MCP_BAD_OTHER;
  }
  JSON_UnPackValue(
      NULL, (char *)json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN),
      data);
  return MCP_OK;
}

static ValueStruct *_Read(DBG_Struct *dbg, DBCOMM_CTRL *ctrl, RecordStruct *rec,
                          ValueStruct *args) {
  ValueStruct *ret, *data;
  ret = NULL;
  ctrl->rc = MCP_BAD_OTHER;

  if (rec->type != RECORD_DB) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if ((data = GetItemLongName(args, "data")) == NULL) {
    ctrl->rc = MCP_BAD_ARG;
    Warning("no [data] record");
    return NULL;
  }
  if (CTX.mode != MODE_READ) {
    Warning("invalid mode:%d", CTX.mode);
    return NULL;
  }
  ret = DuplicateValue(args, TRUE);
  switch (CTX.format) {
  case MSG_XML:
    ctrl->rc = _ReadXML(ret);
    break;
  case MSG_JSON:
    ctrl->rc = _ReadJSON(ret);
    break;
  default:
    Warning("invalid format:%d", CTX.format);
    return NULL;
    break;
  }
  return ret;
}

static int _WriteXML(ValueStruct *ret) {
  ValueStruct *val;
  xmlNodePtr root, node, record;
  char *name;

  if (CTX.doc == NULL) {
    Warning("not reach");
    return MCP_BAD_OTHER;
  }
  root = xmlDocGetRootElement(CTX.doc);

  val = GetRecordItem(ret, "data");
  node = Value2XMLNode("data", val);
  if (node != NULL) {
    name = g_strdup_printf("%s_child", ValueRootRecordName(ret));
    record = xmlNewNode(NULL, name);
    g_free(name);
    xmlNewProp(record, "type", "record");
    xmlAddChildList(record, node);
    xmlAddChildList(root, record);
  } else {
    Warning("node is NULL");
    return MCP_BAD_OTHER;
  }
  return MCP_OK;
}

static int _WriteJSON(ValueStruct *ret) {
  ValueStruct *val;
  unsigned char *buf;
  size_t size;
  json_object *obj;

  if (CTX.obj == NULL) {
    Warning("not reach");
    return MCP_BAD_OTHER;
  }

  val = GetRecordItem(ret, "data");
  size = JSON_SizeValueOmmitString(NULL, val);
  buf = g_malloc(size);
  JSON_PackValueOmmitString(NULL, buf, val);
  obj = json_tokener_parse(buf);
  g_free(buf);
  if (obj == NULL) {
    Warning("_WriteJSON failure");
    return MCP_BAD_ARG;
  }
  json_object_array_add(CTX.obj, obj);
  return MCP_OK;
}

static ValueStruct *_Write(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                           RecordStruct *rec, ValueStruct *args) {
  ValueStruct *data, *ret;
  ctrl->rc = MCP_BAD_OTHER;
  if (rec->type != RECORD_DB) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if (CTX.mode == MODE_WRITE_XML || CTX.mode == MODE_WRITE_JSON) {
  } else {
    Warning("invalid mode:%d", CTX.mode);
    return NULL;
  }
  if ((data = GetItemLongName(args, "data")) == NULL) {
    Warning("no [data] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  ret = DuplicateValue(args, TRUE);
  data = GetItemLongName(ret, "data");
  switch (CTX.mode) {
  case MODE_WRITE_XML:
    ctrl->rc = _WriteXML(ret);
    break;
  case MODE_WRITE_JSON:
    ctrl->rc = _WriteJSON(ret);
    break;
  default:
    Warning("not reach");
    break;
  }
  InitializeValue(data);
  return ret;
}

static int _OpenXML(char *buf, size_t size) {
  CTX.doc = xmlReadMemory(buf, size, "http://www.montsuqi.org/", NULL,
                          XML_PARSE_NOBLANKS | XML_PARSE_NOENT);
  if (CTX.doc == NULL) {
    Warning("xmlREadMemory failure");
    return MCP_BAD_ARG;
  }
  return MCP_OK;
}

static int _OpenJSON(char *buf, size_t size) {
  char *buf2;

  buf2 = g_malloc0(size + 1);
  memcpy(buf2, buf, size);
  CTX.obj = json_tokener_parse(buf2);
  g_free(buf2);
  if (CTX.obj == NULL) {
    Warning("json_tokener_parse failure");
    return MCP_BAD_ARG;
  }
  if (json_object_get_type(CTX.obj) != json_type_array) {
    Warning("invalid json type");
    return MCP_BAD_ARG;
  }
  return MCP_OK;
}

static ValueStruct *_Open(DBG_Struct *dbg, DBCOMM_CTRL *ctrl, RecordStruct *rec,
                          ValueStruct *args) {
  ValueStruct *oid, *mode;
  xmlNodePtr root;
  DBG_Struct *mondbg;
  char *buf;
  size_t size;

  _ResetCTX();
  ctrl->rc = MCP_BAD_ARG;

  if (rec->type != RECORD_DB) {
    return NULL;
  }
  if ((oid = GetItemLongName(args, "object")) == NULL) {
    Warning("no [object] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if ((mode = GetItemLongName(args, "mode")) == NULL) {
    Warning("no [mode] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  CTX.mode = ValueInteger(mode);

  if (CTX.mode == MODE_WRITE) {
    CTX.mode = PrevMode;
  }
  switch (CTX.mode) {
  case MODE_WRITE_XML:
    CTX.doc = xmlNewDoc("1.0");
    root = xmlNewDocNode(CTX.doc, NULL, ValueRootRecordName(args), NULL);
    xmlNewProp(root, "type", "array");
    xmlDocSetRootElement(CTX.doc, root);
    ctrl->rc = MCP_OK;
    break;
  case MODE_WRITE_JSON:
    CTX.obj = json_object_new_array();
    ctrl->rc = MCP_OK;
    break;
  case MODE_READ:
    mondbg = GetDBG_monsys();
    if (monblob_export_mem(mondbg, ValueObjectId(oid), &buf, &size)) {
      monblob_delete(mondbg, ValueObjectId(oid));
      if (size > 0) {
        CTX.format = CheckFormat(buf, size);
        switch (CTX.format) {
        case MSG_XML:
          PrevMode = MODE_WRITE_XML;
          ctrl->rc = _OpenXML(buf, size);
          break;
        case MSG_JSON:
          PrevMode = MODE_WRITE_JSON;
          ctrl->rc = _OpenJSON(buf, size);
          break;
        default:
          Warning("Invalid format(not XML or JSON)");
          break;
        }
      }
      free(buf);
    } else {
      Warning("monblob_export_mem failure");
      ctrl->rc = MCP_BAD_OTHER;
      return NULL;
    }
    break;
  default:
    Warning("not reach here");
    break;
  }
  return NULL;
}

static ValueStruct *_Close(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                           RecordStruct *rec, ValueStruct *args) {
  ValueStruct *ret, *oid;
  DBG_Struct *mondbg;
  xmlChar *buf;
  char *id;
  int size;

  ctrl->rc = MCP_BAD_OTHER;
  ret = NULL;

  if (rec->type != RECORD_DB) {
    return NULL;
  }
  if ((oid = GetItemLongName(args, "object")) == NULL) {
    Warning("no [object] record");
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }

  ret = DuplicateValue(args, TRUE);
  oid = GetItemLongName(ret, "object");

  switch (CTX.mode) {
  case MODE_WRITE_XML:
    if (CTX.doc == NULL) {
      Warning("invalid xml context");
      return ret;
    }
    xmlDocDumpFormatMemoryEnc(CTX.doc, &buf, &size, "UTF-8", TRUE);
    mondbg = GetDBG_monsys();
    id = monblob_import_mem(mondbg, NULL, 0, "MSGARRAY.xml", "application/xml", 0, buf, size);
    if (id != NULL) {
      SetValueString(oid, id, NULL);
      xfree(id);
    }
    xmlFree(buf);
    break;
  case MODE_WRITE_JSON:
    mondbg = GetDBG_monsys();
    buf =
        (char *)json_object_to_json_string_ext(CTX.obj, JSON_C_TO_STRING_PLAIN);
    id = monblob_import_mem(mondbg, NULL, 0, "MSGARRAY.json", "application/json", 0, buf, strlen(buf));
    if (id != NULL) {
      SetValueString(oid, id, NULL);
      xfree(id);
    }
    break;
  }
  _ResetCTX();
  ctrl->rc = MCP_OK;
  return ret;
}

static int _UnEscapeXML() {
  Warning("not implement");
  return MCP_BAD_OTHER;
}

static gboolean eval_cb(const GMatchInfo *info, GString *res, gpointer data) {
  gchar *match;

  match = g_match_info_fetch(info, 0);
  if (!strcmp(match, "\\\"")) {
    g_string_append(res, "\"");
  } else if (!strcmp(match, "\\\\")) {
    g_string_append(res, "\\");
  } else if (!strcmp(match, "\\/")) {
    g_string_append(res, "/");
  } else if (!strcmp(match, "\\b")) {
    g_string_append(res, "\b");
  } else if (!strcmp(match, "\\f")) {
    g_string_append(res, "\f");
  } else if (!strcmp(match, "\\n")) {
    g_string_append(res, "\n");
  } else if (!strcmp(match, "\\r")) {
    g_string_append(res, "\r");
  } else if (!strcmp(match, "\\t")) {
    g_string_append(res, "\t");
  }
  g_free(match);
  return FALSE;
}

json_object *__UnEscapeJSONString(json_object *obj) {
  GRegex *reg;
  int compile_op, match_op;
  const char *pat, *str;
  char *res;
  json_object *newobj;

  compile_op = G_REGEX_DOTALL | G_REGEX_MULTILINE;
  match_op = 0;
  pat = "^\\s*({.*}|\\[.*\\])\\s*$";
  str = json_object_get_string(obj);
  if (!g_regex_match_simple(pat, str, compile_op, match_op)) {
    return NULL;
  }
  reg = g_regex_new(""
                    "\\\\\"|"
                    "\\\\\\\\|"
                    "\\\\/|"
                    "\\\\b|"
                    "\\\\f|"
                    "\\\\n|"
                    "\\\\r|"
                    "\\\\t"
                    "",
                    0, 0, NULL);
  res = g_regex_replace_eval(reg, str, -1, 0, 0, eval_cb, NULL, NULL);
  g_regex_unref(reg);
  if (res == NULL) {
    return NULL;
  }
  newobj = json_tokener_parse(res);
  g_free(res);

  return newobj;
}

static void __UnEscapeJSON(json_object *obj, json_object *parent,
                           const char *key, int idx) {
  json_object *newobj;
  json_object_iter iter;
  json_type type;
  char *newkey;
  int i, l;

  if (obj == NULL) {
    Warning("something wrong");
    return;
  }

  type = json_object_get_type(obj);
  switch (type) {
  case json_type_boolean:
  case json_type_double:
  case json_type_int:
  case json_type_null:
    break;
  case json_type_string:
    newobj = __UnEscapeJSONString(obj);
    if (newobj != NULL && parent != NULL) {
      type = json_object_get_type(parent);
      switch (type) {
      case json_type_object:
        newkey = g_strdup(key);
        json_object_object_del(parent, key);
        json_object_object_add(parent, newkey, newobj);
        g_free(newkey);
        break;
      case json_type_array:
        json_object_array_put_idx(parent, idx, newobj);
        break;
      default:
        Warning("does not reach here");
        break;
      }
    }
    break;
  case json_type_object:
    json_object_object_foreachC(obj, iter) {
      __UnEscapeJSON(iter.val, obj, iter.key, 0);
    }
    break;
  case json_type_array:
    l = json_object_array_length(obj);
    for (i = 0; i < l; i++) {
      __UnEscapeJSON(json_object_array_get_idx(obj, i), obj, NULL, i);
    }
    break;
  default:
    Warning("does not reach here");
    break;
  }
}

static int _UnEscapeJSON() {
  if (CTX.obj == NULL) {
    Warning("invalid json context");
    return MCP_BAD_OTHER;
  }
  __UnEscapeJSON(CTX.obj, NULL, NULL, 0);
  return MCP_OK;
}

static ValueStruct *_UnEscape(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  ctrl->rc = MCP_BAD_OTHER;
  switch (CTX.mode) {
  case MODE_WRITE_XML:
    ctrl->rc = _UnEscapeXML();
    break;
  case MODE_WRITE_JSON:
    ctrl->rc = _UnEscapeJSON();
    break;
  default:
    Warning("not reach");
    break;
  }
  return NULL;
}

extern ValueStruct *_START(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  _ResetCTX();
  return (NULL);
}

extern ValueStruct *_COMMIT(DBG_Struct *dbg, DBCOMM_CTRL *ctrl) {
  _ResetCTX();
  return (NULL);
}

static DB_OPS Operations[] = {
    /*	DB operations		*/
    {"DBOPEN", (DB_FUNC)_DBOPEN},
    {"DBDISCONNECT", (DB_FUNC)_DBDISCONNECT},
    {"DBSTART", (DB_FUNC)_START},
    {"DBCOMMIT", (DB_FUNC)_COMMIT},
    /*	table operations	*/
    {"MSGOPEN", _Open},
    {"MSGREAD", _Read},
    {"MSGWRITE", _Write},
    {"MSGCLOSE", _Close},
    {"MSGUNESCAPE", _UnEscape},

    {NULL, NULL}};

static DB_Primitives Core = {
    _EXEC,
    _DBACCESS,
    _QUERY,
    NULL,
};

extern DB_Func *InitMSGARRAY(void) {
  return (
      EnterDB_Function("MSGARRAY", Operations, DB_PARSER_NULL, &Core, "", ""));
}
