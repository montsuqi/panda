#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <json.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>
#include <libmondai.h>

#include "gettext.h"
#include "notify.h"
#include "protocol.h"
#include "download.h"
#include "tempdir.h"
#include "logger.h"
#include "utils.h"

int
main(
	int argc,
	char **argv)
{
	char *LogFile,*TempDir;
	char *Filename,*Desc;
	LargeByteString *lbs;

	gtk_init(&argc,&argv);
	gtk_panda_init(&argc,&argv);
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

	Filename = getenv("GLPUSH_FILENAME");
	if (Filename == NULL) {
		Filename = "";
	}
	Desc = getenv("GLPUSH_DESCRIPTION");
	if (Desc == NULL) {
		Desc = "";
	}

	lbs = REST_GetBLOB_via_ENV();
	ShowDownloadDialog(Filename,Desc,lbs);
	FreeLBS(lbs);
	return 0;
}
