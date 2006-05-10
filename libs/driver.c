/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
#include	"types.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"DDparser.h"
#define		_DRIVER
#include	"driver.h"
#include	"dirs.h"
#include	"debug.h"

extern	char	*
PureWindowName(
	char	*comp,
	char	*buff)
{
	char	*p;

	if		(  ( p = strrchr(comp,'/') )  ==  NULL  ) {
		p = comp;
	} else {
		p ++;
	}
	strcpy(buff,p);
	if		(  ( p = strchr(buff,'.') )  !=  NULL  ) {
		*p = 0;
	}
	return	(buff);
}

extern	char	*
PureComponentName(
	char	*comp,
	char	*buff)
{
	char	*p;

	if		(  ( p = strrchr(comp,'/') )  ==  NULL  ) {
		p = comp;
	} else {
		p ++;
	}
	strcpy(buff,p);
	return	(buff);
}

extern	WindowData	*
SetWindowName(
	char		*name)
{
	WindowData	*entry;
	RecordStruct	*rec;
	char		fname[SIZE_LONGNAME+1]
		,		wname[SIZE_LONGNAME+1]
		,		msg[SIZE_BUFF];

ENTER_FUNC;
	dbgprintf("SetWindowName(%s)",name);
	if		(  name  !=  NULL  ) {
		if		(  ThisScreen->Windows  ==  NULL  ) {
			ThisScreen->Windows = NewNameHash();
		}
		if		(  ThisScreen->Records  ==  NULL  ) {
			ThisScreen->Records = NewNameHash();
		}
		PureWindowName(name,wname);
		if		(  ( rec = (RecordStruct *)g_hash_table_lookup(ThisScreen->Records,wname) )
				   ==  NULL  ) {
			sprintf(fname,"%s.rec",wname);
			rec = ReadRecordDefine(fname);
			g_hash_table_insert(ThisScreen->Records,rec->name,rec);
		}
		if		(  ( entry = 
					 (WindowData *)g_hash_table_lookup(ThisScreen->Windows,name) )
				   ==  NULL  ) {
			if		(  rec  !=  NULL  ) {
				entry = New(WindowData);
				entry->PutType = SCREEN_NULL;
				entry->fNew = FALSE;
				entry->name = StrDup(name);
				entry->rec = rec;
				g_hash_table_insert(ThisScreen->Windows,entry->name,entry);
			} else {
				sprintf(msg,"window not found [%s:%s]\n", name, wname);
				MessageLog(msg);
			}
		}
	} else {
		entry = NULL;
	}
LEAVE_FUNC;
	return	(entry);
}

extern	RecordStruct	*
GetWindowRecord(
	char		*name)
{
	WindowData	*win;
	RecordStruct	*rec;

ENTER_FUNC;
	if		(  ( win = g_hash_table_lookup(ThisScreen->Windows,name) )  !=  NULL  ) {
		rec = win->rec;
	} else {
		rec = NULL;
	}
LEAVE_FUNC;
	return	(rec); 
}

extern	Bool
PutWindow(
	WindowData	*win,
	byte		type)
{
	Bool		rc;

ENTER_FUNC;
	if		(  win  !=  NULL  ) {
		win->PutType = type;
		dbgprintf("window  = [%s]",win->name);
		dbgprintf("PutType = %02X",type);
		win->fNew = TRUE;
		ThisScreen->status = APL_SESSION_GET;
		rc = TRUE;
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc);
}

extern	ValueStruct	*
GetWindowValue(
	char		*name)
{
	char	wname[SIZE_NAME+1];
	char	*p
	,		*q;
	RecordStruct	*rec;
	ValueStruct	*val;

#ifdef	DEBUG
	printf("GetWindowValue name = [%s]",name);
#endif
	p = name;
	q = wname;
	while	(  *p  !=  '.'  ) {
		*q ++ = *p ++;
	}
	*q = 0;
	p ++;
	
	rec = GetWindowRecord(wname);
	if		(  rec  !=  NULL  ) {
		val = GetItemLongName(rec->value,p);
	} else {
		val = NULL;
	}
#ifdef	DEBUG
	if		(  val  ==  NULL  ) {
		printf(" is NULL\n");
	} else {
		printf("\n");
	}
#endif
	return	(val);
}

extern	WindowData	*
PutWindowByName(
	char		*wname,
	byte		type)
{
	WindowData	*win;

ENTER_FUNC;
	dbgprintf("window = [%s]",wname);
	dbgprintf("type   = %02X",(int)type);
	win = SetWindowName(wname);
	PutWindow(win,type);
LEAVE_FUNC;
	return	(win);
}

extern	void
LinkModule(
	char		*name)
{
	ThisScreen->status = APL_SESSION_LINK;
	strcpy(ThisScreen->cmd,name);
}

extern	ScreenData	*
NewScreenData(void)
{
	ScreenData	*scr;
	char		*encoding
		,		*lang;

ENTER_FUNC;
	scr = New(ScreenData);
	memclear(scr->window,SIZE_NAME+1);
	memclear(scr->widget,SIZE_NAME+1);
	memclear(scr->event,SIZE_EVENT+1);
	memclear(scr->cmd,SIZE_LONGNAME+1);
	memclear(scr->term,SIZE_TERM+1);
	memclear(scr->user,SIZE_USER+1);
	memclear(scr->other,SIZE_OTHER+1);
	scr->Windows = NULL;
	scr->Records = NULL;
	if		(  libmondai_i18n  ) {
		if		(  ( lang = getenv("LANG") )  !=  NULL  &&
				   ( encoding = strchr(lang,'.') )  !=  NULL  ){
			scr->encoding = StrDup(++encoding);
		} else {
			scr->encoding = NULL;
		}
	} else {
		scr->encoding = StrDup("euc-jp");
	}
LEAVE_FUNC;
	return	(scr); 
}
