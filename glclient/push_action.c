#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#include <json.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>
#include <libmondai.h>

#include "gettext.h"
#include "bd_config.h"
#include "desktop.h"
#include "notify.h"
#include "protocol.h"
#include "download.h"
#include "print.h"
#include "tempdir.h"
#include "logger.h"
#include "utils.h"

static char *TempDir;

static void print_report(json_object *obj) {
  json_object *child;
  char *title, *printer, *oid;
  gboolean showdialog;
  LargeByteString *lbs;

  printer = "";
  title = "";
  showdialog = FALSE;

  if (!json_object_object_get_ex(obj, "object_id", &child)) {
    return;
  }
  oid = (char *)json_object_get_string(child);

  if (json_object_object_get_ex(obj, "printer", &child)) {
    printer = (char *)json_object_get_string(child);
  }
  if (json_object_object_get_ex(obj, "title", &child)) {
    title = (char *)json_object_get_string(child);
  }
  if (json_object_object_get_ex(obj, "showdialog", &child)) {
    showdialog = json_object_get_boolean(child);
  }
  if (oid == NULL || strlen(oid) <= 0) {
    return;
  }
  if (printer == NULL || strlen(printer) <= 0) {
    showdialog = TRUE;
  }
  Info("report title:%s printer:%s showdialog:%d oid:%s", title, printer,
       (int)showdialog, oid);

  setenv("GLPUSH_OID", oid, 1);
  lbs = REST_GetBLOB_via_ENV();
  if (showdialog) {
    ShowPrintDialog(title, lbs);
  } else {
    Print(oid, title, printer, lbs);
  }
  FreeLBS(lbs);
}

static void download_file(json_object *obj) {
  json_object *child;
  char *oid, *filename, *desc;
  LargeByteString *lbs;

  filename = "";
  desc = "";

  if (!json_object_object_get_ex(obj, "object_id", &child)) {
    return;
  }
  oid = (char *)json_object_get_string(child);

  if (json_object_object_get_ex(obj, "filename", &child)) {
    filename = (char *)json_object_get_string(child);
  }
  if (json_object_object_get_ex(obj, "description", &child)) {
    desc = (char *)json_object_get_string(child);
  }

  if (oid == NULL || strlen(oid) <= 0) {
    return;
  }

  Info("misc filename:%s description:%s oid:%s", filename, desc, oid);

  setenv("GLPUSH_OID", oid, 1);
  lbs = REST_GetBLOB_via_ENV();
  ShowDownloadDialog(filename, desc, lbs);
  FreeLBS(lbs);
}

static gboolean push_action_exit(gpointer data) {
  exit(0);
  return FALSE;
}

static void event_handler(json_object *obj) {
  json_object *child, *data, *body, *event;
  char *ev, *type, *message;

  if (!json_object_object_get_ex(obj, "data", &data)) {
    Warning("data not found");
    return;
  }
  if (!json_object_object_get_ex(data, "event", &event)) {
    Warning("event not found");
    return;
  }
  if (!json_object_object_get_ex(data, "body", &body)) {
    Warning("body not found");
    return;
  }
  ev = (char *)json_object_get_string(event);
  if (!strcmp(ev, "client_data_ready")) {
    if (!json_object_object_get_ex(body, "type", &child)) {
      Warning("type not found");
      return;
    }
    type = (char *)json_object_get_string(child);
    if (type != NULL) {
      if (!strcmp("report", type)) {
        print_report(body);
      } else if (!strcmp("misc", type)) {
        download_file(body);
      }
    }
  } else if (!strcmp(ev, "announcement")) {
    if (!json_object_object_get_ex(body, "message", &child)) {
      Warning("message not found");
      return;
    }
    message = (char *)json_object_get_string(child);
    Notify(_("orca cloud announce"), message, "gtk-dialog-info", 30);
  } else if (!strcmp(ev, "websocket_reconnect")) {
    Notify(_("websocket_reconnect"), _("websocket_reconnect_message"),
           "gtk-dialog-warning", 30);
  } else if (!strcmp(ev, "websocket_disconnect")) {
    Notify(_("websocket_disconnect"), _("websocket_disconnect_message"),
           "gtk-dialog-warning", 30);
  } else {
    Warning("unknown event:%s", ev);
  }
}

static void execute() {
  json_object *obj;
  char *datafile, *lockfile, *buf;
  int i, len, lockfd, datafd;
  struct stat st;

  datafile = g_build_filename(TempDir, ".client_data_ready.txt", NULL);
  lockfile = g_build_filename(TempDir, ".client_data_ready.lock", NULL);

  lockfd = _flock(lockfile);
  if (stat(datafile, &st) == -1) {
    Error("stat(2) failure:%s", strerror(errno));
  }
  if ((datafd = open(datafile, O_RDONLY)) == -1) {
    Error("open(2) failure:%s %s", datafile, strerror(errno));
  }
  if (unlink(datafile) == -1) {
    Error("unlink(2) failure %s %s", datafile, strerror(errno));
  }
  _funlock(lockfd);
  buf = xmalloc(st.st_size + 1);
  memset(buf, 0, st.st_size + 1);
  if ((read(datafd, buf, st.st_size)) != st.st_size) {
    Error("read(2) failure %s %s", datafile, strerror(errno));
  }
  obj = json_tokener_parse(buf);
  if (obj == NULL) {
    obj = json_object_new_array();
    Warning("invalid json:|%s|", buf);
  }
  xfree(buf);
  if (close(datafd) == -1) {
    Error("close(2) failure %s %s", datafile, strerror(errno));
  }

  len = json_object_array_length(obj);
  for (i = 0; i < len; i++) {
    event_handler(json_object_array_get_idx(obj, i));
  }
  json_object_put(obj);
  g_free(datafile);
  g_free(lockfile);
}

int main(int argc, char **argv) {
  struct sigaction sa;
  char *LogFile;

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_DFL;
  sa.sa_flags |= SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) != 0) {
    Error("sigaction(2) failure");
  }

  gtk_init(&argc, &argv);
  gtk_panda_init(&argc, &argv);
  gtk_set_locale();

  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  LogFile = getenv("GLPUSH_LOGFILE");
  if (LogFile == NULL) {
    InitLogger("glprint");
  } else {
    InitLogger_via_FileName(LogFile);
  }
  TempDir = getenv("GLPUSH_TEMPDIR");
  if (TempDir == NULL) {
    InitTempDir("glprint");
  } else {
    InitTempDir_via_Dir(TempDir);
  }

  gl_config_init();
  InitDesktop();

  execute();

  gtk_timeout_add(30 * 1000, push_action_exit, NULL);
  gtk_main();

  sleep(3);

  return 0;
}
