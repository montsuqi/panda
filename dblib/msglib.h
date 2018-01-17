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

#ifndef	_MSGLIB_H
#define	_MSGLIB_H

#define CAST_BAD(arg)	(char*)(arg)

#define MSG_NONE (0)
#define MSG_XML  (1)
#define MSG_JSON (2)

int		XMLNode2Value(ValueStruct *val,xmlNodePtr root);
xmlNodePtr	Value2XMLNode(char *name,ValueStruct *val);
int CheckFormat(char *buff,size_t size);


#endif
