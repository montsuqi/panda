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
#include	"types.h"
#include	"libmondai.h"
#include	"Lex.h"
#include	"SQLlex.h"
#include	"SQLparser.h"
#include	"debug.h"

#define	MarkCode(lbs)		((lbs)->ptr)
#define	SeekCode(lbs,pos)	((lbs)->ptr = (pos))

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
	exit(1);
}
#undef	Error
#define	Error(msg)	{CURR->fError=TRUE;_Error((msg),CURR->fn,CURR->cLine);}
#undef	GetSymbol
#define	GetSymbol	(ComToken = SQL_Lex(FALSE))
#undef	GetName
#define	GetName		(ComToken = SQL_Lex(TRUE))

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
					if		(  ( use = g_hash_table_lookup(rec->opt.db->use,name) )
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
	,		fAster;
	size_t	mark
	,		current;
	int		into;
	int		n;
	Bool	fInsert;

ENTER_FUNC;
	sql = NewLBS();
	GetSymbol;
	fInto = FALSE;
	mark = 0;
	into = 0;
	fAster = FALSE;
	fInsert = FALSE;
	while	( ComToken != '}' ) {
		switch	(ComToken) {
		  case	T_SYMBOL:
			if		(  ( val = GetRecordItem(rec->value,ComSymbol) )  !=  NULL  ) {
				do {
					LBS_EmitString(sql,ComSymbol);
					if		(  GetSymbol  ==  '.'  ) {
						LBS_EmitChar(sql,'_');
						GetSymbol;
					}
				}	while	(  ComToken  ==  T_SYMBOL  );
			} else {
				LBS_EmitString(sql,ComSymbol);
				GetSymbol;
			}
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
			fInsert = TRUE;
			GetSymbol;
			break;
		  case	T_INTO:
			if		(  !fInsert  ) {
				LBS_Emit(sql,SQL_OP_ESC);
				LBS_Emit(sql,SQL_OP_INTO);
				mark = MarkCode(sql);
				into = 0;
				LBS_EmitInt(sql,0);
				fInto = TRUE;
			} else {
				LBS_EmitString(sql,"INTO ");
			}
			GetSymbol;
			break;
		  case	':':
			if		(  fInto  ) {
				LBS_Emit(sql,SQL_OP_ESC);
				LBS_Emit(sql,SQL_OP_STO);
				into ++;
			} else {
				LBS_Emit(sql,SQL_OP_ESC);
				LBS_Emit(sql,SQL_OP_REF);
			}
			if		(  GetName  ==  T_SYMBOL  ) {
				valr = rec->value;
				valp = argp;
				valf = argf;
				do {
					valr = GetRecordItem(TraceAlias(rec,valr),ComSymbol);
					valp = GetRecordItem(TraceAlias(rec,valp),ComSymbol);
					valf = GetRecordItem(TraceAlias(rec,valf),ComSymbol);
					switch	(GetSymbol) {
					  case	'.':
						GetName;
						break;
					  case	'[':
						if		(  GetSymbol  ==  T_SYMBOL  ) {
							n = atoi(ComSymbol) - 1;
							if		(  GetSymbol  !=  ']'  ) {
								Error("] missing");
							}
						} else {
							n = 0;
						}
						GetSymbol;
						valr = TraceArray(valr,n);
						valp = TraceArray(valp,n);
						valf = TraceArray(valf,n);
						break;
					  default:
						break;
					}
				}	while	(  ComToken  ==  T_SYMBOL  );
				val = (  valp  !=  NULL  ) ? valp : valr;
				val = (  valf  !=  NULL  ) ? valf : val;
				if		(  val  ==  NULL  ) {
					Error("invalid value(item name missing)");
				}
				LBS_EmitPointer(sql,(void *)TraceAlias(rec,val));
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
			if		(  fInto  ) {
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
			fInsert = FALSE;
			break;
		  case	'*':
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

