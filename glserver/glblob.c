/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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
#include <unistd.h>
#include <libmondai.h>

#include "glserver.h"
#include "enum.h"
#include "const.h"
#include "directory.h"
#include "dbgroup.h"
#include "dbutils.h"
#include "monsys.h"
#include "bytea.h"
#include "message.h"
#include "debug.h"

void GLExportBLOB(char* id, char **out, size_t *size) {
  DBG_Struct *dbg;

  if (id == NULL || strlen(id) <= 0) {
    Warning("invalid id");
    return;
  }

  dbg = GetDBG_monsys();
  dbg->dbt = NewNameHash();

  if (OpenDB(dbg) != MCP_OK) {
    Warning("OpenDB failure");
    return;
  }
  monblob_setup(dbg, FALSE);
  TransactionStart(dbg);

  if (monblob_export_mem(dbg, id, out, size)) {
    monblob_delete(dbg, id);
  } else {
    *out = NULL;
    *size = 0;
  }

  TransactionEnd(dbg);
  CloseDB(dbg);
}

char* GLImportBLOB(char *in, size_t size) {
  DBG_Struct *dbg;
  char *id;

  if (in == NULL || size <= 0) {
    return NULL;
  }

  dbg = GetDBG_monsys();
  dbg->dbt = NewNameHash();

  if (OpenDB(dbg) != MCP_OK) {
    Warning("OpenDB failure");
    return NULL;
  }
  monblob_setup(dbg, FALSE);
  TransactionStart(dbg);

  id = monblob_import_mem(dbg, NULL, 0, "glserver.bin", NULL, 0, in, size);

  TransactionEnd(dbg);
  CloseDB(dbg);
  return id;
}
