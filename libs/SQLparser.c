/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#define		_SQL_PARSER
#include	"libmondai.h"
#include	"Lex.h"
#include	"SQLlex.h"
#include	"SQLparser.h"
#include	"debug.h"

#define	MarkCode(lbs)		((lbs)->ptr)
#define	SeekCode(lbs,pos)	((lbs)->ptr = (pos))

#undef	GetSymbol
#define	GetSymbol	(ComToken = SQL_Lex(in,FALSE))
#undef	GetName
#define	GetName		(ComToken = SQL_Lex(in,TRUE))

static	ValueStruct	*
TraceAlias(
	RecordStruct	*rec,
	ValueStruct		*val)
{
	char		*name;
	char		*p;
	RecordStruct	*use;

ENTER_FUNC;
	if		(  val  !=  NULL  ) {
		if		(  ValueType(val)  ==  GL_TYPE_ALIAS  ) {
			name = ValueAliasName(val);
			if		(  *name  !=  '.'  ) {
				if		(  ( p = strchr(name,'.') )  !=  NULL  ) {
					*p = 0;
					if		(  ( use = g_hash_table_lookup(RecordDB(rec)->use,name) )
							   !=  NULL  ) {
						val = GetItemLongName(use->value,p+1);
					} else {
						printf("alias table not found [%s]\n",name);
					}
					*p = '.';
				}
			}
		}
	}
LEAVE_FUNC;
	return	(val);
}

static	ValueStruct	*
TraceArray(
	ValueStruct	*val,
	int			n)
{
ENTER_FUNC;
	if		(  val  !=  NULL  ) {
		if		(  ValueType(val)  ==  GL_TYPE_ARRAY  ) {
			val = GetArrayItem(val,n);
		}
	}
LEAVE_FUNC;
	return	(val);
}

extern	LargeByteString	*
ParSQL(
	CURFILE			*in,
	RecordStruct	*rec,
	ValueStruct		*argp,
	ValueStruct		*argf)
{
	LargeByteString	*sql;
	ValueStruct		*valr
		,			*valp
		,			*valf
		,			*val;
	Bool	fInto
		,	fAster
		,	fDeclare;
	size_t	mark
		,	current;
	int		into;
	int		n;
	int		pred;
#define	PRED_NULL			0
#define	PRED_INSERT			1
#define	PRED_SELECT			2
#define	PRED_SELECT_CURSOR	3

ENTER_FUNC;
	sql = NewLBS();
	GetSymbol;
	fInto = FALSE;
	mark = 0;
	into = 0;
	fAster = FALSE;
	fDeclare = FALSE;
	pred = PRED_NULL;
	while	( ComToken != '}' ) {
		switch	(ComToken) {
		  case	T_SYMBOL:
			LBS_Emit(sql,SQL_OP_ESC);
			LBS_Emit(sql,SQL_OP_SYMBOL);
			do {
				LBS_EmitString(sql,ComSymbol);
				if		(  GetSymbol  ==  '.'  ) {
					LBS_EmitChar(sql,'.');
					GetSymbol;
				} else
					break;
			}	while	(  ComToken  ==  T_SYMBOL  );
			LBS_EmitSpace(sql);
			break;
		  case	T_SCONST:
			LBS_EmitChar(sql,'\'');
			LBS_EmitString(sql,ComSymbol);
			LBS_EmitChar(sql,'\'');
			LBS_EmitSpace(sql);
			GetSymbol;
			break;
		  case	T_SQL:
			LBS_EmitString(sql,ComSymbol);
			LBS_EmitSpace(sql);
			GetSymbol;
			break;
		  case	T_LIKE:
		  case	T_ILIKE:
			LBS_EmitString(sql,ComSymbol);
			LBS_EmitSpace(sql);
			LBS_Emit(sql,SQL_OP_ESC);
			LBS_Emit(sql,SQL_OP_VCHAR);
			GetSymbol;
			break;
		  case	T_INSERT:
			LBS_EmitString(sql,ComSymbol);
			LBS_EmitSpace(sql);
			pred = PRED_INSERT;
			GetSymbol;
			break;
		  case	T_DECLARE:
			LBS_EmitString(sql,"DECLARE ");
			fDeclare = TRUE;
			GetSymbol;
			break;
		  case	T_WHERE:
			LBS_EmitString(sql,"WHERE ");
			fInto = FALSE;
			GetSymbol;
			break;
		  case	T_SELECT:
			LBS_EmitString(sql,"SELECT ");
			if		(  fDeclare  ) {
				pred = PRED_SELECT_CURSOR;
			} else {
				pred = PRED_SELECT;
			}
			GetSymbol;
			break;
		  case	T_INTO:
			switch	(pred) {
			  case	PRED_INSERT:
				LBS_EmitString(sql,"INTO ");
				break;
			  default:
				LBS_Emit(sql,SQL_OP_ESC);
				LBS_Emit(sql,SQL_OP_INTO);
				mark = MarkCode(sql);
				into = 0;
				LBS_EmitInt(sql,0);
				fInto = TRUE;
				break;
			}
			GetSymbol;
			break;
		  case	':':
			if		(  GetName  ==  T_SYMBOL  ) {
				if		(  strcmp(ComSymbol,"$limit")  ==  0  ) {
					LBS_Emit(sql,SQL_OP_ESC);
					LBS_Emit(sql,SQL_OP_LIMIT);
					GetName;
				} else {
					if		(  fInto  ) {
						LBS_Emit(sql,SQL_OP_ESC);
						LBS_Emit(sql,SQL_OP_STO);
						into ++;
					} else {
						LBS_Emit(sql,SQL_OP_ESC);
						LBS_Emit(sql,SQL_OP_REF);
					}
					GString *str = g_string_new("");
					valr = rec->value;
					valp = argp;
					valf = argf;
					do {
						valr = GetRecordItem(TraceAlias(rec,valr),ComSymbol);
						valp = GetRecordItem(TraceAlias(rec,valp),ComSymbol);
						valf = GetRecordItem(TraceAlias(rec,valf),ComSymbol);
						str = g_string_append(str, ComSymbol);
						switch	(GetSymbol) {
						  case	'.':
							GetName;
							str = g_string_append_c(str, '.');
							break;
						  case	'[':
							if (GetSymbol != T_SYMBOL) {
								ParError("parse error missing symbol after `['");
							}
							str = g_string_append_c(str, '[');
							str = g_string_append(str, ComSymbol);
							str = g_string_append_c(str, ']');
							n = atoi(ComSymbol) - 1;
							if (n < 0) {
								ParErrorPrintf("invalid array index: %s\n", ComSymbol);
							}
							if		(  GetSymbol  !=  ']'  ) {
								ParError("parse error missing symbol: `]'");
							}
							GetSymbol;
							valr = TraceArray(valr,n);
							valp = TraceArray(valp,n);
							valf = TraceArray(valf,n);
							break;
						  default:
							break;
						}
						ERROR_BREAK;
					}	while	(  ComToken  ==  T_SYMBOL  );
					val = (  valp  !=  NULL  ) ? valp : valr;
					val = (  valf  !=  NULL  ) ? valf : val;
					if		(  val  ==  NULL  ) {
						ParErrorPrintf("undeclared (first in use this file): `%s'\n", str->str);
					}
					LBS_EmitPointer(sql,(void *)TraceAlias(rec,val));
					g_string_free(str, TRUE);
				}
				if		(  ComToken  ==  ','  ) {
					if		(  !fInto  ) {
						LBS_EmitChar(sql,',');
					}
					GetSymbol;
				} else {
					LBS_EmitSpace(sql);
				}
				fAster = FALSE;
			} else
			if		(  ComToken  ==  '*'  ) {
				fAster = TRUE;
				GetSymbol;
			}
			break;
		  case	';':
			if		(  mark  >  0  ) {
				current = MarkCode(sql);
				SeekCode(sql,mark);
				if		(  fAster  ) {
					LBS_EmitInt(sql,-1);
				} else {
					LBS_EmitInt(sql,into);
				}
				SeekCode(sql,current);
			}
			LBS_Emit(sql,SQL_OP_ESC);
			LBS_Emit(sql,SQL_OP_EOL);
			GetSymbol;
			fInto = FALSE;
			pred = PRED_NULL;
			fDeclare = FALSE;
			break;
		  case	'*':
			switch	(pred) {
			  case	PRED_SELECT:
				LBS_Emit(sql,SQL_OP_ESC);
				LBS_Emit(sql,SQL_OP_INTO);
				mark = MarkCode(sql);
				into = 0;
				LBS_EmitInt(sql,0);
				fInto = TRUE;
				break;
			  default:
				break;
			}
			LBS_EmitChar(sql,'*');
			LBS_EmitSpace(sql);
			GetSymbol;
			break;
		  case	'~':
			LBS_EmitChar(sql,'~');
			LBS_EmitSpace(sql);
			LBS_Emit(sql,SQL_OP_ESC);
			LBS_Emit(sql,SQL_OP_VCHAR);
			GetSymbol;
			break;
		  default:
			LBS_EmitChar(sql,(char)ComToken);
			GetSymbol;
			break;
		}
		ERROR_BREAK;
	}
    
    if (sql->body[sql->ptr - 1] != SQL_OP_EOL) {
        if (fInto) {
            current = MarkCode(sql);
            SeekCode(sql,mark);
            if (fAster) {
                LBS_EmitInt(sql,-1);
            }
            else {
                LBS_EmitInt(sql,into);
            }
            SeekCode(sql,current);
        }
        LBS_Emit(sql,SQL_OP_ESC);
        LBS_Emit(sql,SQL_OP_EOL);
    }
	LBS_EmitEnd(sql);
	LBS_EmitFix(sql);
LEAVE_FUNC;
	return	(sql);
}

extern	void
SQL_ParserInit(void)
{
	SQL_LexInit();
}

