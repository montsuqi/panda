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

#include "const.h"
#include "enum.h"
#include "libmondai.h"
#include "directory.h"
#include "wfcdata.h"
#include "dbgroup.h"
#include "monsys.h"
#include "bytea.h"
#include "dbops.h"
#include "message.h"
#include "debug.h"

static xmlDocPtr XMLDoc;
static int XMLpos;
static int XMLmode;

enum xml_open_mode {
  MODE_READ = 0,
  MODE_WRITE,
  MODE_NONE,
};

static ValueStruct *_OpenXML(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                             RecordStruct *rec, ValueStruct *args) {
  int rc;
  ValueStruct *obj, *mode, *ret;
  char *buff;
  size_t size;
  xmlNodePtr root;
  DBG_Struct *mondbg;

  ENTER_FUNC;
  XMLpos = 0;
  XMLDoc = NULL;
  XMLmode = MODE_NONE;

  rc = MCP_BAD_OTHER;
  ret = NULL;

  if (rec->type != RECORD_DB) {
    rc = MCP_BAD_ARG;
  } else {
    if ((obj = GetItemLongName(args, "object")) != NULL &&
        (mode = GetItemLongName(args, "mode")) != NULL) {
      XMLmode = ValueInteger(mode);
      if (XMLmode == MODE_WRITE) {
        XMLDoc = xmlNewDoc("1.0");
        root = xmlNewDocNode(XMLDoc, NULL, "data", NULL);
        xmlDocSetRootElement(XMLDoc, root);
        ret = DuplicateValue(args, TRUE);
        rc = MCP_OK;
      } else {
        mondbg = GetDBG_monsys();
        if (blob_export_mem(mondbg, ValueObjectId(obj), &buff, &size)) {
          if (size > 0) {
            XMLDoc = xmlReadMemory(buff, size, "http://www.montsuqi.org/", NULL,
                                   XML_PARSE_NOBLANKS | XML_PARSE_NOENT);
            if (XMLDoc != NULL) {
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
  if (ctrl != NULL) {
    ctrl->rc = rc;
  }
  LEAVE_FUNC;
  return ret;
}

static ValueStruct *_CloseXML(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  ValueStruct *obj, *ret;
  xmlChar *buff;
  int size;
  DBG_Struct *mondbg;

  ENTER_FUNC;
  buff = NULL;
  ret = NULL;
  if (rec->type != RECORD_DB) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if ((obj = GetItemLongName(args, "object")) == NULL) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if (XMLDoc == NULL) {
    ctrl->rc = MCP_BAD_OTHER;
    return NULL;
  }
  ctrl->rc = MCP_OK;
  if (XMLmode == MODE_WRITE) {
    xmlDocDumpFormatMemoryEnc(XMLDoc, &buff, &size, "UTF-8", 0);
    if (buff != NULL) {
      mondbg = GetDBG_monsys();
      ValueObjectId(obj) = blob_import_mem(mondbg, 0, "XMLIO.xml",
                                           "application/xml", 0, buff, size);
      if (ValueObjectId(obj) != GL_OBJ_NULL) {
        ret = DuplicateValue(args, TRUE);
        ctrl->rc = MCP_OK;
      } else {
        Warning("monblob_import_mem failure");
        ctrl->rc = MCP_BAD_OTHER;
      }
    }
    xfree(buff);
  }
  xmlFreeDoc(XMLDoc);
  XMLDoc = NULL;
  XMLpos = 0;
  XMLmode = MODE_NONE;
  LEAVE_FUNC;
  return ret;
}

static char *XMLGetString(xmlNodePtr ptr, char *value) {
  char *buff;
  buff = xmlNodeListGetString(ptr->doc, ptr->children, TRUE);
  if (buff == NULL) {
    buff = StrDup(value);
  }
  return buff;
}

static int XMLNode2Value(ValueStruct *val, xmlNodePtr root) {
  int i;
  char *buff1;
  xmlNodePtr node;

  ENTER_FUNC;
  if (val == NULL || root == NULL) {
    return MCP_BAD_OTHER;
  }
  switch (ValueType(val)) {
  case GL_TYPE_INT:
    if (xmlStrcmp(root->name, "int") != 0) {
      break;
    }
    buff1 = XMLGetString(root, "0");
    SetValueStringWithLength(val, buff1, strlen(buff1), NULL);
    free(buff1);
    break;
  case GL_TYPE_FLOAT:
    if (xmlStrcmp(root->name, "float") != 0) {
      break;
    }
    buff1 = XMLGetString(root, "0");
    SetValueStringWithLength(val, buff1, strlen(buff1), NULL);
    free(buff1);
    break;
  case GL_TYPE_NUMBER:
    if (xmlStrcmp(root->name, "number") != 0) {
      break;
    }
    buff1 = XMLGetString(root, "0.0");
    SetValueStringWithLength(val, buff1, strlen(buff1), NULL);
    free(buff1);
    break;
  case GL_TYPE_BOOL:
    if (xmlStrcmp(root->name, "bool") != 0) {
      break;
    }
    buff1 = XMLGetString(root, "FALSE");
    SetValueStringWithLength(val, buff1, strlen(buff1), NULL);
    free(buff1);
    break;
  case GL_TYPE_CHAR:
  case GL_TYPE_VARCHAR:
  case GL_TYPE_SYMBOL:
  case GL_TYPE_DBCODE:
  case GL_TYPE_TEXT:
    if (xmlStrcmp(root->name, "string") != 0) {
      break;
    }
    buff1 = XMLGetString(root, "");
    SetValueStringWithLength(val, buff1, strlen(buff1), NULL);
    free(buff1);
    break;
  case GL_TYPE_ARRAY:
    if (xmlStrcmp(root->name, "array") != 0) {
      break;
    }
    node = root->children;
    for (i = 0; i < ValueArraySize(val); i++) {
      if (node == NULL) {
        break;
      }
      XMLNode2Value(ValueArrayItem(val, i), node);
      node = node->next;
    }
    break;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    if (xmlStrcmp(root->name, "record") != 0) {
      break;
    }
    for (i = 0; i < ValueRecordSize(val); i++) {
      for (node = root->children; node != NULL; node = node->next) {
        if ((buff1 = xmlGetProp(node, "name")) == NULL) {
          continue;
        }
        if (!xmlStrcmp(buff1, ValueRecordName(val, i))) {
          XMLNode2Value(ValueRecordItem(val, i), node);
          free(buff1);
          break;
        }
        free(buff1);
      }
    }
    break;
  case GL_TYPE_OBJECT:
    break;
  default:
    return MCP_BAD_ARG;
    break;
  }
  LEAVE_FUNC;
  return MCP_OK;
}

static xmlNodePtr Value2XMLNode(char *name, ValueStruct *val) {
  xmlNodePtr node;
  xmlNodePtr child;
  int i;

  ENTER_FUNC;
  if (val == NULL) {
    return NULL;
  }
  node = NULL;
  switch (ValueType(val)) {
  case GL_TYPE_INT:
    node = xmlNewNode(NULL, "int");
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_FLOAT:
    node = xmlNewNode(NULL, "float");
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_NUMBER:
    node = xmlNewNode(NULL, "number");
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_BOOL:
    node = xmlNewNode(NULL, "bool");
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_CHAR:
  case GL_TYPE_VARCHAR:
  case GL_TYPE_SYMBOL:
  case GL_TYPE_DBCODE:
  case GL_TYPE_TEXT:
    node = xmlNewNode(NULL, "string");
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_ARRAY:
    node = xmlNewNode(NULL, "array");
    for (i = 0; i < ValueArraySize(val); i++) {
      child = Value2XMLNode(NULL, ValueArrayItem(val, i));
      if (child != NULL) {
        xmlAddChildList(node, child);
      }
    }
    break;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    node = xmlNewNode(NULL, "record");
    for (i = 0; i < ValueRecordSize(val); i++) {
      if (!strcmp(ValueRecordName(val, i), "mode")) {
        continue;
      }
      child = Value2XMLNode(ValueRecordName(val, i), ValueRecordItem(val, i));
      if (child != NULL) {
        xmlAddChildList(node, child);
      }
    }
    break;
  }
  if (node != NULL && name != NULL && strlen(name) > 0) {
    xmlNewProp(node, "name", name);
  }
  return node;
}

static ValueStruct *_ReadXML(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                             RecordStruct *rec, ValueStruct *args) {
  xmlNodePtr node;
  xmlNodePtr root;
  ValueStruct *ret;
  int i;

  ENTER_FUNC;
  ret = NULL;
  ctrl->rc = MCP_BAD_OTHER;
  if (rec->type != RECORD_DB) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if (XMLDoc == NULL || XMLmode != MODE_READ) {
    return NULL;
  }
  root = xmlDocGetRootElement(XMLDoc);
  if (root == NULL || root->children == NULL) {
    return NULL;
  }
  for (node = root->children, i = 0; node != NULL; node = node->next, i++) {
    root = NULL;
    if (i == XMLpos) {
      root = node;
      break;
    }
  }
  XMLpos++;
  if (root == NULL) {
    return NULL;
  }
  ret = DuplicateValue(args, FALSE);
  ctrl->rc = XMLNode2Value(ret, root);
#ifdef TRACE
  DumpValueStruct(ret);
#endif
  LEAVE_FUNC;
  return (ret);
}

static ValueStruct *_WriteXML(DBG_Struct *dbg, DBCOMM_CTRL *ctrl,
                              RecordStruct *rec, ValueStruct *args) {
  ValueStruct *ret;
  xmlNodePtr root;
  xmlNodePtr node;
  ENTER_FUNC;
  ret = NULL;
  if (rec->type != RECORD_DB) {
    ctrl->rc = MCP_BAD_ARG;
    return NULL;
  }
  if (XMLDoc == NULL || XMLmode != MODE_WRITE) {
    ctrl->rc = MCP_BAD_OTHER;
    return NULL;
  }
  root = xmlDocGetRootElement(XMLDoc);
  node = Value2XMLNode(NULL, args);
  if (node != NULL) {
    xmlAddChildList(root, node);
  }
  ctrl->rc = MCP_OK;
  LEAVE_FUNC;
  return (ret);
}

static DB_OPS Operations[] = {
    /*	DB operations		*/
    {"DBOPEN", (DB_FUNC)_DBOPEN},
    {"DBDISCONNECT", (DB_FUNC)_DBDISCONNECT},
    {"DBSTART", (DB_FUNC)_DBSTART},
    {"DBCOMMIT", (DB_FUNC)_DBCOMMIT},
    /*	table operations	*/
    {"XMLOPEN", _OpenXML},
    {"XMLREAD", _ReadXML},
    {"XMLWRITE", _WriteXML},
    {"XMLCLOSE", _CloseXML},

    {NULL, NULL}};

static DB_Primitives Core = {
    _EXEC,
    _DBACCESS,
    _QUERY,
    NULL,
};

extern DB_Func *InitXMLIO(void) {
  return (EnterDB_Function("XMLIO", Operations, DB_PARSER_NULL, &Core, "", ""));
}
