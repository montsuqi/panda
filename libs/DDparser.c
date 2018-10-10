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
#include "DDparser.h"
#include "debug.h"

extern RecordStruct *DD_Parse(CURFILE *in) {
  RecordStruct *ret;
  ValueStruct *value;

  if ((value = RecParseMain(in)) != NULL) {
    ret = New(RecordStruct);
    ret->value = value;
    ret->name = StrDup(in->ValueName);
    if (ValueRootRecordName(value) == NULL) {
      ValueRootRecordName(value) = ret->name;
    }
    ret->type = RECORD_NULL;
  } else {
    ret = NULL;
  }
  return (ret);
}

extern RecordStruct *ParseRecordFile(char *name) {
  RecordStruct *ret;
  ValueStruct *value;
  char *ValueName;

  dbgprintf("name = [%s]\n", name);
  if ((value = RecParseValue(name, &ValueName)) != NULL) {
    ret = New(RecordStruct);
    ret->value = value;
    ret->name = ValueName;
    ret->type = RECORD_NULL;
  } else {
    ret = NULL;
  }
  return (ret);
}

extern RecordStruct *ParseRecordMem(char *mem) {
  RecordStruct *ret;
  ValueStruct *value;
  char *ValueName;

  if ((value = RecParseValueMem(mem, &ValueName)) != NULL) {
    ret = New(RecordStruct);
    ret->value = value;
    ret->name = ValueName;
    ret->type = RECORD_NULL;
  } else {
    ret = NULL;
  }
  return ret;
}

extern RecordStruct *ReadRecordDefine(char *name) {
  gchar *fname;
  static gchar **dirs = NULL;
  int i;
  RecordStruct *rec;
  rec = NULL;
  dirs = g_strsplit_set(RecordDir, ":", -1);
  for (i = 0; dirs[i] != NULL; i++) {
    fname = g_strdup_printf("%s/%s", dirs[i], name);
    if ((rec = ParseRecordFile(fname)) != NULL) {
      g_free(fname);
      break;
    } else {
      g_free(fname);
    }
  }
  g_strfreev(dirs);
  return rec;
}
