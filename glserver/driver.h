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
#include	"wfcio.h"

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
