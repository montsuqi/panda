/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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

#define	_DD_PARSER

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<sys/stat.h>	/*	for stbuf	*/
#include	"types.h"
#include	"value.h"
#include	"misc.h"
#include	"DDlex.h"
#include	"DDparser.h"
#include	"SQLparser.h"
#include	"option.h"
#include	"debug.h"


static	ValueStruct	**MakeValueArray(ValueStruct *template, size_t count);

extern	ValueStruct	*
NewValue(
	PacketDataType	type)
{
	ValueStruct	*ret;

dbgmsg(">NewValue");
	ret = New(ValueStruct);
	ret->fUpdate = FALSE;
	ret->attr = GL_ATTR_NULL;
	ret->type = type;
	switch	(type) {
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		ret->body.CharData.len = 0;
		ret->body.CharData.sval = NULL;
		break;
	  case	GL_TYPE_NUMBER:
		ret->body.FixedData.flen = 0;
		ret->body.FixedData.slen = 0;
		ret->body.FixedData.sval = NULL;
		break;
	  case	GL_TYPE_INT:
		ret->body.IntegerData = 0;
		break;
	  case	GL_TYPE_BOOL:
		ret->body.BoolData = FALSE;
		break;
	  case	GL_TYPE_RECORD:
		ret->body.RecordData.count = 0;
		ret->body.RecordData.members = NewNameHash();
		ret->body.RecordData.item = NULL;
		ret->body.RecordData.names = NULL;
		break;
	  case	GL_TYPE_ARRAY:
		ret->body.ArrayData.count = 0;
		ret->body.ArrayData.item = NULL;
		break;
	  default:
		xfree(ret);
		ret = NULL;
		break;
	}
dbgmsg("<NewValue");
	return	(ret);
}

extern	void
ValueAddRecordItem(
	ValueStruct	*upper,
	char		*name,
	ValueStruct	*value)
{
	ValueStruct	**item;
	char		**names;

dbgmsg(">ValueAddRecordItem");
#ifdef	TRACE
	printf("name = [%s]\n",name); 
#endif
	item = (ValueStruct **)
		xmalloc(sizeof(ValueStruct *) * ( upper->body.RecordData.count + 1 ) );
	names = (char **)
		xmalloc(sizeof(char *) * ( upper->body.RecordData.count + 1 ) );
	if		(  upper->body.RecordData.count  >  0  ) {
		memcpy(item, upper->body.RecordData.item, 
			   sizeof(ValueStruct *) * upper->body.RecordData.count );
		memcpy(names, upper->body.RecordData.names, 
			   sizeof(char *) * upper->body.RecordData.count );
		xfree(upper->body.RecordData.item);
		xfree(upper->body.RecordData.names);
	}
	upper->body.RecordData.item = item;
	upper->body.RecordData.names = names;
	item[upper->body.RecordData.count] = value;
	names[upper->body.RecordData.count] = StrDup(name);
	if		(  name  !=  NULL  ) {
		if		(  g_hash_table_lookup(upper->body.RecordData.members,name)  ==  NULL  ) {
			g_hash_table_insert(upper->body.RecordData.members,
								(gpointer)names[upper->body.RecordData.count],
								(gpointer)upper->body.RecordData.count+1);
		} else {
			printf("name = [%s]\t",name);
			Error("name duplicate");
		}
	}
	upper->body.RecordData.count ++;
dbgmsg("<ValueAddRecordItem");
}

extern	ValueStruct	*
DuplicateValue(
	ValueStruct	*template)
{
	ValueStruct	*p;
	int			i;

	p = New(ValueStruct);
	memcpy(p,template,sizeof(ValueStruct));
	switch	(template->type) {
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		if		(  template->body.CharData.len  >  0  ) {
			p->body.CharData.sval = (char *)xmalloc(template->body.CharData.len+1);
			memclear(p->body.CharData.sval,template->body.CharData.len+1);
		}
		p->body.CharData.len = template->body.CharData.len;
		break;
	  case	GL_TYPE_NUMBER:
		p->body.FixedData.sval = (char *)xmalloc(template->body.FixedData.flen+1);
		memcpy(p->body.FixedData.sval,
			   template->body.FixedData.sval,
			   template->body.FixedData.flen+1);
		p->body.FixedData.flen = template->body.FixedData.flen;
		p->body.FixedData.slen = template->body.FixedData.slen;
		break;
	  case	GL_TYPE_ARRAY:
		p->body.ArrayData.item = 
			MakeValueArray(template->body.ArrayData.item[0],
						   template->body.ArrayData.count);
		p->body.ArrayData.count = template->body.ArrayData.count;
		break;
	  case	GL_TYPE_INT:
		p->body.IntegerData = 0;
		break;
	  case	GL_TYPE_BOOL:
		p->body.BoolData = FALSE;
		break;
	  case	GL_TYPE_RECORD:
		/*	share name table		*/
		p->body.RecordData.members = template->body.RecordData.members;
		p->body.RecordData.names = template->body.RecordData.names;
		/*	duplicate data space	*/
		p->body.RecordData.item = 
			(ValueStruct **)xmalloc(sizeof(ValueStruct *) * template->body.RecordData.count);
		p->body.RecordData.count = template->body.RecordData.count;
		for	( i = 0 ; i < template->body.RecordData.count ; i ++ ) {
			p->body.RecordData.item[i] = 
				DuplicateValue(template->body.RecordData.item[i]);
		}
		break;
	  default:
		break;
	}
	return	(p);
}

static	ValueStruct	**
MakeValueArray(
	ValueStruct	*template,
	size_t		count)
{
	ValueStruct	**ret;
	int			i;

	ret = (ValueStruct **)xmalloc(sizeof(ValueStruct *) * count);
	for	( i = 0 ; i < count ; i ++ ) {
		ret[i] = DuplicateValue(template);
	}
	return	(ret);
}

static	void
SetAttribute(
	ValueStruct	*val,
	ValueAttributeType	attr)
{
	int		i;

	val->attr |= attr;
	switch	(val->type) {
	  case	GL_TYPE_NUMBER:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_INT:
	  case	GL_TYPE_BOOL:
	  case	GL_TYPE_TEXT:
		break;
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < val->body.ArrayData.count ; i ++ ) {
			SetAttribute(val->body.ArrayData.item[i],attr);
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < val->body.RecordData.count ; i ++ ) {
			SetAttribute(val->body.RecordData.item[i],attr);
		}
		break;
	  default:
		break;
	}
}

static	void
_Error(
	char	*msg,
	char	*fn,
	int		line)
{
	printf("%s:%d:%s\n",fn,line,msg);
}
#undef	Error
#define	Error(msg)	{fDD_Error=TRUE;_Error((msg),DD_FileName,DD_cLine);}
#define	GetSymbol	(DD_Token = DD_Lex(FALSE))
#define	GetName		(DD_Token = DD_Lex(TRUE))

static	void
ParValue(
	ValueStruct	*upper)
{
	size_t		size
	,			ssize
	,			count;
	char		name[SIZE_SYMBOL+1];
	ValueStruct	*value
	,			*array;
	ValueAttributeType	attr;
	int			token;

dbgmsg(">ParValue");
	value = NULL;
	while	(  DD_Token  ==  T_SYMBOL  ) {
		attr = GL_ATTR_NULL;
		strcpy(name,DD_ComSymbol);
		switch	(GetSymbol) {
		  case	T_BYTE:
		  case	T_CHAR:
		  case	T_NUMBER:
		  case	T_DBCODE:
		  case	T_VARCHAR:
			token = DD_Token;
			size = 0;
			ssize = 0;
			if		(  GetSymbol  ==  '('  ) {
				if		(  GetSymbol  ==  T_ICONST  ) {
					size = DD_ComInt;
					if		(  token  ==  T_NUMBER  ) {
						switch	(GetSymbol) {
						  case	')':
							ssize = 0;
							break;
						  case	',':
							if		(  GetSymbol  ==  T_ICONST  ) {
								ssize = DD_ComInt;
							} else {
								Error("field size invalud");
							}
							GetSymbol;
							break;
						}
					} else {
						GetSymbol;
					}
					if		(  DD_Token  !=  ')'  ) {
						Error("not closed left paren");
					}
				} else {
					Error("invalid size definition");
				}
				GetSymbol;
			} else {
				size = 1;
			}
			if		(  !fDD_Error  ) {
				switch	(token) {
				  case	T_BYTE:
					value = NewValue(GL_TYPE_BYTE);
					break;
				  case	T_CHAR:
					value = NewValue(GL_TYPE_CHAR);
					break;
				  case	T_VARCHAR:
					value = NewValue(GL_TYPE_VARCHAR);
					break;
				  case	T_DBCODE:
					value = NewValue(GL_TYPE_DBCODE);
					break;
				  case	T_NUMBER:
					value = NewValue(GL_TYPE_NUMBER);
					break;
				  default:
					break;
				}
				if		(  value->type  ==  GL_TYPE_NUMBER  ) {
					value->body.FixedData.flen = size;
					value->body.FixedData.slen = ssize;
					value->body.FixedData.sval = (char *)xmalloc(size + 1);
					value->body.FixedData.sval[size] = 0;
					memset(value->body.FixedData.sval,'0',value->body.FixedData.flen);
				} else {
					value->body.CharData.len = size;
					value->body.CharData.sval = (char *)xmalloc(size + 1);
					memclear(value->body.CharData.sval,size + 1);
				}
			}
			break;
		  case	T_TEXT:
			value = NewValue(GL_TYPE_TEXT);
			GetSymbol;
			break;
		  case	T_INT:
			value = NewValue(GL_TYPE_INT);
			GetSymbol;
			break;
		  case	T_BOOL:
			value = NewValue(GL_TYPE_BOOL);
			GetSymbol;
			break;
		  case	'{':
			value = NewValue(GL_TYPE_RECORD);
			GetName;
			ParValue(value);
			break;
		  default:
			value = NULL;
			Error("not supported");
			break;
		}
		while	(  DD_Token  ==  '['  ) {	
			if		(  GetSymbol  == T_ICONST  ) {
				count = DD_ComInt;
				GetSymbol;
			} else {
				count = 0;
			}
			if		(  DD_Token  !=  ']'  ) {
				Error("unmatch lbracket");
				break;
			}
			GetSymbol;
			array = New(ValueStruct);
			array->type = GL_TYPE_ARRAY;
			array->attr = GL_ATTR_NULL;
			array->body.ArrayData.count = count;
			if		(  count  >  0  ) {
				array->body.ArrayData.item = MakeValueArray(value,count);
			} else {
				array->body.ArrayData.item = MakeValueArray(value,1);
			}
			value = array;
		}
		while	(  DD_Token  ==  ','  ) {
			switch	(GetSymbol) {
			  case	T_INPUT:
				attr |= GL_ATTR_INPUT;
				break;
			  case	T_OUTPUT:
				attr |= GL_ATTR_OUTPUT;
				break;
			  case	T_VIRTUAL:
				attr |= GL_ATTR_VIRTUAL;
				break;
			  default:
				Error("invalit attribute modifier");
				break;
			}
			GetSymbol;
		}
		if		(  DD_Token  ==  ';'  ) {
			GetSymbol;
			/*	OK	*/
		} else {
			Error("; missing");
		}
		if		(  !fDD_Error  ) {
			value->attr = GL_ATTR_NULL;
			SetAttribute(value,attr);
			ValueAddRecordItem(upper,name,value);
		}
	}
	if		(	(  !fDD_Error         )
			 &&	(  DD_Token  ==  '}'  ) ) {
		GetSymbol;
		/*	OK	*/
	} else {
		printf("token = %d [%c]\n",DD_Token,DD_Token);
		Error("syntax error");
	}
	if		(  value  ==  NULL  ) {
		value = NewValue(GL_TYPE_INT);
		value->attr = GL_ATTR_NULL;
		ValueAddRecordItem(upper,NULL,value);
	}
dbgmsg("<ParValue");
}

extern	void
DD_ParserInit(void)
{
	DD_LexInit();
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
			name[count] = StrDup(DD_ComSymbol);
			name[count+1] = NULL;
			count ++;
			GetSymbol;
			if		(  DD_Token  ==  '.'  ) {
				GetName;
			} else
			if		(  DD_Token  ==  ','  ) {
				break;
			} else
			if		(  DD_Token  !=  ';'  ) {
					Error("; not found");
			}
		}	while	(  DD_Token  ==  T_SYMBOL  );
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
	if		(  DD_Token  !=  '}'  ) {
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

extern	DB_Struct	*
InitDB_Struct(void)
{
	DB_Struct	*ret;

	ret = New(DB_Struct);
	ret->paths = NewNameHash();
	ret->pkey = NULL;
	ret->path = NULL;
	ret->dbg = NULL;
	ret->pcount = 0;

	return	(ret);
}

extern	PathStruct	*
InitPathStruct(void)
{
	PathStruct	*ret;

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
	return	(ret);
}

extern	RecordStruct	*
DD_ParserDataDefines(
	char	*name)
{
	FILE	*fp;
	RecordStruct	*ret;
	struct	stat	stbuf;
	int		ix
	,		pcount;
	PathStruct		**path;
	LargeByteString	**ops;

dbgmsg(">ParserDataDefines");
	if		(  stat(name,&stbuf)  ==  0  ) { 
		fDD_Error = FALSE;

		if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
			DD_FileName = StrDup(name);
			DD_cLine = 1;
			DD_File = fp;
			ret = New(RecordStruct);
			ret->rec = NULL;
			ret->name = NULL;
			if		(  !stricmp(GetExt(name),".db")  ) {
				ret->type = RECORD_DB;
				ret->opt.db = InitDB_Struct();
			} else {
				ret->type = RECORD_NULL;
			}
			while	(  	GetSymbol  !=  T_EOF  ) {
				switch	(DD_Token) {
				  case	T_SYMBOL:
					ret->name = StrDup(DD_ComSymbol);
					if		(  GetSymbol  == '{'  ) {
						ret->rec = NewValue(GL_TYPE_RECORD);
						ret->rec->attr = GL_ATTR_NULL;
						GetName;
						ParValue(ret->rec);
					} else {
						Error("syntax error");
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
						Error("syntax error");
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
					path[pcount]->name = StrDup(DD_ComSymbol);
					g_hash_table_insert(ret->opt.db->paths,path[pcount]->name,(void *)(pcount+1));
					ret->opt.db->pcount ++;
					ret->opt.db->path = path;
					if		(  GetSymbol  !=  '{'  ) {
						Error("{ missing");
					}
					while	(  GetName  ==  T_SYMBOL  ) {
						if		(  ( ix = (int)g_hash_table_lookup(
										 path[pcount]->opHash,DD_ComSymbol) )  ==  0  ) {
							ix = path[pcount]->ocount;
							ops = (LargeByteString **)xmalloc(sizeof(LargeByteString *) * ( ix + 1 ));
							memcpy(ops,path[pcount]->ops,(sizeof(LargeByteString *) * ix));
							xfree(path[pcount]->ops);
							path[pcount]->ops = ops;
							g_hash_table_insert(path[pcount]->opHash,
												StrDup(DD_ComSymbol),
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
					if		(  DD_Token  ==  '}'  ) {
						if		(  GetSymbol  !=  ';'  ) {
							Error("; missing");
						}
					} else {
						Error("} missing");
					}
					break;
				  default:
					Error("syntax error");
					printf("token = [%X]\n",DD_Token);
					break;
				}
			}
			fclose(DD_File);
			if		(  fDD_Error  ) {
				FreeValueStruct(ret->rec);
				if		(  ret->name  !=  NULL ) {
					xfree(ret->name);
				}
				xfree(ret);
				ret = NULL;
			}
		} else {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
dbgmsg("<ParserDataDefines");
	return	(ret);
}

