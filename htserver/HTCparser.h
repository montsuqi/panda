/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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

#ifndef	_INC_HTC_PARSER_H
#define	_INC_HTC_PARSER_H
#include	<glib.h>
#include	"const.h"
#include	"libmondai.h"
#include	"cgi.h"

extern	HTCInfo		*NewHTCInfo(void);
extern	HTCInfo		*HTCParserCore(void);
extern	HTCInfo		*HTCParseFile(char *fname);
extern	HTCInfo		*HTCParseMemory(char *buff);
extern	void		DestroyHTC(HTCInfo *htc);
extern	void		HTC_Error(char *msg, ...);
extern	HTCInfo		*ParseScreen(char *name, Bool fComm, Bool fBody);

#endif
