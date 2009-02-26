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

#ifndef	_INC_DRIVER_H
#define	_INC_DRIVER_H

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"
#include	"struct.h"
#include	"glterm.h"

typedef	struct {
	char		window[SIZE_NAME+1];
	char		widget[SIZE_NAME+1];
	char		event[SIZE_EVENT+1];
	char		cmd[SIZE_LONGNAME+1];
	char		user[SIZE_USER+1];
	char		term[SIZE_TERM+1];
	char		other[SIZE_OTHER+1];
	int			status;
	GHashTable	*Windows;			/*	for WindowData		*/
	GHashTable	*Records;			/*	for	RecordStruct	*/
	char		lang[SIZE_NAME+1];
	char		*encoding;
}	ScreenData;

typedef	struct {
	Bool			fNew;
	byte			PutType;
	char			*name;
	RecordStruct	*rec;
}	WindowData;

extern	WindowData		*SetWindowName(char *name);
extern	Bool			PutWindow(WindowData *win, byte type);
extern	RecordStruct	*GetWindowRecord(char *wname);
extern	RecordStruct	*SetWindowRecord(char *wname);
extern	void			LinkModule(char *name);
extern	ScreenData		*NewScreenData(void);
extern	char			*PureWindowName(char *comp, char *buff);
extern	char			*PureComponentName(char *comp, char *buff);
extern	void			RemoveWindowRecord(char *name);
extern	void			SaveScreenData(ScreenData *scr, Bool fSaveRecords);
extern	ScreenData		*LoadScreenData(char *term);
extern	void			FreeScreenData(ScreenData *scr);
extern	void			PargeScreenData(ScreenData *scr);
extern	void			DumpScreenData(ScreenData *scr);

/*	C API	*/
extern	ValueStruct	*GetWindowValue(char *name);
extern	WindowData	*PutWindowByName(char *wname, byte type);

#define	ThisWindow	(ThisScreen->window)
#define	ThisWidget	(ThisScreen->widget)
#define	ThisEvent	(ThisScreen->event)
#define	ThisUser	(ThisScreen->user)
#define	ThisTerm	(ThisScreen->term)
#define	ThisLang	(ThisScreen->lang)

#undef	GLOBAL
#ifdef	_DRIVER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
GLOBAL	ScreenData	*ThisScreen;	/*	current applications ScreenData		*/
#undef	GLOBAL
#endif
