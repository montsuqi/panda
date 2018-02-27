/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_MONPUSHEVENT_H
#define	_MONPUSHEVENT_H

#include	"dblib.h"
#define 	MONPUSHEVENT		"monpushevent"
#define 	MONPUSHEVENT_EXPIRE "7"
#define 	SEQMONPUSHEVENT 	"seqmonpushevent"

extern int new_monpushevent_id(DBG_Struct *dbg);
extern Bool	monpushevent_setup(DBG_Struct *dbg);

gboolean push_event_via_value(DBG_Struct *dbg,ValueStruct *val);
gboolean push_event_via_json(DBG_Struct *dbg,const char *event,json_object*);
#endif
