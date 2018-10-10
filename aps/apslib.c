/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan.
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
#include <string.h>
#include <glib.h>

#include "const.h"
#include "libmondai.h"
#include "directory.h"
#include "handler.h"
#include "dbgroup.h"
#include "apslib.h"
#include "debug.h"

extern int MCP_PutWindow(ProcessNode *node, char *wname, char *type) {
  ValueStruct *mcp;

  mcp = node->mcprec->value;
  if (wname != NULL) {
    dbgprintf("window = [%s]", wname);
    SetValueString(GetItemLongName(mcp, "dc.window"), wname, NULL);
  }
  SetValueString(GetItemLongName(mcp, "dc.widget"), "", NULL);
  SetValueString(GetItemLongName(mcp, "dc.puttype"), type, NULL);
  SetValueString(GetItemLongName(mcp, "dc.status"), "PUTG", NULL);
  SetValueInteger(GetItemLongName(mcp, "rc"), 0);
  return (0);
}

extern int MCP_ReturnComponent(ProcessNode *node, char *event) {
  ValueStruct *mcp;

  mcp = node->mcprec->value;
  if (event != NULL) {
    SetValueString(GetItemLongName(mcp, "dc.event"), event, NULL);
  }
  SetValueString(GetItemLongName(mcp, "dc.puttype"), MCP_PUT_RETURN, NULL);
  SetValueString(GetItemLongName(mcp, "dc.status"), "PUTG", NULL);
  SetValueInteger(GetItemLongName(mcp, "rc"), 0);
  return (0);
}

extern RecordStruct *MCP_GetWindowRecord(ProcessNode *node, char *name) {
  WindowBind *bind;
  RecordStruct *ret;

  bind = (WindowBind *)g_hash_table_lookup(node->bhash, name);
  ret = bind->rec;
  return (ret);
}

extern void *MCP_GetEventHandler(GHashTable *StatusTable, ProcessNode *node) {
  char *status, *event;
  GHashTable *EventTable;
  void (*handler)(ProcessNode *);

  status =
      ValueStringPointer(GetItemLongName(node->mcprec->value, "dc.status"));
  event = ValueStringPointer(GetItemLongName(node->mcprec->value, "dc.event"));

  dbgprintf("status = [%s]", status);
  dbgprintf("event  = [%s]", status, event);
  if ((EventTable = g_hash_table_lookup(StatusTable, status)) == NULL) {
    EventTable = g_hash_table_lookup(StatusTable, "");
  }
  if (EventTable != NULL) {
    if ((handler = g_hash_table_lookup(EventTable, event)) == NULL) {
      handler = g_hash_table_lookup(EventTable, "");
    }
  } else {
    handler = NULL;
  }

  return (handler);
}

extern void MCP_RegistHandler(GHashTable *StatusTable, char *status,
                              char *event, void (*handler)(ProcessNode *)) {
  GHashTable *EventTable;

  if ((EventTable = g_hash_table_lookup(StatusTable, status)) == NULL) {
    EventTable = NewNameHash();
    g_hash_table_insert(StatusTable, StrDup(status), EventTable);
  }
  if (g_hash_table_lookup(EventTable, event) == NULL) {
    g_hash_table_insert(EventTable, StrDup(event), handler);
  }
}

extern ValueStruct *MCP_GetDB_Define(char *name) {
  char buff[SIZE_LONGNAME + 1];
  int rno, pno, ono;
  RecordStruct *rec;
  PathStruct *path;
  DB_Operation *op;
  ValueStruct *val, *ret;
  char *p, *pname, *oname;

  strcpy(buff, name);
  if ((p = strchr(buff, ':')) != NULL) {
    *p = 0;
    pname = p + 1;
    if ((p = strchr(pname, ':')) != NULL) {
      *p = 0;
      oname = p + 1;
    } else {
      oname = NULL;
    }
  } else {
    oname = NULL;
    pname = NULL;
  }

  val = NULL;
  if ((rno = (int)(long)g_hash_table_lookup(DB_Table, name)) != 0) {
    rec = ThisDB[rno - 1];
    val = rec->value;
    if ((pname != NULL) && ((pno = (int)(long)g_hash_table_lookup(
                                 rec->opt.db->paths, pname)) != 0)) {
      path = rec->opt.db->path[pno - 1];
      val = (path->args != NULL) ? path->args : val;
      if ((oname != NULL) &&
          ((ono = (int)(long)g_hash_table_lookup(path->opHash, oname)) != 0)) {
        op = path->ops[ono - 1];
        val = (op->args != NULL) ? op->args : val;
      }
    }
  }
  if (val != NULL) {
    ret = DuplicateValue(val, FALSE);
  } else {
    ret = NULL;
  }
  return (ret);
}
