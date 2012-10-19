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
#include	"glterm.h"
#include	"glclient.h"
#include	"styleParser.h"
#include	"protocol.h"
#include	"widgetOPS.h"
#include	"action.h"
#include	"marshaller.h"
#include	"queue.h"
#include	"widgetcache.h"
#include	"interface.h"
#include	"download.h"
#include	"notify.h"
#include	"message.h"
#include	"debug.h"

static	void
SetState(
	GtkWidget	*widget,
	GtkStateType	state)
{
	if (state != GTK_STATE_INSENSITIVE)
		gtk_widget_set_sensitive(widget,TRUE);
	gtk_widget_set_state(widget,state);
}

static	void
SetStyle(
	GtkWidget	*widget,
	char	*str)
{
	int i;
	GtkRcStyle *rc_style;
	GtkStyle *style;

	style = GetStyle(str);

	if (style != NULL) {
		rc_style = gtk_widget_get_modifier_style(widget);
		for(i = GTK_STATE_NORMAL ; i <= GTK_STATE_INSENSITIVE; i++) {
			rc_style->fg[i] = style->fg[i];
			rc_style->bg[i] = style->bg[i];
			rc_style->text[i] = style->text[i];
			rc_style->base[i] = style->base[i];
			rc_style->color_flags[i] = GTK_RC_FG | GTK_RC_BG | GTK_RC_TEXT | GTK_RC_BASE;
		}
		gtk_widget_modify_style(widget, rc_style);
    	if (GTK_IS_CONTAINER(widget)){
    	    gtk_container_foreach(GTK_CONTAINER(widget),
				(GtkCallback)SetStyle,str);
    	}
	}
}

static void
SetCommon(
	GtkWidget	*widget,
	WidgetData	*data)
{
	SetState(widget,data->state);
	SetStyle(widget,data->style);
	gtk_widget_set_visible(widget,data->visible);
}

static	void
SetWidgetLabelRecursive(
	GtkWidget	*widget,
	char		*label)
{
	if		(  GTK_IS_LABEL(widget)  ) {
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
SetPandaPreview(
	GtkWidget	*widget,
	WidgetData	*wdata,
	_PREVIEW	*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	gtk_panda_pdf_set(GTK_PANDA_PDF(widget), 
		LBS_Size(data->binary), LBS_Body(data->binary));
LEAVE_FUNC;
}

static	void
SetPandaTimer(
	GtkWidget	*widget,
	WidgetData	*wdata,
	_Timer		*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	gtk_panda_timer_set(GTK_PANDA_TIMER(widget),
		data->duration);
LEAVE_FUNC;
}

static	void
SetPandaDownload(
	GtkWidget	*widget,
	WidgetData	*wdata,
	_Download	*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	g_return_if_fail(data->binary != NULL);
	if (LBS_Size(data->binary) > 0) {
		show_download_dialog(widget,
			data->filename,
			data->description,
			data->binary);
	}
LEAVE_FUNC;
}

static	void
GetPandaTimer(
	GtkWidget	*widget,
	_Timer*		data)
{
ENTER_FUNC;
	data->duration = GTK_PANDA_TIMER(widget)->duration / 1000;
LEAVE_FUNC;
}

static	void
SetNumberEntry(
	GtkWidget		*widget,
	WidgetData	*wdata,
	_NumberEntry	*data)
{
	Numeric	value;

ENTER_FUNC;
	SetCommon(widget,wdata);
	g_object_set(G_OBJECT(widget),"editable",data->editable,NULL);
	value = FixedToNumeric(data->fixed);
	gtk_number_entry_set_value(GTK_NUMBER_ENTRY(widget), value);
	NumericFree(value);
LEAVE_FUNC;
}

static	void
GetNumberEntry(
	GtkWidget		*widget,
	_NumberEntry	*data)
{
	Numeric	value;

ENTER_FUNC;
	value = gtk_number_entry_get_value(GTK_NUMBER_ENTRY(widget));
	xfree(data->fixed->sval);
	data->fixed->sval = NumericToFixed(value,
		data->fixed->flen,data->fixed->slen);
	NumericFree(value);
LEAVE_FUNC;
}

static	void
SetPandaCombo(
	GtkWidget	*widget,
	WidgetData	*wdata,
	_Combo		*data)
{
	GtkPandaCombo	*combo;

ENTER_FUNC;
	SetCommon(widget,wdata);
	combo = GTK_PANDA_COMBO(widget);
	gtk_panda_combo_set_popdown_strings(combo,data->itemdata);
LEAVE_FUNC;
}

static	void
SetPandaCList(
	GtkWidget	*widget,
	WidgetData	*wdata,
	_CList		*data)
{
	int j;
	char **rdata;

ENTER_FUNC;
	SetCommon(widget,wdata);

	// items
	gtk_widget_hide(widget);
	gtk_panda_clist_clear(GTK_PANDA_CLIST(widget));

	for	( j = 0 ; j < g_list_length(data->clistdata) ; j ++ ) {
		if (j < data->count) {
			rdata = g_list_nth_data(data->clistdata,j);
			gtk_panda_clist_append(GTK_PANDA_CLIST(widget), rdata);
		}
	}
	gtk_widget_show(widget);
	for	( j = 0 ; data->states[j] != NULL ; j ++ ) {
		if (j < data->count) {
			if (*(data->states[j]) == 'T') {
				gtk_panda_clist_select_row(GTK_PANDA_CLIST(widget),j,0);
			} else {
				gtk_panda_clist_unselect_row(GTK_PANDA_CLIST(widget),j,0);
			}
		}
	}
	if (data->count > 0) {
		gtk_panda_clist_moveto(GTK_PANDA_CLIST(widget), 
			data->row, 0, data->rowattr, 0.0); 
	}
LEAVE_FUNC;
}

static	void
GetPandaCList(
	GtkWidget	*widget,
	_CList		*data)
{
	int				i;
	int				nrows;
	Bool			getRow;

ENTER_FUNC;
	nrows = gtk_panda_clist_get_n_rows(GTK_PANDA_CLIST(widget));
	getRow = FALSE;

	if (data->states !=NULL) {
		g_strfreev(data->states);
	}
	data->states = g_malloc0(sizeof(gchar*)*(nrows+1));
	data->states[nrows] = NULL;

	for( i = 0; i < nrows; i++) {
		if(gtk_panda_clist_row_is_selected(GTK_PANDA_CLIST(widget),i)) {
			data->states[i] = g_strdup("T");
		} else {
			data->states[i] = g_strdup("F");
		}
		if (!getRow) {
			if (gtk_panda_clist_row_is_visible(GTK_PANDA_CLIST(widget), i)) {
				/*to be 1origin for COBOL*/
				data->row = i + 1;
				getRow = TRUE;
			}
		}
	}
LEAVE_FUNC;
}

static	void
SetPandaHTML(
	GtkWidget		*widget,
	WidgetData	*wdata,
	_HTML			*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	if (data->uri != NULL) {
		gtk_panda_html_set_uri (GTK_PANDA_HTML(widget), data->uri);
	}
LEAVE_FUNC;
}

static	void
GetPandaTable(
	GtkWidget	*widget,
	_Table		*data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	int i,j;
	gchar *text;
	gchar **rowdata;
ENTER_FUNC;
	data->trow = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),
		"send_data_row"));
	data->tcolumn = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),
		"send_data_column"));
	data->tvalue = g_strdup((gchar*)g_object_get_data(G_OBJECT(widget),
		"send_data_value"));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gtk_tree_model_get_iter_first(model,&iter);
	i = 0;
	do {
		rowdata = (gchar**)(g_list_nth_data(data->tabledata,i));
		for(j=0;rowdata[j]!=NULL;j++) {
			gtk_tree_model_get(model,&iter,j,&text,-1);
			g_free(rowdata[j]);
			rowdata[j] = text;
		}
		i+=1;
	} while(gtk_tree_model_iter_next(model,&iter));
LEAVE_FUNC;
}

static	void
SetPandaTable(
	GtkWidget	*widget,
	WidgetData	*wdata,
	_Table		*data)
{
	int				j;
	char			**rowdata;
	char			**fgrowdata;
	char			**bgrowdata;

ENTER_FUNC;
	SetCommon(widget,wdata);
	for	( j = 0 ; j < g_list_length(data->tabledata) ; j ++ ) {
		rowdata = g_list_nth_data(data->tabledata,j);
		gtk_panda_table_set_row(GTK_PANDA_TABLE(widget),j,rowdata);
	}
	for	( j = 0 ; j < g_list_length(data->fgdata) ; j ++ ) {
		fgrowdata = g_list_nth_data(data->fgdata,j);
		gtk_panda_table_set_fgcolor(GTK_PANDA_TABLE(widget),j,fgrowdata);
	}
	for	( j = 0 ; j < g_list_length(data->bgdata) ; j ++ ) {
		bgrowdata = g_list_nth_data(data->bgdata,j);
		gtk_panda_table_set_bgcolor(GTK_PANDA_TABLE(widget),j,bgrowdata);
	}
	if (data->trow >= 0 && data->tcolumn >= 0) {
		gtk_panda_table_moveto(GTK_PANDA_TABLE(widget), 
			data->trow, data->tcolumn, TRUE, data->trowattr, 0.0); 
	}
LEAVE_FUNC;
}

/******************************************************************************/
/* gtk+ widget                                                                */
/******************************************************************************/

static	void
SetEntry(
	GtkWidget		*widget,
	WidgetData		*wdata,
	_Entry			*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	g_object_set(G_OBJECT(widget),"editable",data->editable,NULL);
	if (strcmp (gtk_entry_get_text(GTK_ENTRY(widget)), data->text)) {
		gtk_entry_set_text(GTK_ENTRY(widget), data->text);
	}
	if (!gtk_widget_has_focus(widget)
		&& (gtk_editable_get_position (GTK_EDITABLE(widget)) != 0)) {
		gtk_editable_set_position (GTK_EDITABLE(widget), 0);
	}
LEAVE_FUNC;
}

static	void
GetEntry(
	GtkWidget		*widget,
	_Entry			*data)
{
ENTER_FUNC;
	xfree(data->text);
	data->text = gtk_editable_get_chars(GTK_EDITABLE(widget),0 , -1);
LEAVE_FUNC;
}

static	void
SetLabel(
	GtkWidget		*widget,
	WidgetData	*wdata,
	_Label			*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	if (data->text != NULL) {
		if (pango_parse_markup(data->text,-1,0,NULL,NULL,NULL,NULL)) {
			gtk_label_set_markup(GTK_LABEL(widget),data->text);
		} else {
			gtk_label_set_text(GTK_LABEL(widget),data->text);
		}
	}
LEAVE_FUNC;
}

static	void
SetText(
	GtkWidget	*widget,
	WidgetData	*wdata,
	_Text		*data)
{
	GtkTextBuffer	*buffer;
	GtkTextIter		iter;
ENTER_FUNC;
	SetCommon(widget,wdata);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	gtk_text_buffer_set_text(buffer, data->text, strlen(data->text));
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget), &iter, 0, 0);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(widget), &iter, 
		0.0, TRUE, 0.0, 0.0);
LEAVE_FUNC;
}

static	void
GetText(
	GtkWidget		*widget,
	_Text			*data)
{
	GtkTextBuffer	*buffer;
	GtkTextIter		start;
	GtkTextIter		end;
ENTER_FUNC;
	g_free(data->text);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	data->text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
LEAVE_FUNC;
}

static	void
SetButton(
	GtkWidget			*widget,
	WidgetData	*wdata,
	_Button				*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	if (data->label != NULL) {
		SetWidgetLabelRecursive(widget,data->label);
	}
	if (data->have_button_state == TRUE) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 
			data->button_state);
	}
LEAVE_FUNC;
}

static	void
GetButton(
	GtkWidget			*widget,
	_Button				*data)
{
ENTER_FUNC;
	if (data->have_button_state) {
		data->button_state = 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	}
LEAVE_FUNC;
}

static	void
SetCalendar(
	GtkWidget			*widget,
	WidgetData	*wdata,
	_Calendar			*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	if (data->year > 0) {
		gtk_calendar_select_month(GTK_CALENDAR(widget),
			data->month - 1, data->year);
		gtk_calendar_select_day(GTK_CALENDAR(widget),data->day);
	}
LEAVE_FUNC;
}

static	void
GetCalendar(
	GtkWidget			*widget,
	_Calendar			*data)
{
ENTER_FUNC;
	gtk_calendar_get_date(GTK_CALENDAR(widget), 
		&(data->year),&(data->month),&(data->day));
LEAVE_FUNC;
}

static	void
SetNotebook(
	GtkWidget			*widget,
	WidgetData	*wdata,
	_Notebook			*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	if (!fV47) {
		g_object_set_data(G_OBJECT(widget),"pageno",
			GINT_TO_POINTER(data->pageno));
	}
	gtk_notebook_set_current_page(GTK_NOTEBOOK(widget),data->pageno);
LEAVE_FUNC;
}

static	void
GetNotebook(
	GtkWidget			*widget,
	_Notebook			*data)
{
ENTER_FUNC;
	if (fV47) {
		data->pageno = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(widget),"new_pageno"));
	} else {
		data->pageno = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(widget),"pageno"));
	}
LEAVE_FUNC;
}

static	void
SetProgressBar(
	GtkWidget				*widget,
	WidgetData	*wdata,
	_ProgressBar			*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
#ifdef LIBGTK_3_0_0
	gtk_panda_progress_bar_set_value(
      GTK_PANDA_PROGRESS_BAR(widget),data->value);
#else
	gtk_progress_set_value(GTK_PROGRESS(widget),data->value);
#endif
LEAVE_FUNC;
}

static	void
GetProgressBar(
	GtkWidget				*widget,
	_ProgressBar			*data)
{
ENTER_FUNC;
#ifdef LIBGTK_3_0_0
	data->value = gtk_panda_progress_bar_get_value(GTK_PANDA_PROGRESS_BAR(widget));
#else
	data->value = gtk_progress_get_value(GTK_PROGRESS(widget));
#endif
LEAVE_FUNC;
}

static	void
SetWindow(
	GtkWidget			*widget,
	WidgetData	*wdata,
	_Window				*data)
{
ENTER_FUNC;
#if 0
	SetCommon(widget,wdata);
#endif
	if (data->title != NULL && strlen(data->title) > 0) {
		SetSessionTitle(data->title);
		SetTitle(TopWindow);
	}
	if (data->summary != NULL && strlen(data->summary) > 0 &&
		data->body != NULL && strlen(data->body) > 0) {
		Notify(data->summary,data->body,data->icon,data->timeout);
	}
LEAVE_FUNC;
}

static	void
SetFrame(
	GtkWidget		*widget,
	WidgetData	*wdata,
	_Frame			*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	gtk_frame_set_label(GTK_FRAME(widget),data->label);
LEAVE_FUNC;
}

static	void
SetScrolledWindow(
	GtkWidget					*widget,
	WidgetData	*wdata,
	_ScrolledWindow				*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
LEAVE_FUNC;
}

static	void
GetScrolledWindow(
	GtkWidget					*widget,
	_ScrolledWindow				*data)
{
	GtkAdjustment	*vad
	,				*had;

ENTER_FUNC;
	vad = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget));
	had = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(widget));
	data->hpos = (int)gtk_adjustment_get_value(had);
	data->vpos = (int)gtk_adjustment_get_value(vad);
LEAVE_FUNC;
}

static	void
SetFileChooserButton(
	GtkWidget			*widget,
	WidgetData			*wdata,
	_FileChooserButton	*data)
{
	GtkFileChooserButton 	*fcb;
	gchar *folder;
	char *longname;
ENTER_FUNC;
	SetCommon(widget,wdata);
	fcb = GTK_FILE_CHOOSER_BUTTON(widget);
	gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(fcb));
	longname = (char *)glade_get_widget_long_name(widget);
	folder = GetWidgetCache(longname);
	if (folder == NULL) {
		folder = "";
	}
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fcb),folder);
	_AddChangedWidget(widget);
LEAVE_FUNC;
}

static	void
GetFileChooserButton(
	GtkWidget			*widget,
	_FileChooserButton	*data)
{
	GtkFileChooserButton *fcb;
	GError *error = NULL;
	gchar *contents;
	gchar *fname;
	gsize size;
	gchar *folder;
	char *longname;
ENTER_FUNC;
	fcb = GTK_FILE_CHOOSER_BUTTON(widget);
	fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcb));
	if (fname == NULL) {
		data->filename = NULL;
		return;
	}
	data->filename = g_path_get_basename(fname);
	if(!g_file_get_contents(fname, 
			&contents, &size, &error)) {
		g_error_free(error);
		return;
	}
	data->binary->body = contents;
	data->binary->size = data->binary->asize = data->binary->ptr = size;
	longname = (char *)glade_get_widget_long_name(widget);
	folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(fcb));
	if (folder != NULL) {
		SetWidgetCache(longname,folder);
		g_free(folder);
	}
LEAVE_FUNC;
}

static	void
SavePreviousFolder(
	GtkWidget	*widget,
	WidgetData	*wdata,
	gpointer 	data) 
{
	GtkPandaFileEntry 	*fentry;
	char				*longname;

	fentry = GTK_PANDA_FILE_ENTRY(widget);
	longname = (char *)glade_get_widget_long_name(widget);
	SetWidgetCache(longname, fentry->folder);
}

static	void
SetColorButton(
	GtkWidget		*widget,
	WidgetData		*wdata,
	_ColorButton	*data)
{
	GtkColorButton 	*cb;
	GdkColor color;
ENTER_FUNC;
	SetCommon(widget,wdata);
	cb = GTK_COLOR_BUTTON(widget);
	if (gdk_color_parse(data->color,&color)) {
		gtk_color_button_set_color(cb,&color);
	}
LEAVE_FUNC;
}

static	void
GetColorButton(
	GtkWidget		*widget,
	_ColorButton	*data)
{
	GtkColorButton *cb;
	GdkColor color;
ENTER_FUNC;
	cb = GTK_COLOR_BUTTON(widget);
	gtk_color_button_get_color(cb,&color);
	data->color = g_strdup_printf("#%02X%02X%02X",
		(guint)(color.red/256.0),
		(guint)(color.green/256.0),
		(guint)(color.blue/256.0)
		);
LEAVE_FUNC;
}

static	void
SetFileEntry(
	GtkWidget			*widget,
	WidgetData	*wdata,
	_FileEntry			*data)
{
static GHashTable		*connectlist = NULL;

	GtkPandaFileEntry 	*fentry;
	GtkWidget			*subWidget;
	WidgetData			*subdata;
	char				*longname;
	char				*folder;

ENTER_FUNC;

	SetCommon(widget,wdata);
	fentry = GTK_PANDA_FILE_ENTRY(widget);
	g_return_if_fail(data->binary != NULL);
	longname = (char *)glade_get_widget_long_name(widget);

	if (connectlist == NULL) {
		connectlist = NewNameHash();
	}
    if (g_hash_table_lookup(connectlist, longname) == NULL) {
		g_hash_table_insert(connectlist, longname, longname);
		g_signal_connect_after(G_OBJECT(widget), "done_action", 
			G_CALLBACK(SavePreviousFolder), NULL);
	}

	folder = GetWidgetCache(longname);
	if (folder == NULL) {
		folder = "";
	}
	gtk_panda_file_entry_set_folder(fentry, folder);

	if (LBS_Size(data->binary) > 0) {
		//download
		gtk_panda_file_entry_set_mode(fentry, 
			GTK_FILE_CHOOSER_ACTION_SAVE);
		gtk_panda_file_entry_set_data(fentry,
			LBS_Size(data->binary), LBS_Body(data->binary));
		//set subwidget
		subdata = g_hash_table_lookup(WidgetDataTable, data->subname);
		subWidget = GetWidgetByLongName(data->subname);
		if (subdata != NULL || subWidget != NULL) {
			SetEntry(subWidget, subdata,(_Entry *)subdata->attrs);
		}
		g_signal_emit_by_name(G_OBJECT(widget), "browse_clicked", NULL);
	} else {
		//upload
		gtk_panda_file_entry_set_mode(GTK_PANDA_FILE_ENTRY(widget), 
			GTK_FILE_CHOOSER_ACTION_OPEN);
	}
LEAVE_FUNC;
}

static	void
GetFileEntry(
	GtkWidget			*widget,
	_FileEntry			*data)
{
	GtkFileChooserAction mode;
	GtkPandaFileEntry *fe;
	GError *error = NULL;
	gchar *contents;
	gsize size;

ENTER_FUNC;
	fe = GTK_PANDA_FILE_ENTRY(widget);
	mode = gtk_panda_file_entry_get_mode(fe);
	if (mode == GTK_FILE_CHOOSER_ACTION_SAVE) {
		data->path = NULL;
	} else {
		data->path = gtk_editable_get_chars(
			GTK_EDITABLE(fe->entry),0,-1);
		if(!g_file_get_contents(data->path, 
				&contents, &size, &error)) {
			g_error_free(error);
			return;
		}
		data->binary->body = contents;
		data->binary->size = data->binary->asize = data->binary->ptr = size;
	}
LEAVE_FUNC;
}

static void
SetPixmap(
	GtkWidget			*widget,
	WidgetData	*wdata,
	_Pixmap				*data)
{
ENTER_FUNC;
	SetCommon(widget,wdata);
	if ( LBS_Size(data->binary) <= 0) {
		gtk_widget_hide(widget); 
	} else {
		gtk_widget_show(widget); 
		gtk_panda_pixmap_set_image(GTK_PANDA_PIXMAP(widget), 
			(gchar*)LBS_Body(data->binary),(gsize)LBS_Size(data->binary));
	}
LEAVE_FUNC;
}

/******************************************************************************/
/* api                                                                        */
/******************************************************************************/

extern	WidgetType
GetWidgetType(
	char	*wname,
	char	*name)
{
	WindowData	*wdata;
	GtkWidget	*widget;
	long 		type;

	wdata = g_hash_table_lookup(WindowTable, wname);
	if (wdata != NULL && wdata->xml != NULL) {
		widget = glade_xml_get_widget_by_long_name(
    				(GladeXML *)wdata->xml, name);
	} else {
		MessageLogPrintf("window data [%s] is not found", wname);
		return WIDGET_TYPE_UNKNOWN;
	}

	if (widget != NULL) {
		type = (long)(G_OBJECT_TYPE(widget));
		if (type == GTK_TYPE_NUMBER_ENTRY) {
			return WIDGET_TYPE_NUMBER_ENTRY;
		} else if (type == GTK_PANDA_TYPE_COMBO) {
			return WIDGET_TYPE_PANDA_COMBO;
		} else if (type == GTK_PANDA_TYPE_CLIST) {
			return WIDGET_TYPE_PANDA_CLIST;
		} else if (type == GTK_PANDA_TYPE_ENTRY) {
			return WIDGET_TYPE_PANDA_ENTRY;
		} else if (type == GTK_PANDA_TYPE_TEXT) {
			return WIDGET_TYPE_PANDA_TEXT;
		} else if (type == GTK_PANDA_TYPE_PDF) {
			return WIDGET_TYPE_PANDA_PREVIEW;
		} else if (type == GTK_PANDA_TYPE_TIMER) {
			return WIDGET_TYPE_PANDA_TIMER;
		} else if (type == GTK_PANDA_TYPE_DOWNLOAD) {
			return WIDGET_TYPE_PANDA_DOWNLOAD;
		} else if (type == GTK_PANDA_TYPE_PRINT) {
			return WIDGET_TYPE_PANDA_PRINT;
		} else if (type == GTK_PANDA_TYPE_HTML) {
			return WIDGET_TYPE_PANDA_HTML;
		} else if (type == GTK_PANDA_TYPE_TABLE) {
			return WIDGET_TYPE_PANDA_TABLE;
		} else if (type == GTK_TYPE_WINDOW) {
			return WIDGET_TYPE_WINDOW;
		} else if (type == GTK_TYPE_ENTRY) {
			return WIDGET_TYPE_ENTRY;
		} else if (type == GTK_PANDA_TYPE_TEXT) {
			return WIDGET_TYPE_TEXT;
		} else if (type == GTK_TYPE_LABEL) {
			return WIDGET_TYPE_LABEL;
		} else if (type == GTK_TYPE_BUTTON) {
			return WIDGET_TYPE_BUTTON;
		} else if (type == GTK_TYPE_TOGGLE_BUTTON) {
			return WIDGET_TYPE_TOGGLE_BUTTON;
		} else if (type == GTK_TYPE_CHECK_BUTTON) {
			return WIDGET_TYPE_CHECK_BUTTON;
		} else if (type == GTK_TYPE_RADIO_BUTTON) {
			return WIDGET_TYPE_RADIO_BUTTON;
		} else if (type == GTK_TYPE_CALENDAR) {
			return WIDGET_TYPE_CALENDAR;
		} else if (type == GTK_TYPE_NOTEBOOK) {
			return WIDGET_TYPE_NOTEBOOK;
		} else if (type == GTK_TYPE_PROGRESS_BAR) {
			return WIDGET_TYPE_PROGRESS_BAR;
		} else if (type == GTK_TYPE_FRAME) {
			return WIDGET_TYPE_FRAME;
		} else if (type == GTK_TYPE_SCROLLED_WINDOW) {
			return WIDGET_TYPE_SCROLLED_WINDOW;
		} else if (type == GTK_TYPE_FILE_CHOOSER_BUTTON) {
			return WIDGET_TYPE_FILE_CHOOSER_BUTTON;
		} else if (type == GTK_TYPE_COLOR_BUTTON) {
			return WIDGET_TYPE_COLOR_BUTTON;
		} else if (type == GTK_PANDA_TYPE_PIXMAP) {
			return WIDGET_TYPE_PIXMAP;
		} else if (type == GTK_PANDA_TYPE_FILE_ENTRY) {
			return WIDGET_TYPE_FILE_ENTRY;
		}
	}
	return WIDGET_TYPE_UNKNOWN;
}

extern	void
GetWidgetData(WidgetData	*data)
{
	GtkWidget	*widget;

	widget = GetWidgetByLongName(data->name);
	g_return_if_fail(widget != NULL);

	switch (data->type) {
// gtk+panda
	case WIDGET_TYPE_NUMBER_ENTRY:
		GetNumberEntry(widget, (_NumberEntry*)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_CLIST:
		GetPandaCList(widget, (_CList*)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_ENTRY:
	case WIDGET_TYPE_ENTRY:
		GetEntry(widget, (_Entry*)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TEXT:
		GetText(widget, (_Text*)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TIMER:
		GetPandaTimer(widget, (_Timer*)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TABLE:
		GetPandaTable(widget, (_Table*)data->attrs);
		break;
	case WIDGET_TYPE_TEXT:
		GetText(widget, (_Text*)data->attrs);
		break;
	case WIDGET_TYPE_TOGGLE_BUTTON:
	case WIDGET_TYPE_CHECK_BUTTON:
	case WIDGET_TYPE_RADIO_BUTTON:
		GetButton(widget, (_Button*)data->attrs);
		break;
	case WIDGET_TYPE_CALENDAR:
		GetCalendar(widget, (_Calendar*)data->attrs);
		break;
	case WIDGET_TYPE_NOTEBOOK:
		GetNotebook(widget, (_Notebook*)data->attrs);
		break;
	case WIDGET_TYPE_PROGRESS_BAR:
		GetProgressBar(widget, (_ProgressBar*)data->attrs);
		break;
	case WIDGET_TYPE_SCROLLED_WINDOW:
		GetScrolledWindow(widget, (_ScrolledWindow*)data->attrs);
		break;
	case WIDGET_TYPE_FILE_CHOOSER_BUTTON:
		GetFileChooserButton(widget, (_FileChooserButton*)data->attrs);
		break;
	case WIDGET_TYPE_COLOR_BUTTON:
		GetColorButton(widget, (_ColorButton*)data->attrs);
		break;
// gtk+
	case WIDGET_TYPE_FILE_ENTRY:
		GetFileEntry(widget, (_FileEntry*)data->attrs);
		break;
	default:
		MessageLogPrintf("invalid widget [%s]", data->name);
		break;
	}
}

static	void
UpdateWidget(WidgetData *data)
{
	GtkWidget	*widget;

	g_return_if_fail(data != NULL);
	widget = GetWidgetByLongName(data->name);
	if (widget == NULL) {
		MessagePrintf("widget [%s] is not found", data->name);
		return;
	}

	switch (data->type) {
// gtk+panda
	case WIDGET_TYPE_NUMBER_ENTRY:
		SetNumberEntry(widget, data,(_NumberEntry *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_COMBO:
		SetPandaCombo(widget, data,(_Combo *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_CLIST:
		SetPandaCList(widget, data,(_CList *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_ENTRY:
		SetEntry(widget, data,(_Entry *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TEXT:
		SetText(widget, data,(_Text *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_PREVIEW:
		SetPandaPreview(widget, data,(_PREVIEW *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TIMER:
		SetPandaTimer(widget, data,(_Timer *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_DOWNLOAD:
		SetPandaDownload(widget, data,(_Download *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_HTML:
		SetPandaHTML(widget, data,(_HTML *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TABLE:
		SetPandaTable(widget, data,(_Table *)data->attrs);
		break;
// gtk+
	case WIDGET_TYPE_ENTRY:
		SetEntry(widget, data,(_Entry *)data->attrs);
		break;
	case WIDGET_TYPE_TEXT:
		SetText(widget, data,(_Text *)data->attrs);
		break;
	case WIDGET_TYPE_LABEL:
		SetLabel(widget, data,(_Label *)data->attrs);
		break;
	case WIDGET_TYPE_BUTTON:
	case WIDGET_TYPE_TOGGLE_BUTTON:
	case WIDGET_TYPE_CHECK_BUTTON:
	case WIDGET_TYPE_RADIO_BUTTON:
		SetButton(widget, data,(_Button *)data->attrs);
		break;
	case WIDGET_TYPE_CALENDAR:
		SetCalendar(widget, data,(_Calendar*)data->attrs);
		break;
	case WIDGET_TYPE_NOTEBOOK:
		SetNotebook(widget, data,(_Notebook*)data->attrs);
		break;
	case WIDGET_TYPE_PROGRESS_BAR:
		SetProgressBar(widget, data,(_ProgressBar*)data->attrs);
		break;
	case WIDGET_TYPE_WINDOW:
		SetWindow(widget, data,(_Window*)data->attrs);
		break;
	case WIDGET_TYPE_FRAME:
		SetFrame(widget, data,(_Frame *)data->attrs);
		break;
	case WIDGET_TYPE_SCROLLED_WINDOW:
		SetScrolledWindow(widget, data,(_ScrolledWindow *)data->attrs);
		break;
	case WIDGET_TYPE_FILE_CHOOSER_BUTTON:
		SetFileChooserButton(widget, data,(_FileChooserButton *)data->attrs);
		break;
	case WIDGET_TYPE_COLOR_BUTTON:
		SetColorButton(widget, data,(_ColorButton *)data->attrs);
		break;
// Gnome
	case WIDGET_TYPE_FILE_ENTRY:
		SetFileEntry(widget, data,(_FileEntry *)data->attrs);
		break;
	case WIDGET_TYPE_PIXMAP:
		SetPixmap(widget, data,(_Pixmap *)data->attrs);
		break;
	default:
		//MessageLogPrintf("invalid widget [%s]", data->name);
		break;
	}
}

extern	void
UpdateWindow(char *windowName)
{
	WindowData	*wdata;
	WidgetData	*data;

	wdata = (WindowData *)g_hash_table_lookup(WindowTable, windowName);
	g_return_if_fail(wdata != NULL);
	while(
		(data = (WidgetData *)DeQueueNoWait(wdata->UpdateWidgetQueue)) != NULL
	) {
		UpdateWidget(data);
	}
}
