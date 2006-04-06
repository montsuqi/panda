/*
 * PANDA -- a simple transaction monitor
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

#ifndef	_INC_HTC_H
#define	_INC_HTC_H

#include	"libmondai.h"

typedef	struct {
	LargeByteString	*code;
	GHashTable	*Trans;
	GHashTable	*Radio;
	GHashTable	*FileSelection;
    char *DefaultEvent;
    size_t EnctypePos;
    int FormNo;
}	HTCInfo;

typedef	struct {
    char *filename;
    char *filesize;
}	FileSelectionInfo;

#ifndef	_INC_VALUE_H
#define	ValueStruct	void
#endif

typedef	ValueStruct	*(*GET_VALUE)(char *name, Bool fClear);

extern	char	*GetHostValue(char *name, Bool fClear);
extern	void	InitHTC(char *script_name, GET_VALUE func);

#include	"tags.h"
#include	"HTCparser.h"
#include	"exec.h"

#endif
