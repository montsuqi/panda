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

#define	_DB_PARSER

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<sys/stat.h>	/*	for stbuf	*/
#include	"libmondai.h"
#include	"RecParser.h"
#include	"Lex.h"
#include	"DBparser.h"
#include	"DDparser.h"
#include	"SQLlex.h"
#include	"SQLparser.h"
#include	"directory.h"
#include	"debug.h"

static	GHashTable	*DB_Reserved;

#define	T_PRIMARY		(T_YYBASE +1)
#define	T_PATH			(T_YYBASE +2)
#define	T_USE			(T_YYBASE +3)
#define	T_OPERATION		(T_YYBASE +4)
#define	T_PROCEDURE		(T_YYBASE +5)
#define	T_READONLY		(T_YYBASE +6)
#define	T_UPDATE		(T_YYBASE +7)

static	TokenTable	DB_Tokentable[] = {
	{	"primary"	,T_PRIMARY		},
	{	"path"		,T_PATH			},
	{	"use"		,T_USE			},
	{	"operation"	,T_OPERATION	},
	{	"procedure"	,T_PROCEDURE	},
	{	"readonly"	,T_READONLY		},
	{	"update"	,T_UPDATE		},
	{	""			,0				}
};

extern	void
DB_ParserInit(void)
{
	LexInit();
	DB_Reserved = MakeReservedTable(DB_Tokentable);

	SQL_ParserInit();
}


static	char	***
ParKeyItem(
	CURFILE		*in,
	ValueStruct	*root)
{
	char	**name
	,		**p;
	size_t	count
	,		rcount;
	char	***ret
	,		***r;
	char	*elm;
	ValueStruct	*value;

ENTER_FUNC;
	ret = NULL;
	rcount = 0;
	while	(  GetName  ==  T_SYMBOL  ) {
		name = NULL;
		count = 0;
		value = root;
		do {
			p = (char **)xmalloc(sizeof(char *) * ( count + 2 ));
			if		(  name  !=  NULL  ) {
				memcpy(p,name,sizeof(char **)*count);
				xfree(name);
			}
			name = p;
			elm = StrDup(ComSymbol);
			if		(  value  !=  NULL  ) {
				if		(  ( value = GetRecordItem(value,elm) )  ==  NULL  ) {
					printf("%s:%d:not in record item [%s]\n",in->fn,in->cLine,elm);
				}
			}
			name[count] = elm;
			name[count+1] = NULL;
			count ++;
			GetSymbol;
			if		(  ComToken  ==  '.'  ) {
				GetName;
			} else
			if		(  ComToken  ==  ','  ) {
				break;
			} else
			if		(  ComToken  !=  ';'  ) {
					ParError("; not found");
			}
		}	while	(  ComToken  ==  T_SYMBOL  );
		r = (char ***)xmalloc(sizeof(char **) * ( rcount + 2 ));
		if		(  ret  !=  NULL  ) {
			memcpy(r,ret,sizeof(char ***) * rcount);
			xfree(ret);
		}
		ret = r;
		ret[rcount] = name;
		ret[rcount+1] = NULL;
		rcount ++;
		ERROR_BREAK;
	}
	if		(  ComToken  !=  '}'  ) {
		ParError("} not found");
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
ParKey(
	CURFILE			*in,
	RecordStruct	*rec)
{
	DB_Struct	*db;
	KeyStruct	*skey;

ENTER_FUNC;
	db = RecordDB(rec);
	skey = New(KeyStruct);
	skey->item = ParKeyItem(in,rec->value);
	db->pkey = skey;
LEAVE_FUNC;
}

static	DB_Struct	*
InitDB_Struct(
	char	*gname)
{
	DB_Struct	*ret;

ENTER_FUNC;
	ret = New(DB_Struct);
	ret->paths = NewNameHash();
	ret->use = NewNameHash();
	ret->opHash = NewNameHash();
	ret->ocount = 0;
	ret->ops = NULL;
	ret->pkey = NULL;
	ret->path = NULL;
	ret->dbg = NULL;
	if		(  gname  ==  NULL  ) {
		ret->gname = NULL;
	} else
	if		(  ( ret->dbg = GetDBG(gname) )  ==  NULL  ) {
		ret->gname = StrDup(gname);
	} else {
		ret->gname = NULL;
	}
	ret->pcount = 0;
LEAVE_FUNC;
	return	(ret);
}

static	DB_Operation	*
NewOperation(
	char	*name)
{
	DB_Operation	*op;

	op = New(DB_Operation);
	op->name = StrDup(name);
	op->proc = NULL;
	op->args = NULL;
	return	(op);
}

static	void
InsertBuildIn(
	PathStruct	*ret,
	char		*name,
	int			func)
{
	DB_Operation	*op;

	op = NewOperation(name);
	ret->ops[func] = op;
	g_hash_table_insert(ret->opHash,op->name,(gpointer)(long)(func+1));
}

static	PathStruct	*
NewPathStruct(
	int		usage)
{
	PathStruct	*ret;

ENTER_FUNC;
	ret = New(PathStruct);
	ret->opHash = NewNameHash();
	ret->ops = (DB_Operation **)xmalloc(sizeof(DB_Operation *) * SIZE_DBOP);
	InsertBuildIn(ret,"DBSELECT",DBOP_SELECT);
	InsertBuildIn(ret,"DBFETCH",DBOP_FETCH);
	InsertBuildIn(ret,"DBUPDATE",DBOP_UPDATE);
	InsertBuildIn(ret,"DBINSERT",DBOP_INSERT);
	InsertBuildIn(ret,"DBDELETE",DBOP_DELETE);
	InsertBuildIn(ret,"DBCLOSECURSOR",DBOP_CLOSE);
	ret->ocount = 6;
	ret->usage = usage;
	ret->args = NULL;
LEAVE_FUNC;
	return	(ret);
}

static	void
EnterUse(
	RecordStruct	*root,
	char			*name,
	RecordStruct	*rec)
{
	if		(  g_hash_table_lookup(RecordDB(root)->use,name)  ==  NULL  ) {
		g_hash_table_insert(RecordDB(root)->use,name,rec);
	}
}

static	Bool	fTop;
static	int
SCRIPT_Lex(
	CURFILE	*in)
{
	int		c;

  retry: 
	c = GetChar(in);
	if		(  c  ==  '\n'  ) {
		in->cLine ++;
	}
	if		(	(  fTop  )
			&&	(  c  ==  '#'  ) ) {
		while	(	(  ( c = GetChar(in) )  !=  0    )
				&&	(  c                    !=  '\n' ) );
		in->cLine ++;
		fTop = TRUE;
		goto	retry;
	}
	return	(c);
}

static	LargeByteString	*
ParSCRIPT(
	CURFILE			*in,
	RecordStruct	*rec,
	ValueStruct		*argp,
	ValueStruct		*argf)
{
	LargeByteString	*scr;
	int		pc
		,	c;

ENTER_FUNC;
	scr = NewLBS();
	pc = 1;
	while	(  ( c = SCRIPT_Lex(in) )  !=  EOF ) {
		fTop = FALSE;
		switch	(c) {
		  case	'{':
			pc ++;
			break;
		  case	'}':
			pc --;
			break;
		  case	'\n':
			fTop = TRUE;
			break;
		  default:
			break;
		}
		if		(  pc  >  0  ) {
			LBS_Emit(scr,c);
		} else
			break;
	}
	LBS_EmitEnd(scr);
	LBS_EmitFix(scr);
LEAVE_FUNC;
	return	(scr);
}

static	LargeByteString	*
ParScript(
	CURFILE			*in,
	RecordStruct	*rec,
	ValueStruct		*argp,
	ValueStruct		*argf)
{
	LargeByteString	*ret;
	DB_Struct		*db = RecordDB(rec);

ENTER_FUNC;
	if		(  db->dbg  ==  NULL  ) {
		Error("'db_group' must be before LD and BD, in directory (%s).",rec->name);
	}
	switch	(db->dbg->func->type) {
	  case	DB_PARSER_SCRIPT:
		ret = ParSCRIPT(in,rec,argp,argf);
		dbgprintf("ret = [%s]",(char *)LBS_Body(ret));
		break;
	  case	DB_PARSER_SQL:
		ret = ParSQL(in,rec,argp,argf);
		break;
	  case	DB_PARSER_NULL:
	  default:
		ret = NULL;
		break;
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
ParTableOperation(
	CURFILE			*in,
	RecordStruct	*rec,
	PathStruct		*path)
{
	int		ix;
	DB_Operation	**ops
	,				*op;
	ValueStruct		*value;
	char			name[SIZE_SYMBOL+1];

ENTER_FUNC;
	if		(  ( ix = (int)(long)g_hash_table_lookup(path->opHash,ComSymbol) )  ==  0  ) {
		ix = path->ocount;
		ops = (DB_Operation **)xmalloc(sizeof(DB_Operation *) * ( ix + 1 ));
		memcpy(ops,path->ops,(sizeof(DB_Operation *) * ix));
		xfree(path->ops);
		path->ops = ops;
		op = NewOperation(ComSymbol);
		g_hash_table_insert(path->opHash, op->name, (gpointer)((long)ix + 1));
		path->ops[ix] = op;
		path->ocount ++;
	} else {
		ix --;
		op = NewOperation(ComSymbol);
		path->ops[ix] = op;
	}
	if		(  GetSymbol  ==  '('  ) {
		op->args = NewValue(GL_TYPE_RECORD);
		GetName;
		while	(  ComToken  ==  T_SYMBOL  ) {
			strcpy(name,ComSymbol);
			value = ParValueDefine(in);
			if		(  ComToken  ==  ','  ) {
				GetName;
			}
			SetValueAttribute(value,GL_ATTR_NULL);
			ValueAddRecordItem(op->args,name,value);
		}
		if		(  ComToken  ==  ')'  ) {
			GetSymbol;
		} else {
			ParError(") missing");
		}
		SetReserved(in,DB_Reserved); 
	}
	if		(  ComToken  == '{'  ) {
		if		(  op->proc  ==  NULL  ) {
			op->proc = ParScript(in,rec,path->args,op->args);
			if		(  GetSymbol  !=  ';'  ) {
				ParError("; missing");
			}
		} else {
			ParError("function duplicate");
		}
	} else {
		ParError("{ missing");
	}
LEAVE_FUNC;
}

static	void
ParPath(
	CURFILE			*in,
	RecordStruct	*rec,
	int				usage)
{
	int		pcount;
	PathStruct		**paths
		,			*path;
	ValueStruct		*value;
	char			name[SIZE_SYMBOL+1];

ENTER_FUNC;
	pcount = RecordDB(rec)->pcount;
	paths = (PathStruct **)xmalloc(sizeof(PathStruct *) * (pcount + 1));
	if		(  pcount  >  0  ) {
		memcpy(paths,RecordDB(rec)->path,(sizeof(PathStruct *) * pcount));
		xfree(RecordDB(rec)->path);
	}
	path = NewPathStruct(usage);
	paths[pcount] = path;
	path->name = StrDup(ComSymbol);
	if ( g_hash_table_lookup(RecordDB(rec)->paths,path->name) != NULL) {
		Warning("path:%s key:%s duplicate", rec->name, path->name);
	}
	g_hash_table_insert(RecordDB(rec)->paths,path->name,(void *)((long)pcount+1));
	RecordDB(rec)->pcount ++;
	RecordDB(rec)->path = paths;
	if		(  GetSymbol  ==  '('  ) {
		path->args = NewValue(GL_TYPE_RECORD);
		GetName;
		while	(  ComToken  ==  T_SYMBOL  ) {
			strcpy(name,ComSymbol);
			value = ParValueDefine(in);
			if		(  ComToken  ==  ','  ) {
				GetName;
			}
			SetValueAttribute(value,GL_ATTR_NULL);
			ValueAddRecordItem(path->args,name,value);
		}
		if		(  ComToken  ==  ')'  ) {
			GetSymbol;
		} else {
			ParError(") missing");
		}
		SetReserved(in,DB_Reserved);
	}
	if		(  ComToken  !=  '{'  ) {
		ParError("{ missing");
	}
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_PROCEDURE:
			ParError("not implemented");
			break;
		  case	T_OPERATION:
			if		(  GetName  !=  T_SYMBOL  ) {
				ParError("invalid operation name");
			}
			/*	path thrue	*/
		  case	T_SYMBOL:
			ParTableOperation(in,rec,path);
			break;
		  default:
			ParError("invalid token");
			break;
		}
		ERROR_BREAK;
	}
	if		(  GetSymbol  !=  ';'  ) {
		ParError("; missing");
	}
LEAVE_FUNC;
}

static	void
ParDBOperation(
	CURFILE			*in,
	RecordStruct	*rec)
{
	DB_Struct	*db = RecordDB(rec);
	int		ix;
	DB_Operation	**ops
	,				*op;

ENTER_FUNC;
	if		(  ( ix = (int)(long)g_hash_table_lookup(db->opHash,ComSymbol) )  ==  0  ) {
		ix = db->ocount;
		ops = (DB_Operation **)xmalloc(sizeof(DB_Operation *) * ( ix + 1 ));
		if		(  db->ops  !=  NULL  ) {
			memcpy(ops,db->ops,(sizeof(DB_Operation *) * ix));
			xfree(db->ops);
		}
		db->ops = ops;
		op = NewOperation(ComSymbol);
		g_hash_table_insert(db->opHash, op->name, (gpointer)((long)ix + 1));
		db->ops[db->ocount] = op;
		db->ocount ++;
	} else {
		op = NewOperation(ComSymbol);
		db->ops[ix-1] = op;
	}
	if		(  GetSymbol  == '{'  ) {
		if		(  op->proc  ==  NULL  ) {
			op->proc = ParScript(in,rec,NULL,NULL);
			if		(  GetSymbol  !=  ';'  ) {
				ParError("; missing");
			}
		} else {
			ParError("function duplicate");
		}
	} else {
		ParError("{ missing");
	}
LEAVE_FUNC;
}

static	RecordStruct	*
DB_Parse(
	CURFILE	*in,
	char	*name,
	char	*gname,
	Bool	fScript)
{
	RecordStruct	*ret
	,				*use;
	char	buff[SIZE_LONGNAME+1];
	PathStruct		*path;

ENTER_FUNC;
	dbgprintf("fScript = %d",fScript);
	ret = DD_Parse(in);
	if		(  ret  ==  NULL  ) {
		exit(1);
    }
	if		(  !stricmp(strrchr(name,'.'),".db")  ) {
		ret->type = RECORD_DB;
		RecordDB(ret) = InitDB_Struct(gname);
	} else {
		ret->type = RECORD_NULL;
	}
	SetReserved(in,DB_Reserved); 
	while	(  	GetSymbol  !=  T_EOF  ) {
		switch	(ComToken) {
		  case	T_USE:
			switch	(GetName) {
			  case	T_SYMBOL:
				sprintf(buff,"%s.db",ComSymbol);
				use = ReadRecordDefine(buff);
				if		(  use  ==  NULL  ) {
					ParError("define not found");
				} else {
					EnterUse(ret,use->name,use);
				}
				if		(  GetSymbol  !=  ';'  ) {
					ParError("use statement error");
				}
				break;
			  default:
				ParError("use invalid symbol");
				break;
			}
			break;
		  case	T_PRIMARY:
			if		(  ret->type  ==  RECORD_NULL  ) {
				ret->type = RECORD_DB;
				RecordDB(ret) = InitDB_Struct(gname);
			}
			if		(  GetSymbol  == '{'  ) {
				ParKey(in,ret);
				if		(  GetSymbol  !=  ';'  ) {
					ParError("; missing");
				}
			} else {
				ParError("syntax error(PRIMARY)");
			}
			break;
		  case	T_READONLY:
			if		(  GetSymbol  ==  T_PATH  ) {
				if		(  !fScript  )	goto	quit;
				if		(  GetName  !=  T_SYMBOL  ) {
					ParError("path name invalid");
				} else {
					ParPath(in,ret,DB_READONLY);
				}
			} else {
				ParError("path missing");
			}
			break;
		  case	T_UPDATE:
			if		(  GetSymbol  ==  T_PATH  ) {
				if		(  !fScript  )	goto	quit;
				if		(  GetName  !=  T_SYMBOL  ) {
					ParError("path name invalid");
				} else {
					ParPath(in,ret,DB_UPDATE);
				}
			} else {
				ParError("path missing");
			}
			break;
		  case	T_PATH:
			if		(  !fScript  )	goto	quit;
			if		(  GetName  !=  T_SYMBOL  ) {
				ParError("path name invalid");
			} else {
				ParPath(in,ret,DB_UNDEF);
			}
			break;
		  case	T_OPERATION:
			if		(  GetName  !=  T_SYMBOL  ) {
				ParError("operation name invalid");
			} else {
				ParDBOperation(in,ret);
			}
			break;
		  default:
			ParError("syntax error(other)");
			printf("token = [%X]\n",ComToken);
			exit(1);
			break;
		}
		ERROR_BREAK;
	}
  quit:
	if		(	(  ret->type              ==  RECORD_DB  )
			&&	(  RecordDB(ret)->pcount  ==  0          ) ) {
		RecordDB(ret)->path = (PathStruct **)xmalloc(sizeof(PathStruct *) * 1);
		path = NewPathStruct(DB_UNDEF);
		path->name = StrDup("primary");
		g_hash_table_insert(RecordDB(ret)->paths,path->name,(void *)1);
		RecordDB(ret)->pcount ++;
		RecordDB(ret)->path[0] = path;
	}
LEAVE_FUNC;
	return	(ret);
}


static	void
ResolveAlias(
	RecordStruct	*root,
	ValueStruct		*val)
{
	char		*name;
	int			i;
	char		*p;
	RecordStruct	*use;
	ValueStruct		*item;

ENTER_FUNC;
	if		(  val  !=  NULL  ) {
		switch	(val->type) {
		  case	GL_TYPE_ARRAY:
			for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
				ResolveAlias(root,ValueArrayItem(val,i));
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
				item = ValueRecordItem(val,i);
				if		(  ValueType(item)  ==  GL_TYPE_ALIAS  ) {
					name = ValueAliasName(item);
					if		(  ( p = strchr(name,'.') )  !=  NULL  ) {
						*p = 0;
						if		(  ( use = g_hash_table_lookup(RecordDB(root)->use,name) )
								   !=  NULL  ) {
							ValueRecordItem(val,i) = GetItemLongName(use->value,p+1);
							xfree(name);
							xfree(item);
							ValueAttribute(ValueRecordItem(val,i)) |= GL_ATTR_ALIAS;
						} else {
							printf("alias table not found [%s]\n",name);
						}
					}
				} else {
					ResolveAlias(root,item);
				}
			}
			break;
		  case	GL_TYPE_ALIAS:
			break;
		  case	GL_TYPE_BYTE:
		  case	GL_TYPE_CHAR:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
		  case	GL_TYPE_TEXT:
		  case	GL_TYPE_NUMBER:
		  default:
			break;
		}
	}
LEAVE_FUNC;
}	

extern	RecordStruct	*
DB_Parser(
	char	*name,
	char	*gname,
	char	**ValueName,
	Bool	fScript)
{
	struct	stat	stbuf;
	RecordStruct	*ret;
	CURFILE		*in
		,		root;

ENTER_FUNC;
	root.next = NULL;
	dbgprintf("name  = [%s]",name);
	dbgprintf("gname = [%s]",gname);
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  ( in = PushLexInfo(&root,name,RecordDir,DB_Reserved) )  !=  NULL  ) {
			ret = DB_Parse(in,name,gname,fScript);
			if		(  ValueName  !=  NULL  ) {
				*ValueName = StrDup(in->ValueName);
			}
			DropLexInfo(&in);
			ResolveAlias(ret,ret->value);
		} else {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}
