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
#ifdef	_PROTOCOL_C
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
	WIDGET_TYPE_PANDA_HTML,
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
	void 		*attrs;
} WidgetData;

typedef struct __Entry {
	int 			state;
	char 			*style;
	char 			*text;
	char 			*text_name;
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

typedef struct __NumberEntry{
	int 			state;
	char 			*style;
	Fixed 			*fixed;
	char 			*fixed_name;
	PacketDataType 	ptype;
} _NumberEntry;

typedef struct __Label{
	char 			*style;
	char 			*text;
	char 			*text_name;
	PacketDataType 	ptype;
} _Label;

typedef struct __Text{
	int 			state;
	char 			*style;
	char 			*text;
	char 			*text_name;
	PacketDataType 	ptype;
} _Text;

typedef struct __Button{
	int 			state;
	char 			*style;
	char 			*label;
	Bool 			have_button_state;
	Bool 			button_state;
	char 			*button_state_name;
	PacketDataType 	ptype;
} _Button;

typedef struct __Combo{
	int 	state;
	char 	*style;
	int 	count;
	GList 	*item_list;
    char 	*subname;
} _Combo;

typedef struct __CList{
	int 	state;
	char 	*style;
	int 	count;
	int 	from;
	int 	row;
	float 	rowattr;
	int 	column;
	GList 	*item_list;
	GList 	*state_list;
	char 	*state_list_name;
} _CList;

typedef struct __Calendar{
	int 	state;
	char 	*style;
	int 	year;
	int 	month;
	int 	day;
} _Calendar;

typedef struct __Notebook{
	int 			state;
	char 			*style;
	int 			pageno;
	char 			*subname;
	PacketDataType 	ptype;
} _Notebook;

typedef struct __ProgressBar{
	int 			state;
	char 			*style;
	int 			value;
	PacketDataType 	ptype;
} _ProgressBar;

typedef struct __Window{
	int 	state;
	char 	*style;
	char 	*title;
} _Window;

typedef struct __Frame{
	int 	state;
	char 	*style;
	char 	*label;
	char 	*subname;
} _Frame;

typedef struct __ScrolledWindow{
	int				state;
	char 			*style;
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
