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
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<errno.h>
#include	<gtkpanda/gtkpanda.h>
#include	<gdk/gdk.h>
#include	<gtk/gtk.h>
#include 	"gettext.h"
#include	<gdk-pixbuf/gdk-pixbuf.h>

#include	"types.h"
#include	"glclient.h"
#include	"styleParser.h"
#include	"protocol.h"
#include	"widgetOPS.h"
#include	"action.h"
#include	"widgetcache.h"
#include	"download.h"
#include	"dialogs.h"
#include	"printservice.h"
#include	"notify.h"
#include	"message.h"
#include	"debug.h"

static	Bool
CheckJSONObject(
	json_object *obj,
	enum json_type type)
{
	if (obj == NULL) {
		return FALSE;
	}
	if (is_error(obj)) {
		return FALSE;
	}
	if (!json_object_is_type(obj,type)) {
		return FALSE;
	}
	return TRUE;
}

static	void
SetState(
	GtkWidget *widget,
	GtkStateType state)
{
	if (state != GTK_STATE_INSENSITIVE) {
		gtk_widget_set_sensitive(widget,TRUE);
	}
	gtk_widget_set_state(widget,state);
}

static	void
SetStyle(
	GtkWidget *widget,
	const char *str)
{
	int i;
	GtkRcStyle *rc_style;
	GtkStyle *style;

	style = GetStyle((char*)str);

	if (style != NULL) {
		rc_style = gtk_widget_get_modifier_style(widget);
		for(i = GTK_STATE_NORMAL ; i <= GTK_STATE_INSENSITIVE; i++) {
			rc_style->fg[i] = style->fg[i];
			rc_style->bg[i] = style->bg[i];
			rc_style->text[i] = style->text[i];
			rc_style->base[i] = style->base[i];
			rc_style->color_flags[i] = 
				GTK_RC_FG | GTK_RC_BG | GTK_RC_TEXT | GTK_RC_BASE;
		}
		gtk_widget_modify_style(widget, rc_style);
    	if (GTK_IS_CONTAINER(widget)){
    	    gtk_container_foreach(GTK_CONTAINER(widget),
				(GtkCallback)SetStyle,(gpointer)str);
    	}
	}
}

static void
SetCommon(
	GtkWidget	*widget,
	json_object *obj)
{
	json_object *child;

	child = json_object_object_get(obj,"state");
	if (CheckJSONObject(child,json_type_int)) {
		SetState(widget,json_object_get_int(child));
	}

	child = json_object_object_get(obj,"style");
	if (CheckJSONObject(child,json_type_string)) {
		SetStyle(widget,json_object_get_string(child));
	}

	child = json_object_object_get(obj,"visible");
	if (CheckJSONObject(child,json_type_boolean)) {
		gtk_widget_set_visible(widget,json_object_get_boolean(child));
	}
}

static	void
SetWidgetLabelRecursive(
	GtkWidget	*widget,
	char		*label)
{
	if (GTK_IS_LABEL(widget)) {
		gtk_label_set_text(GTK_LABEL(widget),label);
	} else {
		gtk_container_foreach(GTK_CONTAINER(widget),
			(GtkCallback)SetWidgetLabelRecursive,label);
	}
}


/******************************************************************************/
/* gtk+panda widget                                                           */
/******************************************************************************/

static	void
SetPandaPDF(
	GtkWidget	*widget,
	json_object	*obj)
{
	LargeByteString *lbs;
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"objectdata");
	if (CheckJSONObject(child,json_type_string)) {
		lbs = REST_GetBLOB((char*)json_object_get_string(child));
		if (lbs != NULL) {
			gtk_panda_pdf_set(GTK_PANDA_PDF(widget),
				LBS_Size(lbs),LBS_Body(lbs));
			FreeLBS(lbs);
		} else {
			gtk_panda_pdf_set(GTK_PANDA_PDF(widget),0,NULL);
		}
	}
LEAVE_FUNC;
}

static	void
SetPandaTimer(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"duration");
	if (CheckJSONObject(child,json_type_int)) {
		gtk_panda_timer_set(GTK_PANDA_TIMER(widget),
			json_object_get_int(child));
	}
LEAVE_FUNC;
}

static	void
GetPandaTimer(
	GtkWidget	*widget,
	json_object	*obj)
{
ENTER_FUNC;
	json_object_object_add(obj,"duration",
		json_object_new_int(GTK_PANDA_TIMER(widget)->duration / 1000));
LEAVE_FUNC;
}

static	void
SetPandaDownload(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	const char *filename, *desc, *oid;
	LargeByteString *lbs;
ENTER_FUNC;
	filename = desc = NULL;

	SetCommon(widget,obj);
	child = json_object_object_get(obj,"filename");
	if (CheckJSONObject(child,json_type_string)) {
		filename = json_object_get_string(child);
	}

	child = json_object_object_get(obj,"description");
	if (CheckJSONObject(child,json_type_string)) {
		desc = json_object_get_string(child);
	}

	child = json_object_object_get(obj,"objectdata");
	if (CheckJSONObject(child,json_type_string)) {
		oid = json_object_get_string(child);
		if (oid != NULL && strlen(oid) > 0 && strcmp(oid,"0")) {
			lbs = REST_GetBLOB(oid);
			if (lbs != NULL && LBS_Size(lbs) > 0) {
				ShowDownloadDialog(widget,(char*)filename,(char*)desc,lbs);
				FreeLBS(lbs);
			}
		}
	}
LEAVE_FUNC;
}

static	void
SetPandaDownload2(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *arr,*item,*child;
	char *filename,*desc,*path;
	int i,nretry;
ENTER_FUNC;
	arr = json_object_object_get(obj,"item");
	if (!CheckJSONObject(arr,json_type_array)) {
		return;
	}
	for (i=0;i<json_object_array_length(arr);i++) {
		item = json_object_array_get_idx(arr,i);
		if (!CheckJSONObject(item,json_type_object)) {
			continue;
		}
		path = filename = desc = NULL;
		nretry = 0;

		child = json_object_object_get(item,"path");
		if (CheckJSONObject(child,json_type_string)) {
			path = (char*)json_object_get_string(child);
		}

		child = json_object_object_get(item,"filename");
		if (CheckJSONObject(child,json_type_string)) {
			filename = (char*)json_object_get_string(child);
		}

		child = json_object_object_get(item,"description");
		if (CheckJSONObject(child,json_type_string)) {
			desc = (char*)json_object_get_string(child);
		}

		child = json_object_object_get(item,"nretry");
		if (CheckJSONObject(child,json_type_int)) {
			nretry = json_object_get_int(child);
		}

		if (path != NULL && strlen(path) > 0 && filename != NULL) {
			if (strcmp(SERVERTYPE(Session),"ginbee")) {
			DLRequest *req;
			req = (DLRequest*)xmalloc(sizeof(DLRequest));
			req->path = StrDup(path);
			req->filename = StrDup(filename);
			req->description = StrDup(desc);
			req->nretry = nretry;
			DLLIST(Session) = g_list_append(DLLIST(Session),req);
			}
		}
	}
LEAVE_FUNC;
}

static	void
SetPandaPrint(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *arr,*item,*child;
	char *title,*path;
	int i,nretry,showdialog;
ENTER_FUNC;

	arr = json_object_object_get(obj,"item");
	if (!CheckJSONObject(arr,json_type_array)) {
		return;
	}
	for (i=0;i<json_object_array_length(arr);i++) {
		item = json_object_array_get_idx(arr,i);
		if (!CheckJSONObject(item,json_type_object)) {
			continue;
		}
		path = title = NULL;
		nretry = showdialog = 0;

		child = json_object_object_get(item,"path");
		if (CheckJSONObject(child,json_type_string)) {
			path = (char*)json_object_get_string(child);
		}

		child = json_object_object_get(item,"title");
		if (CheckJSONObject(child,json_type_string)) {
			title = (char*)json_object_get_string(child);
		}

		child = json_object_object_get(item,"nretry");
		if (CheckJSONObject(child,json_type_int)) {
			nretry = json_object_get_int(child);
		}

		child = json_object_object_get(item,"showdialog");
		if (CheckJSONObject(child,json_type_int)) {
			showdialog = json_object_get_int(child);
		}

		if (path != NULL && strlen(path) > 0 && title != NULL) {
			if (strcmp(SERVERTYPE(Session),"ginbee")) {
			PrintRequest *req;
			req = (PrintRequest*)xmalloc(sizeof(PrintRequest));
			req->path = StrDup(path);
			req->title = StrDup(title);
			req->nretry = nretry;
			req->showdialog = showdialog;
			PRINTLIST(Session) = g_list_append(PRINTLIST(Session),req);
			}
		}
	}
LEAVE_FUNC;
}

static	void
SetNumberEntry(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	Numeric	num;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"editable");
	if (CheckJSONObject(child,json_type_boolean)) {
		g_object_set(G_OBJECT(widget),"editable",
			json_object_get_boolean(child),NULL);
	}
	child = json_object_object_get(obj,"numdata");
	if (CheckJSONObject(child,json_type_double)) {
		num =DoubleToNumeric(json_object_get_double(child));
		gtk_number_entry_set_value(GTK_NUMBER_ENTRY(widget),num);
		NumericFree(num);
	}
LEAVE_FUNC;
}

static	void
GetNumberEntry(
	GtkWidget	*widget,
	json_object	*obj)
{
	Numeric	num;
	json_object *child;
ENTER_FUNC;
	child = json_object_object_get(obj,"numdata");
	if (CheckJSONObject(child,json_type_double)) {
		num = gtk_number_entry_get_value(GTK_NUMBER_ENTRY(widget));
		json_object_object_add(obj,"numdata",
			json_object_new_double(NumericToDouble(num)));
		NumericFree(num);
	}
LEAVE_FUNC;
}

#define MAX_COMBO_ITEM (256)

static	void
SetPandaCombo(
	GtkWidget	*widget,
	json_object	*obj)
{
	GtkPandaCombo *combo;
	json_object *child,*val;
	int count,n,i;
	char *item[MAX_COMBO_ITEM];
ENTER_FUNC;
	SetCommon(widget,obj);
	count = 0;
	child = json_object_object_get(obj,"count");
	if (CheckJSONObject(child,json_type_int)) {
		count = json_object_get_int(child);
	}
	child = json_object_object_get(obj,"item");
	if (CheckJSONObject(child,json_type_array)) {
		n = json_object_array_length(child);
		if (count > 0) {
			n = n > count ? count : n;
		}
		n = n > MAX_COMBO_ITEM ? MAX_COMBO_ITEM : n;
		for(i=0;i<MAX_COMBO_ITEM;i++) {
			item[i] = NULL;
		}
		item[0] = g_strdup("");
		for(i=0;i<n;i++) {
			if (i < count) {
				val = json_object_array_get_idx(child,i);
				if (CheckJSONObject(val,json_type_string)) {
					item[i+1] = g_strdup(json_object_get_string(val));
				}
			} else {
				item[i+1] = NULL;
			}
		}
		combo = GTK_PANDA_COMBO(widget);
		gtk_panda_combo_set_popdown_strings(combo,item);
	}
LEAVE_FUNC;
}

#define MAX_CLIST_COLUMNS (256)

static	void
SetPandaCList(
	GtkWidget	*widget,
	json_object	*obj)
{
	int count,n,m,i,j,row;
	double rowattr;
	char *rdata[MAX_CLIST_COLUMNS],name[16];
	json_object *child,*val,*rowobj,*colobj,*boolobj;
ENTER_FUNC;
	SetCommon(widget,obj);
	gtk_widget_hide(widget);

	count = -1;
	child = json_object_object_get(obj,"count");
	if (CheckJSONObject(child,json_type_int)) {
		count = json_object_get_int(child);
	}

	row = 0;
	child = json_object_object_get(obj,"row");
	if (CheckJSONObject(child,json_type_int)) {
		row = json_object_get_int(child);
		row = row > 1 ? row - 1 : 0;
	}

	rowattr = 0.0;
	child = json_object_object_get(obj,"rowattr");
	if (CheckJSONObject(child,json_type_int)) {
		switch(json_object_get_int(child)) {
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

	child = json_object_object_get(obj,"item");
	if (CheckJSONObject(child,json_type_array)) {
		n = json_object_array_length(child);
		if (count < 0 || count > n) {
			count = n;
		}
		gtk_panda_clist_set_rows(GTK_PANDA_CLIST(widget),count);
		n = n > count ? count : n;
		m = gtk_panda_clist_get_columns(GTK_PANDA_CLIST(widget));
		for(i=0;i<n;i++) {
			rowobj = json_object_array_get_idx(child,i);
			if (!CheckJSONObject(rowobj,json_type_object)) {
				continue;
			}
			for(j=0;j<MAX_CLIST_COLUMNS;j++) {
				rdata[j] = NULL;
			}
			for(j=0;j<m;j++) {
				sprintf(name,"column%d",j+1);
				colobj = json_object_object_get(rowobj,name);
				if (CheckJSONObject(colobj,json_type_string)) {
					rdata[j] = (char*)json_object_get_string(colobj);
				}
			}
			gtk_panda_clist_set_row(GTK_PANDA_CLIST(widget),i,rdata);
		}
	}

	child = json_object_object_get(obj,"fgcolor");
	if (CheckJSONObject(child,json_type_array)) {
		n = json_object_array_length(child);
		n = n > count ? count : n;
		for(i=0;i<n;i++) {
			val = json_object_array_get_idx(child,i);
			if (json_object_is_type(val,json_type_string)) {
				gtk_panda_clist_set_fgcolor(GTK_PANDA_CLIST(widget),
					i,(char*)json_object_get_string(val));
			}
		}
	}

	child = json_object_object_get(obj,"bgcolor");
	if (CheckJSONObject(child,json_type_array)) {
		n = json_object_array_length(child);
		n = n > count ? count : n;
		for(i=0;i<n;i++) {
			val = json_object_array_get_idx(child,i);
			if (json_object_is_type(val,json_type_string)) {
				gtk_panda_clist_set_bgcolor(GTK_PANDA_CLIST(widget),
					i,(char*)json_object_get_string(val));
			}
		}
	}

	gtk_widget_show(widget);

	child = json_object_object_get(obj,"selectdata");
	if (CheckJSONObject(child,json_type_array)) {
		n = json_object_array_length(child);
		n = n > count ? count : n;
		for(i=0;i<n;i++) {
			boolobj = json_object_array_get_idx(child,i);
			if (!CheckJSONObject(boolobj,json_type_boolean)) {
				continue;
			}
			if (json_object_get_boolean(boolobj)) {
				gtk_panda_clist_select_row(GTK_PANDA_CLIST(widget),i,0);
			} else {
				gtk_panda_clist_unselect_row(GTK_PANDA_CLIST(widget),i,0);
			}
		}
	}

	if (count > 0 && row < count) {
		gtk_panda_clist_moveto(GTK_PANDA_CLIST(widget),row,0,rowattr,0.0); 
	}
LEAVE_FUNC;
}

static	void
GetPandaCList(
	GtkWidget	*widget,
	json_object	*obj)
{
	int i,nrows,visible,row,selected;
	json_object *child;
ENTER_FUNC;
	row = 0;
	visible = 0;
	nrows = gtk_panda_clist_get_rows(GTK_PANDA_CLIST(widget));
	for(i=0;i<nrows;i++) {
		if (!visible && 
			gtk_panda_clist_row_is_visible(GTK_PANDA_CLIST(widget),i)) {
			row = i+1;
			visible = 1;
		}
	}

	if (visible) {
		child = json_object_object_get(obj,"row");
		if (CheckJSONObject(child,json_type_int)) {
			json_object_object_add(obj,"row",
				json_object_new_int(row));
		}
	}

	child = json_object_object_get(obj,"selectdata");
	if (CheckJSONObject(child,json_type_array)) {
		nrows = gtk_panda_clist_get_rows(GTK_PANDA_CLIST(widget));
		for(i=0;i<nrows;i++) {
			selected = gtk_panda_clist_row_is_selected(
				GTK_PANDA_CLIST(widget),i);
			json_object_array_put_idx(child,i,
				json_object_new_boolean(selected));
		}
	}
LEAVE_FUNC;
}

static	void
SetPandaHTML(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object	*child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"uri");
	if (CheckJSONObject(child,json_type_string)) {
		gtk_panda_html_set_uri(GTK_PANDA_HTML(widget),
			json_object_get_string(child));
	}
LEAVE_FUNC;
}

static	void
SetPandaTable(
	GtkWidget	*widget,
	json_object	*obj)
{
	int			n,i,j,trow,tcolumn;
	double		trowattr;
	json_object	*child,*child2,*rowobj,*colobj;
	char		*rowval[GTK_PANDA_TABLE_MAX_COLS];
	char		*fgval[GTK_PANDA_TABLE_MAX_COLS];
	char		*bgval[GTK_PANDA_TABLE_MAX_COLS];
	char		name[16];
ENTER_FUNC;
	SetCommon(widget,obj);

	trow = 0;
	child = json_object_object_get(obj,"trow");
	if (CheckJSONObject(child,json_type_int)) {
		trow = json_object_get_int(child);
		if (trow >= 1) {
			trow -= 1;
		}
	}

	trowattr = 0.0;
	child = json_object_object_get(obj,"trowattr");
	if (CheckJSONObject(child,json_type_int)) {
		switch(json_object_get_int(child)) {
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
	child = json_object_object_get(obj,"tcolumn");
	if (CheckJSONObject(child,json_type_int)) {
		tcolumn = json_object_get_int(child);
		if (tcolumn >= 1) {
			tcolumn -= 1;
		}
	}

	n = gtk_panda_table_get_columns(GTK_PANDA_TABLE(widget));
	child = json_object_object_get(obj,"rowdata");
	if (CheckJSONObject(child,json_type_array)) {
		for(i=0;i<json_object_array_length(child);i++) {
			rowobj = json_object_array_get_idx(child,i);
			if (!CheckJSONObject(rowobj,json_type_object)) {
				continue;
			}
			for(j=0;j<GTK_PANDA_TABLE_MAX_COLS;j++) {
				rowval[j] = NULL;
				fgval[j] = NULL;
				bgval[j] = NULL;
			}
			for(j=0;j<n;j++) {
				sprintf(name,"column%d",j+1);
				colobj = json_object_object_get(rowobj,name);
				if (!CheckJSONObject(colobj,json_type_object)) {
					continue;
				}
				child2 = json_object_object_get(colobj,"celldata");
				if (CheckJSONObject(child2,json_type_string)) {
					rowval[j] = (char*)json_object_get_string(child2);
				}
				child2 = json_object_object_get(colobj,"fgcolor");
				if (CheckJSONObject(child2,json_type_string)) {
					fgval[j] = (char*)json_object_get_string(child2);
				}
				child2 = json_object_object_get(colobj,"bgcolor");
				if (CheckJSONObject(child2,json_type_string)) {
					bgval[j] = (char*)json_object_get_string(child2);
				}
			}
			gtk_panda_table_set_row(GTK_PANDA_TABLE(widget),i,rowval);
			gtk_panda_table_set_fgcolor(GTK_PANDA_TABLE(widget),i,fgval);
			gtk_panda_table_set_bgcolor(GTK_PANDA_TABLE(widget),i,bgval);
		}
	}
	if (!strcmp(gtk_widget_get_name(widget),FOCUSEDWIDGET(Session))) {
		if (trow >=0 && tcolumn >= 0) {
			gtk_panda_table_moveto(GTK_PANDA_TABLE(widget),
				trow,tcolumn,TRUE,trowattr,0.0);
		} else {
			gtk_panda_table_stay(GTK_PANDA_TABLE(widget));
		}
	}
	_AddChangedWidget(widget);
LEAVE_FUNC;
}

static	void
GetPandaTable(
	GtkWidget	*widget,
	json_object	*obj)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	int n,i,j,trow,tcolumn;
	gchar *tvalue,name[16];
	json_object *child,*rowdata,*colobj;
ENTER_FUNC;
	gtk_widget_child_focus(widget,GTK_DIR_TAB_FORWARD);
	trow = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),
		"changed_row")) + 1;
	tcolumn = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),
		"changed_column")) + 1;
	tvalue = (gchar*)g_object_get_data(G_OBJECT(widget),
		"changed_value");;
	tvalue = tvalue == NULL ? (gchar*)"" : tvalue;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gtk_tree_model_get_iter_first(model,&iter);

	child = json_object_object_get(obj,"trow");
	if (CheckJSONObject(child,json_type_int)) {
		json_object_object_add(obj,"trow",
			json_object_new_int(trow));
	}

	child = json_object_object_get(obj,"tcolumn");
	if (CheckJSONObject(child,json_type_int)) {
		json_object_object_add(obj,"tcolumn",
			json_object_new_int(tcolumn));
	}

	child = json_object_object_get(obj,"tvalue");
	if (CheckJSONObject(child,json_type_string)) {
		json_object_object_add(obj,"tvalue",
			json_object_new_string(tvalue));
	}

	n = gtk_panda_table_get_columns(GTK_PANDA_TABLE(widget));
	child = json_object_object_get(obj,"rowdata");
	if (CheckJSONObject(child,json_type_array)) {
		i = 0;
		do {
			rowdata = json_object_array_get_idx(child,i);
			if (!CheckJSONObject(rowdata,json_type_object)) {
				continue;
			}
			for(j=0;j<n;j++) {
				sprintf(name,"column%d",j+1);
				colobj = json_object_object_get(rowdata,name);
				if (!CheckJSONObject(colobj,json_type_object)) {
					continue;
				}
				gtk_tree_model_get(model,&iter,j,&tvalue,-1);
	 			json_object_object_add(colobj,"celldata",
	 				json_object_new_string(tvalue));
	 			g_free(tvalue);
			}
			i+=1;
		} while(gtk_tree_model_iter_next(model,&iter));
	}
LEAVE_FUNC;
}

/******************************************************************************/
/* gtk+ widget                                                                */
/******************************************************************************/

static	void
SetEntry(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	char *prev,*next;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"editable");
	if (CheckJSONObject(child,json_type_boolean)) {
		g_object_set(G_OBJECT(widget),"editable",
			json_object_get_boolean(child),NULL);
	}
	child = json_object_object_get(obj,"textdata");
	if (CheckJSONObject(child,json_type_string)) {
		prev = gtk_editable_get_chars(GTK_EDITABLE(widget),0 , -1);
		next = (char*)json_object_get_string(child);
		if (strcmp(prev,next)) {
			gtk_entry_set_text(GTK_ENTRY(widget),next);
		}
		g_free(prev);
	}

	if (!gtk_widget_has_focus(widget)
		&& (gtk_editable_get_position (GTK_EDITABLE(widget)) != 0)) {
		gtk_editable_set_position (GTK_EDITABLE(widget), 0);
	}
LEAVE_FUNC;
}

static	void
GetEntry(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	char *text;
ENTER_FUNC;
	text = gtk_editable_get_chars(GTK_EDITABLE(widget),0 , -1);
	child = json_object_object_get(obj,"textdata");
	if (CheckJSONObject(child,json_type_string)) {
		json_object_object_add(obj,"textdata",json_object_new_string(text));
	}
	g_free(text);
LEAVE_FUNC;
}

static	void
SetLabel(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	char *text;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"textdata");
	if (CheckJSONObject(child,json_type_string)) {
		text = (char*)json_object_get_string(child);
		if (pango_parse_markup(text,-1,0,NULL,NULL,NULL,NULL)) {
			gtk_label_set_markup(GTK_LABEL(widget),text);
		} else {
			gtk_label_set_text(GTK_LABEL(widget),text);
		}
	}
LEAVE_FUNC;
}

static	void
SetText(
	GtkWidget	*widget,
	json_object	*obj)
{
	char *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"textdata");
	if (CheckJSONObject(child,json_type_string)) {
		text = (char*)json_object_get_string(child);
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		gtk_text_buffer_set_text(buffer,text,strlen(text));
		gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget),&iter,0,0);
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(widget),&iter,
			0.0,TRUE,0.0,0.0);
	}
LEAVE_FUNC;
}

static	void
GetText(
	GtkWidget	*widget,
	json_object	*obj)
{
	GtkTextBuffer *buffer;
	GtkTextIter start;
	GtkTextIter end;
	json_object *child;
	char *text;
ENTER_FUNC;
	child = json_object_object_get(obj,"textdata");
	if (CheckJSONObject(child,json_type_string)) {
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		gtk_text_buffer_get_start_iter(buffer,&start);
		gtk_text_buffer_get_end_iter(buffer,&end);
		text = gtk_text_buffer_get_text(buffer,&start,&end,FALSE);
		json_object_object_add(obj,"textdata",json_object_new_string(text));
		g_free(text);
	}
LEAVE_FUNC;
}

static	void
SetButton(
	GtkWidget	*widget,
	json_object *obj)
{
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"label");
	if (CheckJSONObject(child,json_type_string)) {
		SetWidgetLabelRecursive(widget,(char*)json_object_get_string(child));
	}

	child = json_object_object_get(obj,"isactive");
	if (CheckJSONObject(child,json_type_boolean)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
			json_object_get_boolean(child));
	}
LEAVE_FUNC;
}

static	void
GetButton(
	GtkWidget	*widget,
	json_object	*obj)
{
	gboolean isactive;
	json_object *child;
ENTER_FUNC;
	child = json_object_object_get(obj,"isactive");
	if (CheckJSONObject(child,json_type_boolean)) {
		isactive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		json_object_object_add(obj,"isactive",
			json_object_new_boolean(isactive));
	}
LEAVE_FUNC;
}

static	void
SetCalendar(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	int y,m,d;
ENTER_FUNC;
	SetCommon(widget,obj);
	y = m = d = 0;
	child = json_object_object_get(obj,"year");
	if (CheckJSONObject(child,json_type_int)) {
		y = json_object_get_int(child);
	}

	child = json_object_object_get(obj,"month");
	if (CheckJSONObject(child,json_type_int)) {
		m = json_object_get_int(child);
	}

	child = json_object_object_get(obj,"day");
	if (CheckJSONObject(child,json_type_int)) {
		d = json_object_get_int(child);
	}

	if (y > 0) {
		gtk_calendar_select_month(GTK_CALENDAR(widget),m - 1,y);
		gtk_calendar_select_day(GTK_CALENDAR(widget),d);
	}
LEAVE_FUNC;
}

static	void
GetCalendar(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	int y,m,d;
ENTER_FUNC;
	gtk_calendar_get_date(GTK_CALENDAR(widget),&y,&m,&d);
	child = json_object_object_get(obj,"year");
	if (CheckJSONObject(child,json_type_int)) {
		json_object_object_del(obj,"year");
		json_object_object_add(obj,"year",json_object_new_int(y));
	}

	child = json_object_object_get(obj,"month");
	if (CheckJSONObject(child,json_type_int)) {
		json_object_object_del(obj,"month");
		json_object_object_add(obj,"month",json_object_new_int(m));
	}

	child = json_object_object_get(obj,"day");
	if (CheckJSONObject(child,json_type_int)) {
		json_object_object_del(obj,"day");
		json_object_object_add(obj,"day",json_object_new_int(d));
	}
LEAVE_FUNC;
}

static	void
SetNotebook(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"pageno");
	if (CheckJSONObject(child,json_type_int)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(widget),
			json_object_get_int(child));
	}
LEAVE_FUNC;
}

static	void
GetNotebook(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	int pageno;
ENTER_FUNC;
	child = json_object_object_get(obj,"pageno");
	if (CheckJSONObject(child,json_type_int)) {
		pageno = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"new_pageno"));
		json_object_object_add(obj,"pageno",json_object_new_int(pageno));
	}
LEAVE_FUNC;
}

static	void
SetProgressBar(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"value");
	if (CheckJSONObject(child,json_type_int)) {
#ifdef LIBGTK_3_0_0
		gtk_panda_progress_bar_set_value(GTK_PANDA_PROGRESS_BAR(widget),
			json_object_get_int(child));
#else
		gtk_progress_set_value(GTK_PROGRESS(widget),
			json_object_get_int(child));
#endif
	}
LEAVE_FUNC;
}

static	void
SetWindow(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
	char *summary,*body,*icon;
	int timeout;
ENTER_FUNC;
#if 0
	SetCommon(widget,obj);
#endif
	child = json_object_object_get(obj,"bgcolor");
	if (CheckJSONObject(child,json_type_string)) {
		SetSessionBGColor(json_object_get_string(child));
	}

	child = json_object_object_get(obj,"title");
	if (CheckJSONObject(child,json_type_string)) {
		SetSessionTitle(json_object_get_string(child));
		SetTitle(TopWindow);
	}

	summary = NULL;
	child = json_object_object_get(obj,"popup_summary");
	if (CheckJSONObject(child,json_type_string)) {
		summary = (char*)json_object_get_string(child);
	}

	body = NULL;
	child = json_object_object_get(obj,"popup_body");
	if (CheckJSONObject(child,json_type_string)) {
		body = (char*)json_object_get_string(child);
	}

	icon = NULL;
	child = json_object_object_get(obj,"popup_icon");
	if (CheckJSONObject(child,json_type_string)) {
		icon = (char*)json_object_get_string(child);
	}

	timeout = 0;
	child = json_object_object_get(obj,"popup_timeout");
	if (CheckJSONObject(child,json_type_int)) {
		timeout = json_object_get_int(child);
	}

	if (summary != NULL && strlen(summary) > 0 &&
		body != NULL && strlen(body) > 0) {
		Notify(summary,body,icon,timeout);
	}
LEAVE_FUNC;
}

static	void
SetFrame(
	GtkWidget	*widget,
	json_object	*obj)
{
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);
	child = json_object_object_get(obj,"label");
	if (CheckJSONObject(child,json_type_string)) {
		gtk_frame_set_label(GTK_FRAME(widget),json_object_get_string(child));
	}
LEAVE_FUNC;
}

static	void
SetScrolledWindow(
	GtkWidget	*widget,
	json_object	*obj)
{
ENTER_FUNC;
	SetCommon(widget,obj);
LEAVE_FUNC;
}

static	void
SetFileChooserButton(
	GtkWidget	*widget,
	json_object	*obj)
{
	GtkFileChooserButton *fcb;
	gchar *folder,*longname;
ENTER_FUNC;
	SetCommon(widget,obj);
	fcb = GTK_FILE_CHOOSER_BUTTON(widget);
	gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(fcb));
	longname = (char *)glade_get_widget_long_name(widget);
	folder = (char*)GetWidgetCache(longname);
	if (folder == NULL) {
		folder = "";
	}
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fcb),folder);
	_AddChangedWidget(widget);
LEAVE_FUNC;
}

static	void
GetFileChooserButton(
	GtkWidget	*widget,
	json_object	*obj)
{
	GtkFileChooserButton *fcb;
	GError *error = NULL;
	gchar *contents;
	gchar *fname,*basename;
	gsize size;
	gchar *folder;
	char *longname,*oid;
	LargeByteString *lbs;
	json_object *child;
ENTER_FUNC;
	fcb = GTK_FILE_CHOOSER_BUTTON(widget);
	fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcb));
	if (fname == NULL) {
		return;
	}
	if(!g_file_get_contents(fname, 
			&contents, &size, &error)) {
		g_error_free(error);
		return;
	}
	lbs = NewLBS();
	LBS_ReserveSize(lbs,size,FALSE);
	memcpy(LBS_Body(lbs),contents,size);
	g_free(contents);
	oid = REST_PostBLOB(lbs);
	FreeLBS(lbs);

	child = json_object_object_get(obj,"objectdata");
	if (CheckJSONObject(child,json_type_string)) {
		if (oid != NULL) {
		json_object_object_del(obj,"objectdata");
		json_object_object_add(obj,"objectdata",json_object_new_string(oid));
		}
	}
	child = json_object_object_get(obj,"filename");
	if (CheckJSONObject(child,json_type_string)) {
		basename = g_path_get_basename(fname);
		json_object_object_del(obj,"filename");
		json_object_object_add(obj,"filename",json_object_new_string(basename));
		g_free(basename);
	}

	longname = (char *)glade_get_widget_long_name(widget);
	folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(fcb));
	if (folder != NULL) {
		SetWidgetCache(longname,folder);
		g_free(folder);
	}
LEAVE_FUNC;
}

static	void
SetColorButton(
	GtkWidget	*widget,
	json_object	*obj)
{
	GtkColorButton 	*cb;
	GdkColor color;
	char *strcolor;
	json_object *child;
ENTER_FUNC;
	cb = GTK_COLOR_BUTTON(widget);
	SetCommon(widget,obj);

	child = json_object_object_get(obj,"color");
	if (CheckJSONObject(child,json_type_string)) {
		strcolor = (char*)json_object_get_string(child);
		if (gdk_color_parse(strcolor,&color)) {
			gtk_color_button_set_color(cb,&color);
		}
	}
LEAVE_FUNC;
}

static	void
GetColorButton(
	GtkWidget	*widget,
	json_object	*obj)
{
	GtkColorButton *cb;
	GdkColor color;
	char strcolor[256];
	json_object *child;
ENTER_FUNC;
	cb = GTK_COLOR_BUTTON(widget);
	gtk_color_button_get_color(cb,&color);
	sprintf(strcolor,"#%02X%02X%02X",
		(guint)(color.red/256.0),
		(guint)(color.green/256.0),
		(guint)(color.blue/256.0)
		);

	child = json_object_object_get(obj,"color");
	if (CheckJSONObject(child,json_type_string)) {
		json_object_object_add(obj,"color",json_object_new_string(strcolor));
	}
LEAVE_FUNC;
}

static void
SetPixmap(
	GtkWidget	*widget,
	json_object	*obj)
{
	LargeByteString *lbs;
	json_object *child;
ENTER_FUNC;
	SetCommon(widget,obj);

	child = json_object_object_get(obj,"objectdata");
	if (CheckJSONObject(child,json_type_string)) {
		lbs = REST_GetBLOB(json_object_get_string(child));
		if (lbs != NULL && LBS_Size(lbs) > 0) {
			gtk_widget_show(widget); 
			gtk_panda_pixmap_set_image(GTK_PANDA_PIXMAP(widget), 
				(gchar*)LBS_Body(lbs),(gsize)LBS_Size(lbs));
			FreeLBS(lbs);
		} else {
			gtk_widget_hide(widget); 
		}
	}
LEAVE_FUNC;
}

static	void
SetWidgetData(
	GtkWidget *widget,
	json_object *info)
{
	long 		type;

	if (widget == NULL) {
		return;
	}
	if (!json_object_is_type(info,json_type_object)) {
		return;
	}

	type = (long)(G_OBJECT_TYPE(widget));
	if (type == GTK_TYPE_NUMBER_ENTRY) {
		SetNumberEntry(widget,info);
	} else if (type == GTK_PANDA_TYPE_COMBO) {
		SetPandaCombo(widget,info);
	} else if (type == GTK_PANDA_TYPE_CLIST) {
		SetPandaCList(widget,info);
	} else if (type == GTK_PANDA_TYPE_ENTRY) {
		SetEntry(widget,info);
	} else if (type == GTK_PANDA_TYPE_TEXT) {
		SetText(widget,info);
	} else if (type == GTK_PANDA_TYPE_PDF) {
		SetPandaPDF(widget,info);
	} else if (type == GTK_PANDA_TYPE_TIMER) {
		SetPandaTimer(widget,info);
	} else if (type == GTK_PANDA_TYPE_DOWNLOAD) {
		SetPandaDownload(widget,info);
	} else if (type == GTK_PANDA_TYPE_DOWNLOAD2) {
		SetPandaDownload2(widget,info);
	} else if (type == GTK_PANDA_TYPE_PRINT) {
		SetPandaPrint(widget,info);
	} else if (type == GTK_PANDA_TYPE_HTML) {
		SetPandaHTML(widget,info);
	} else if (type == GTK_PANDA_TYPE_TABLE) {
		SetPandaTable(widget,info);
	} else if (type == GTK_TYPE_WINDOW) {
		SetWindow(widget,info);
	} else if (type == GTK_TYPE_LABEL) {
		SetLabel(widget,info);
	} else if (type == GTK_TYPE_BUTTON) {
		SetButton(widget,info);
	} else if (type == GTK_TYPE_TOGGLE_BUTTON) {
		SetButton(widget,info);
	} else if (type == GTK_TYPE_CHECK_BUTTON) {
		SetButton(widget,info);
	} else if (type == GTK_TYPE_RADIO_BUTTON) {
		SetButton(widget,info);
	} else if (type == GTK_TYPE_CALENDAR) {
		SetCalendar(widget,info);
	} else if (type == GTK_TYPE_NOTEBOOK) {
		SetNotebook(widget,info);
	} else if (type == GTK_TYPE_PROGRESS_BAR) {
		SetProgressBar(widget,info);
	} else if (type == GTK_TYPE_FRAME) {
		SetFrame(widget,info);
	} else if (type == GTK_TYPE_SCROLLED_WINDOW) {
		SetScrolledWindow(widget,info);
	} else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON) {
		SetFileChooserButton(widget,info);
	} else if (type == GTK_TYPE_COLOR_BUTTON) {
		SetColorButton(widget,info);
	} else if (type == GTK_PANDA_TYPE_PIXMAP) {
		SetPixmap(widget,info);
	}
}

extern	void
UpdateWidget(
	const char *longname,
	json_object *w)
{
	GtkWidget *widget;
	json_object *val;
	char childname[SIZE_LONGNAME+1];
	int i,length;
	enum json_type type;

	widget = GetWidgetByLongName(longname);
	if (widget != NULL) {
		SetWidgetData(widget,w);
	}
	type = json_object_get_type(w);
	if (type == json_type_object) {
		{
			json_object_object_foreach(w,k,v) {
				snprintf(childname,SIZE_LONGNAME,"%s.%s",longname,k);
				childname[SIZE_LONGNAME] = 0;
				UpdateWidget(childname,v);
			}
		}
	} else if (type == json_type_array) {
		length = json_object_array_length(w);
		for(i=0;i<length;i++) {
			val = json_object_array_get_idx(w,i);
			snprintf(childname,SIZE_LONGNAME,"%s[%d]",longname,i);
			childname[SIZE_LONGNAME] = 0;
			UpdateWidget(childname,val);
		}
	}
}

static	void
GetWidgetData(
	GtkWidget *widget,
	json_object *obj)
{
	long 		type;

	if (widget == NULL) {
		return;
	}
	if (obj == NULL || !json_object_is_type(obj,json_type_object)) {
		return;
	}

	type = (long)(G_OBJECT_TYPE(widget));
// gtk+panda
	if (type == GTK_TYPE_NUMBER_ENTRY) {
		GetNumberEntry(widget,obj);
	} else if (type == GTK_PANDA_TYPE_CLIST) {
		GetPandaCList(widget,obj);
	} else if (type == GTK_PANDA_TYPE_ENTRY) {
		GetEntry(widget,obj);
	} else if (type == GTK_PANDA_TYPE_TEXT) {
		GetText(widget,obj);
	} else if (type == GTK_PANDA_TYPE_TIMER) {
		GetPandaTimer(widget,obj);
	} else if (type == GTK_PANDA_TYPE_TABLE) {
		GetPandaTable(widget,obj);
	} else if (
		type == GTK_TYPE_BUTTON ||
		type == GTK_TYPE_TOGGLE_BUTTON ||
		type == GTK_TYPE_CHECK_BUTTON ||
		type == GTK_TYPE_RADIO_BUTTON) {
		GetButton(widget,obj);
	} else if (type == GTK_TYPE_CALENDAR) {
		GetCalendar(widget,obj);
	} else if (type == GTK_TYPE_NOTEBOOK) {
		GetNotebook(widget,obj);
	} else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON) {
		GetFileChooserButton(widget,obj);
	} else if (type == GTK_TYPE_COLOR_BUTTON) {
		GetColorButton(widget,obj);
	} else {
		MessageLogPrintf("invalid widget [%s]", gtk_widget_get_name(widget));
	}
}

extern	json_object *
_MakeScreenData(
	const char *longname,
	json_object *w,
	WindowData *wdata)
{
	GtkWidget *widget;
	char childname[SIZE_LONGNAME+1];
	json_object *obj,*child,*v;
	int i,length;
	enum json_type type;

	type = json_object_get_type(w);
	switch(type) {
	case json_type_boolean:
		obj = json_object_new_boolean(json_object_get_boolean(w));
		break;
	case json_type_double:
		obj = json_object_new_double(json_object_get_double(w));
		break;
	case json_type_int:
		obj = json_object_new_int(json_object_get_int(w));
		break;
	case json_type_string:
		obj = json_object_new_string(json_object_get_string(w));
		break;
	case json_type_array:
		obj = json_object_new_array();
		length = json_object_array_length(w);
		for(i=0;i<length;i++) {
			v = json_object_array_get_idx(w,i);
			snprintf(childname,SIZE_LONGNAME,"%s[%d]",longname,i);
			childname[SIZE_LONGNAME] = 0;
			child = _MakeScreenData(childname,v,wdata);
			json_object_array_add(obj,child);
		}
		break;
	case json_type_object:
		obj = json_object_new_object();
		json_object_object_foreach(w,k,v) {
			snprintf(childname,SIZE_LONGNAME,"%s.%s",longname,k);
			childname[SIZE_LONGNAME] = 0;
			child = _MakeScreenData(childname,v,wdata);
			json_object_object_add(obj,k,child);
		}
		break;
	default:
		obj = json_object_new_string("");
		break;
	}

	widget = GetWidgetByLongName(longname);
	if (widget != NULL) {
		if (g_hash_table_lookup(wdata->ChangedWidgetTable,longname) != NULL) {
			GetWidgetData(widget,obj);
		}
	}
	return obj;
}

static gboolean
RemoveChangedWidget(
	gpointer	key,
	gpointer	value,
	gpointer	user_data) 
{
	g_free((char *)key);
	return TRUE;
}

extern struct json_object *
MakeScreenData(
	const char *wname)
{
	json_object *ret,*result,*window_data,*windows,*w,*window,*screen_data;
	int i,length;
	WindowData *wdata;

	if (*wname == '_') {
		return json_object_new_object();
	}

	ret = NULL;
	result = json_object_object_get(SCREENDATA(Session),"result");
	window_data = json_object_object_get(result,"window_data");
	windows = json_object_object_get(window_data,"windows");
	length = json_object_array_length(windows);
	for(i=0;i<length;i++) {
		w = json_object_array_get_idx(windows,i);
		window = json_object_object_get(w,"window");
		if (!strcmp(wname,json_object_get_string(window))) {
			screen_data = json_object_object_get(w,"screen_data");
			wdata = GetWindowData(wname);
	 		ret = _MakeScreenData(wname,screen_data,wdata);
			g_hash_table_foreach_remove(wdata->ChangedWidgetTable, 
				RemoveChangedWidget, NULL);
		}
	}
	if (ret == NULL) {
		Error("could not make screen_data for window[%s]",wname);
	}
	return ret;
}
