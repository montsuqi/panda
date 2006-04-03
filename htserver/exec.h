/*
PANDA -- a simple transaction monitor
Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef	_INC_EXEC_H
#define	_INC_EXEC_H
#include	"LBSfunc.h"

#define	OPC_REF			0x01
#define	OPC_VAR			0x02
#define	OPC_ICONST		0x03
#define	OPC_HIVAR		0x04
#define	OPC_REFINT		0x05
#define	OPC_LEND		0x06
#define	OPC_STORE		0x07
#define	OPC_LDVAR		0x08
#define	OPC_SCONST		0x09
#define	OPC_EVAL		0x0A
#define	OPC_PHSTR		0x0B
#define	OPC_PHINT		0x0E
#define	OPC_PHBOOL		0x0F
#define	OPC_BREAK		0x10
#define	OPC_HBES		0x11
#define	OPC_JNZP		0x12
#define	OPC_JNZNP		0x13
#define	OPC_DROP		0x14
#define	OPC_LOCURI		0x15
#define	OPC_UTF8URI		0x16
#define	OPC_EMITSTR		0x17
#define	OPC_ESCJSS		0x18
#define	OPC_CALENDAR	0x19
#define	OPC_TOINT		0x1A
#define	OPC_SUB			0x1B
#define	OPC_SPY			0x1C
#define	OPC_SCMP		0x1D
#define	OPC_EMITRAW		0x1E

#define	OPC_FLJS		0x7F

#define	VAR_NULL	0
#define	VAR_INT		1
#define	VAR_STR		2
#define	VAR_PTR		3

#define	EXPR_NONE			0

#define	EXPR_SYMBOL			1
#define	EXPR_VALUE			2
#define	EXPR_FUNC			6

#define	EXPR_NEG			10
#define	EXPR_ADD			11
#define	EXPR_SUB			12
#define	EXPR_MUL			13
#define	EXPR_DIV			14
#define	EXPR_MOD			15
#define	EXPR_CAT			16

#define	EXPR_SEQ			30
#define	EXPR_FUNCALL		31
#define	EXPR_VREF			32
#define	EXPR_ITEM			33

typedef	struct	_Expr	{
	int		type;
	union {
		char	*name;
		int		ival;
		char	*sval;
		struct	_Expr	*(*func)(void *args);
		struct	_Expr		*ptr;
		struct {
			struct	_Expr	*left
			,				*right;
		}	cons;
	}	body;
}	Expr;

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#undef	GLOBAL

extern	void	ExecCode(LargeByteString *html, HTCInfo *htc);
extern	char	*ParseInput(HTCInfo *htc);

#endif
