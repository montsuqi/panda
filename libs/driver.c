/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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
#include	"misc.h"
#include	"comm.h"
#include	"DDparser.h"
#define		_DRIVER
#include	"driver.h"
#include	"dirs.h"
#include	"debug.h"

extern	WindowData	*
SetWindowName(
	char		*name)
{
	WindowData	*entry;

dbgmsg(">SetWindowName");
#ifdef	TRACE
	printf("SetWindowName(%s)\n",name);
#endif
	if		(  name  !=  NULL  ) {
		if		(  ThisScreen->Windows  ==  NULL  ) {
			ThisScreen->Windows = NewNameHash();
		}
		if		(  ( entry = 
					 (WindowData *)g_hash_table_lookup(ThisScreen->Windows,name) )
				   ==  NULL  ) {
			entry = New(WindowData);
			entry->name = StrDup(name);
			entry->PutType = SCREEN_NULL;
			entry->fNew = FALSE;
			entry->Value = ReadRecordDefine(name);
			g_hash_table_insert(ThisScreen->Windows,entry->name,entry);
		}
	} else {
		entry = NULL;
	}
dbgmsg("<SetWindowName");
	return	(entry);
}

extern	ValueStruct	*
GetWindowRecord(
	char		*wname)
{
	WindowData	*win;
	ValueStruct	*value;

dbgmsg(">GetWindowRecord");
	if		(  ( win = g_hash_table_lookup(ThisScreen->Windows,wname) )  !=  NULL  ) {
		value = win->Value;
	} else {
		value = NULL;
	}
dbgmsg("<GetWindowRecord");
	return	(value); 
}

extern	Bool
PutWindow(
	WindowData	*win,
	int			type)
{
	Bool		rc;

dbgmsg(">PutWindow");
	if		(  win  !=  NULL  ) {
		win->PutType = type;
		win->fNew = TRUE;
		ThisScreen->status = APL_SESSION_GET;
		rc = TRUE;
	} else {
		rc = FALSE;
	}
dbgmsg("<PutWindow");
	return	(rc);
}

extern	ValueStruct	*
GetWindowValue(
	char		*name)
{
	char	wname[SIZE_NAME+1];
	char	*p
	,		*q;
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
	
	val = GetWindowRecord(wname);
	if		(  val  !=  NULL  ) {
		val = GetItemLongName(val,p);
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
	int			type)
{
	WindowData	*win;

	win = SetWindowName(wname);
	PutWindow(win,type);
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

dbgmsg(">NewScreenData");
	scr = New(ScreenData);
	memclear(scr->window,SIZE_NAME+1);
	memclear(scr->widget,SIZE_NAME+1);
	memclear(scr->event,SIZE_EVENT+1);
	memclear(scr->cmd,SIZE_BUFF);
	memclear(scr->term,SIZE_TERM+1);
	memclear(scr->user,SIZE_USER+1);
	memclear(scr->other,SIZE_OTHER+1);
	scr->Windows = NULL;
dbgmsg("<NewScreenData");
	return	(scr); 
}
