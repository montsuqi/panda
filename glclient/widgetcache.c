/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 NaCl & JMA (Japan Medical Association).
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <glib.h>
#include <json.h>

#define WIDGETCACHE

#include "widgetcache.h"
#include "tempdir.h"
#include "utils.h"
#include "logger.h"

static json_object *obj = NULL;

static char *get_path(void) {
  return g_build_filename(GetRootDir(), "widgetcache.json", NULL);
}

static void LoadWidgetCache(void) {
  gchar *path, *buf;
  size_t size;

  gl_lock();
  path = get_path();
  if (!g_file_get_contents(path, &buf, &size, NULL)) {
    obj = json_object_new_object();
  } else {
    obj = json_tokener_parse(buf);
    g_free(buf);
    if (obj == NULL ||
        !json_object_is_type(obj, json_type_object)) {
      obj = json_object_new_object();
    }
  }
  gl_unlock();
  g_free(path);
}

static void SaveWidgetCache(void) {
  gchar *path;
  const char *jsonstr;

  jsonstr = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
  path = get_path();
  gl_lock();
  if (!g_file_set_contents(path, jsonstr, strlen(jsonstr), NULL)) {
    Error("could not create %s", path);
  }
  gl_unlock();
  g_free(path);
}

void SetWidgetCache(const char *key, const char *value) {
  if (obj == NULL) {
    LoadWidgetCache();
  }
  json_object_object_add(obj, key, json_object_new_string(value));
  SaveWidgetCache();
}

const char *GetWidgetCache(const char *key) {
  json_object *val;

  if (obj == NULL) {
    LoadWidgetCache();
  }

  if (!json_object_object_get_ex(obj, key, &val)) {
    return NULL;
  } else {
    return json_object_get_string(val);
  }
}
