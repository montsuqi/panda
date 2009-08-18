/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_KEYVALUEREQ_H
#define	_INC_KEYVALUEREQ_H

#include	"libmondai.h"
#include	"keyvaluecom.h"
#include	"net.h"

#define	KVREQ_TEXT_SIZE	256

extern	int			KVREQ_Request(NETFILE *fp, PacketClass c, ValueStruct *args);
extern	ValueStruct	*KVREQ_NewQuery(int num);
extern	ValueStruct	*KVREQ_NewQueryWithValue(char *id, int num, char **keys, char **values);
extern	int			KVREQ_GetNum(ValueStruct *value);
extern	char		*KVREQ_GetValue(ValueStruct *value,int num);
extern	char		*KVREQ_GetKey(ValueStruct *value,int num);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
