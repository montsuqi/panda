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

#ifndef	_MARSHALLER_H
#define	_MARSHALLER_H

#undef	GLOBAL
#ifdef	MARSHALLER
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
GLOBAL	GHashTable	*WidgetDataTable;

typedef enum _WidgetType {
// gtk+panda
	WIDGET_TYPE_NUMBER_ENTRY,
	WIDGET_TYPE_PANDA_COMBO,
	WIDGET_TYPE_PANDA_CLIST,
	WIDGET_TYPE_PANDA_ENTRY,
	WIDGET_TYPE_PANDA_TEXT,
	WIDGET_TYPE_PANDA_PREVIEW,
	WIDGET_TYPE_PANDA_TIMER,
	WIDGET_TYPE_PANDA_DOWNLOAD,
	WIDGET_TYPE_PANDA_PRINT,
	WIDGET_TYPE_PANDA_HTML,
	WIDGET_TYPE_PANDA_TABLE,
// gtk
	WIDGET_TYPE_ENTRY,
	WIDGET_TYPE_TEXT,
	WIDGET_TYPE_LABEL,
	WIDGET_TYPE_BUTTON,
	WIDGET_TYPE_TOGGLE_BUTTON,
	WIDGET_TYPE_CHECK_BUTTON,
	WIDGET_TYPE_RADIO_BUTTON,
	WIDGET_TYPE_CALENDAR,
	WIDGET_TYPE_NOTEBOOK,
	WIDGET_TYPE_PROGRESS_BAR,
	WIDGET_TYPE_WINDOW,
	WIDGET_TYPE_FRAME,
	WIDGET_TYPE_SCROLLED_WINDOW,
// gnome
	WIDGET_TYPE_FILE_ENTRY,
	WIDGET_TYPE_PIXMAP,
	WIDGET_TYPE_UNKNOWN
} WidgetType;

typedef struct _WidgetData {
	WidgetType	type;
	char 		*name;
	WindowData 	*window;
	/* common attrs */
	int 		state;
	char 		*style;
	gboolean	visible;
	/* specific attrs*/
	void 		*attrs;
} WidgetData;

typedef struct __Entry {
	char 			*text;
	char 			*text_name;
	Bool			editable;
	PacketDataType 	ptype;
} _Entry;

typedef struct __PREVIEW {
	LargeByteString 	*binary;
} _PREVIEW;

typedef struct __HTML {
	char 	*uri;
} _HTML;

typedef struct __Timer {
	int 			duration;
	PacketDataType 	ptype;
} _Timer;

typedef struct __Download {
	LargeByteString	*binary;
	char			*filename;
} _Download;

typedef struct __NumberEntry{
	Fixed 			*fixed;
	char 			*fixed_name;
	Bool			editable;
	PacketDataType 	ptype;
} _NumberEntry;

typedef struct __Label{
	char 			*text;
	char 			*text_name;
	PacketDataType 	ptype;
} _Label;

typedef struct __Text{
	char 			*text;
	char 			*text_name;
	PacketDataType 	ptype;
} _Text;

typedef struct __Button{
	char 			*label;
	Bool 			have_button_state;
	Bool 			button_state;
	char 			*button_state_name;
	PacketDataType 	ptype;
} _Button;

typedef struct __Combo{
	int 	count;
	char 	**itemdata;
    char 	*subname;
} _Combo;

typedef struct __CList{
	int 	count;
	int 	from;
	int 	row;
	float 	rowattr;
	int 	column;
	GList 	*clistdata;
	char 	**states;
	char 	*states_name;
} _CList;

typedef struct __Table{
	gboolean	dofocus;
	int			trow;
	float		trowattr;
	int			tcolumn;
	gchar 		*tvalue;
	gchar 		**fgcolors;
	gchar 		**bgcolors;
	GList		*tdata;
} _Table;

typedef struct __Calendar{
	int 	year;
	int 	month;
	int 	day;
} _Calendar;

typedef struct __Notebook{
	int 			pageno;
	char 			*subname;
	PacketDataType 	ptype;
} _Notebook;

typedef struct __ProgressBar{
	int 			value;
	PacketDataType 	ptype;
} _ProgressBar;

typedef struct __Window{
	char 	*title;
	char	*summary;
	char	*body;
	char	*icon;
	int		timeout;
} _Window;

typedef struct __Frame{
	char 	*label;
	char 	*subname;
} _Frame;

typedef struct __ScrolledWindow{
	int 			vpos;
	int 			hpos;
	char 			*subname;
	PacketDataType 	ptype;
} _ScrolledWindow;

typedef struct __Pixmap{
	LargeByteString	*binary;
} _Pixmap;

typedef struct __FileEntry{
	LargeByteString	*binary;
	char 			*subname;
	char 			*path;
} _FileEntry;


extern	Bool	RecvWidgetData(char *widgetName,NETFILE *fp);
extern	void	SendWidgetData(gpointer key,gpointer value,gpointer user_data);

#endif
