/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).

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
#ifndef	_INC_EXEC_H
#define	_INC_EXEC_H
#include	"LBSfunc.h"

#define	OPC_REF		0x01
#define	OPC_VAR		0x02
#define	OPC_ICONST	0x03
#define	OPC_HIVAR	0x04
#define	OPC_REFINT	0x05
#define	OPC_LEND	0x06
#define	OPC_STORE	0x07
#define	OPC_LDVAR	0x08
#define	OPC_SCONST	0x09
#define	OPC_NAME	0x0A
#define	OPC_HSNAME	0x0B
#define	OPC_EHSNAME	0x0C
#define	OPC_REFSTR	0x0D
#define	OPC_HINAME	0x0E
#define	OPC_HBNAME	0x0F
#define	OPC_BREAK	0x10
#define	OPC_HBES	0x11
#define	OPC_JNZP	0x12
#define	OPC_JNZNP	0x13
#define	OPC_DROP	0x14
#define	OPC_LOCURI	0x15
#define	OPC_UTF8URI	0x16
#define	OPC_EMITSTR	0x17
#define	OPC_ESCJSS	0x18
#define	OPC_CALENDAR	0x19

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#undef	GLOBAL

extern	void	ExecCode(LargeByteString *html, HTCInfo *htc);
//extern	char	*LBS_EmitUTF8(LargeByteString *lbs, char *str, char *codeset);
extern	char	*ParseInput(HTCInfo *htc);

#endif
