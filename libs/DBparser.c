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

#define	_DB_PARSER

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<sys/stat.h>	/*	for stbuf	*/
#include	"types.h"
#include	"libmondai.h"
#include	"Lex.h"
#include	"DBparser.h"
#include	"DDparser.h"
#include	"SQLparser.h"
#include	"debug.h"

static	GHashTable	*Reserved;

#define	T_PRIMARY		(T_YYBASE +1)
#define	T_PATH			(T_YYBASE +2)
#define	T_USE			(T_YYBASE +3)
#define	T_OPERATION		(T_YYBASE +4)
#define	T_PROCEDURE		(T_YYBASE +5)

static	TokenTable	tokentable[] = {
	{	"primary"	,T_PRIMARY		},
	{	"path"		,T_PATH			},
	{	"use"		,T_USE			},
	{	"operation"	,T_OPERATION	},
	{	"procedure"	,T_PROCEDURE	},
	{	""			,0				}
};

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
}

#undef	Error
#define	Error(msg)		{CURR->fError=TRUE;_Error((msg),CURR->fn,CURR->cLine);}

extern	void
DB_ParserInit(void)
{
	LexInit();
	Reserved = MakeReservedTable(tokentable);

	SQL_ParserInit();
}


static	char	***
ParKeyItem(
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
					printf("%s:%d:not in record item [%s]\n",CURR->fn,CURR->cLine,elm);
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
					Error("; not found");
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
	}
	if		(  ComToken  !=  '}'  ) {
		Error("} not found");
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
ParKey(
	RecordStruct	*rec)
{
	DB_Struct	*db;
	KeyStruct	*skey;

ENTER_FUNC;
	db = rec->opt.db;
	skey = New(KeyStruct);
	skey->item = ParKeyItem(rec->value);
	db->pkey = skey;
LEAVE_FUNC;
}

static	DB_Struct	*
InitDB_Struct(void)
{
	DB_Struct	*ret;

ENTER_FUNC;
	ret = New(DB_Struct);
	ret->paths = NewNameHash();
	ret->use = NewNameHash();
	ret->pkey = NULL;
	ret->path = NULL;
	ret->dbg = NULL;
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
	g_hash_table_insert(ret->opHash,op->name,(gpointer)(func+1));
}

static	PathStruct	*
InitPathStruct(void)
{
	PathStruct	*ret;

ENTER_FUNC;
	ret = New(PathStruct);
	ret->opHash = NewNameHash();
	ret->ops = (DB_Operation **)xmalloc(sizeof(DB_Operation *) * 5);
	InsertBuildIn(ret,"DBSELECT",DBOP_SELECT);
	InsertBuildIn(ret,"DBFETCH",DBOP_FETCH);
	InsertBuildIn(ret,"DBUPDATE",DBOP_UPDATE);
	InsertBuildIn(ret,"DBINSERT",DBOP_INSERT);
	InsertBuildIn(ret,"DBDELETE",DBOP_DELETE);
	ret->ocount = 5;
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
	if		(  g_hash_table_lookup(root->opt.db->use,name)  ==  NULL  ) {
		g_hash_table_insert(root->opt.db->use,name,rec);
	}
}

static	void
ParOperation(
	RecordStruct	*rec,
	PathStruct		*path)
{
	int		ix;
	DB_Operation	**ops
	,				*op;
	ValueStruct		*value;
	char			name[SIZE_SYMBOL+1];

ENTER_FUNC;
	if		(  ( ix = (int)g_hash_table_lookup(path->opHash,ComSymbol) )  ==  0  ) {
		ix = path->ocount;
		ops = (DB_Operation **)xmalloc(sizeof(DB_Operation *) * ( ix + 1 ));
		memcpy(ops,path->ops,(sizeof(DB_Operation *) * ix));
		xfree(path->ops);
		path->ops = ops;
		op = NewOperation(ComSymbol);
		g_hash_table_insert(path->opHash, op->name, (gpointer)(ix + 1));
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
			value = ParValueDefine();
			if		(  ComToken  ==  ','  ) {
				GetName;
			}
			SetValueAttribute(value,GL_ATTR_NULL);
			ValueAddRecordItem(op->args,name,value);
		}
		if		(  ComToken  ==  ')'  ) {
			GetSymbol;
		} else {
			Error(") missing");
		}
		SetReserved(Reserved); 
	}
	if		(  ComToken  == '{'  ) {
		if		(  op->proc  ==  NULL  ) {
			op->proc = ParSQL(rec,path->args,op->args);
			if		(  GetSymbol  !=  ';'  ) {
				Error("; missing");
			}
		} else {
			Error("function duplicate");
		}
	} else {
		Error("{ missing");
	}
LEAVE_FUNC;
}

static	void
ParPath(
	RecordStruct	*rec)
{
	int		pcount;
	PathStruct		**paths
		,			*path;
	ValueStruct		*value;
	char			name[SIZE_SYMBOL+1];

ENTER_FUNC;
	pcount = rec->opt.db->pcount;
	paths = (PathStruct **)xmalloc(sizeof(PathStruct *) * (pcount + 1));
	if		(  pcount  >  0  ) {
		memcpy(paths,rec->opt.db->path,(sizeof(PathStruct *) * pcount));
		xfree(rec->opt.db->path);
	}
	path = InitPathStruct();
	paths[pcount] = path;
	path->name = StrDup(ComSymbol);
	g_hash_table_insert(rec->opt.db->paths,path->name,(void *)(pcount+1));
	rec->opt.db->pcount ++;
	rec->opt.db->path = paths;
	if		(  GetSymbol  ==  '('  ) {
		path->args = NewValue(GL_TYPE_RECORD);
		GetName;
		while	(  ComToken  ==  T_SYMBOL  ) {
			strcpy(name,ComSymbol);
			value = ParValueDefine();
			if		(  ComToken  ==  ','  ) {
				GetName;
			}
			SetValueAttribute(value,GL_ATTR_NULL);
			ValueAddRecordItem(path->args,name,value);
		}
		if		(  ComToken  ==  ')'  ) {
			GetSymbol;
		} else {
			Error(") missing");
		}
		SetReserved(Reserved);
	}
	if		(  ComToken  !=  '{'  ) {
		Error("{ missing");
	}
	while	(  GetSymbol  !=  '}'  ) {
		switch	(ComToken) {
		  case	T_PROCEDURE:
			Error("not implemented");
			break;
		  case	T_OPERATION:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("invalid operation name");
			}
			/*	path thrue	*/
		  case	T_SYMBOL:
			ParOperation(rec,path);
			break;
		  default:
			Error("invalid token");
			break;
		}
	}
	if		(  GetSymbol  !=  ';'  ) {
		Error("; missing");
	}
LEAVE_FUNC;
}

static	RecordStruct	*
DB_Parse(
	char	*name)
{
	RecordStruct	*ret
	,				*use;
	char	buff[SIZE_LONGNAME+1];
	PathStruct		*path;

ENTER_FUNC;
	ret = DD_Parse();
	if		(  ret  ==  NULL  ) {
		exit(1);
    }
	if		(  !stricmp(strrchr(name,'.'),".db")  ) {
		ret->type = RECORD_DB;
		ret->opt.db = InitDB_Struct();
	} else {
		ret->type = RECORD_NULL;
	}
	SetReserved(Reserved); 
	while	(  	GetSymbol  !=  T_EOF  ) {
		switch	(ComToken) {
		  case	T_USE:
			switch	(GetName) {
			  case	T_SYMBOL:
				sprintf(buff,"%s.db",ComSymbol);
				use = ReadRecordDefine(buff);
				if		(  use  ==  NULL  ) {
					Error("define not found");
				} else {
					EnterUse(ret,use->name,use);
				}
				if		(  GetSymbol  !=  ';'  ) {
					Error("use statement error");
				}
				break;
			  default:
				Error("use invalid symbol");
				break;
			}
			break;
		  case	T_PRIMARY:
			if		(  ret->type  ==  RECORD_NULL  ) {
				ret->type = RECORD_DB;
				ret->opt.db = InitDB_Struct();
			}
			if		(  GetSymbol  == '{'  ) {
				ParKey(ret);
				if		(  GetSymbol  !=  ';'  ) {
					Error("; missing");
				}
			} else {
				Error("syntax error(PRIMARY)");
			}
			break;
		  case	T_PATH:
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("path name invalid");
			} else {
				ParPath(ret);
			}
			break;
		  default:
			Error("syntax error(other)");
			printf("token = [%X]\n",ComToken);
			exit(1);
			break;
		}
	}
	if		(	(  ret->type            ==  RECORD_DB  )
			&&	(  ret->opt.db->pcount  ==  0          ) ) {
		ret->opt.db->path = (PathStruct **)xmalloc(sizeof(PathStruct *) * 1);
		path = InitPathStruct();
		path->name = StrDup("primary");
		g_hash_table_insert(ret->opt.db->paths,path->name,(void *)1);
		ret->opt.db->pcount ++;
		ret->opt.db->path[0] = path;
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
						if		(  ( use = g_hash_table_lookup(root->opt.db->use,name) )
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
	char	*name)
{
	struct	stat	stbuf;
	RecordStruct	*ret;

ENTER_FUNC;
	if		(  stat(name,&stbuf)  ==  0  ) { 
		if		(  PushLexInfo(name,RecordDir,Reserved)  !=  NULL  ) {
			ret = DB_Parse(name);
			DropLexInfo();
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
