/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <json.h>
#include <json_object_private.h> /*for json_object_object_foreachC()*/

#include "gettext.h"
#include "types.h"
#include "glclient.h"
#include "styleParser.h"
#include "protocol.h"
#include "widgetOPS.h"
#include "action.h"
#include "widgetcache.h"
#include "download.h"
#include "dialogs.h"
#include "notify.h"
#include "logger.h"

static void SetState(GtkWidget *widget, GtkStateType state) {
  if (state != GTK_STATE_INSENSITIVE) {
    gtk_widget_set_sensitive(widget, TRUE);
  } else {
    gtk_widget_set_sensitive(widget, FALSE);
  }
}

static void SetStyle(GtkWidget *widget, const char *str) {
  int i;
  GtkRcStyle *rc_style;
  GtkStyle *style;

  style = GetStyle((char *)str);

  if (style != NULL) {
    rc_style = gtk_widget_get_modifier_style(widget);
    for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++) {
      rc_style->fg[i] = style->fg[i];
      rc_style->bg[i] = style->bg[i];
      rc_style->text[i] = style->text[i];
      rc_style->base[i] = style->base[i];
      rc_style->color_flags[i] =
          GTK_RC_FG | GTK_RC_BG | GTK_RC_TEXT | GTK_RC_BASE;
    }
    gtk_widget_modify_style(widget, rc_style);
    if (GTK_IS_CONTAINER(widget)) {
      gtk_container_foreach(GTK_CONTAINER(widget), (GtkCallback)SetStyle,
                            (gpointer)str);
    }
  }
}

static void SetCommon(GtkWidget *widget, json_object *obj) {
  json_object *child;

  if (json_object_object_get_ex(obj, "state", &child)) {
    SetState(widget, json_object_get_int(child));
  }

  if (json_object_object_get_ex(obj, "style", &child)) {
    SetStyle(widget, json_object_get_string(child));
  }

  if (json_object_object_get_ex(obj, "visible", &child)) {
    gtk_widget_set_visible(widget, json_object_get_boolean(child));
  }
}

static void SetWidgetLabelRecursive(GtkWidget *widget, char *label) {
  if (GTK_IS_LABEL(widget)) {
    gtk_label_set_text(GTK_LABEL(widget), label);
  } else {
    gtk_container_foreach(GTK_CONTAINER(widget),
                          (GtkCallback)SetWidgetLabelRecursive, label);
  }
}

/******************************************************************************/
/* gtk+panda widget                                                           */
/******************************************************************************/

static void SetPandaPDF(GtkWidget *widget, json_object *obj) {
  LargeByteString *lbs;
  json_object *child;
  const char *oid;

  SetCommon(widget, obj);
  if (json_object_object_get_ex(obj, "objectdata", &child)) {
    oid = (const char *)json_object_get_string(child);
    lbs = REST_GetBLOB(GLP(Session), oid);
    if (lbs != NULL) {
      gtk_panda_pdf_set(GTK_PANDA_PDF(widget), LBS_Size(lbs), LBS_Body(lbs));
      FreeLBS(lbs);
    } else {
      gtk_panda_pdf_set(GTK_PANDA_PDF(widget), 0, NULL);
    }
  }
}

static void SetPandaTimer(GtkWidget *widget, json_object *obj) {
  json_object *child;

  SetCommon(widget, obj);
  if (json_object_object_get_ex(obj, "duration", &child)) {
    gtk_panda_timer_set(GTK_PANDA_TIMER(widget), json_object_get_int(child));
  }
}

static void GetPandaTimer(GtkWidget *widget, json_object *tmpl,
                          json_object *obj) {
  json_object_object_add(
      obj, "duration",
      json_object_new_int(GTK_PANDA_TIMER(widget)->duration / 1000));
}

static void SetPandaDownload(GtkWidget *widget, json_object *obj) {
  json_object *child;
  const char *filename, *desc, *oid;
  LargeByteString *lbs;

  filename = desc = NULL;

  SetCommon(widget, obj);
  if (json_object_object_get_ex(obj, "filename", &child)) {
    filename = json_object_get_string(child);
  }

  if (json_object_object_get_ex(obj, "description", &child)) {
    desc = json_object_get_string(child);
  }

  if (json_object_object_get_ex(obj, "objectdata", &child)) {
    oid = json_object_get_string(child);
    if (oid != NULL && strlen(oid) > 0 && strcmp(oid, "0")) {
      lbs = REST_GetBLOB(GLP(Session), oid);
      if (lbs != NULL && LBS_Size(lbs) > 0) {
        ShowDownloadDialog((char *)filename, (char *)desc, lbs);
        FreeLBS(lbs);
      }
    }
  }
}

static void SetNumberEntry(GtkWidget *widget, json_object *obj) {
  json_object *child;
  Numeric num;

  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "editable", &child)) {
    g_object_set(G_OBJECT(widget), "editable", json_object_get_boolean(child),
                 NULL);
  }

  if (json_object_object_get_ex(obj, "numdata", &child)) {
    num = DoubleToNumeric(json_object_get_double(child));
    gtk_number_entry_set_value(GTK_NUMBER_ENTRY(widget), num);
    NumericFree(num);
  }
}

static void GetNumberEntry(GtkWidget *widget, json_object *tmpl,
                           json_object *obj) {
  Numeric num;
  json_object *child;

  if (json_object_object_get_ex(tmpl, "numdata", &child)) {
    num = gtk_number_entry_get_value(GTK_NUMBER_ENTRY(widget));
    json_object_object_add(obj, "numdata",
                           json_object_new_double(NumericToDouble(num)));
    NumericFree(num);
  }
}

#define MAX_COMBO_ITEM (256)

static void SetPandaCombo(GtkWidget *widget, json_object *obj) {
  GtkPandaCombo *combo;
  json_object *child, *val;
  int count, n, i;
  char *item[MAX_COMBO_ITEM];

  SetCommon(widget, obj);
  count = 0;

  if (json_object_object_get_ex(obj, "count", &child)) {
    count = json_object_get_int(child);
  }

  if (json_object_object_get_ex(obj, "item", &child)) {
    n = json_object_array_length(child);
    if (count > 0) {
      n = n > count ? count : n;
    }
    n = n > MAX_COMBO_ITEM ? MAX_COMBO_ITEM : n;
    for (i = 0; i < MAX_COMBO_ITEM; i++) {
      item[i] = NULL;
    }
    item[0] = g_strdup("");
    for (i = 0; i < n; i++) {
      if (i < count) {
        val = json_object_array_get_idx(child, i);
        if (CheckJSONObject(val, json_type_string)) {
          item[i + 1] = g_strdup(json_object_get_string(val));
        }
      } else {
        item[i + 1] = NULL;
      }
    }
    combo = GTK_PANDA_COMBO(widget);
    gtk_panda_combo_set_popdown_strings(combo, item);
  }
}

#define MAX_CLIST_COLUMNS (256)

static void SetPandaCList(GtkWidget *widget, json_object *obj) {
  int count, n, m, i, j, row;
  double rowattr;
  char *rdata[MAX_CLIST_COLUMNS], name[16];
  json_object *child, *val, *rowobj, *colobj, *boolobj;

  SetCommon(widget, obj);
  gtk_widget_hide(widget);

  count = -1;

  if (json_object_object_get_ex(obj, "count", &child)) {
    count = json_object_get_int(child);
  }

  row = 0;
  if (json_object_object_get_ex(obj, "row", &child)) {
    row = json_object_get_int(child);
    row = row > 1 ? row - 1 : 0;
  }

  rowattr = 0.0;
  if (json_object_object_get_ex(obj, "rowattr", &child)) {
    switch (json_object_get_int(child)) {
    case 1: /* DOWN */
      rowattr = 1.0;
      break;
    case 2: /* MIDDLE */
      rowattr = 0.5;
      break;
    case 3: /* QUATER */
      rowattr = 0.25;
      break;
    case 4: /* THREE QUATER */
      rowattr = 0.75;
      break;
    default: /* [0] TOP */
      rowattr = 0.0;
      break;
    }
  }

  if (json_object_object_get_ex(obj, "item", &child)) {
    n = json_object_array_length(child);
    if (count < 0 || count > n) {
      count = n;
    }
    gtk_panda_clist_set_rows(GTK_PANDA_CLIST(widget), count);
    n = n > count ? count : n;
    m = gtk_panda_clist_get_columns(GTK_PANDA_CLIST(widget));
    for (i = 0; i < n; i++) {
      rowobj = json_object_array_get_idx(child, i);
      if (!CheckJSONObject(rowobj, json_type_object)) {
        continue;
      }
      for (j = 0; j < MAX_CLIST_COLUMNS; j++) {
        rdata[j] = NULL;
      }
      for (j = 0; j < m; j++) {
        sprintf(name, "column%d", j + 1);
        if (json_object_object_get_ex(rowobj, name, &colobj)) {
          rdata[j] = (char *)json_object_get_string(colobj);
        } else {
          rdata[j] = "";
        }
      }
      gtk_panda_clist_set_row(GTK_PANDA_CLIST(widget), i, rdata);
    }
  }

  if (json_object_object_get_ex(obj, "fgcolor", &child)) {
    n = json_object_array_length(child);
    n = n > count ? count : n;
    for (i = 0; i < n; i++) {
      val = json_object_array_get_idx(child, i);
      if (json_object_is_type(val, json_type_string)) {
        gtk_panda_clist_set_fgcolor(GTK_PANDA_CLIST(widget), i,
                                    (char *)json_object_get_string(val));
      }
    }
  }

  if (json_object_object_get_ex(obj, "bgcolor", &child)) {
    n = json_object_array_length(child);
    n = n > count ? count : n;
    for (i = 0; i < n; i++) {
      val = json_object_array_get_idx(child, i);
      if (json_object_is_type(val, json_type_string)) {
        gtk_panda_clist_set_bgcolor(GTK_PANDA_CLIST(widget), i,
                                    (char *)json_object_get_string(val));
      }
    }
  }
  gtk_widget_show(widget);

  if (json_object_object_get_ex(obj, "selectdata", &child)) {
    n = json_object_array_length(child);
    n = n > count ? count : n;
    for (i = 0; i < n; i++) {
      boolobj = json_object_array_get_idx(child, i);
      if (!CheckJSONObject(boolobj, json_type_boolean)) {
        continue;
      }
      if (json_object_get_boolean(boolobj)) {
        gtk_panda_clist_select_row(GTK_PANDA_CLIST(widget), i, 0);
      } else {
        gtk_panda_clist_unselect_row(GTK_PANDA_CLIST(widget), i, 0);
      }
    }
  }

  if (count > 0 && row < count) {
    gtk_panda_clist_moveto(GTK_PANDA_CLIST(widget), row, 0, rowattr, 0.0);
  }
}

static void GetPandaCList(GtkWidget *widget, json_object *tmpl,
                          json_object *obj) {
  int i, nrows, visible, row, selected;
  json_object *child, *array;

  row = 0;
  visible = 0;
  nrows = gtk_panda_clist_get_rows(GTK_PANDA_CLIST(widget));
  for (i = 0; i < nrows; i++) {
    if (!visible &&
        gtk_panda_clist_row_is_visible(GTK_PANDA_CLIST(widget), i)) {
      row = i + 1;
      visible = 1;
    }
  }

  if (visible) {
    if (json_object_object_get_ex(tmpl, "row", &child)) {
      json_object_object_add(obj, "row", json_object_new_int(row));
    }
  }

  if (json_object_object_get_ex(tmpl, "selectdata", &child)) {
    nrows = gtk_panda_clist_get_rows(GTK_PANDA_CLIST(widget));
    array = json_object_new_array();
    json_object_object_add(obj, "selectdata", array);
    for (i = 0; i < nrows; i++) {
      selected = gtk_panda_clist_row_is_selected(GTK_PANDA_CLIST(widget), i);
      json_object_array_add(array, json_object_new_boolean(selected));
    }
  }
}

static void SetPandaHTML(GtkWidget *widget, json_object *obj) {
  json_object *child;

  SetCommon(widget, obj);
  if (json_object_object_get_ex(obj, "uri", &child)) {
    gtk_panda_html_set_uri(GTK_PANDA_HTML(widget),
                           json_object_get_string(child));
  }
}

static GtkWidget *panda_table = NULL;
static int panda_table_row;
static int panda_table_column;
static double panda_table_rowattr;

static void SetPandaTable(GtkWidget *widget, json_object *obj) {
  int n, i, j, trow, tcolumn;
  double trowattr;
  json_object *child, *child2, *rowobj, *colobj;
  char *rowval[GTK_PANDA_TABLE_MAX_COLS];
  char *fgval[GTK_PANDA_TABLE_MAX_COLS];
  char *bgval[GTK_PANDA_TABLE_MAX_COLS];
  char name[16];

  SetCommon(widget, obj);

  trow = 0;
  if (json_object_object_get_ex(obj, "trow", &child)) {
    trow = json_object_get_int(child);
    if (trow >= 1) {
      trow -= 1;
    }
  }

  trowattr = 0.0;
  if (json_object_object_get_ex(obj, "trowattr", &child)) {
    switch (json_object_get_int(child)) {
    case 1: /* DOWN */
      trowattr = 1.0;
      break;
    case 2: /* MIDDLE */
      trowattr = 0.5;
      break;
    case 3: /* QUATER */
      trowattr = 0.25;
      break;
    case 4: /* THREE QUATER */
      trowattr = 0.75;
      break;
    default: /* [0] TOP */
      trowattr = 0.0;
      break;
    }
  }

  tcolumn = 0;
  if (json_object_object_get_ex(obj, "tcolumn", &child)) {
    tcolumn = json_object_get_int(child);
    if (tcolumn >= 1) {
      tcolumn -= 1;
    }
  }

  n = gtk_panda_table_get_columns(GTK_PANDA_TABLE(widget));

  if (json_object_object_get_ex(obj, "rowdata", &child)) {
    for (i = 0; i < json_object_array_length(child); i++) {
      rowobj = json_object_array_get_idx(child, i);
      if (!CheckJSONObject(rowobj, json_type_object)) {
        continue;
      }
      for (j = 0; j < GTK_PANDA_TABLE_MAX_COLS; j++) {
        rowval[j] = NULL;
        fgval[j] = NULL;
        bgval[j] = NULL;
      }
      for (j = 0; j < n; j++) {
        sprintf(name, "column%d", j + 1);
        if (!json_object_object_get_ex(rowobj, name, &colobj)) {
          continue;
        }
        if (json_object_object_get_ex(colobj, "celldata", &child2)) {
          rowval[j] = (char *)json_object_get_string(child2);
        }
        if (json_object_object_get_ex(colobj, "fgcolor", &child2)) {
          fgval[j] = (char *)json_object_get_string(child2);
        }
        if (json_object_object_get_ex(colobj, "bgcolor", &child2)) {
          bgval[j] = (char *)json_object_get_string(child2);
        }
      }
      gtk_panda_table_set_row(GTK_PANDA_TABLE(widget), i, rowval);
      gtk_panda_table_set_fgcolor(GTK_PANDA_TABLE(widget), i, fgval);
      gtk_panda_table_set_bgcolor(GTK_PANDA_TABLE(widget), i, bgval);
    }
  }

  panda_table = widget;
  panda_table_row = trow;
  panda_table_column = tcolumn;
  panda_table_rowattr = trowattr;

  _AddChangedWidget(widget);
}

void PandaTableFocusCell(const char *wname) {
  if (panda_table == NULL) {
    return;
  }
  if (strcmp(wname, gtk_widget_get_name(panda_table))) {
    return;
  }
  if (panda_table_row >= 0 && panda_table_column >= 0) {
    gtk_panda_table_moveto(GTK_PANDA_TABLE(panda_table), panda_table_row,
                           panda_table_column, TRUE, panda_table_rowattr, 0.0);
  }
  if (fKeyBuff) {
    if (GTK_WIDGET_VISIBLE(panda_table) && GTK_WIDGET_DRAWABLE(panda_table)) {
      gdk_window_process_updates(panda_table->window, FALSE);
    }
  }
  panda_table = NULL;
}

static void GetPandaTable(GtkWidget *widget, json_object *tmpl,
                          json_object *obj) {
  GtkTreeModel *model;
  GtkTreeIter iter;
  int n, i, j, k, trow, tcolumn;
  gchar *tvalue, name[16];
  json_object *child, *rowdata, *colobj, *cell, *array;
  GList *list, *l;

  gtk_widget_child_focus(widget, GTK_DIR_TAB_FORWARD);
  trow =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "changed_row")) + 1;
  tcolumn =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "changed_column")) +
      1;
  tvalue = (gchar *)g_object_get_data(G_OBJECT(widget), "changed_value");
  ;
  tvalue = tvalue == NULL ? (gchar *)"" : tvalue;
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
  gtk_tree_model_get_iter_first(model, &iter);

  if (json_object_object_get_ex(tmpl, "trow", &child)) {
    json_object_object_add(obj, "trow", json_object_new_int(trow));
  }

  if (json_object_object_get_ex(tmpl, "tcolumn", &child)) {
    json_object_object_add(obj, "tcolumn", json_object_new_int(tcolumn));
  }

  if (json_object_object_get_ex(tmpl, "tvalue", &child)) {
    json_object_object_add(obj, "tvalue", json_object_new_string(tvalue));
  }

  list = NULL;
  n = gtk_panda_table_get_columns(GTK_PANDA_TABLE(widget));
  if (json_object_object_get_ex(tmpl, "rowdata", &child)) {
    i = k = 0;
    do {
      rowdata = json_object_array_get_idx(child, i);
      if (!CheckJSONObject(rowdata, json_type_object)) {
        Warning("rowdata not found");
        return;
      }
      for (j = 0; j < n; j++) {
        sprintf(name, "column%d", j + 1);
        if (!json_object_object_get_ex(rowdata, name, &colobj)) {
          Warning("%s not found", name);
          return;
        }
        if (!json_object_object_get_ex(colobj, "celldata", &cell)) {
          Warning("celldata not found", name);
          return;
        }
        gtk_tree_model_get(model, &iter, j, &tvalue, -1);
        if (strcmp(tvalue, json_object_get_string(cell))) {
          k = i + 1;
        }
        list = g_list_append(list, tvalue);
      }
      i += 1;
    } while (gtk_tree_model_iter_next(model, &iter));

    array = json_object_new_array();
    json_object_object_add(obj, "rowdata", array);
    l = list;
    for (i = 0; i < k; i++) {
      rowdata = json_object_new_object();
      json_object_array_add(array, rowdata);
      for (j = 0; j < n; j++) {
        sprintf(name, "column%d", j + 1);
        colobj = json_object_new_object();
        json_object_object_add(rowdata, name, colobj);
        json_object_object_add(colobj, "celldata",
                               json_object_new_string((char *)l->data));
        l = l->next;
      }
    }
    for (l = list; l != NULL; l = l->next) {
      g_free(l->data);
    }
  }
}

/******************************************************************************/
/* gtk+ widget                                                                */
/******************************************************************************/

static void SetEntry(GtkWidget *widget, json_object *obj) {
  json_object *child;
  char *prev, *next;

  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "editable", &child)) {
    g_object_set(G_OBJECT(widget), "editable", json_object_get_boolean(child),
                 NULL);
  }

  if (json_object_object_get_ex(obj, "textdata", &child)) {
    prev = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
    next = (char *)json_object_get_string(child);
    if (strcmp(prev, next)) {
      gtk_entry_set_text(GTK_ENTRY(widget), next);
    }
    g_free(prev);
  }

  if (!gtk_widget_has_focus(widget) &&
      (gtk_editable_get_position(GTK_EDITABLE(widget)) != 0)) {
    gtk_editable_set_position(GTK_EDITABLE(widget), 0);
  }
}

static void GetEntry(GtkWidget *widget, json_object *tmpl, json_object *obj) {
  json_object *child;
  char *text;

  text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  if (json_object_object_get_ex(tmpl, "textdata", &child)) {
    json_object_object_add(obj, "textdata", json_object_new_string(text));
  }
  g_free(text);
}

static void SetLabel(GtkWidget *widget, json_object *obj) {
  json_object *child;
  char *text;

  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "textdata", &child)) {
    text = (char *)json_object_get_string(child);
    if (pango_parse_markup(text, -1, 0, NULL, NULL, NULL, NULL)) {
      gtk_label_set_markup(GTK_LABEL(widget), text);
    } else {
      gtk_label_set_text(GTK_LABEL(widget), text);
    }
  }
}

static void SetText(GtkWidget *widget, json_object *obj) {
  char *text;
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  json_object *child;

  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "textdata", &child)) {
    text = (char *)json_object_get_string(child);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    gtk_text_buffer_set_text(buffer, text, strlen(text));
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget), &iter, 0, 0);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(widget), &iter, 0.0, TRUE, 0.0,
                                 0.0);
  }
}

static void GetText(GtkWidget *widget, json_object *tmpl, json_object *obj) {
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;
  json_object *child;
  char *text;

  if (json_object_object_get_ex(tmpl, "textdata", &child)) {
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    json_object_object_add(obj, "textdata", json_object_new_string(text));
    g_free(text);
  }
}

static void SetButton(GtkWidget *widget, json_object *obj) {
  json_object *child;

  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "label", &child)) {
    SetWidgetLabelRecursive(widget, (char *)json_object_get_string(child));
  }

  if (json_object_object_get_ex(obj, "isactive", &child)) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                 json_object_get_boolean(child));
  }
}

static void GetButton(GtkWidget *widget, json_object *tmpl, json_object *obj) {
  gboolean isactive;
  json_object *child;

  if (json_object_object_get_ex(tmpl, "isactive", &child)) {
    isactive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    json_object_object_add(obj, "isactive", json_object_new_boolean(isactive));
  }
}

static void SetCalendar(GtkWidget *widget, json_object *obj) {
  json_object *child;
  int y, m, d;

  SetCommon(widget, obj);
  y = m = d = 0;

  if (json_object_object_get_ex(obj, "year", &child)) {
    y = json_object_get_int(child);
  }

  if (json_object_object_get_ex(obj, "month", &child)) {
    m = json_object_get_int(child);
  }

  if (json_object_object_get_ex(obj, "day", &child)) {
    d = json_object_get_int(child);
  }

  if (y > 0) {
    gtk_calendar_select_month(GTK_CALENDAR(widget), m - 1, y);
    gtk_calendar_select_day(GTK_CALENDAR(widget), d);
  }
}

static void GetCalendar(GtkWidget *widget, json_object *tmpl,
                        json_object *obj) {
  json_object *child;
  int y, m, d;

  gtk_calendar_get_date(GTK_CALENDAR(widget), &y, &m, &d);
  if (json_object_object_get_ex(tmpl, "year", &child)) {
    json_object_object_del(obj, "year");
    json_object_object_add(obj, "year", json_object_new_int(y));
  }

  if (json_object_object_get_ex(tmpl, "month", &child)) {
    json_object_object_del(obj, "month");
    json_object_object_add(obj, "month", json_object_new_int(m + 1));
  }

  if (json_object_object_get_ex(tmpl, "day", &child)) {
    json_object_object_del(obj, "day");
    json_object_object_add(obj, "day", json_object_new_int(d));
  }
}

static void SetNotebook(GtkWidget *widget, json_object *obj) {
  json_object *child;

  SetCommon(widget, obj);
  if (json_object_object_get_ex(obj, "pageno", &child)) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(widget),
                                  json_object_get_int(child));
  }
}

static void GetNotebook(GtkWidget *widget, json_object *tmpl,
                        json_object *obj) {
  json_object *child;
  int pageno;

  if (json_object_object_get_ex(tmpl, "pageno", &child)) {
    pageno = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "new_pageno"));
    json_object_object_add(obj, "pageno", json_object_new_int(pageno));
  }
}

static void SetProgressBar(GtkWidget *widget, json_object *obj) {
  json_object *child;
  int i;

  SetCommon(widget, obj);
  if (json_object_object_get_ex(obj, "value", &child)) {
    i = json_object_get_int(child);
#ifdef LIBGTK_3_0_0
    gtk_panda_progress_bar_set_value(GTK_PANDA_PROGRESS_BAR(widget), i);
#else
    gtk_progress_set_value(GTK_PROGRESS(widget), i);
#endif
  }
}

static void SetWindow(GtkWidget *widget, json_object *obj) {
  json_object *child;
  char *summary, *body, *icon;
  int timeout;

#if 0
	SetCommon(widget,obj);
#endif
  if (json_object_object_get_ex(obj, "bgcolor", &child)) {
    SetSessionBGColor(json_object_get_string(child));
  }

  if (json_object_object_get_ex(obj, "title", &child)) {
    SetSessionTitle(json_object_get_string(child));
  }
  SetTitle(TopWindow);

  summary = NULL;
  if (json_object_object_get_ex(obj, "popup_summary", &child)) {
    summary = (char *)json_object_get_string(child);
  }

  body = NULL;
  if (json_object_object_get_ex(obj, "popup_body", &child)) {
    body = (char *)json_object_get_string(child);
  }

  icon = NULL;
  if (json_object_object_get_ex(obj, "popup_icon", &child)) {
    icon = (char *)json_object_get_string(child);
  }

  timeout = 0;
  if (json_object_object_get_ex(obj, "popup_timeout", &child)) {
    timeout = json_object_get_int(child);
  }

  if (summary != NULL && strlen(summary) > 0 && body != NULL &&
      strlen(body) > 0) {
    Notify(summary, body, icon, timeout);
  }
}

static void SetFrame(GtkWidget *widget, json_object *obj) {
  json_object *child;

  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "label", &child)) {
    gtk_frame_set_label(GTK_FRAME(widget), json_object_get_string(child));
  }
}

static void SetScrolledWindow(GtkWidget *widget, json_object *obj) {
  SetCommon(widget, obj);
}

static void SetFileChooserButton(GtkWidget *widget, json_object *obj) {
  GtkFileChooserButton *fcb;
  gchar *folder, *longname;

  SetCommon(widget, obj);

  fcb = GTK_FILE_CHOOSER_BUTTON(widget);
  gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(fcb));
  longname = (char *)glade_get_widget_long_name(widget);
  folder = (char *)GetWidgetCache(longname);
  if (folder == NULL) {
    folder = "";
  }
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fcb), folder);
  _AddChangedWidget(widget);
}

static void GetFileChooserButton(GtkWidget *widget, json_object *tmpl,
                                 json_object *obj) {
  GtkFileChooserButton *fcb;
  GError *error = NULL;
  gchar *contents;
  gchar *fname, *basename;
  gsize size;
  gchar *folder;
  char *longname, *oid;
  LargeByteString *lbs;
  json_object *child;

  fcb = GTK_FILE_CHOOSER_BUTTON(widget);
  fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcb));
  if (fname == NULL) {
    return;
  }
  if (!g_file_get_contents(fname, &contents, &size, &error)) {
    g_error_free(error);
    return;
  }
  lbs = NewLBS();
  LBS_ReserveSize(lbs, size, FALSE);
  memcpy(LBS_Body(lbs), contents, size);
  g_free(contents);
  oid = REST_PostBLOB(GLP(Session), lbs);
  FreeLBS(lbs);

  if (json_object_object_get_ex(tmpl, "objectdata", &child)) {
    if (strlen(oid) > 0) {
      json_object_object_add(obj, "objectdata", json_object_new_string(oid));
    }
  }
  if (json_object_object_get_ex(tmpl, "filename", &child)) {
    basename = g_path_get_basename(fname);
    json_object_object_add(obj, "filename", json_object_new_string(basename));
    g_free(basename);
  }

  longname = (char *)glade_get_widget_long_name(widget);
  folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(fcb));
  if (folder != NULL) {
    SetWidgetCache(longname, folder);
    g_free(folder);
  }
  g_free(oid);
}

static void SetColorButton(GtkWidget *widget, json_object *obj) {
  GtkColorButton *cb;
  GdkColor color;
  char *strcolor;
  json_object *child;

  cb = GTK_COLOR_BUTTON(widget);
  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "color", &child)) {
    strcolor = (char *)json_object_get_string(child);
    if (gdk_color_parse(strcolor, &color)) {
      gtk_color_button_set_color(cb, &color);
    }
  }
}

static void GetColorButton(GtkWidget *widget, json_object *tmpl,
                           json_object *obj) {
  GtkColorButton *cb;
  GdkColor color;
  char strcolor[256];
  json_object *child;

  cb = GTK_COLOR_BUTTON(widget);
  gtk_color_button_get_color(cb, &color);
  sprintf(strcolor, "#%02X%02X%02X", (guint)(color.red / 256.0),
          (guint)(color.green / 256.0), (guint)(color.blue / 256.0));

  if (json_object_object_get_ex(tmpl, "color", &child)) {
    json_object_object_add(obj, "color", json_object_new_string(strcolor));
  }
}

static void SetPixmap(GtkWidget *widget, json_object *obj) {
  LargeByteString *lbs;
  json_object *child;
  const char *oid;

  SetCommon(widget, obj);

  if (json_object_object_get_ex(obj, "objectdata", &child)) {
    oid = (const char *)json_object_get_string(child);
    lbs = REST_GetBLOB(GLP(Session), oid);
    if (lbs != NULL && LBS_Size(lbs) > 0) {
      gtk_widget_show(widget);
      gtk_panda_pixmap_set_image(GTK_PANDA_PIXMAP(widget),
                                 (gchar *)LBS_Body(lbs), (gsize)LBS_Size(lbs));
      FreeLBS(lbs);
    } else {
      gtk_widget_hide(widget);
    }
  }
}

static void SetWidgetData(GtkWidget *widget, json_object *info) {
  long type;

  if (widget == NULL) {
    return;
  }

  type = (long)(G_OBJECT_TYPE(widget));
  if (type == GTK_TYPE_NUMBER_ENTRY) {
    SetNumberEntry(widget, info);
  } else if (type == GTK_PANDA_TYPE_COMBO) {
    SetPandaCombo(widget, info);
  } else if (type == GTK_PANDA_TYPE_CLIST) {
    SetPandaCList(widget, info);
  } else if (type == GTK_PANDA_TYPE_ENTRY) {
    SetEntry(widget, info);
  } else if (type == GTK_PANDA_TYPE_TEXT) {
    SetText(widget, info);
  } else if (type == GTK_PANDA_TYPE_PDF) {
    SetPandaPDF(widget, info);
  } else if (type == GTK_PANDA_TYPE_TIMER) {
    SetPandaTimer(widget, info);
  } else if (type == GTK_PANDA_TYPE_DOWNLOAD) {
    SetPandaDownload(widget, info);
  } else if (type == GTK_PANDA_TYPE_HTML) {
    SetPandaHTML(widget, info);
  } else if (type == GTK_PANDA_TYPE_TABLE) {
    SetPandaTable(widget, info);
  } else if (type == GTK_TYPE_WINDOW) {
    SetWindow(widget, info);
  } else if (type == GTK_TYPE_LABEL) {
    SetLabel(widget, info);
  } else if (type == GTK_TYPE_BUTTON) {
    SetButton(widget, info);
  } else if (type == GTK_TYPE_TOGGLE_BUTTON) {
    SetButton(widget, info);
  } else if (type == GTK_TYPE_CHECK_BUTTON) {
    SetButton(widget, info);
  } else if (type == GTK_TYPE_RADIO_BUTTON) {
    SetButton(widget, info);
  } else if (type == GTK_TYPE_CALENDAR) {
    SetCalendar(widget, info);
  } else if (type == GTK_TYPE_NOTEBOOK) {
    SetNotebook(widget, info);
  } else if (type == GTK_TYPE_PROGRESS_BAR) {
    SetProgressBar(widget, info);
  } else if (type == GTK_TYPE_FRAME) {
    SetFrame(widget, info);
  } else if (type == GTK_TYPE_SCROLLED_WINDOW) {
    SetScrolledWindow(widget, info);
  } else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON) {
    SetFileChooserButton(widget, info);
  } else if (type == GTK_TYPE_COLOR_BUTTON) {
    SetColorButton(widget, info);
  } else if (type == GTK_PANDA_TYPE_PIXMAP) {
    SetPixmap(widget, info);
  }
}

extern void UpdateWidget(GtkWidget *widget, json_object *data) {
  json_object_iter iter;
  char childname[SIZE_LONGNAME + 1];
  GtkWidget *child;

  if (widget == NULL) {
    Warning("do not reach");
    return;
  }
  SetWidgetData(widget, data);
  if (json_object_get_type(data) == json_type_object) {
    json_object_object_foreachC(data, iter) {
      snprintf(childname, SIZE_LONGNAME, "%s.%s",
               glade_get_widget_long_name(widget), iter.key);
      childname[SIZE_LONGNAME] = 0;
      child = GetWidgetByLongName(childname);
      if (child != NULL) {
        UpdateWidget(child, iter.val);
      }
    }
  }
}

static void GetWidgetData(GtkWidget *widget, json_object *tmpl,
                          json_object *obj) {
  long type;

  if (widget == NULL) {
    return;
  }
  if (obj == NULL || !json_object_is_type(obj, json_type_object)) {
    return;
  }

  type = (long)(G_OBJECT_TYPE(widget));
  // gtk+panda
  if (type == GTK_TYPE_NUMBER_ENTRY) {
    GetNumberEntry(widget, tmpl, obj);
  } else if (type == GTK_PANDA_TYPE_CLIST) {
    GetPandaCList(widget, tmpl, obj);
  } else if (type == GTK_PANDA_TYPE_ENTRY) {
    GetEntry(widget, tmpl, obj);
  } else if (type == GTK_PANDA_TYPE_TEXT) {
    GetText(widget, tmpl, obj);
  } else if (type == GTK_PANDA_TYPE_TIMER) {
    GetPandaTimer(widget, tmpl, obj);
  } else if (type == GTK_PANDA_TYPE_TABLE) {
    GetPandaTable(widget, tmpl, obj);
  } else if (type == GTK_TYPE_BUTTON || type == GTK_TYPE_TOGGLE_BUTTON ||
             type == GTK_TYPE_CHECK_BUTTON || type == GTK_TYPE_RADIO_BUTTON) {
    GetButton(widget, tmpl, obj);
  } else if (type == GTK_TYPE_CALENDAR) {
    GetCalendar(widget, tmpl, obj);
  } else if (type == GTK_TYPE_NOTEBOOK) {
    GetNotebook(widget, tmpl, obj);
  } else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON) {
    GetFileChooserButton(widget, tmpl, obj);
  } else if (type == GTK_TYPE_COLOR_BUTTON) {
    GetColorButton(widget, tmpl, obj);
  } else {
    Warning("invalid widget [%s]", gtk_widget_get_name(widget));
  }
}

extern json_object *_MakeScreenData(GtkWidget *widget, json_object *w,
                                    WindowData *wdata, gboolean *changed) {
  char childname[SIZE_LONGNAME + 1];
  const char *longname;
  json_object *obj, *child_obj;
  json_object_iter iter;
  enum json_type type;
  gboolean _changed;
  GtkWidget *child;

  _changed = FALSE;
  obj = json_object_new_object();
  if (widget == NULL) {
    return obj;
  }
  longname = glade_get_widget_long_name(widget);
  type = json_object_get_type(w);
  if (type == json_type_object) {
    json_object_object_foreachC(w, iter) {
      snprintf(childname, SIZE_LONGNAME, "%s.%s", longname, iter.key);
      childname[SIZE_LONGNAME] = 0;
      child = GetWidgetByLongName(childname);
      if (child != NULL) {
        _changed = FALSE;
        child_obj = _MakeScreenData(child, iter.val, wdata, &_changed);
        if (_changed) {
          json_object_object_add(obj, iter.key, child_obj);
          *changed = TRUE;
        } else {
          json_object_put(child_obj);
        }
      }
    }
    if (g_hash_table_lookup(wdata->ChangedWidgetTable, longname) != NULL) {
      GetWidgetData(widget, w, obj);
      *changed = TRUE;
    }
  }
  return obj;
}

static gboolean RemoveChangedWidget(gpointer key, gpointer value,
                                    gpointer user_data) {
  g_free((char *)key);
  return TRUE;
}

extern json_object *MakeScreenData(const char *wname) {
  json_object *ret, *result, *window_data, *windows, *w, *window;
  int i, length;
  WindowData *wdata;
  GtkWidget *widget;
  gboolean changed;

  if (*wname == '_') {
    return json_object_new_object();
  }

  ret = NULL;
  json_object_object_get_ex(SCREENDATA(Session), "result", &result);
  json_object_object_get_ex(result, "window_data", &window_data);
  json_object_object_get_ex(window_data, "windows", &windows);
  length = json_object_array_length(windows);
  for (i = 0; i < length; i++) {
    w = json_object_array_get_idx(windows, i);
    json_object_object_get_ex(w, "window", &window);
    if (!strcmp(wname, json_object_get_string(window))) {
      wdata = GetWindowData(wname);
      widget = GetWidgetByLongName(wname);
      ret = _MakeScreenData(widget, wdata->tmpl, wdata, &changed);
      g_hash_table_foreach_remove(wdata->ChangedWidgetTable,
                                  RemoveChangedWidget, NULL);
    }
  }
  if (ret == NULL) {
    Error("could not make screen_data for window[%s]", wname);
  }
  return ret;
}
