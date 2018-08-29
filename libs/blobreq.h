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

#ifndef _INC_BLOBREQ_H
#define _INC_BLOBREQ_H

#include "libmondai.h"
#include "net.h"
#include "sysdatacom.h"

extern Bool RequestExportBLOB(NETFILE *fp, MonObjectType obj, char *fname);
extern Bool RequestExportBLOBMem(NETFILE *fp, MonObjectType obj, char **,
                                 size_t *);
extern MonObjectType RequestImportBLOB(NETFILE *fp, const char *fname);
extern MonObjectType RequestImportBLOBMem(NETFILE *fp, char *, size_t);

#undef GLOBAL
#ifdef MAIN
#define GLOBAL /*	*/
#else
#define GLOBAL extern
#endif

#endif
