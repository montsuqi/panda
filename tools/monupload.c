/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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

#define MAIN

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>
#include <pthread.h>
#include <sys/file.h>
#include <locale.h>

#include "enum.h"
#include "net.h"
#include "comm.h"
#include "comms.h"
#include "const.h"
#include "wfcdata.h"
#include "dbgroup.h"
#include "sysdatacom.h"
#include "socket.h"
#include "blobreq.h"
#include "message.h"
#include "debug.h"

#define MAX_USER 100

static gchar *SysDataPort = SYSDATA_PORT;
static gchar *Type = "misc";
static gchar *Printer = NULL;
static gchar *Title = NULL;
static gchar *Filename = NULL;
static gchar *Desc = NULL;
static gboolean ShowDialog = FALSE;

static json_object *ReadMetaFile(const char *metafile) {
  char *buf, *buf2;
  size_t size;
  json_object *obj, *result;

  if (!g_file_get_contents(metafile, &buf, &size, NULL)) {
    return NULL;
  }
  buf2 = strndup(buf, size);
  obj = json_tokener_parse(buf2);
  g_free(buf);
  g_free(buf2);

  if (is_error(obj)) {
    return NULL;
  }
  if (!json_object_object_get_ex(obj, "result", &result)) {
    return NULL;
  }
  return obj;
}

static void WriteMetaFile(const char *metafile, json_object *obj) {
  char *jsonstr;

  jsonstr = (char *)json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
  if (!g_file_set_contents(metafile, jsonstr, strlen(jsonstr), NULL)) {
    g_warning("g_file_set_contents failure:%s", metafile);
  }
  json_object_put(obj);
}

static void AddMetaData(MonObjectType mobj, const char *metafile) {
  json_object *obj, *result, *child;
  char mobjstr[256];

  obj = ReadMetaFile(metafile);
  if (obj == NULL) {
    obj = json_object_new_object();
    result = json_object_new_array();
    json_object_object_add(obj, "result", result);
  } else {
    json_object_object_get_ex(obj, "result", &result);
  }

  child = json_object_new_object();
  json_object_object_add(child, "type", json_object_new_string(Type));
  sprintf(mobjstr, "%ld", (long)mobj);
  json_object_object_add(child, "object_id", json_object_new_string(mobjstr));
  if (Printer != NULL) {
    json_object_object_add(child, "printer", json_object_new_string(Printer));
  }
  if (Title != NULL) {
    json_object_object_add(child, "title", json_object_new_string(Title));
  }
  if (ShowDialog) {
    json_object_object_add(child, "showdialog", json_object_new_boolean(TRUE));
  }
  if (Filename != NULL) {
    json_object_object_add(child, "filename", json_object_new_string(Filename));
  }
  if (Desc != NULL) {
    json_object_object_add(child, "description", json_object_new_string(Desc));
  }

  json_object_array_add(result, child);

  WriteMetaFile(metafile, obj);
}

static void _MonUpload(const char *file, const char *metafile) {
  NETFILE *fp;
  Port *port;
  int fd;
  MonObjectType mobj;

  /* upload sysdata */
  fp = NULL;
  port = ParPort(SysDataPort, SYSDATA_PORT);
  fd = ConnectSocket(port, SOCK_STREAM);
  DestroyPort(port);
  if (fd > 0) {
    fp = SocketToNet(fd);
  } else {
    g_warning("cannot connect sysdata server");
    return;
  }

  mobj = RequestImportBLOB(fp, file);
  if (mobj == GL_OBJ_NULL) {
    return;
  }
  CloseNet(fp);

  AddMetaData(mobj, metafile);
}

static void MonUpload(const char *file) {
  char *tempdir, *lockfile, *metafile;
  int fd;

  /* lock */
  if ((tempdir = getenv("MCP_TEMPDIR")) == NULL) {
    g_warning("can't get MCP_TEMPDIR envval");
    exit(1);
  }
  lockfile = g_strdup_printf("%s/__download.lock", tempdir);
  metafile = g_strdup_printf("%s/__download.json", tempdir);
  if ((fd = open(lockfile, O_WRONLY | O_CREAT, S_IRWXU)) == -1) {
    g_warning("open(2) failure %s %s", lockfile, strerror(errno));
    exit(1);
  }
  if (flock(fd, LOCK_EX) == -1) {
    g_warning("flock(2) failure %s %s", lockfile, strerror(errno));
    exit(1);
  }

  _MonUpload(file, metafile);

  /* unlock  */
  if (flock(fd, LOCK_UN) == -1) {
    g_warning("flock(2) failure %s %s", lockfile, strerror(errno));
    exit(1);
  }
  if ((close(fd)) == -1) {
    g_warning("close(2) failure %s %s", lockfile, strerror(errno));
    exit(1);
  }
  g_free(lockfile);
  g_free(metafile);
}

static GOptionEntry entries[] = {
    {"type", 't', 0, G_OPTION_ARG_STRING, &Type, "type <report> or <misc>",
     "T"},
    {"printer", 'p', 0, G_OPTION_ARG_STRING, &Printer, "printer P", "P"},
    {"title", 'T', 0, G_OPTION_ARG_STRING, &Title, "report title for report",
     "T"},
    {"showdialog", 's', 0, G_OPTION_ARG_NONE, &ShowDialog, "show dialog", NULL},
    {"filename", 'f', 0, G_OPTION_ARG_STRING, &Filename, "filename", "F"},
    {"description", 'd', 0, G_OPTION_ARG_STRING, &Desc, "description for file",
     "D"},
    {"port", 'P', 0, G_OPTION_ARG_STRING, &SysDataPort, "SystemDataThread port",
     "P"},
    {NULL}};

extern int main(int argc, char *argv[]) {
  GError *error = NULL;
  GOptionContext *context;

  setlocale(LC_CTYPE, "ja_JP.UTF-8");
  context = g_option_context_new("file");
  g_option_context_add_main_entries(context, entries, NULL);
  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print("option parsing failed: %s\n", error->message);
    exit(1);
  }

  if (argc < 2) {
    g_print("$ monupload [options] file\n");
    exit(1);
  }

  MonUpload(argv[1]);

  return 0;
}
