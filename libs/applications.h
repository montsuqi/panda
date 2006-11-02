/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_APPLICATION_H
#define	_INC_APPLICATION_H

#include	<glib.h>
#include	"glterm.h"
#include	"driver.h"

typedef	Bool	(*APL_FUNC)(char *);

typedef	struct {
	char	*name;
	APL_FUNC	main;
	APL_FUNC	link;
	APL_FUNC	exit;
}	ApplicationStruct;

extern	void		DestroyValues(GHashTable *value);
extern	void		ApplicationsCall(int sts, ScreenData *scr);
extern	void		ApplicationsInit(int argc, char **argv);
extern	APL_FUNC	ApplicationsLookup(char *name);
#endif
