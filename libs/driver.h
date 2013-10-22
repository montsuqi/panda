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

#define	SCREEN_DATA_NULL		0
#define	SCREEN_DATA_CONNECT		1
#define	SCREEN_DATA_END			2

typedef	struct {
	char			window[SIZE_NAME+1];
	char			widget[SIZE_NAME+1];
	char			event[SIZE_EVENT+1];
	char			cmd[SIZE_LONGNAME+1];
	char			user[SIZE_USER+1];
	char			term[SIZE_TERM+1];
	char			host[SIZE_HOST+1];
	char			agent[SIZE_TERM+1];
	int				status;
	unsigned char	puttype;
	WindowStack		w;
	GHashTable		*Windows;
}	ScreenData;

typedef	struct {
	unsigned char	puttype;
	RecordStruct	*rec;
}	WindowData;

extern	ScreenData	*NewScreenData(void);
extern	void		FreeScreenData(ScreenData *scr);
extern	WindowData	*RegisterWindow(ScreenData *scr,const char *name);
extern	ValueStruct	*GetWindowValue(ScreenData *scr,const char *name);
extern	void		PutWindow(ScreenData *scr,const char *wname, unsigned char type);

#undef	GLOBAL
#ifdef	_DRIVER
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
#undef	GLOBAL
#endif
