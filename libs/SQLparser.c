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
#define	MAIN
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
	return	(val);
}

extern	LargeByteString	*
ParSQL(
	RecordStruct	*rec)
{
	LargeByteString	*sql;
	ValueStruct		*val;
	Bool	fInto
	,		fAster;
	size_t	mark
	,		current;
	int		into;
	int		n;
	Bool	fInsert;

dbgmsg(">ParSQL");
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
				LBS_Emit(sql,SQL_OP_STO);
				into ++;
			} else {
				LBS_Emit(sql,SQL_OP_REF);
			}
			if		(  GetName  ==  T_SYMBOL  ) {
				val = rec->value;
				do {
					val = GetRecordItem(TraceAlias(rec,val),ComSymbol);
					if		(  val  ==  NULL  ) {
						printf("[%s]\n",ComSymbol);
						Error("item name missing");
					}
					switch	(GetSymbol) {
					  case	'.':
						GetName;
						break;
					  case	'[':
						if		(  ValueType(val)  ==  GL_TYPE_ARRAY  ) {
							if		(  GetSymbol  ==  T_SYMBOL  ) {
								n = atoi(ComSymbol) - 1;
								val = GetArrayItem(val,n);
								if		(  GetSymbol  !=  ']'  ) {
									Error("] missing");
								}
							}
						} else {
							Error("not array");
						}
						GetSymbol;
						break;
					  default:
						break;
					}
				}	while	(  ComToken  ==  T_SYMBOL  );
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
dbgmsg("<ParSQL");
	return	(sql);
}

extern	void
SQL_ParserInit(void)
{
	SQL_LexInit();
}

#ifdef	MAIN
#include	"dirs.h"
static	void
DumpPKey(
	KeyStruct	*pkey)
{
	char	***pka
	,		**pk;

	printf("*** dump pkey ***\n");
	pka = pkey->item;
	while	(  *pka  !=  NULL  ) {
		pk = *pka;
		while	(  *pk  !=  NULL  ) {
			printf("[%s]",*pk);
			pk ++;
		}
		printf("\n");
		pka ++;
	}
	
}

static	void
DumpSQL(
	char	*name,
	int		ix,
	PathStruct	*path)
{
	int		c;
	ValueStruct	*val;
	LargeByteString	*sql;

	printf("*** dump SQL ***\n");
	printf("command name = [%s]\n",name);
	sql = path->ops[ix-1];
	if		(  sql  !=  NULL  ) {
		while	(  ( c = FetchByte(sql) )  >=  0  ) {
			if		(  c  < 0x7F  ) {
				printf("%c",c);
			} else {
				switch	(c) {
				  case	SQL_OP_MSB:
					printf("%c",c | 0x80);
					break;
				  case	SQL_OP_INTO:
					printf("$into\n");
					val = (ValueStruct *)FetchPointer(sql);
					DumpValueStruct(val);
					break;
				  case	SQL_OP_REF:
					printf("$ref\n");
					val = (ValueStruct *)FetchPointer(sql);
					DumpValueStruct(val);
					break;
				  case	SQL_OP_EOL:
					printf(";\n");
					break;
				  default:
					break;
				}
			}
		}
	}
}

static	void
DumpPath(
	PathStruct	*path)
{
	printf("*** dump path ***\n");
	printf("path name = [%s]\n",path->name);
	g_hash_table_foreach(path->opHash,(GHFunc)DumpSQL,path);
}

static	void
DumpDB_Struct(
	DB_Struct	*db)
{
	int		i;

	DumpPKey(db->pkey);
	for	( i = 0 ; i < db->pcount ; i ++ ) {
		DumpPath(db->path[i]);
	}
}

extern	int
main(
	int		argc,
	char	**argv)
{
	RecordStruct	*ret;

	DB_ParserInit();
	ret = DB_ParserDataDefines(argv[1]);

	printf("*** dump ***\n");
	DumpValueStruct(ret->rec);
	DumpDB_Struct(ret->opt.db);

	return	(0);
}
#endif
