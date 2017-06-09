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
#include "bd_config.h"
#include "protocol.h"
#include "print.h"
#include "tempdir.h"
#include "logger.h"
#include "utils.h"

#if 0
	env[0] = g_strdup_printf("GLPUSH-TITLE=%s",title);
	env[1] = g_strdup_printf("GLPUSH-PRINTER=%s",printer);
	env[2] = g_strdup_printf("GLPUSH-SHOW-DIALOG=%d",(int)showdialog);
	env[3] = g_strdup_printf("GLPUSH-OID=%s",oid);
#endif

int
main(
	int argc,
	char **argv)
{
	char *LogFile,*TempDir,*ConfigNum;
	char *Title,*Printer,*ShowDialog,*oid;
	int n;
	gboolean show;
	LargeByteString *lbs;

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	gtk_init(&argc,&argv);
	gtk_panda_init(&argc,&argv);
	gtk_set_locale();

	ConfigNum = getenv("GLPUSH_CONFIG_NUMBER");
	if (ConfigNum == NULL) {
		n = 0;
	} else {
		n = atoi(ConfigNum);
	}
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

	Title      = getenv("GLPUSH_TITLE");
	if (Title == NULL) {
		Title = "";
	}
	Printer    = getenv("GLPUSH_PRINTER");
	ShowDialog = getenv("GLPUSH_SHOW_DIALOG");
	if (ShowDialog != NULL && !strcmp("0",ShowDialog)) {
		show = FALSE;
	} else {
		show = TRUE;
	}
	if (Printer == NULL || strlen(Printer) <=0 ) {
		show = FALSE;
	}
	oid = getenv("GLPUSH_OID");

	gl_config_init();

	lbs = REST_GetBLOB_via_ENV();

	if (show) {
		ShowPrintDialog(Title,lbs);
	} else {
		Print(oid,Title,Printer,lbs);
	}
	FreeLBS(lbs);
	return 0;
}
