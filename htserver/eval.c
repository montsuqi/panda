static	char		*InString;
static	int			CharBack;
static	GHashTable	*Reserved;
static	int			ComToken;
static	int			ComInt;
static	char		ComSymbol[SIZE_LONGNAME+1];

static	Expr	*ParSequence(void);
static	Expr	*ParExpr(void);

#define	SKIP_SPACE		\
	while	(	(  ( c = GetChar() )  !=  0  )		\
			&&	(  isspace(c)              ) );

#define	GetSymbol		(ComToken = Lex(FALSE))
#define	GetName			(ComToken = Lex(TRUE))

static	struct	{
	char	*str;
	int		token;
}	tokentable[] = {
	{	""			,0			}
};

static	int
GetChar(void)
{
	int		c;

	if		(  CharBack  >=  0  ) {
		c = CharBack;
		CharBack = -1;
	} else {
		c = *InString;
		InString ++;
	}
	return	(c);
}

static	void
UnGetChar(
	int		c)
{
	CharBack = c;
}

static	void
InitStringLex(void)
{
	int		i;

	Reserved = NewNameHash();
	for	( i = 0 ; tokentable[i].token  !=  0 ; i ++ ) {
		g_hash_table_insert(Reserved,tokentable[i].str,(gpointer)tokentable[i].token);
	}
}

static	int
CheckReserved(
	char	*str)
{
	gpointer	p;
	int		ret;

	if		(  ( p = g_hash_table_lookup(Reserved,str) ) !=  NULL  ) {
#ifndef	__LP64__
		ret = (int)p;
#else
		long tmp =(long)p;
		ret = (int)tmp;
#endif
	} else {
		ret = T_SYMBOL;
	}
	return	(ret);
}

static	int
Lex(
	Bool	fName)
{
	int		c;
	char	*p;
	Bool	fDot;
	int		token;

ENTER_FUNC;
  retry:
	SKIP_SPACE;
	switch	(c) {
	  case	'/':
		if		(  ( c = GetChar() )  !=  '*'  ) {
			UnGetChar(c);
			token = '/';
		} else {
			do {
				while	(  ( c = GetChar() )  !=  '*'  );
				if		(  ( c = GetChar() )  ==  '/'  )	break;
				UnGetChar(c);
			}	while	(TRUE);
			goto	retry;
		}
		break;
	  case	'"':
		p = ComSymbol;
		while	(  ( c = GetChar() )  !=  '"'  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar();
			}
			*p ++ = c;
		}
		*p = 0;
		token = T_SCONST;
		break;
	  case	'\'':
		p = ComSymbol;
		while	(  ( c = GetChar() )  !=  '\''  ) {
			if		(  c  ==  '\\'  ) {
				c = GetChar();
			}
			*p ++ = c;
		}
		*p = 0;
		token = T_SCONST;
		break;
	  case	'<':
		if		(  ( c = GetChar() )  ==  '='  ) {
			token = T_LE;
		} else {
			token = T_LT;
			UnGetChar(c);
		}
		break;
	  case	'>':
		if		(  ( c = GetChar() )  ==  '='  ) {
			token = T_GE;
		} else {
			token = T_GT;
			UnGetChar(c);
		}
		break;
	  case	'=':
		switch(c = GetChar())	{
		  case	'=':
			token = T_EQ;
			break;
		  case	'>':
			token = T_GE;
			break;
		  case	'<':
			token = T_LE;
			break;
		  default:
			token = T_EQ;
			UnGetChar(c);
			break;
		}
		break;
	  case	'!':
		if		(  ( c = GetChar() )  ==  '='  ) {
			token = T_NE;
		} else {
			token = '!';
			UnGetChar(c);
		}
		break;
	  case	0:
		token = T_EOF;
		break;
	  default:
		p = ComSymbol;
		if		(	(  isalpha(c)  )
				||	(  c  ==  '_'  ) ) {
			do {
				*p ++ = c;
				if		(  ( c = GetChar() )  ==  T_EOF  )	break;
			}	while	(	(  isalpha(c)  )
						||	(  isdigit(c)  )
						||	(  c  ==  '_'  ) );
			UnGetChar(c);
			*p = 0;
			if		(  fName  ) {
				token = T_SYMBOL;
			} else {
				token = CheckReserved(ComSymbol);
			}
		} else
		if		(  isdigit(c) )	{
			fDot = FALSE;
			do {
				*p ++ = c;
				if		(  ( c = GetChar() )  ==  T_EOF  )	break;
				if		(  c  ==  '.'  )
					fDot = TRUE;
			}	while	(	(  isalpha(c)  )
						||	(  isdigit(c)  )
						||	(  c  ==  '.'  ) );
			UnGetChar(c);
			if		(  fDot  ) {
				token = T_NCONST;
			} else {
				ComInt = atol(ComSymbol);
				token = T_ICONST;
			}
		} else {
			switch	(c) {
			  case	0:
				token = T_EOF;
				break;
			  default:
				token = c;
				break;
			}
		}
		break;
	}
	dbgprintf("[%s]",ComSymbol);
LEAVE_FUNC;
	return	(token);
}

static	Expr	*
NewCons(
	int		type,
	Expr	*left,
	Expr	*right)
{
	Expr	*expr;

	expr = New(Expr);
	expr->type = type;
	expr->body.cons.left = left;
	expr->body.cons.right = right;
	return	(expr);
}

static	Expr	*
ParFactor(void)
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
				arg->body.name = StrDup(ComSymbol);
				GetName;
				break;
			  case	'[':
				GetSymbol;
				arg = ParExpr();
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
			exprw = &(*exprw)->body.cons.right;
			if		(  ComToken  ==  '.'  ) {
				GetName;
			}
		}	while	(	(  ComToken  ==  T_SYMBOL  )
					||	(  ComToken  ==  '['       ) );
		expr = NewCons(EXPR_VREF,expr,NULL);
		break;
	  case	'(':
		GetSymbol;
		expr = ParExpr();
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
		expr->body.sval = StrDup(ComSymbol);
		GetSymbol;
		break;
	  default:
		fprintf(stderr,"? token = %d\n",ComToken);
		expr = NULL;
		break;
	}
LEAVE_FUNC;
	return	(expr);
}

static	Expr	*
ParTerm(void)
{
	Expr	*expr;
	int		op;

ENTER_FUNC;
	expr = ParFactor();
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
		expr = NewCons(op,expr,ParFactor());
	}
LEAVE_FUNC;
	return	(expr);
}

static	Expr	*
ParExpr(void)
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
	expr = ParTerm();
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
		expr = NewCons(op,expr,ParTerm());
	}
LEAVE_FUNC;
	return	(expr);
}

static	Expr	*
ParSequence(void)
{
	Expr	**expr
		,	*expr2
		,	*ret;
	Bool	fExit;

ENTER_FUNC;
	fExit = FALSE;
	expr = &ret;
	do {
		expr2 = ParExpr();
		*expr = NewCons(EXPR_SEQ,expr2,NULL);
		switch	(ComToken) {
		  case	',':
		  case	')':
			expr = (Expr **)&((*expr)->body.cons.right);
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
	Expr		*expr;

ENTER_FUNC;
	InString = mem;
	CharBack = -1;
	dbgprintf("parse [%s]",InString);
	GetSymbol;
	expr = ParExpr();
LEAVE_FUNC;
	return	(expr);
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
				left = temp->body.cons.left;
				switch	(left->type) {
				  case	EXPR_SYMBOL:
					p += sprintf(p,"%s",left->body.name);
					break;
				  default:
					ix = EvalExpr(left);
					p += sprintf(p,"[%d]",atoi(ix->body.sval));
				}
				temp = temp->body.cons.right;
				if		(  temp  !=  NULL  ) {
					p += sprintf(p,".");
				}
			}
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(buff);
			break;
		  case	EXPR_VREF:
			dbgmsg("VREF");
			left = EvalExpr(expr->body.cons.left);
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(ValueSymbol(left->body.sval));
			break;
		  case	EXPR_SEQ:
			dbgmsg(">SEQ");
			temp = expr;
			res = &result;
			do {
				*res = New(Expr);
				(*res)->type = EXPR_SEQ;
				(*res)->body.cons.left = EvalExpr(temp->body.cons.left);
				(*res)->body.cons.right = NULL;
				res = (Expr **)&((*res)->body.cons.right);
				temp = temp->body.cons.right;
			}	while	(  temp  !=  NULL  );
			dbgmsg("<SEQ");
			break;
		  case	EXPR_CAT:
			dbgmsg("CAT");
			left = EvalExpr(expr->body.cons.left);
			right = EvalExpr(expr->body.cons.right);
			sprintf(buff,"%s%s",left->body.sval,right->body.sval);
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(buff);
			break;
		  case	EXPR_ADD:
			dbgmsg("ADD");
			left = EvalExpr(expr->body.cons.left);
			right = EvalExpr(expr->body.cons.right);
			sprintf(buff,"%d",atoi(left->body.sval)+atoi(right->body.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(buff);
			break;
		  case	EXPR_SUB:
			dbgmsg("SUB");
			left = EvalExpr(expr->body.cons.left);
			right = EvalExpr(expr->body.cons.right);
			sprintf(buff,"%d",atoi(left->body.sval)-atoi(right->body.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(buff);
			break;
		  case	EXPR_MUL:
			dbgmsg("MUL");
			left = EvalExpr(expr->body.cons.left);
			right = EvalExpr(expr->body.cons.right);
			sprintf(buff,"%d",atoi(left->body.sval)*atoi(right->body.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(buff);
			break;
		  case	EXPR_DIV:
			dbgmsg("DIV");
			left = EvalExpr(expr->body.cons.left);
			right = EvalExpr(expr->body.cons.right);
			sprintf(buff,"%d",atoi(left->body.sval)/atoi(right->body.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(buff);
			break;
		  case	EXPR_MOD:
			dbgmsg("MOD");
			left = EvalExpr(expr->body.cons.left);
			right = EvalExpr(expr->body.cons.right);
			sprintf(buff,"%d",atoi(left->body.sval)%atoi(right->body.sval));
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(buff);
			break;
		  case	EXPR_VALUE:
			dbgmsg("VALUE");
			result = New(Expr);
			result->type = EXPR_VALUE;
			result->body.sval = StrDup(expr->body.sval);
			break;
		  default:
			Error("not support expression");
			break;
		}
	}
LEAVE_FUNC;
	return	(result);
}

