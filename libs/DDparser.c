/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
#include <ctype.h>
#include <glib.h>
#include <sys/stat.h> /*	for stbuf	*/
#include <libmondai.h>
#include <RecParser.h>
#include "struct.h"
#include "monstring.h"
#include "directory.h"
#include "DDparser.h"
#include "DBparser.h"
#include "debug.h"

extern RecordStruct *DD_Parse(CURFILE *in,const char *filename) {
  RecordStruct *rec;
  ValueStruct *value;

  if ((value = RecParseMain(in)) != NULL) {
    rec = New(RecordStruct);
    rec->value = value;
    InitializeValue(rec->value);
    rec->name = StrDup(in->ValueName);
    if (ValueRootRecordName(value) == NULL) {
      ValueRootRecordName(value) = StrDup(in->ValueName);
    }
    rec->type = RECORD_NULL;
  } else {
    rec = NULL;
  }
  return (rec);
}

extern RecordStruct *ParseRecordMem(const char *mem) {
  RecordStruct *rec;
  ValueStruct *value;
  char *ValueName;

  if ((value = RecParseValueMem(mem, &ValueName)) != NULL) {
    rec = New(RecordStruct);
    rec->value = value;
    rec->name = ValueName;
    rec->type = RECORD_NULL;
  } else {
    rec = NULL;
  }
  return rec;
}

extern RecordStruct *ReadRecordDefine(const char *name,Bool use_cache) {
  RecordStruct *rec;
  int i;
  char *vname;
  gchar *fname;
  static gchar **dirs = NULL;
  rec = New(RecordStruct);
  rec->value = NULL;
  rec->name = NULL;
  rec->type = RECORD_NULL;
  if (dirs == NULL) {
    dirs = g_strsplit_set(RecordDir, ":", -1);
  }
  for (i = 0; dirs[i] != NULL; i++) {
    fname = g_strdup_printf("%s/%s", dirs[i], name);
    if (use_cache) {
      rec->value = RecParseValue(fname, &vname);
    } else {
      rec->value = RecParseValueNoCache(fname, &vname);
    }
    if (rec->value != NULL) {
      InitializeValue(rec->value);
      rec->name = vname;
      g_free(fname);
      break;
    }
    g_free(fname);
  }
  if (rec->value != NULL) {
    return rec;
  } else {
    Warning("ReadRecordDeifne failure");
    return NULL;
  }
}

extern void FreeRecordStruct(RecordStruct *rec) {
  if (rec == NULL) {
    return;
  }
  if (rec->name != NULL) {
    xfree(rec->name);
  }
  if (rec->value != NULL) {
    FreeValueStruct(rec->value);
  }
  if (rec->type == RECORD_DB && RecordDB(rec) != NULL) {
    FreeDB_Struct(RecordDB(rec));
  }
  xfree(rec);
}

extern RecordStructMeta *NewRecordStructMeta(const char *name, const char *gname) {
  RecordStructMeta *meta;
  meta = New(RecordStructMeta);
  meta->name = NULL;
  if (name != NULL) {
    meta->name = StrDup(name);
  }
  meta->gname = NULL;
  if (gname != NULL) {
    meta->gname = StrDup(gname);
  }
  return meta;
}
