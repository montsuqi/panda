/*
 * PANDA -- a simple transaction monitor
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

#ifndef	_INC_BLOB_H
#define	_INC_BLOB_H
#include	"net.h"

//#define	BLOB_VERSION	2
#define	BLOB_VERSION	1

#define	SIZE_BLOB_HEADER	4

#if	BLOB_VERSION == 1
#include	"blob_v1.h"
#endif
#if	BLOB_VERSION == 2
#include	"blob_v2.h"
#endif

extern	int			VersionBLOB(char *space);

#endif
