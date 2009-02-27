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

#ifndef	_INC_MONAPI_H
#define	_INC_MONAPI_H

#include	"const.h"
#include	"enum.h"
#include	"libmondai.h"

typedef	struct {
	char			ld[SIZE_NAME+1];
	char			user[SIZE_USER+1];
	char			term[SIZE_TERM+1];
	PacketClass		method;
	LargeByteString	*arguments;
	LargeByteString	*headers;
	LargeByteString	*body;
}	MonAPIData;

extern	MonAPIData	*NewMonAPIData(void);
extern	void		FreeMonAPIData(MonAPIData *data);
extern	gboolean	CallMonAPI(MonAPIData *data);

#undef	GLOBAL
#ifdef	_MONAPI
#define	GLOBAL	/*	*/
#else
#define	GLOBAL	extern
#endif
/* GLOBAL define here*/
#undef	GLOBAL
#endif
