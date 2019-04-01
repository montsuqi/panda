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
#include <libmondai.h>

#include "const.h"
#include "enum.h"
#include "msglib.h"
#include "message.h"
#include "debug.h"

static gchar *XMLGetString(xmlNodePtr node, char *value) {
  gchar *buf;
  buf = xmlNodeGetContent(node);
  if (buf == NULL) {
    buf = g_strdup(value);
  }
  return buf;
}

int XMLNode2Value(ValueStruct *val, xmlNodePtr root) {
  int i;
  xmlNodePtr node;
  char *type;
  char *buf;

  if (val == NULL || root == NULL) {
    Warning("XMLNode2Value val = NULL || root = NULL");
    return MCP_BAD_OTHER;
  }
  type = xmlGetProp(root, "type");
  if (type == NULL) {
    Warning("XMLNode2Value root type is NULL");
    return MCP_BAD_OTHER;
  }
  switch (ValueType(val)) {
  case GL_TYPE_INT:
    if (xmlStrcmp(type, "int") != 0) {
      break;
    }
    buf = XMLGetString(root, "0");
    SetValueStringWithLength(val, buf, strlen(buf), NULL);
    g_free(buf);
    break;
  case GL_TYPE_FLOAT:
    if (xmlStrcmp(type, "float") != 0) {
      break;
    }
    buf = XMLGetString(root, "0");
    SetValueStringWithLength(val, buf, strlen(buf), NULL);
    g_free(buf);
    break;
  case GL_TYPE_NUMBER:
    if (xmlStrcmp(type, "number") != 0) {
      break;
    }
    buf = XMLGetString(root, "0.0");
    SetValueStringWithLength(val, buf, strlen(buf), NULL);
    g_free(buf);
    break;
  case GL_TYPE_BOOL:
    if (xmlStrcmp(type, "bool") != 0) {
      break;
    }
    buf = XMLGetString(root, "FALSE");
    SetValueStringWithLength(val, buf, strlen(buf), NULL);
    g_free(buf);
    break;
  case GL_TYPE_CHAR:
  case GL_TYPE_VARCHAR:
  case GL_TYPE_SYMBOL:
  case GL_TYPE_DBCODE:
  case GL_TYPE_TEXT:
    if (xmlStrcmp(type, "string") != 0) {
      break;
    }
    buf = XMLGetString(root, "");
    SetValueStringWithLength(val, buf, strlen(buf), NULL);
    g_free(buf);
    break;
  case GL_TYPE_ARRAY:
    if (xmlStrcmp(type, "array") != 0) {
      break;
    }
    i = 0;
    for (node = root->children; node != NULL; node = node->next) {
      if (node->type != XML_ELEMENT_NODE) {
        continue;
      }
      if (i < ValueArraySize(val)) {
        XMLNode2Value(ValueArrayItem(val, i), node);
      } else {
        break;
      }
      i++;
    }
    break;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    if (xmlStrcmp(type, "record") != 0) {
      break;
    }
    for (i = 0; i < ValueRecordSize(val); i++) {
      for (node = root->children; node != NULL; node = node->next) {
        if (!xmlStrcmp(node->name, ValueRecordName(val, i))) {
          XMLNode2Value(ValueRecordItem(val, i), node);
          break;
        }
      }
    }
    break;
  case GL_TYPE_OBJECT:
    break;
  default:
    Warning("XMLNode2Value");
    return MCP_BAD_ARG;
    break;
  }
  if (type != NULL) {
    xmlFree(type);
  }
  return MCP_OK;
}

static gboolean IsEmptyValue(ValueStruct *val) {
  gboolean ret;
  int i;

  ret = FALSE;
  switch (ValueType(val)) {
  case GL_TYPE_CHAR:
  case GL_TYPE_VARCHAR:
  case GL_TYPE_SYMBOL:
  case GL_TYPE_DBCODE:
  case GL_TYPE_TEXT:
    if (strlen(ValueToString(val, NULL)) == 0) {
      ret = TRUE;
    }
    break;
  case GL_TYPE_ARRAY:
    for (i = 0; i < ValueArraySize(val); i++) {
      ret = IsEmptyValue(ValueArrayItem(val, i));
      if (ret == FALSE) {
        break;
      }
    }
    break;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    for (i = 0; i < ValueRecordSize(val); i++) {
      ret = IsEmptyValue(ValueRecordItem(val, i));
      if (ret == FALSE) {
        break;
      }
    }
    break;
  }
  return ret;
}

xmlNodePtr Value2XMLNode(char *name, ValueStruct *val) {
  xmlNodePtr node;
  xmlNodePtr child;
  char *type;
  char *childname;
  int i;
  gboolean have_data;

  if (val == NULL || name == NULL) {
    Warning("val or name = null,val:%p name:%p", val, name);
    return NULL;
  }
  node = NULL;
  type = NULL;

  switch (ValueType(val)) {
  case GL_TYPE_INT:
    type = "int";
    node = xmlNewNode(NULL, name);
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_FLOAT:
    type = "float";
    node = xmlNewNode(NULL, name);
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_NUMBER:
    type = "number";
    node = xmlNewNode(NULL, name);
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_BOOL:
    type = ("bool");
    node = xmlNewNode(NULL, name);
    xmlNodeAddContent(node, ValueToString(val, NULL));
    break;
  case GL_TYPE_CHAR:
  case GL_TYPE_VARCHAR:
  case GL_TYPE_SYMBOL:
  case GL_TYPE_DBCODE:
  case GL_TYPE_TEXT:
    type = ("string");
    if (!IsEmptyValue(val)) {
      node = xmlNewNode(NULL, name);
      xmlNodeAddContent(node, ValueToString(val, NULL));
    }
    break;
  case GL_TYPE_ARRAY:
    have_data = FALSE;
    for (i = 0; i < ValueArraySize(val); i++) {
      if (!IsEmptyValue(ValueArrayItem(val, i))) {
        have_data = TRUE;
      }
    }
    if (have_data) {
      type = ("array");
      node = xmlNewNode(NULL, name);
      childname = g_strdup_printf("%s_child", name);
      for (i = 0; i < ValueArraySize(val); i++) {
        if (!IsEmptyValue(ValueArrayItem(val, i))) {
          child = Value2XMLNode(childname, ValueArrayItem(val, i));
          if (child != NULL) {
            xmlAddChildList(node, child);
          }
        }
      }
      g_free(childname);
    }
    break;
  case GL_TYPE_ROOT_RECORD:
  case GL_TYPE_RECORD:
    have_data = FALSE;
    for (i = 0; i < ValueRecordSize(val); i++) {
      if (!IsEmptyValue(ValueRecordItem(val, i))) {
        have_data = TRUE;
      }
    }
    if (have_data) {
      type = ("record");
      node = xmlNewNode(NULL, name);
      for (i = 0; i < ValueRecordSize(val); i++) {
        if (!IsEmptyValue(ValueRecordItem(val, i))) {
          child =
              Value2XMLNode(ValueRecordName(val, i), ValueRecordItem(val, i));
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

int CheckFormat(char *buf, size_t size) {
  int i;
  char *p;

  p = buf;
  for (i = 0; i < size; i++) {
    if (*p == '<') {
      return MSG_XML;
    } else if (*p == '{') {
      return MSG_JSON;
    } else if (*p == '[') {
      return MSG_JSON;
    }
    p++;
  }
  return MSG_NONE;
}
