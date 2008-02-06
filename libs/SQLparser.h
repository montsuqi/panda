/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_SQL_PARSER_H
#define	_INC_SQL_PARSER_H

#define	SIZE_SQL		65538

#include	"libmondai.h"
#include	"Lex.h"
#include	"struct.h"

#define	SQL_OP_ESC		0x00
#define	SQL_OP_INTO		0x01
#define	SQL_OP_REF		0x02
#define	SQL_OP_STO		0x03
#define	SQL_OP_ASTER	0x04
#define	SQL_OP_EOL		0x05
#define	SQL_OP_VCHAR	0x06
#define	SQL_OP_SYMBOL	0x07
#define	SQL_OP_LIMIT	0x08

extern	LargeByteString	*ParSQL(CURFILE *in,RecordStruct *rec, ValueStruct *argp,
								ValueStruct *argf);
extern	void			SQL_ParserInit(void);

#endif
