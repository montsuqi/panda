/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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
PathWindowName(
	char	*comp,
	char	*buff)
{
	char	*p;

	strcpy(buff,comp);
	if		(  ( p = strchr(buff,'.') )  !=  NULL  ) {
		*p = 0;
	}
	return	(buff);
}

extern	char	*
PureWindowName(
	char	*comp,
	char	*buff)
{
	char	*p;

ENTER_FUNC;
	if		(  ( p = strrchr(comp,'/') )  ==  NULL  ) {
		p = comp;
	} else {
		p ++;
	}
	strcpy(buff,p);
	if		(  ( p = strchr(buff,'.') )  !=  NULL  ) {
		*p = 0;
	}
LEAVE_FUNC;
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

extern	RecordStruct	*
SetWindowRecord(
	char	*wname)
{
	RecordStruct	*rec;
	char		fname[SIZE_LONGNAME+1];

ENTER_FUNC;
	if		(  ( rec = (RecordStruct *)g_hash_table_lookup(ThisScreen->Records,wname) )
			   ==  NULL  ) {
		sprintf(fname,"%s.rec",wname);
		if		(  ( rec = ReadRecordDefine(fname) )  !=  NULL  ) {
			InitializeValue(rec->value);
			g_hash_table_insert(ThisScreen->Records,rec->name,rec);
		}
	}
LEAVE_FUNC;
	return	(rec);
}

extern	RecordStruct	*
GetWindowRecord(
	char		*name)
{
	RecordStruct	*rec;
	char	wname[SIZE_LONGNAME+1];

ENTER_FUNC;
	PureWindowName(name,wname);
	rec = (RecordStruct *)g_hash_table_lookup(ThisScreen->Records,wname);
LEAVE_FUNC;
	return	(rec); 
}

extern	WindowData	*
SetWindowName(
	char		*name)
{
	WindowData	*entry;
	RecordStruct	*rec;
	char		wname[SIZE_LONGNAME+1]
		,		path[SIZE_LONGNAME+1]
		,		msg[SIZE_BUFF];

ENTER_FUNC;
	dbgprintf("SetWindowName(%s)",name);
	if		(  name  !=  NULL  ) {
		PureWindowName(name,wname);
		if		(  ( rec = SetWindowRecord(wname) )  ==  NULL  ) {
			strcpy(path,name);	/*	exitting system	*/
			exit(1);
		} else {
			PathWindowName(name,path);
		}
		if		(  ( entry = 
					 (WindowData *)g_hash_table_lookup(ThisScreen->Windows,path) )
				   ==  NULL  ) {
			entry = New(WindowData);
			entry->PutType = SCREEN_NULL;
			entry->fNew = FALSE;
			entry->name = StrDup(path);
			entry->rec = rec;
			g_hash_table_insert(ThisScreen->Windows,entry->name,entry);
			if		(  rec  ==  NULL  ) {
				entry->PutType = SCREEN_END_SESSION;
				sprintf(msg,"window not found [%s:%s:%s]\n", name, wname, path);
				MessageLog(msg);
			}
		}
	} else {
		entry = NULL;
	}
LEAVE_FUNC;
	return	(entry);
}

extern	void
RemoveWindowRecord(
	char		*name)
{
	WindowData	*win;
	RecordStruct	*rec;
	char	path[SIZE_LONGNAME+1];
	char	wname[SIZE_LONGNAME+1];

ENTER_FUNC;
	if		(  ThisScreen->Windows  !=  NULL  ) {
		PureWindowName(name,wname);
		PathWindowName(name,path);
		if		(  ( win = g_hash_table_lookup(ThisScreen->Windows,path) )  !=  NULL  ) {
			g_hash_table_remove(ThisScreen->Windows,path);
			g_hash_table_remove(ThisScreen->Records,wname);
			xfree(win->name);
			rec = win->rec;
			xfree(rec->name);
			FreeValueStruct(rec->value);
			xfree(rec);
			xfree(win);
		}
	}
LEAVE_FUNC;
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
	char	wname[SIZE_LONGNAME+1];
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
dbgmsg("*");
	memclear(scr->window,SIZE_NAME+1);
	memclear(scr->widget,SIZE_NAME+1);
	memclear(scr->event,SIZE_EVENT+1);
	memclear(scr->cmd,SIZE_LONGNAME+1);
	memclear(scr->term,SIZE_TERM+1);
	memclear(scr->user,SIZE_USER+1);
	memclear(scr->other,SIZE_OTHER+1);
	memclear(scr->encoding,SIZE_ENCODING+1);
	scr->Windows = NewNameHash();
	scr->Records = NewNameHash();
dbgmsg("*");
	if		(  libmondai_i18n  ) {
		if		(  ( lang = getenv("LANG") )  !=  NULL  &&
				   ( encoding = strchr(lang,'.') )  !=  NULL  ){
			strcpy(scr->encoding,++encoding);
		} else {
			scr->encoding[0] = NULL;
		}
	} else {
		strcpy(scr->encoding,"euc-jp");
	}
LEAVE_FUNC;
	return	(scr); 
}

#define	RECORD_WINDOW_NAME	1
#define	RECORD_RECORD		2

static	void
_SaveWindowName(
	char	*name,
	WindowData	*win,
	FILE	*fp)
{
	size_t	size;

	fputc(RECORD_WINDOW_NAME,fp);
	size = strlen(name) + 1;
	fwrite(&size,sizeof(size_t),1,fp);
	fwrite(name,size,1,fp);
	fwrite(&win->PutType,sizeof(byte),1,fp);
}

static	void
_SaveRecords(
	char			*name,
	RecordStruct	*rec,
	FILE			*fp)
{
	LargeByteString	*buff;
	size_t	size;
	char 	sum = 0;
	char	*buff2;

	fputc(RECORD_RECORD,fp);

	size = strlen(name) + 1;
	fwrite(&size,sizeof(size_t),1,fp);
	fwrite(name,size,1,fp);

	size = NativeSizeValue(NULL,rec->value);
	buff = NewLBS();
	LBS_ReserveSize(buff,size,FALSE);
	NativePackValue(NULL,LBS_Body(buff),rec->value);
	fwrite(&size,sizeof(size_t),1,fp);
	fwrite(LBS_Body(buff),size,1,fp);
	FreeLBS(buff);
}

extern	void
_SaveScreenData(
	char		*path,
	ScreenData	*scr,
	Bool		fSaveRecords)
{
	FILE	*fp;
	if		(  ( fp = Fopen(path,"w") )  !=  NULL  ) {
		fwrite(&fSaveRecords,sizeof(Bool),1,fp);
		fwrite(scr->window,SIZE_NAME+1,1,fp);
		fwrite(scr->widget,SIZE_NAME+1,1,fp);
		fwrite(scr->event,SIZE_EVENT+1,1,fp);
		fwrite(scr->cmd,SIZE_LONGNAME+1,1,fp);
		fwrite(scr->user,SIZE_USER+1,1,fp);
		fwrite(scr->term,SIZE_TERM+1,1,fp);
		fwrite(scr->other,SIZE_OTHER+1,1,fp);
		fwrite(scr->encoding,SIZE_ENCODING+1,1,fp);
		fwrite(&scr->status, sizeof(int), 1, fp);
		if		(  fSaveRecords  ) {
			g_hash_table_foreach(scr->Records,(GHFunc)_SaveRecords,fp);
			g_hash_table_foreach(scr->Windows,(GHFunc)_SaveWindowName,fp);
		}
		fclose(fp);
	}
}

extern	void
SaveScreenData(
	ScreenData	*scr,
	Bool		fSaveRecords)
{
	char	path[SIZE_LONGNAME+1];

	sprintf(path,"%s/%s.d",SesDir,scr->term);
	_SaveScreenData(path, scr, fSaveRecords);
}

extern	void
PargeScreenData(
	ScreenData	*scr)
{
	char	fname[SIZE_LONGNAME+1];

	sprintf(fname,"%s/%s.d",SesDir,scr->term);
	remove(fname);
}

static	void
LoadRecords(
	ScreenData	*scr,
	FILE		*fp)
{
	size_t	size;
	int		flag;
	char	name[SIZE_LONGNAME+1];
	RecordStruct	*rec;
	byte	*buff;

	while	(  ( flag = fgetc(fp) )  ==  RECORD_RECORD  ) {
		fread(&size,sizeof(size_t),1,fp);
		fread(name,size,1,fp);
		rec  = SetWindowRecord(name);
		fread(&size,sizeof(size_t),1,fp);
		buff = (byte *)xmalloc(size);
		fread(buff,size,1,fp);
		NativeUnPackValue(NULL,buff,rec->value);
		xfree(buff);
	}
	ungetc(flag,fp);
}

static	void
LoadWindows(
	ScreenData	*scr,
	FILE		*fp)
{
	size_t	size;
	char	name[SIZE_LONGNAME+1];
	int		flag;
	byte	PutType;
	WindowData	*win;

	while	(  ( flag = fgetc(fp) )  ==  RECORD_WINDOW_NAME  ) {
		fread(&size,sizeof(size_t),1,fp);
		fread(name,size,1,fp);
		fread(&PutType,sizeof(byte),1,fp);
		win = SetWindowName(name);
		win->PutType = PutType;
	}
	ungetc(flag,fp);
}

extern	ScreenData	*
_LoadScreenData(
	char	*path,
	char	*term)
{
	FILE		*fp;
	ScreenData	*scr;
	Bool		fSaveRecords;

ENTER_FUNC;
	scr = NULL;
	if		(  ( fp = fopen(path,"r") )  !=  NULL  ) {
		scr = NewScreenData();
		ThisScreen = scr;

		fread(&fSaveRecords,sizeof(Bool),1,fp);
		fread(scr->window,SIZE_NAME+1,1,fp);
		fread(scr->widget,SIZE_NAME+1,1,fp);
		fread(scr->event,SIZE_EVENT+1,1,fp);
		fread(scr->cmd,SIZE_LONGNAME+1,1,fp);
		fread(scr->user,SIZE_USER+1,1,fp);
		fread(scr->term,SIZE_TERM+1,1,fp);
		fread(scr->other,SIZE_OTHER+1,1,fp);
		fread(scr->encoding,SIZE_ENCODING+1,1,fp);
		fread(&scr->status,sizeof(int),1,fp);
		if		(  fSaveRecords  ) {
			LoadRecords(scr,fp);
			LoadWindows(scr,fp);
		}
		strcpy(scr->term,term);
		fclose(fp);
	}
LEAVE_FUNC;
	return	(scr);
}

extern	ScreenData	*
LoadScreenData(
	char	*term)
{
	char	path[SIZE_LONGNAME+1];

	sprintf(path,"%s/%s.d",SesDir,term);
	return	_LoadScreenData(path, term);
}


static	guint
FreeWindows(
	char	*name,
	WindowData	*entry,
	void		*dummy)
{
	xfree(entry->name);
	return	(TRUE);
}

static	guint
FreeRecords(
	char	*name,
	RecordStruct	*rec,
	void		*dummy)
{
	xfree(rec->name);
	FreeValueStruct(rec->value);
	return	(TRUE);
}

extern	void
FreeScreenData(
	ScreenData	*scr)
{

ENTER_FUNC;
	if		(  scr->Windows  !=  NULL  ) {
		g_hash_table_foreach_remove(scr->Windows,(GHRFunc)FreeWindows,NULL);
		g_hash_table_destroy(scr->Windows);
	}
	if		(  scr->Records  !=  NULL  ) {
		g_hash_table_foreach_remove(scr->Records,(GHRFunc)FreeRecords,NULL);
		g_hash_table_destroy(scr->Records);
	}
	xfree(scr);
LEAVE_FUNC;
}
