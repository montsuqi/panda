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
#include "debug.h"

extern RecordStruct *DD_Parse(CURFILE *in,const char *filename) {
  RecordStruct *rec;
  ValueStruct *value;

  if ((value = RecParseMain(in)) != NULL) {
    rec = New(RecordStruct);
    rec->filename = g_strdup(filename);
    rec->value = value;
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

static Bool _MallocValue(RecordStruct *rec,const char *fname,Bool use_cache) {
    char *valuename;
    if (use_cache) {
      rec->value = RecParseValue(fname, &valuename);
    } else {
      rec->value = RecParseValueNoCache(fname, &valuename);
    }
    if (rec->value != NULL) {
      if (rec->name == NULL) {
        rec->name = valuename;
      } else {
        /* name malloced by RecParseValue */
        g_free(valuename);
      }
      return TRUE;
    }
    return FALSE;
}

static void MallocValue(RecordStruct *rec,Bool use_cache) {
  int i;
  gchar *fname;
  static gchar **dirs = NULL;
  if (rec->value != NULL) {
    Warning("MallocValue rec->value not NULL");
    return;
  }
  if (_MallocValue(rec,rec->filename,use_cache)) {
    return;
  }
  dirs = g_strsplit_set(RecordDir, ":", -1);
  for (i = 0; dirs[i] != NULL; i++) {
    fname = g_strdup_printf("%s/%s", dirs[i], rec->filename);
    if (_MallocValue(rec,fname,use_cache)) {
      g_free(fname);
      break;
    }
    g_free(fname);
  }
  g_strfreev(dirs);
}

extern RecordStruct *ParseRecordMem(const char *mem) {
  RecordStruct *rec;
  ValueStruct *value;
  char *ValueName;

  if ((value = RecParseValueMem(mem, &ValueName)) != NULL) {
    rec = New(RecordStruct);
    rec->value = value;
    rec->name = ValueName;
    rec->filename = NULL;
    rec->type = RECORD_NULL;
  } else {
    rec = NULL;
  }
  return rec;
}

extern RecordStruct *ReadRecordDefine(const char *fname,Bool use_cache) {
  RecordStruct *rec;
  rec = New(RecordStruct);
  rec->value = NULL;
  rec->name = NULL;
  rec->filename = g_strdup(fname);
  rec->type = RECORD_NULL;
  MallocValue(rec, use_cache);
  if (rec->value != NULL) {
    return rec;
  } else {
    Warning("ReadRecordDeifne failure");
    if (rec->name != NULL) {
      g_free(rec->name);
    }
    if (rec->filename != NULL) {
      g_free(rec->filename);
    }
    g_free(rec);
    return NULL;
  }
}

extern void MallocRecordValue(RecordStruct *rec) {
  if (rec != NULL && rec->filename != NULL && rec->value == NULL) {
    MallocValue(rec,FALSE);
  }
}

extern void FreeRecordValue(RecordStruct *rec) {
  if (rec != NULL && rec->filename != NULL && rec->value != NULL) {
    FreeValueStruct(rec->value);
    rec->value = NULL;
  }
}
