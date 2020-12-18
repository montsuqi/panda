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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h> /* strlen */
#include <ctype.h>  /* isblank */
#include <glib.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>

#include "gettext.h"
#include "glclient.h"
#include "const.h"
#include "bd_config.h"
#include "bd_component.h"
#include "logviewer.h"
#include "action.h"
#include "dialogs.h"
#include "logger.h"

#define LINE_BUF_SIZE (1024)
static void update_list(GtkWidget *label,GtkWidget *list, const char *fname) {
  GtkListStore *model;
  GtkTreeIter iter;
  GRegex *re1,*re2,*re3;
  GMatchInfo *mi;
  FILE *fp;
  char buf[LINE_BUF_SIZE],*host,*datetime,*window,*widget,*event;
  char *rpc,*server,*app,*com;

  re1 = g_regex_new("start_session (.*)",0,0,NULL);
  re2 = g_regex_new("(\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}).*window\\:(.*) widget\\:(.*) event\\:(.*)",0,0,NULL);
  re3 = g_regex_new("rpc_total\\:(\\d+)(?:ms)? server_total\\:(\\d+)ms server_app\\:(\\d+)ms",0,0,NULL);

  model = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(list));
  gtk_list_store_clear(model);

  host = datetime = window = widget = event = NULL ;
  if ((fp = fopen(fname, "r")) == NULL) {
    Error(_("file open failure: %s"),fname);
    return;
  }
  while (fgets(buf,LINE_BUF_SIZE -1,fp) != NULL) {
    if (host == NULL) {
      g_regex_match(re1,buf,0,&mi);
      if (g_match_info_matches(mi)) {
        host = g_match_info_fetch(mi,1);
        gtk_label_set_text(GTK_LABEL(label),g_strdup_printf("host: %s",host));
      }
      g_match_info_free(mi);
    }

    g_regex_match(re2,buf,0,&mi);
    if (g_match_info_matches(mi)) {
      datetime = g_match_info_fetch(mi,1);
      window = g_match_info_fetch(mi,2);
      widget = g_match_info_fetch(mi,3);
      event = g_match_info_fetch(mi,4);
    }
    g_match_info_free(mi);

    g_regex_match(re3,buf,0,&mi);
    if (g_match_info_matches(mi)) {
      rpc = g_match_info_fetch(mi,1);
      server = g_match_info_fetch(mi,2);
      app = g_match_info_fetch(mi,3);
      com = g_strdup_printf("%d",(atoi(rpc) - atoi(server)));
      gtk_list_store_append(model, &iter);
      // g_match_info_fetchのret-valueはfreeしないといけない
      // そのためmodelの内容はleakするが気にしない
      gtk_list_store_set(model, &iter, 0, datetime, -1);
      gtk_list_store_set(model, &iter, 1, window, -1);
      gtk_list_store_set(model, &iter, 2, widget, -1);
      gtk_list_store_set(model, &iter, 3, event, -1);
      gtk_list_store_set(model, &iter, 4, com, -1);
      gtk_list_store_set(model, &iter, 5, server, -1);
      gtk_list_store_set(model, &iter, 6, app, -1);
    }
    g_match_info_free(mi);
  }
  g_regex_unref(re1);
  g_regex_unref(re2);
  g_regex_unref(re3);
  fclose(fp);
}

static void output_csv(const char *fname,void *data) {
  FILE *fp;
  char *val;
  int i;
  if ((fp = fopen(fname, "w")) == NULL) {
    Error(_("file open failure: %s"),fname);
    return;
  }
  GtkTreeModel *model = (GtkTreeModel*)data;
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      for(i=0;i<7;i++) {
        gtk_tree_model_get(model,&iter,i,&val,-1);
        if (i != 6) {
          fprintf(fp,"%s,",val);
        } else {
          fprintf(fp,"%s\n",val);
        }
        free(val);
      }
    } while(gtk_tree_model_iter_next(model,&iter));
  }
  fclose(fp);
}

static gboolean on_csv(GtkWidget *widget,void *data) {
  GtkWidget *dialog;
  gchar *fname;

  dialog = gtk_file_chooser_dialog_new(
      _("Specify filename..."), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
      GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "out.csv");
  gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    output_csv(fname,data);
    g_free(fname);
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
  return TRUE;
}

static gboolean on_close(GtkWidget *widget,void *data) {
  gtk_main_quit();
  return TRUE;
}

static void LogView(const char *fname) {
  GtkWidget *dialog, *button, *scroll, *list, *label, *vbox;
  GtkTreeModel *model;
  GtkCellRenderer *renderer;

  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), _("LogViewer"));
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
                   G_CALLBACK(on_close), NULL);

  /* contents */
  vbox = gtk_vbox_new(FALSE,2);
  label = gtk_label_new("");

  scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scroll, 850, 750);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     vbox, TRUE, TRUE, 5);

  model = (GtkTreeModel *)gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING, 
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  list = gtk_tree_view_new_with_model(model);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(list), -1, _("Datetime"), renderer, "text", 0, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(list), -1, _("Window"), renderer, "text", 1, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(list), -1, _("Widget"), renderer, "text", 2, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(list), -1, _("Event"), renderer, "text", 3, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(list), -1, _("ComTime"), renderer, "text", 4, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(list), -1, _("ServerTime"), renderer, "text", 5, NULL);
  gtk_tree_view_insert_column_with_attributes(
      GTK_TREE_VIEW(list), -1, _("AppTime"), renderer, "text", 6, NULL);

  gtk_container_add(GTK_CONTAINER(scroll), list);
  gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(vbox),scroll,TRUE,TRUE,2);

  /* csv out button */
  button = gtk_button_new_with_label(_("CSV Out"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(on_csv), model);
  gtk_widget_set_can_default(button, TRUE);

  /* close button */
  button = gtk_button_new_with_label(_("Close"));
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
                     button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(on_close), NULL);
  gtk_widget_set_can_default(button, TRUE);
  gtk_widget_grab_default(button);

  update_list(label, list, fname);

  gtk_widget_show_all(dialog);
  gtk_widget_grab_focus(dialog);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_main();
  gtk_widget_destroy(dialog);
}

void LogViewerRun() {
  GtkWidget *dialog;
  char *fname = NULL;

  dialog = gtk_file_chooser_dialog_new(
      _("Specify filename..."), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
      GTK_RESPONSE_ACCEPT, NULL);
  gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), GetLogDir());

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
  if (fname != NULL) {
    LogView(fname);
    g_free(fname);
  }
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
