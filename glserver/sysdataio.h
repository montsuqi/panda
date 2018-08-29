/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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

#ifndef _SYSDATAIO_H
#define _SYSDATAIO_H

extern void GLExportBLOB(MonObjectType obj, char **, size_t *);
extern MonObjectType GLImportBLOB(char *, size_t);
extern void GetSessionMessage(const char *, char **, char **, char **);
extern void ResetSessionMessage(const char *);
extern gboolean CheckSession(const char *term);

#endif
