/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_INC_SQL_PARSER_H
#define	_INC_SQL_PARSER_H

#define	SIZE_SQL		65538

#include	"libmondai.h"
#include	"struct.h"

#define	SQL_OP_ESC		0x00
#define	SQL_OP_INTO		0x01
#define	SQL_OP_REF		0x02
#define	SQL_OP_STO		0x03
#define	SQL_OP_ASTER	0x04
#define	SQL_OP_EOL		0x05
#define	SQL_OP_VCHAR	0x06

extern	LargeByteString	*ParSQL(RecordStruct *rec);
extern	void			SQL_ParserInit(void);

#endif
