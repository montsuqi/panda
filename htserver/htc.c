/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan.

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
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<glib.h>
#include	<time.h>
#include	<sys/time.h>
#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"Lex.h"
#include	"cgi.h"
extern	void	HTCLexInit(void);
#include	"htc.h"
#include	"debug.h"

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

static	GHashTable			*Reserved;
static	GET_VALUE	_GetValue;
static	Bool		fClear;

static	TokenTable	tokentable[] = {
	{	""				,0			}
};

typedef	struct	_Expr	{
	int		type;
	union {
		char	*name;
		char	*sval;
		struct	_Expr	*(*func)(void *args);
		struct {
			struct	_Expr	*left
			,				*right;
		}	cons;
	}	value;
}	Expr;

static	Expr	*ParSequence(CURFILE *in);
static	Expr	*ParExpr(CURFILE *in);

static	Expr	*
NewCons(
	int		type,
	Expr	*left,
	Expr	*right)
{
	Expr	*expr;

	expr = New(Expr);
	expr->type = type;
	expr->value.cons.left = left;
	expr->value.cons.right = right;
	return	(expr);
}

static	Expr	*
ParFactor(
	CURFILE			*in)
{
	Expr	*expr
		,	*arg
		,	**exprw;

ENTER_FUNC;
	switch	(ComToken) {
	  case	T_SYMBOL:
		exprw = &expr;
		do {
			switch	(ComToken) {
			  case	T_SYMBOL:
				arg = New(Expr);
				arg->type = EXPR_SYMBOL;
				arg->value.name = StrDup(ComSymbol);
				GetName;
				break;
			  case	'[':
				GetSymbol;
				arg = ParExpr(in);
				if		(  ComToken  ==  ']'  ) {
					GetName;
				} else {
					printf("[%s]\n",ComSymbol);
					Error("factor invalid symbol");
				}
				break;
			  default:
				break;
			}
			*exprw = NewCons(EXPR_ITEM,arg,NULL);
			exprw = &(*exprw)->value.cons.right;
			if		(  ComToken  ==  '.'  ) {
				GetName;
			}
		}	while	(	(  ComToken  ==  T_SYMBOL  )
					||	(  ComToken  ==  '['       ) );
		expr = NewCons(EXPR_VREF,expr,NULL);
		break;
	  case	'(':
		GetSymbol;
		expr = ParExpr(in);
		if		(  ComToken  ==  ')'  ) {
			GetSymbol;
		} else {
			Error("factor invalid (");
		}
		break;
	  case	T_SCONST:
	  case	T_ICONST:
		expr = New(Expr);
		expr->type = EXPR_VALUE;
		expr->value.sval = StrDup(ComSymbol);
		GetSymbol;
		break;
	  default:
		Error("? token = %d\n",ComToken);
		expr = NULL;
		break;
	}
LEAVE_FUNC;
	return	(expr);
}

static	Expr	*
ParTerm(
	CURFILE			*in)
{
	Expr	*expr;
	int		op;

ENTER_FUNC;
	expr = ParFactor(in);
	while	(	(  ComToken  ==  '*'  )
			 ||	(  ComToken  ==  '/'  )
			 ||	(  ComToken  ==  '%'  ) ) {
		switch	( ComToken)	{
		  case	'*':
			op = EXPR_MUL;
			break;
		  case	'/':
			op = EXPR_DIV;
			break;
		  case	'%':
			op = EXPR_MOD;
			break;
		}
		GetSymbol;
		expr = NewCons(op,expr,ParFactor(in));
	}
LEAVE_FUNC;
	return	(expr);
}

static	Expr	*
ParExpr(
	CURFILE			*in)
{
	Expr	*expr;
	int		op;
	Bool	fMinus;

ENTER_FUNC;
	if		(	(  ComToken  ==  '+'  )
			 ||	(  ComToken  ==  '-'  ) )	{
		fMinus = ( ComToken  ==  '-'  );
		GetSymbol;
	} else {
		fMinus = FALSE;
	}
	expr = ParTerm(in);
	if		(  fMinus  ) {
		expr = NewCons(EXPR_NEG,expr,NULL);
	}
	while	(	(  ComToken  ==  '+'  )
			||	(  ComToken  ==  '-'  )
			||	(  ComToken  ==  '&'  ) )	{
		switch	(ComToken) {
		  case	'&':
			op = EXPR_CAT;
			break;
		  case	'+':
			op = EXPR_ADD;
			break;
		  case	'-':
			op = EXPR_SUB;
			break;
		}
		GetSymbol;
		expr = NewCons(op,expr,ParTerm(in));
	}
LEAVE_FUNC;
	return	(expr);
}

static	Expr	*
ParSequence(
	CURFILE			*in)
{
	Expr	**expr
		,	*expr2
		,	*ret;
	Bool	fExit;

ENTER_FUNC;
	fExit = FALSE;
	expr = &ret;
	do {
		expr2 = ParExpr(in);
		*expr = NewCons(EXPR_SEQ,expr2,NULL);
		switch	(ComToken) {
		  case	',':
		  case	')':
			expr = (Expr **)&((*expr)->value.cons.right);
			if		(  ComToken  ==  ')'  ) {
				fExit = TRUE;
			} else {
				GetSymbol;
			}
			break;
		  default:
			fExit = TRUE;
			break;
		}
	}	while	(  !fExit  );
LEAVE_FUNC;
	return	(ret);
}

static	Expr	*
ParseMem(
	char	*mem)
{
	CURFILE		*in
		,		root;
	Expr		*expr;

ENTER_FUNC;
	root.next = NULL;
	if		(  ( in = PushLexInfoMem(&root,mem,NULL,Reserved) )  !=  NULL  ) {
		GetSymbol;
		expr = ParExpr(in);
		DropLexInfo(&in);
	} else {
		expr = NULL;
	}
LEAVE_FUNC;
	return	(expr);
}

static	char	*
ValueSymbol(
	char	*name)
{
	ValueStruct			*item;
	char	*value;

	dbgprintf("name = [%s]\n",name);
	if		(  ( value = LoadValue(name) )  ==  NULL  )	{
		if		(  _GetValue  !=  NULL  ) {
			if		(  ( item = (_GetValue)(name, fClear) )  ==  NULL  ) {
				value = "";
			} else {
				value = ValueToString(item, NULL);
			}
			value = SaveValue(name,value,FALSE);
		}
	}
	dbgprintf("value = [%s]\n",value);
	return	(value);
}

static	Expr	*
EvalExpr(
	Expr	*expr)
{
	char	buff[SIZE_LONGNAME+1];
	Expr	*temp
		,	*ix
		,	*result
		,	*left
		,	*right
		,	**res;
	char	*p;

ENTER_FUNC;
	if		(  expr  !=  NULL  ) {
		dbgprintf("expr->type = %d\n",expr->type);
		switch	(expr->type) {
		  case	EXPR_ITEM:
			dbgmsg("ITEM");
			p = buff;
			temp = expr;
			while	(  temp  !=  NULL  ) {
				left = temp->value.cons.left;
				switch	(left->type) {
				  case	EXPR_SYMBOL:
					p += sprintf(p,"%s",left->value.name);
					break;
				  default:
					ix = EvalExpr(left);
					p += sprintf(p,"[%d]",atoi(ix->value.sval));
				}
				temp = temp->value.cons.right;
				if		(  temp  !=  NULL  ) {
					p += sprintf(p,".");
				}
			}
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(buff);
			break;
		  case	EXPR_VREF:
			dbgmsg("VREF");
			left = EvalExpr(expr->value.cons.left);
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(ValueSymbol(left->value.sval));
			break;
		  case	EXPR_SEQ:
			dbgmsg(">SEQ");
			temp = expr;
			res = &result;
			do {
				*res = New(Expr);
				(*res)->type = EXPR_SEQ;
				(*res)->value.cons.left = EvalExpr(temp->value.cons.left);
				(*res)->value.cons.right = NULL;
				res = (Expr **)&((*res)->value.cons.right);
				temp = temp->value.cons.right;
			}	while	(  temp  !=  NULL  );
			dbgmsg("<SEQ");
			break;
		  case	EXPR_CAT:
			dbgmsg("CAT");
			left = EvalExpr(expr->value.cons.left);
			right = EvalExpr(expr->value.cons.right);
			sprintf(buff,"%s%s",left->value.sval,right->value.sval);
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(buff);
			break;
		  case	EXPR_ADD:
			dbgmsg("ADD");
			left = EvalExpr(expr->value.cons.left);
			right = EvalExpr(expr->value.cons.right);
			sprintf(buff,"%d",atoi(left->value.sval)+atoi(right->value.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(buff);
			break;
		  case	EXPR_SUB:
			dbgmsg("SUB");
			left = EvalExpr(expr->value.cons.left);
			right = EvalExpr(expr->value.cons.right);
			sprintf(buff,"%d",atoi(left->value.sval)-atoi(right->value.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(buff);
			break;
		  case	EXPR_MUL:
			dbgmsg("MUL");
			left = EvalExpr(expr->value.cons.left);
			right = EvalExpr(expr->value.cons.right);
			sprintf(buff,"%d",atoi(left->value.sval)*atoi(right->value.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(buff);
			break;
		  case	EXPR_DIV:
			dbgmsg("DIV");
			left = EvalExpr(expr->value.cons.left);
			right = EvalExpr(expr->value.cons.right);
			sprintf(buff,"%d",atoi(left->value.sval)/atoi(right->value.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(buff);
			break;
		  case	EXPR_MOD:
			dbgmsg("MOD");
			left = EvalExpr(expr->value.cons.left);
			right = EvalExpr(expr->value.cons.right);
			sprintf(buff,"%d",atoi(left->value.sval)%atoi(right->value.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(buff);
			break;
		  case	EXPR_VALUE:
			dbgmsg("VALUE");
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->value.sval = StrDup(expr->value.sval);
			break;
		  default:
			Error("not support expression");
			break;
		}
	}
LEAVE_FUNC;
	return	(result);
}

extern	char	*
GetHostValue(
	char	*name,
	Bool	_fClear)
{
	Expr	*expr;
	char	*value;

ENTER_FUNC;
dbgprintf("name = [%s]\n",name);
	fClear = _fClear;
	expr = ParseMem(name);
	expr = EvalExpr(expr);
	dbgprintf("type = %d\n",expr->type);
	value = expr->value.sval;
dbgprintf("value = [%s]\n",value);
LEAVE_FUNC;
	return	(value);
}

extern	void
InitHTC(
	char	*script_name,
	GET_VALUE	func)
{
ENTER_FUNC;
    if (script_name == NULL) {
        script_name = "mon.cgi";
    }
	HTCLexInit();
	JslibInit();
	Reserved = MakeReservedTable(tokentable);

	TagsInit(script_name);
	Codeset = "utf-8";
	_GetValue = func;
LEAVE_FUNC;
}

