/*
 * PANDA -- a simple transaction monitor
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
#include	<string.h>
#include	<glib.h>
#include	"enum.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"DDparser.h"
#define		_DRIVER
#include	"driver.h"
#include	"dirs.h"
#include	"debug.h"

extern	ScreenData	*
NewScreenData(void)
{
	ScreenData	*scr;

ENTER_FUNC;
	scr = New(ScreenData);
	memclear(scr->window,SIZE_NAME+1);
	memclear(scr->widget,SIZE_NAME+1);
	memclear(scr->event,SIZE_EVENT+1);
	memclear(scr->cmd,SIZE_LONGNAME+1);
	memclear(scr->term,SIZE_TERM+1);
	memclear(scr->host,SIZE_HOST+1);
	memclear(scr->user,SIZE_USER+1);
	scr->Windows = NewNameHash();
LEAVE_FUNC;
	return	(scr); 
}

static	gboolean
FreeWindows(
	gpointer	key,
	gpointer	value,
	gpointer	user_data)
{
	WindowData *data;

	xfree(key);
	data = (WindowData*)value;
	/*
	should free data->rec
	*/
	xfree(data);
	return TRUE;
}

extern	void
FreeScreenData(
	ScreenData	*scr)
{

ENTER_FUNC;
	g_hash_table_foreach_remove(scr->Windows,FreeWindows,NULL);
	g_hash_table_destroy(scr->Windows);
	xfree(scr);
LEAVE_FUNC;
}

extern WindowData	*
RegisterWindow(
	ScreenData	*scr,
	const char	*name)
{
	WindowData		*data;
	RecordStruct	*rec;
	char			fname[SIZE_LONGNAME+1];
ENTER_FUNC;
	dbgprintf("RegisterWindowData(%s)",name);
	if (name == NULL) {
		Warning("invalid window name[%s]",name);
		return NULL;
	}
	data = (WindowData *)g_hash_table_lookup(scr->Windows,name);
	if (data == NULL) {
		sprintf(fname,"%s.rec",name);
		rec = ReadRecordDefine(fname);
		if (rec == NULL) {
			Warning("window not found [%s]\n",name);
			return NULL;
		}
		data = New(WindowData);
		data->puttype = SCREEN_NULL;
		data->rec = rec;
		InitializeValue(data->rec->value);
		g_hash_table_insert(scr->Windows,StrDup(name),data);
	} else {
#if 0
		Warning("double registration");
#endif
	}
LEAVE_FUNC;
	return data;
}

static WindowData	*
GetWindowData(
	ScreenData	*scr,
	const char	*name)
{
ENTER_FUNC;
	dbgprintf("GetWindowData(%s)",name);
	if (name == NULL) {
		Warning("invalid window name[%s]",name);
		return NULL;
	}
LEAVE_FUNC;
	return (WindowData *)g_hash_table_lookup(scr->Windows,name);
}

extern ValueStruct *
GetWindowValue(
	ScreenData		*scr,
	const char		*wname)
{
	WindowData	*data;
ENTER_FUNC;
	data = GetWindowData(scr,wname);
	if (data == NULL) {
		return NULL;
	}
	return data->rec->value;
LEAVE_FUNC;
}

extern	void
PutWindow(
	ScreenData		*scr,
	const char		*wname,
	unsigned char	type)
{
	WindowData	*data;
ENTER_FUNC;
	dbgprintf("window = [%s]",wname);
	dbgprintf("type   = %02X",(int)type);
	data = GetWindowData(scr,wname);
	if (data != NULL) {
		data->puttype = type;
	} else {
		Warning("window data not found[%s]",wname);
		scr->status = SCREEN_DATA_END;
	}
LEAVE_FUNC;
}

