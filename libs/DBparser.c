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
#include	"hash.h"
#include	"value.h"
#include	"misc.h"
#include	"monstring.h"
#include	"Lex.h"
#include	"DBparser.h"
#include	"DDparser.h"
#include	"SQLparser.h"
#include	"debug.h"

static	GHashTable	*Reserved;

#define	T_PRIMARY		(T_YYBASE +1)
#define	T_PATH			(T_YYBASE +2)
#define	T_USE			(T_YYBASE +3)

static	TokenTable	tokentable[] = {
	{	"primary"	,T_PRIMARY	},
	{	"path"		,T_PATH		},
	{	"use"		,T_USE		},
	{	""			,0			}
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
ParKeyItem(void)
{
	char	**name
	,		**p;
	size_t	count
	,		rcount;
	char	***ret
	,		***r;

dbgmsg(">ParKeyItem");
	ret = NULL;
	rcount = 0;
	while	(  GetName  ==  T_SYMBOL  ) {
		name = NULL;
		count = 0;
		do {
			p = (char **)xmalloc(sizeof(char *) * ( count + 2 ));
			if		(  name  !=  NULL  ) {
				memcpy(p,name,sizeof(char **)*count);
				xfree(name);
			}
			name = p;
			name[count] = StrDup(ComSymbol);
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
dbgmsg("<ParKeyItem");
	return	(ret);
}

static	DB_Struct	*
ParKey(
	DB_Struct	*db)
{
	KeyStruct	*skey;

dbgmsg("<ParKey");
	skey = New(KeyStruct);
	skey->item = ParKeyItem();
	db->pkey = skey;
dbgmsg("<ParKey");
	return	(db);
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

static	PathStruct	*
InitPathStruct(void)
{
	PathStruct	*ret;

ENTER_FUNC;
	ret = New(PathStruct);
	ret->opHash = NewNameHash();
	ret->ops = (LargeByteString **)xmalloc(sizeof(LargeByteString *) * 5);
	ret->ops[DBOP_SELECT] = NULL;
	ret->ops[DBOP_FETCH ] = NULL;
	ret->ops[DBOP_UPDATE] = NULL;
	ret->ops[DBOP_INSERT] = NULL;
	ret->ops[DBOP_DELETE] = NULL;
	g_hash_table_insert(ret->opHash,"DBSELECT",(gpointer)(DBOP_SELECT+1));
	g_hash_table_insert(ret->opHash,"DBFETCH",(gpointer)(DBOP_FETCH+1));
	g_hash_table_insert(ret->opHash,"DBUPDATE",(gpointer)(DBOP_UPDATE+1));
	g_hash_table_insert(ret->opHash,"DBINSERT",(gpointer)(DBOP_INSERT+1));
	g_hash_table_insert(ret->opHash,"DBDELETE",(gpointer)(DBOP_DELETE+1));
	ret->ocount = 5;
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
	


static	RecordStruct	*
DB_Parse(
	char	*name)
{
	RecordStruct	*ret
	,				*use;
	int		ix
	,		pcount;
	PathStruct		**path;
	LargeByteString	**ops;

dbgmsg(">DB_Parse");
	ret = DD_Parse();
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
				if		(  GetName  ==  T_SYMBOL  ) {
					char	buff[SIZE_LONGNAME+1];
					sprintf(buff,"%s.db",ComSymbol);
					use = DB_Parser(name);
					EnterUse(ret,use->name,use);
				} else {
					Error("invalid token in use statement");
				}
				break;
			  case	'{':
				use = DB_Parse(name);
				EnterUse(ret,use->name,use);
				break;
			  default:
				Error("use invalid symbol");
				break;
			}
			break;
		  case	T_PRIMARY:
			dbgmsg("primary");
			if		(  ret->type  ==  RECORD_NULL  ) {
				ret->type = RECORD_DB;
				ret->opt.db = InitDB_Struct();
			}
			if		(  GetSymbol  == '{'  ) {
				ret->opt.db = ParKey(ret->opt.db);
				if		(  GetSymbol  !=  ';'  ) {
					Error("; missing");
				}
			} else {
				Error("syntax error(PRIMARY)");
			}
			break;
		  case	T_PATH:
			dbgmsg("path");
			if		(  GetName  !=  T_SYMBOL  ) {
				Error("path name invalid");
			}
			pcount = ret->opt.db->pcount;
			path = (PathStruct **)xmalloc(sizeof(PathStruct *) * (pcount + 1));
			if		(  pcount  >  0  ) {
				memcpy(path,ret->opt.db->path,(sizeof(PathStruct *) * pcount));
				xfree(ret->opt.db->path);
			}
			path[pcount] = InitPathStruct();
			path[pcount]->name = StrDup(ComSymbol);
			g_hash_table_insert(ret->opt.db->paths,path[pcount]->name,(void *)(pcount+1));
			ret->opt.db->pcount ++;
			ret->opt.db->path = path;
			if		(  GetSymbol  !=  '{'  ) {
				Error("{ missing");
			}
			while	(  GetName  ==  T_SYMBOL  ) {
				if		(  ( ix = (int)g_hash_table_lookup(
								 path[pcount]->opHash,ComSymbol) )  ==  0  ) {
					ix = path[pcount]->ocount;
					ops = (LargeByteString **)xmalloc(sizeof(LargeByteString *) * ( ix + 1 ));
					memcpy(ops,path[pcount]->ops,(sizeof(LargeByteString *) * ix));
					xfree(path[pcount]->ops);
					path[pcount]->ops = ops;
					g_hash_table_insert(path[pcount]->opHash,
										StrDup(ComSymbol),
										(gpointer)(ix + 1));
					path[pcount]->ocount ++;
				} else {
					ix --;
				}
				if		(  GetSymbol  == '{'  ) {
					path[pcount]->ops[ix] = ParSQL(ret);
					if		(  GetSymbol  !=  ';'  ) {
						Error("; missing");
					}
				} else {
					Error("{ missing");
				}
			}
			if		(  ComToken  ==  '}'  ) {
				if		(  GetSymbol  !=  ';'  ) {
					Error("; missing");
				}
			} else {
				Error("} missing");
			}
			break;
		  default:
			Error("syntax error(other)");
			printf("token = [%X]\n",ComToken);
			exit(1);
			break;
		}
	}
dbgmsg("<DB_Parse");
	return	(ret);
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
		} else {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}
