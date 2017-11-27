/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Kouji TAKAO & JMA (Japan Medical Association).
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
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<glib.h>
#include	<gtk/gtk.h>
#include	<gtkpanda/gtkpanda.h>
#include	<errno.h>
#include	<libmondai.h>

#include	"gettext.h"
#include	"print.h"
#include	"utils.h"
#include	"notify.h"
#include	"logger.h"
#include	"tempdir.h"

static char *DataDir = NULL;
static GHashTable *PrinterTable = NULL;;
static GHashTable *CopiesTable = NULL;

void
ShowPrintDialog(
	const char		*title,
	LargeByteString	*lbs)
{
	GtkWidget *dialog;
	GtkWidget *content;
	GtkWidget *pandapdf;
	gchar *_title;

	pandapdf = gtk_panda_pdf_new();
	if (!gtk_panda_pdf_set(GTK_PANDA_PDF(pandapdf),LBS_Size(lbs),LBS_Body(lbs))) {
		gtk_widget_destroy(pandapdf);
		return;
	}
	
	_title = g_strdup_printf(_("client printing - %s"),title);

	dialog = gtk_dialog_new_with_buttons(_("Preview"),NULL,
		GTK_DIALOG_MODAL,
		GTK_STOCK_CLOSE,
		GTK_RESPONSE_NONE,NULL);
	gtk_window_set_title(GTK_WINDOW(dialog),_title);
	gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
	gtk_window_set_keep_above(GTK_WINDOW(dialog),TRUE);
	gtk_widget_set_size_request(pandapdf,800,600);
	gtk_widget_set_size_request(dialog,800,600);
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(content),pandapdf);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
	g_free(_title);
}

static void
_Print(
	const char *title,
	const char *printer,
	int copies,
	LargeByteString *lbs)
{
	GtkWidget *pandapdf;
	char buf[1024];


	pandapdf = gtk_panda_pdf_new();
	if (!gtk_panda_pdf_set(GTK_PANDA_PDF(pandapdf),LBS_Size(lbs),LBS_Body(lbs))) {
		gtk_widget_destroy(pandapdf);
		return;
	}
	gtk_panda_pdf_print_with_printer(GTK_PANDA_PDF(pandapdf),copies,printer);
	snprintf(buf,sizeof(buf),_(
		"starting print\n \n"
		"title:%s\n"
		"printer:%s\n"),title,printer);
	Notify(_("glclient print notify"),buf,"gtk-print",0);
	gtk_widget_destroy(pandapdf);
}

static gboolean
CheckPrinter(const char *name)
{
	static GList *List = NULL;
	int i;
	
	if (List == NULL) {
		List = gtk_panda_pdf_get_printer_list();
	}
	for(i=0;i<g_list_length(List);i++) {
		if (!g_strcmp0(name,(char*)g_list_nth_data(List,i))) {
			return TRUE;
		}
	}
	return FALSE;
}

static void
InitPrinterTable()
{
	int i,cp;
	gchar **entries,**kv,*config,*k,*v;
	GRegex *re;
	GMatchInfo *match;

	PrinterTable = NewNameHash();
	CopiesTable = NewNameHash();

	config = getenv("GLPRINTER_CONFIG");
	if (config == NULL) {
		return;
	}
	
	re = g_regex_new("^(.*)#(\\d+)$",0,0,NULL);
	entries = g_strsplit(config,",",-1);
	for(i=0;entries[i]!=NULL;i++) {
		kv = g_strsplit(entries[i],":=:",-1);
		if (kv[0] != NULL && kv[1] != NULL && CheckPrinter(kv[1])) {
			if (g_regex_match(re,kv[0],0,&match)) {
				k = g_match_info_fetch(match,1);
				v = g_match_info_fetch(match,2);
				cp = atoi(v);
				g_hash_table_insert(PrinterTable,g_strdup(k),g_strdup(kv[1]));
				g_hash_table_insert(CopiesTable,g_strdup(k),GINT_TO_POINTER(cp));
				g_free(k);
				g_free(v);
				g_match_info_free(match);
			} else {
				g_hash_table_insert(PrinterTable,g_strdup(kv[0]),g_strdup(kv[1]));
				g_hash_table_insert(CopiesTable,g_strdup(kv[0]),GINT_TO_POINTER(1));
			}
		}
		g_strfreev(kv);
	}
	g_strfreev(entries);
	g_regex_unref(re);
}

void
Print(
	const char *oid,
	const char *title,
	const char *pname,
	LargeByteString *lbs)
{
	char buf[1024],path[1024],*printer;
	struct tm cur;
	time_t t;
	int cp;

	if (PrinterTable == NULL) {
		InitPrinterTable();
	}

	if (getenv("GLCLIENT_SAVE_PRINT_DATA") != NULL) {
		if (DataDir == NULL) {
			DataDir = MakeTempSubDir("print_data");
		}
		t = time(NULL);
		gmtime_r(&t,&cur);
		strftime(buf,sizeof(buf),"%Y%m%d%H%M%S",&cur);
		snprintf(path,sizeof(path),"%s/%s_%s_%p.pdf",DataDir,oid,buf,lbs);
		Warning(path);
		g_file_set_contents(path,LBS_Body(lbs),LBS_Size(lbs),NULL);
	}

	printer = (char*)g_hash_table_lookup(PrinterTable,pname);
	cp = GPOINTER_TO_INT(g_hash_table_lookup(CopiesTable,pname));
	if (printer == NULL) {
		printer = (char*)g_hash_table_lookup(PrinterTable,"default");
		cp = GPOINTER_TO_INT(g_hash_table_lookup(CopiesTable,"default"));
	}
	if (printer != NULL) {
		Info("%s %s(%s) %d",title,pname,printer,cp);
		_Print(title,printer,cp,lbs);
	} else {
		ShowPrintDialog(title,lbs);
	}
}
