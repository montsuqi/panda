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
#include 	<gnome.h>
#include	<gtkpanda/gtkpanda.h>

#include	"types.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"styleParser.h"
#include	"net.h"
#include	"comm.h"
#include	"protocol.h"
#include	"widgetOPS.h"
#include	"action.h"
#include	"fileEntry.h"
#include	"marshaller.h"
#include	"queue.h"
#include	"widgetcache.h"
#include	"toplevel.h"
#include	"message.h"
#include	"debug.h"

static gchar *
NewTempname(void)
{
	return g_strconcat(g_get_tmp_dir(), "/__glclientXXXXXX", NULL);
}

static FILE *
CreateTempfile(
	gchar *tmpname)
{
    int fildes;
    FILE *file;
	mode_t mode;

ENTER_FUNC;
	mode = umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if ((fildes = mkstemp(tmpname)) == -1) {
		Error("Couldn't make tempfile %s",tmpname );
	}
	if ((file = fdopen(fildes, "wb")) == NULL ) {
		Error("Couldn't open tempfile %s",tmpname);
	}
	umask(mode);
LEAVE_FUNC;
	return file;
}

static	char *
CreateBinaryFile(LargeByteString *binary)
{
    FILE *file;
    gchar *tmpname;

	tmpname = NewTempname();
	file = CreateTempfile(tmpname);
	fwrite(LBS_Body(binary), sizeof(byte), LBS_Size(binary), file);
	fclose(file);

	return tmpname;
}

static	void
DestroyBinaryFile(char *tmpname)
{
	unlink(tmpname);
	g_free(tmpname);
}

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
SetWidgetLabelRecursive(
	GtkWidget	*widget,
	char		*label)
{
	if		(  GTK_IS_LABEL(widget)  ) {
		gtk_label_set(GTK_LABEL(widget),label);
	} else {
		gtk_container_foreach(GTK_CONTAINER(widget),
			(GtkCallback)SetWidgetLabelRecursive,label);
	}
}

static	void
SetStyle(
	GtkWidget	*widget,
	GtkStyle	*style)
{
	static GdkFont *font = NULL;
	GtkStyle *s;
	if (style != NULL) {
		if (font == NULL) {
			s = gtk_rc_get_style(widget);
			font = s->font;
		}
		gtk_widget_set_style(widget, style);
		widget->style->font = font;
    	if (GTK_IS_CONTAINER(widget)){
    	    gtk_container_foreach(GTK_CONTAINER(widget),
				(GtkCallback)SetStyle,style);
    	}
	}
}

/******************************************************************************/
/* gtk+panda widget                                                           */
/******************************************************************************/

static	void
SetPandaPS(
	GtkWidget	*widget,
	_PREVIEW	*data)
{
ENTER_FUNC;
	gtk_panda_ps_set(GTK_PANDA_PS(widget), 
		LBS_Size(data->binary), LBS_Body(data->binary));
LEAVE_FUNC;
}

static	void
SetPandaTimer(
	GtkWidget	*widget,
	_Timer		*data)
{
ENTER_FUNC;
	gtk_panda_timer_set(GTK_PANDA_TIMER(widget),
		data->duration);
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
	_NumberEntry	*data)
{
	Numeric	value;

ENTER_FUNC;
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget, GetStyle(data->style));
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
	_Combo		*data)
{
	GtkPandaCombo	*combo;

ENTER_FUNC;
	combo = GTK_PANDA_COMBO(widget);
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget,GetStyle(data->style));
	gtk_signal_handler_block(GTK_OBJECT(combo->list), combo->list_change_id);
	gtkpanda_combo_set_popdown_strings(combo,data->item_list);
	gtk_signal_handler_unblock(GTK_OBJECT(combo->list), combo->list_change_id);
LEAVE_FUNC;
}

static	void
SetPandaCList(
	GtkWidget	*widget,
	_CList		*data)
{
	int			j
	,			k;
	char		**rdata;
	Bool		*fActive;
	GList		*cell_list;

ENTER_FUNC;
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget,GetStyle(data->style));

	// items
	gtkpanda_clist_freeze(GTK_PANDA_CLIST(widget));
	gtkpanda_clist_clear(GTK_PANDA_CLIST(widget));

	for	( j = 0 ; j < g_list_length(data->item_list) ; j ++ ) {
		cell_list = g_list_nth_data(data->item_list, j);
		rdata = (char **)xmalloc(sizeof(char *)*g_list_length(cell_list));
		for	( k = 0 ; k < g_list_length(cell_list) ; k ++ ) {
			rdata[k] = StrDup(g_list_nth_data(cell_list,k));
		}
		if ((j >= data->from) && ((j - data->from) < data->count)) {
			gtkpanda_clist_append(GTK_PANDA_CLIST(widget), rdata);
		}
		for	( k = 0 ; k < g_list_length(cell_list) ; k ++ ) {
			g_free(rdata[k]);
		}
		xfree(rdata);
	}
	gtkpanda_clist_thaw(GTK_PANDA_CLIST(widget));
	gtkpanda_clist_moveto(GTK_PANDA_CLIST(widget), 
		data->row - 1, 0, data->rowattr, 0.0); 

	for	( j = 0 ; j < g_list_length(data->state_list) ; j ++ ) {
		if ((j >= data->from) && ((j - data->from) < data->count)) {
			fActive = g_list_nth_data(data->state_list, j);
			if (*fActive) {
				gtkpanda_clist_select_row(GTK_PANDA_CLIST(widget),
					(j - data->from),0);
			} else {
				gtkpanda_clist_unselect_row(GTK_PANDA_CLIST(widget),
					(j - data->from),0);
			}
		}
	}
LEAVE_FUNC;
}

static	void
GetPandaCList(
	GtkWidget	*widget,
	_CList		*data)
{
	GList				*children;
	GtkPandaCListRow	*clist_row;
	int					i;
	GtkVisibility		visi;
	gboolean			fVisibleRow;
	Bool				*bool;

ENTER_FUNC;
	fVisibleRow = FALSE;
	for	( children = GTK_PANDA_CLIST(widget)->row_list , i = 0 ;
		  children  !=  NULL ; children = children->next , i ++ ) {
		if		(  ( clist_row = GTK_PANDA_CLIST_ROW(children) )  !=  NULL  ) {
			bool = (Bool*)g_list_nth_data(data->state_list, i);
			*bool = (clist_row->state == GTK_STATE_SELECTED) ? TRUE : FALSE;
		}
		if	( !fVisibleRow ) {
            visi = gtkpanda_clist_row_is_visible(GTK_PANDA_CLIST(widget),i);
			if	( visi == GTK_VISIBILITY_FULL ) {
				data->row = i + 1;
				fVisibleRow = TRUE;
			}
		} 
	}
LEAVE_FUNC;
}

static	void
SetPandaHTML(
	GtkWidget		*widget,
	_HTML			*data)
{
ENTER_FUNC;
	if (data->uri != NULL) {
		gtk_panda_html_set_uri (GTK_PANDA_HTML(widget), data->uri);
	}
LEAVE_FUNC;
}

/******************************************************************************/
/* gtk+ widget                                                                */
/******************************************************************************/

static	void
SetEntry(
	GtkWidget		*widget,
	_Entry			*data)
{
	gchar *text;
ENTER_FUNC;
	SetState(widget,(GtkStateType)(data->state));
	SetStyle(widget,GetStyle(data->style));
	text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	if (text != NULL && strcmp (text, data->text)) {
		gtk_entry_set_text(GTK_ENTRY(widget), data->text);
	}
	g_free(text);
	if (!GTK_WIDGET_HAS_FOCUS(widget)
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
	data->text = strdup((char *)gtk_entry_get_text(GTK_ENTRY(widget)));
LEAVE_FUNC;
}

static	void
SetLabel(
	GtkWidget		*widget,
	_Label			*data)
{
ENTER_FUNC;
	SetStyle(widget, GetStyle(data->style));
	gtk_label_set(GTK_LABEL(widget),data->text);
	gtk_widget_show(widget);
LEAVE_FUNC;
}

static	void
SetText(
	GtkWidget	*widget,
	_Text		*data)
{
	gfloat		vvalue;
	gfloat		hvalue;

ENTER_FUNC;
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget,GetStyle(data->style));
	gtk_text_freeze(GTK_TEXT(widget));
	vvalue = GTK_TEXT(widget)->vadj->value;
	hvalue = GTK_TEXT(widget)->hadj->value;
	gtk_text_set_point(GTK_TEXT(widget), 0);
	gtk_text_forward_delete(GTK_TEXT(widget),
		gtk_text_get_length((GTK_TEXT(widget))));
	gtk_text_insert(GTK_TEXT(widget),NULL,NULL,NULL,
		data->text,strlen(data->text));
	gtk_text_thaw(GTK_TEXT(widget));
	gtk_adjustment_set_value(GTK_TEXT(widget)->vadj, vvalue);
	gtk_adjustment_set_value(GTK_TEXT(widget)->hadj, hvalue);
LEAVE_FUNC;
}

static	void
GetText(
	GtkWidget		*widget,
	_Text			*data)
{
ENTER_FUNC;
	g_free(data->text);
	data->text = gtk_editable_get_chars(GTK_EDITABLE(widget),0,-1); 
LEAVE_FUNC;
}

static	void
SetButton(
	GtkWidget			*widget,
	_Button				*data)
{
ENTER_FUNC;
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget,GetStyle(data->style));
	if (data->label != NULL) {
		SetWidgetLabelRecursive(widget,data->label);
		gtk_widget_show(widget);
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
	_Calendar			*data)
{
ENTER_FUNC;
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget,GetStyle(data->style));
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
	_Notebook			*data)
{
ENTER_FUNC;
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget,GetStyle(data->style));
	SetObjectData(widget, "recv_page", (void *)&(data->pageno));
	gtk_notebook_set_page(GTK_NOTEBOOK(widget),data->pageno);
LEAVE_FUNC;
}

static	void
GetNotebook(
	GtkWidget			*widget,
	_Notebook			*data)
{
	gpointer	*object;

ENTER_FUNC;
	object = GetObjectData(GTK_WIDGET(widget), "page");
	data->pageno = (int)(*object);
LEAVE_FUNC;
}

static	void
SetProgressBar(
	GtkWidget				*widget,
	_ProgressBar			*data)
{
ENTER_FUNC;
	SetState(widget,(GtkStateType)(data->state));
	SetStyle(widget,GetStyle(data->style));
	gtk_progress_set_value(GTK_PROGRESS(widget),data->value);
LEAVE_FUNC;
}

static	void
GetProgressBar(
	GtkWidget				*widget,
	_ProgressBar			*data)
{
ENTER_FUNC;
	data->value = gtk_progress_get_value(GTK_PROGRESS(widget));
LEAVE_FUNC;
}

static	void
SetWindow(
	GtkWidget			*widget,
	_Window				*data)
{
ENTER_FUNC;
	WindowData *wdata;

	wdata = g_hash_table_lookup(WindowTable, (char *)gtk_widget_get_name(widget));
	SetState(widget,(GtkStateType)(data->state));
	SetStyle(widget,GetStyle(data->style));
	if (data->title != NULL && wdata != NULL) {
		SetSessionTitle(data->title);
		SetTitle(TopWindow, wdata->title);
	}
LEAVE_FUNC;
}

static	void
SetFrame(
	GtkWidget		*widget,
	_Frame			*data)
{
ENTER_FUNC;
	SetState(widget,(GtkStateType)(data->state));
	SetStyle(widget,GetStyle(data->style));
	gtk_frame_set_label(GTK_FRAME(widget),data->label);
LEAVE_FUNC;
}

static	void
SetScrolledWindow(
	GtkWidget					*widget,
	_ScrolledWindow				*data)
{
ENTER_FUNC;
	SetState(widget,(GtkStateType)data->state);
	SetStyle(widget,GetStyle(data->style));
LEAVE_FUNC;
}

static	void
GetScrolledWindow(
	GtkWidget					*widget,
	_ScrolledWindow				*data)
{
	GtkAdjustment	*vad
		,			*had;

ENTER_FUNC;
	vad = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget));
	had = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(widget));
	data->hpos = (int)had->value;
	data->vpos = (int)vad->value;
LEAVE_FUNC;
}

/******************************************************************************/
/* Gnome widget                                                               */
/******************************************************************************/
static	void
SetFileEntry(
	GtkWidget			*widget,
	_FileEntry			*data)
{
	GtkWidget 		*window;
	GtkWidget		*subWidget;
	WidgetData		*subdata;

ENTER_FUNC;
	g_return_if_fail(data->binary != NULL);
	window = gtk_widget_get_toplevel(widget);
	if (LBS_Size(data->binary) > 0) {
		//set subwidget
		subdata = g_hash_table_lookup(WidgetDataTable, data->subname);
		subWidget = GetWidgetByLongName(data->subname);
		if (subdata != NULL || subWidget != NULL) {
			SetEntry(subWidget, (_Entry *)subdata->attrs);
		}

		gtk_object_set_data(GTK_OBJECT(widget), "recvobject", data->binary);
		gtk_signal_connect_after(GTK_OBJECT(widget),
								 "browse_clicked",
								 GTK_SIGNAL_FUNC(browse_clicked),
								 window);
		gtk_signal_emit_by_name (GTK_OBJECT (widget), "browse_clicked");
	} else {
		//upload
		gtk_signal_connect_after(GTK_OBJECT(widget),
								 "browse_clicked",
								 GTK_SIGNAL_FUNC(browse_clicked),
								 window);
		gtk_object_set_data(GTK_OBJECT(widget), "recvobject", NULL);
	}
LEAVE_FUNC;
}

static	void
GetFileEntry(
	GtkWidget			*widget,
	_FileEntry			*data)
{
	LargeByteString	*binary;
	GtkWidget		*subWidget;
	FILE			*fp;
	struct stat		st;
	gchar			*dir;

ENTER_FUNC;
	binary = gtk_object_get_data(GTK_OBJECT(widget), "recvobject");
	if (binary != NULL) {
		data->path = NULL;
	} else {
		subWidget = gnome_file_entry_gtk_entry(
			GNOME_FILE_ENTRY(widget));
		data->path = gtk_editable_get_chars(GTK_EDITABLE(subWidget),
			0,-1);
		if ( stat(data->path,&st) ){
			return;
		}
		if ( (fp = fopen(data->path,"r")) != NULL) {
			LBS_ReserveSize(data->binary,st.st_size,FALSE);
			fread(LBS_Body(data->binary),st.st_size,1,fp);
			fclose(fp);
			dir = g_dirname(data->path);
			SetWidgetCache(StrDup((char *)glade_get_widget_long_name(widget)), 
				dir);
			g_free(dir);
		}
	}
LEAVE_FUNC;
}

static void
SetPixmap(
	GtkWidget			*widget,
	_Pixmap				*data)
{
	gchar 			*tmpname;
	GtkRequisition 	requisition;
	GdkImlibImage 	*im;
	gint 			width
	, 				height;
	gdouble 		scale
	, 				xscale
	, 				yscale;

ENTER_FUNC;
	if ( LBS_Size(data->binary) <= 0) {
		gtk_widget_hide(widget); 
	} else {
		tmpname = CreateBinaryFile(data->binary);
		gtk_widget_size_request(widget, &requisition);
		if ( requisition.width && requisition.height ) {
			width = requisition.width;
			height = requisition.height;
			im = gdk_imlib_load_image ((char *)tmpname);
			if ( im ) {
				xscale = (gdouble)width / im->rgb_width;
				yscale = (gdouble)height / im->rgb_height;
				scale = MIN(xscale, yscale);
				width = im->rgb_width * scale;
				height = im->rgb_height * scale;
				gnome_pixmap_load_imlib_at_size(GNOME_PIXMAP(widget), 
					im, width, height);
			}
		} else {
			gnome_pixmap_load_file(GNOME_PIXMAP(widget), tmpname);
		}
		DestroyBinaryFile(tmpname);
		gtk_widget_show(widget); 
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
		type = (long)(((GtkTypeObject *)widget)->klass->type);
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
		} else if (type == GTK_PANDA_TYPE_PS) {
			return WIDGET_TYPE_PANDA_PREVIEW;
		} else if (type == GTK_PANDA_TYPE_TIMER) {
			return WIDGET_TYPE_PANDA_TIMER;
		} else if (type == GTK_PANDA_TYPE_HTML) {
			return WIDGET_TYPE_PANDA_HTML;
		} else if (type == GTK_TYPE_WINDOW) {
			return WIDGET_TYPE_WINDOW;
		} else if (type == GTK_TYPE_ENTRY) {
			return WIDGET_TYPE_ENTRY;
		} else if (type == GTK_TYPE_TEXT) {
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
		} else if (type == GNOME_TYPE_PIXMAP) {
			return WIDGET_TYPE_PIXMAP;
		} else if (type == GNOME_TYPE_FILE_ENTRY) {
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
		GetEntry(widget, (_Entry*)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TEXT:
		GetText(widget, (_Text*)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TIMER:
		GetPandaTimer(widget, (_Timer*)data->attrs);
		break;
// gtk+
	case WIDGET_TYPE_ENTRY:
		GetEntry(widget, (_Entry*)data->attrs);
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
		SetNumberEntry(widget, (_NumberEntry *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_COMBO:
		SetPandaCombo(widget, (_Combo *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_CLIST:
		SetPandaCList(widget, (_CList *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_ENTRY:
		SetEntry(widget, (_Entry *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TEXT:
		SetText(widget, (_Text *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_PREVIEW:
		SetPandaPS(widget, (_PREVIEW *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_TIMER:
		SetPandaTimer(widget, (_Timer *)data->attrs);
		break;
	case WIDGET_TYPE_PANDA_HTML:
		SetPandaHTML(widget, (_HTML *)data->attrs);
		break;
// gtk+
	case WIDGET_TYPE_ENTRY:
		SetEntry(widget, (_Entry *)data->attrs);
		break;
	case WIDGET_TYPE_TEXT:
		SetText(widget, (_Text *)data->attrs);
		break;
	case WIDGET_TYPE_LABEL:
		SetLabel(widget, (_Label *)data->attrs);
		break;
	case WIDGET_TYPE_BUTTON:
	case WIDGET_TYPE_TOGGLE_BUTTON:
	case WIDGET_TYPE_CHECK_BUTTON:
	case WIDGET_TYPE_RADIO_BUTTON:
		SetButton(widget, (_Button *)data->attrs);
		break;
	case WIDGET_TYPE_CALENDAR:
		SetCalendar(widget, (_Calendar*)data->attrs);
		break;
	case WIDGET_TYPE_NOTEBOOK:
		SetNotebook(widget, (_Notebook*)data->attrs);
		break;
	case WIDGET_TYPE_PROGRESS_BAR:
		SetProgressBar(widget, (_ProgressBar*)data->attrs);
		break;
	case WIDGET_TYPE_WINDOW:
		SetWindow(widget, (_Window*)data->attrs);
		break;
	case WIDGET_TYPE_FRAME:
		SetFrame(widget, (_Frame *)data->attrs);
		break;
	case WIDGET_TYPE_SCROLLED_WINDOW:
		SetScrolledWindow(widget, (_ScrolledWindow *)data->attrs);
		break;
// Gnome
	case WIDGET_TYPE_FILE_ENTRY:
		SetFileEntry(widget, (_FileEntry *)data->attrs);
		break;
	case WIDGET_TYPE_PIXMAP:
		SetPixmap(widget, (_Pixmap *)data->attrs);
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
