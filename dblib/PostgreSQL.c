/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<numeric.h>

#include	<libpq-fe.h>

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"SQLparser.h"
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"redirect.h"
#include	"debug.h"

static	int		level;
static	char	*rname[SIZE_RNAME];
static	int		alevel;
static	int		Dim[SIZE_RNAME];
static	Bool	fInArray;

static	void
SetValueOid(
	ValueStruct	*value,
	Oid			id)
{
	memclear(&ValueObjectID(value),sizeof(ValueObjectID(value)));
	memcpy(&ValueObjectID(value),&id,sizeof(Oid));
}

static	Oid
ValueOid(
	ValueStruct	*value)
{
	Oid	id;

	memcpy(&id,&ValueObjectID(value),sizeof(Oid));
	return	(id);
}

static	size_t
EncodeString(
	char	*p,
	char	del,
	char	*s)
{
	char	*q;

	q = p;
	while	(  *s  !=  0  ) {
		if		(  *s  ==  del  ) {
			*p ++ = del;
			*p ++ = del;
		} else {
			*p ++ = *s;
		}
		s ++;
	}
	*p = 0;
	return	(p-q);
}

static	char	*
ValueToSQL(
	DBG_Struct	*dbg,
	ValueStruct	*val)
{
	static	char	buff[SIZE_BUFF];
	char	str[SIZE_BUFF+1];
	Numeric	nv;
	char	del
	,		*p
	,		*s;
	Oid		id;

	if		(  IS_VALUE_NIL(val)  ) {
		sprintf(buff,"null");
	} else
	switch	(ValueType(val)) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_TEXT:
		if		(  fInArray  ) {
			del = '"';
		} else {
			del = '\'';
		}
		p = buff;
		*p ++ = del;
		p += EncodeString(p,del,ValueToString(val,dbg->coding));
		*p ++ = del;
		*p = 0;
		break;
	  case	GL_TYPE_DBCODE:
		p = buff;
		p += EncodeString(p,'\'',ValueToString(val,dbg->coding));
		*p = 0;
		break;
	  case	GL_TYPE_NUMBER:
		nv = FixedToNumeric(&ValueFixed(val));
		sprintf(buff,"%s",NumericOutput(nv));
		NumericFree(nv);
		break;
	  case	GL_TYPE_INT:
		sprintf(buff,"%d",ValueToInteger(val));
		break;
	  case	GL_TYPE_FLOAT:
		sprintf(buff,"%g",ValueToFloat(val));
		break;
	  case	GL_TYPE_BOOL:
		sprintf(buff,"'%s'",ValueToBool(val) ? "t" : "f");
		break;
	  case	GL_TYPE_OBJECT:
		id = ValueOid(val);
		sprintf(buff,"%u",id);
		break;
	  default:
		*buff = 0;
	}
	return	(buff);
}

static	char	*
KeyValue(
	DBG_Struct	*dbg,
	ValueStruct	*args,
	char		**pk)
{
	ValueStruct	*val;

dbgmsg(">KeyValue");
	val = args;
	while	(  *pk  !=  NULL  ) {
		val = GetRecordItem(args,*pk);
		pk ++;
	}
dbgmsg("<KeyValue");
	return	(ValueToSQL(dbg,val));
}

static	char	*
ItemName(void)
{
	static	char	buff[SIZE_BUFF];
	char	*p;
	int		i;

	p = buff;
	if		(  level  >  1  ) {
		for	( i = 0 ; i < level - 1 ; i ++ ) {
			p += sprintf(p,"%s_",rname[i]);
		}
	}
	p += sprintf(p,"%s",rname[level-1]);
#ifdef	TRACE
	printf("item = [%s]\n",buff);
#endif
	return	(buff);
}

static	char	*
ParArray(
	DBG_Struct	*dbg,
	char		*p,
	ValueStruct	*val)
{
	ValueStruct	*item;
	Bool	fMinus;
	size_t	len;
	char	*pp
	,		*q
	,		*qq;
	Oid		id;
	int		i
	,		j;
	int		ival;

	if		(  *p  ==  '{'  ) {
		p = ParArray(dbg,p+1,val);
		if		(  *p  ==  '}'  ) {
			p ++;
		}
	} else {
		for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
			item = ValueArrayItem(val,i);
			switch	(ValueType(item)) {
			  case	GL_TYPE_INT:
				if		(  *p  ==  '-'  ) {
					fMinus = TRUE;
					p ++;
				} else {
					fMinus = FALSE;
				}
				ival = 0;
				while	(  isdigit(*p)  ) {
					ival *= 10;
					ival += ( *p - '0' );
					p ++;
				}
				ival = ( fMinus ) ? -ival : ival;
				SetValueInteger(item,ival);
				break;
			  case	GL_TYPE_OBJECT:
				id = 0;
				while	(  isdigit(*p)  ) {
					id *= 10;
					id += ( *p - '0' );
					p ++;
				}
				SetValueOid(item,id);
				break;
			  case	GL_TYPE_BOOL:
				SetValueBool(item,*p);
				p ++;
				break;
			  case	GL_TYPE_BYTE:
			  case	GL_TYPE_CHAR:
			  case	GL_TYPE_VARCHAR:
				if		(  *p  ==  '"'  ) {
					p ++;
					q = ValueToString(item,dbg->coding);
					len = 0;
					while	(  *p  !=  '"'  ) {
						if		(  len  <  ValueStringLength(item)  ) {
							len ++;
							*q ++ = *p;
						}
						p ++;
					}
					*q = 0;
					p ++;
				}
				break;
			  case	GL_TYPE_TEXT:
				pp = p;
				if		(  *p  ==  '"'  ) {
					p ++;
					len = 0;
					while	(  *p  !=  '"'  ) {
						p ++;
						len ++;
					}
				} else break;
				qq = (char *)xmalloc(len+1);
				p = pp + 1;
				q = qq;
				while	(  *p  !=  '"'  ) {
					*q ++ = *p ++;
				}
				*q = 0;
				p ++;
				SetValueString(item,qq,dbg->coding);
				xfree(qq);
				break;
			  case	GL_TYPE_ARRAY:
				p = ParArray(dbg,p,item);
				break;
			  case	GL_TYPE_RECORD:
				for	( j = 0 ; j < ValueRecordSize(item) ; j ++ ) {
					p = ParArray(dbg,p,ValueRecordItem(item,i));
				}
				break;
			  case	GL_TYPE_DBCODE:
			  default:
				break;
			}
			while	(  isspace(*p)  )	p ++;
			if		(  *p  ==  ','  )	p ++;
			while	(  isspace(*p)  )	p ++;
		}
	}
	return	(p);
}

static	void
GetTable(
	DBG_Struct	*dbg,
	PGresult	*res,
	ValueStruct	*val)
{
	int		i;
	ValueStruct	*tmp;
	int		fnum;
	Numeric	nv;
	char	*str;
	Oid		id;

dbgmsg(">GetTable");
	if		(  val  ==  NULL  )	return;

	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
		dbgmsg("int");
		fnum = PQfnumber(res,ItemName());
		if		(  fnum  <  0  ) {
			if		(  !IS_VALUE_VIRTUAL(val)  ) {
				ValueIsNil(val);
			}
		} else {
			SetValueInteger(val,atoi((char *)PQgetvalue(res,0,fnum)));
		}
		break;
	  case	GL_TYPE_BOOL:
		dbgmsg("bool");
		fnum = PQfnumber(res,ItemName());
		if		(  fnum  <  0  ) {
			if		(  !IS_VALUE_VIRTUAL(val)  ) {
				ValueIsNil(val);
			}
		} else {
			SetValueBool(val,*(char *)PQgetvalue(res,0,fnum));
		}
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		dbgmsg("char");
		fnum = PQfnumber(res,ItemName());
		if		(  fnum  <  0  ) {
			if		(  !IS_VALUE_VIRTUAL(val)  ) {
				ValueIsNil(val);
			}
		} else {
			SetValueString(val,(char *)PQgetvalue(res,0,fnum),dbg->coding);
		}
		break;
	  case	GL_TYPE_NUMBER:
		dbgmsg("number");
		fnum = PQfnumber(res,ItemName());
		if		(  fnum  <  0  ) {
			if		(  !IS_VALUE_VIRTUAL(val)  ) {
				ValueIsNil(val);
			}
		} else {
			nv = NumericInput((char *)PQgetvalue(res,0,fnum),
						  ValueFixedLength(val),ValueFixedSlen(val));
			str = NumericToFixed(nv,ValueFixedLength(val),ValueFixedSlen(val));
			strcpy(ValueFixedBody(val),str);
			xfree(str);
			NumericFree(nv);
		}
		break;
	  case	GL_TYPE_ARRAY:
		dbgmsg("array");
		fnum = PQfnumber(res,ItemName());
		if		(  fnum  <  0  ) {
			if		(  !IS_VALUE_VIRTUAL(val)  ) {
				ValueIsNil(val);
			}
		} else {
			ParArray(dbg,PQgetvalue(res,0,fnum),val);
		}
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		dbgmsg(">record");
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			rname[level-1] = ValueRecordName(val,i);
			GetTable(dbg,res,tmp);
		}
		dbgmsg("<record");
		level --;
		break;
	  case	GL_TYPE_OBJECT:
		dbgmsg("object");
		fnum = PQfnumber(res,ItemName());
		if		(  fnum  <  0  ) {
			if		(  !IS_VALUE_VIRTUAL(val)  ) {
				ValueIsNil(val);
			}
		} else {
			id = (Oid)atol((char *)PQgetvalue(res,0,fnum));
			SetValueOid(val,id);
		}
		break;
	  case	GL_TYPE_ALIAS:
		printf("invalid data type\n");
		break;
	  default:
		break;
	}
dbgmsg("<GetTable");
}

static	char	*
PutDim(void)
{
	static	char	buff[SIZE_NAME+1];
	int		i;
	char	*p;

	p = buff;
	*p = 0;
	for	( i = 0 ; i < alevel ; i ++ ) {
		p += sprintf(p,"[%d]",Dim[i]+1);
	}
	return	(buff);
}

static	char	*
UpdateValue(
	DBG_Struct	*dbg,
	char		*p,
	ValueStruct	*val)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;

	if		(  val  ==  NULL  )	return	(p);
	if		(  IS_VALUE_NIL(val)  ) {
		p += sprintf(p,"%s%s is null",ItemName(),PutDim());
	} else
	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
	  case	GL_TYPE_BOOL:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_NUMBER:
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_OBJECT:
		p += sprintf(p,"%s%s = %s",ItemName(),PutDim(),ValueToSQL(dbg,val));
		break;
	  case	GL_TYPE_ARRAY:
		fComm = FALSE;
		for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
			tmp = ValueArrayItem(val,i);
			if		(  ( tmp->attr & GL_ATTR_VIRTUAL )  !=  GL_ATTR_VIRTUAL  ) {
				if		(  fComm  ) {
					p += sprintf(p,",");
				}
				fComm = TRUE;
				Dim[alevel] = i;
				alevel ++;
				p = UpdateValue(dbg,p,tmp);
				alevel --;
			}
		}
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			if		(  ( tmp->attr & GL_ATTR_VIRTUAL )  !=  GL_ATTR_VIRTUAL  ) {
				if		(  fComm  ) {
					p += sprintf(p,",");
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				p = UpdateValue(dbg,p,tmp);
			}
		}
		level --;
		break;
	  default:
		break;
	}
	return	(p);
}

static	char	*
InsertNames(
	char		*p,
	ValueStruct	*val)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;

	if		(  val  ==  NULL  ) {
	} else
	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
	  case	GL_TYPE_BOOL:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_NUMBER:
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_OBJECT:
	  case	GL_TYPE_ARRAY:
		p += sprintf(p,"%s%s",ItemName(),PutDim());
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			if		(  ( tmp->attr & GL_ATTR_VIRTUAL )  !=  GL_ATTR_VIRTUAL  ) {
				if		(  fComm  ) {
					p += sprintf(p,",");
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				p = InsertNames(p,tmp);
			}
		}
		level --;
		break;
	  default:
		break;
	}
	return	(p);
}

static	char	*
InsertValues(
	DBG_Struct	*dbg,
	char		*p,
	ValueStruct	*val)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;

	if		(  val  ==  NULL  )	return	(p);
	if		(  IS_VALUE_NIL(val)  ) {
		p += sprintf(p,"null");
	} else
	switch	(ValueType(val)) {
	  case	GL_TYPE_INT:
	  case	GL_TYPE_BOOL:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_NUMBER:
	  case	GL_TYPE_OBJECT:
	  case	GL_TYPE_TEXT:
		p += sprintf(p,"%s",ValueToSQL(dbg,val));
		break;
	  case	GL_TYPE_ARRAY:
		p += sprintf(p,"'{");
		fInArray = TRUE;
		fComm = FALSE;
		for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
			tmp = ValueArrayItem(val,i);
			if		(  !IS_VALUE_VIRTUAL(tmp)  )	{
				if		(  fComm  ) {
					p += sprintf(p,",");
				}
				fComm = TRUE;
				Dim[alevel] = i;
				alevel ++;
				p = InsertValues(dbg,p,tmp);
				alevel --;
			}
		}
		fInArray = FALSE;
		p += sprintf(p,"}' ");
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			if		(  !IS_VALUE_VIRTUAL(tmp)  )	{
				if		(  fComm  ) {
					p += sprintf(p,", ");
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				p = InsertValues(dbg,p,tmp);
			}
		}
		level --;
		break;
	  default:
		break;
	}
	return	(p);
}

static	void
_PQclear(
	PGresult	*res)
{
	if		(  res  !=  NULL  ) {
		PQclear(res);
	}
}

static	PGresult	*
_PQexec(
	DBG_Struct	*dbg,
	char	*sql,
	Bool	fRed)
{
	PGresult	*res;

dbgmsg(">_PQexec");
#ifdef	TRACE
	printf("%s;\n",sql);fflush(stdout);
#endif
	res = PQexec((PGconn *)dbg->conn,sql);
	if		(  fRed  ) {
		PutDB_Redirect(dbg,sql);
	}
dbgmsg("<_PQexec");
	return	(res);
}

static	void
GetValue(
	DBG_Struct	*dbg,
	PGresult	*res,
	int			tnum,
	int			fnum,
	ValueStruct	*val)
{
	Numeric	nv;
	char	*str;

	if		(  val  ==  NULL  )	return;

dbgmsg(">GetValue");
	if		(  PQgetisnull(res,tnum,fnum)  ==  1  ) { 	/*	null	*/
		ValueIsNil(val);
	} else {
		ValueIsNonNil(val);
		switch	(ValueType(val)) {
		  case	GL_TYPE_INT:
			SetValueInteger(val,atoi((char *)PQgetvalue(res,tnum,fnum)));
			break;
		  case	GL_TYPE_OBJECT:
			SetValueOid(val,(Oid)atol((char *)PQgetvalue(res,tnum,fnum)));
			break;
		  case	GL_TYPE_BOOL:
			SetValueBool(val,*(char *)PQgetvalue(res,tnum,fnum));
			break;
		  case	GL_TYPE_BYTE:
		  case	GL_TYPE_CHAR:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
		  case	GL_TYPE_TEXT:
			SetValueString(val,(char *)PQgetvalue(res,tnum,fnum),dbg->coding);
			break;
		  case	GL_TYPE_NUMBER:
			nv = NumericInput((char *)PQgetvalue(res,tnum,fnum),
							  ValueFixedLength(val),ValueFixedSlen(val));
			str = NumericToFixed(nv,ValueFixedLength(val),ValueFixedSlen(val));
			strcpy(ValueFixedBody(val),str);
			xfree(str);
			NumericFree(nv);
			break;
		  case	GL_TYPE_ARRAY:
			ParArray(dbg,PQgetvalue(res,tnum,fnum),val);
			break;
		  case	GL_TYPE_RECORD:
			break;
		  default:
			break;
		}
	}
dbgmsg("<GetValue");
}

static	void
ExecPGSQL(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	LargeByteString	*src,
	ValueStruct		*args)
{
	char	sql[SIZE_SQL+1]
	,		*p;
	int		c;
	ValueStruct	*val;
	PGresult	*res;
	ValueStruct	**tuple;
	int		n
	,		i
	,		j
	,		items
	,		ntuples;
	ExecStatusType	status;
	Bool	fIntoAster;

dbgmsg(">ExecPGSQL");
	dbg =  rec->opt.db->dbg;
	p = sql;
	if	(  src  ==  NULL )	{
		printf("function \"%s\" is not found.\n",ctrl->func);
		exit(1);
	}
	RewindLBS(src);
	items = 0;
	tuple = NULL;
	fIntoAster = FALSE;
	while	(  ( c = LBS_FetchByte(src) )  >=  0  ) {
		if		(  c  !=  SQL_OP_ESC  ) {
			p += sprintf(p,"%c",c);
		} else {
			c = LBS_FetchByte(src);
			switch	(c) {
			  case	SQL_OP_INTO:
				n = LBS_FetchInt(src);
				if		(  n  >  0  ) {
					if		(  tuple  !=  NULL  ) {
						xfree(tuple);
					}
					tuple = (ValueStruct **)xmalloc(sizeof(ValueStruct *) * n);
					items = 0;
					fIntoAster = FALSE;
				} else {
					fIntoAster = TRUE;
				}
				break;
			  case	SQL_OP_STO:
				if		(  !fIntoAster  ) {
					tuple[items] = (ValueStruct *)LBS_FetchPointer(src);
					items ++;
				}
				break;
			  case	SQL_OP_REF:
				val = (ValueStruct *)LBS_FetchPointer(src);
				p = InsertValues(dbg,p,val);
				break;
			  case	SQL_OP_VCHAR:
				break;
			  case	SQL_OP_EOL:
				*p = 0;
				res = _PQexec(dbg,sql,TRUE);
				p = sql;
				if		(	(  res ==  NULL  )
						||	(  ( status = PQresultStatus(res) )
							           ==  PGRES_BAD_RESPONSE    )
						||	(  status  ==  PGRES_FATAL_ERROR     )
						||	(  status  ==  PGRES_NONFATAL_ERROR  ) ) {
					dbgmsg("NG");
					ctrl->rc = - status;
					_PQclear(res);
					break;
				} else {
					switch	(status) {
					  case	PGRES_TUPLES_OK:
						if		(  fIntoAster  ) {
							if		(  ( n = PQntuples(res) )  >  0  ) {
								dbgmsg("OK");
								level = 0;
								alevel = 0;
								GetTable(dbg,res,args);
								ctrl->rc += MCP_OK;
							} else {
								dbgmsg("EOF");
								ctrl->rc += MCP_EOF;
							}
						} else
						if		(	(  items  ==  0     )
								||	(  tuple  ==  NULL  ) ) {
							dbgmsg("SQL error");
							ctrl->rc = MCP_BAD_SQL;
						} else
						if		(  ( ntuples = PQntuples(res) )  >  0  ) {
							dbgmsg("+");
							for	( i = 0 ; i < ntuples ; i ++ ) {
								for	( j = 0 ; j < items ; j ++ ) {
									GetValue(dbg,res,i,j,tuple[j]);
								}
							}
							ctrl->rc += MCP_OK;
						} else {
							dbgmsg("EOF");
							ctrl->rc += MCP_EOF;
						}
						if		(  tuple  !=  NULL  ) {
							xfree(tuple);
							tuple = NULL;
						}
						break;
					  case	PGRES_COMMAND_OK:
					  case	PGRES_COPY_OUT:
					  case	PGRES_COPY_IN:
						dbgmsg("OK");
						ctrl->rc += MCP_OK;
						break;
					  case	PGRES_EMPTY_QUERY:
					  case	PGRES_NONFATAL_ERROR:
					  default:
						dbgmsg("NONFATAL");
						ctrl->rc =+ MCP_NONFATAL;
						break;
					}
					_PQclear(res);
				}
				break;
			  default:
				dbgprintf("[%X]\n",c);
				break;
			}
		}
	}
	if		(  tuple  !=  NULL  ) {
		dbgmsg("exception free");
		xfree(tuple);
	}
dbgmsg("<ExecPGSQL");
}

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql)
{
	PGresult	*res;
	ExecStatusType	status;
	int			rc
	,			n;

	res = _PQexec(dbg,sql,TRUE);
	if		(	(  res ==  NULL  )
			||	(  ( status = PQresultStatus(res) )
				   ==  PGRES_BAD_RESPONSE    )
			||	(  status  ==  PGRES_FATAL_ERROR     )
			||	(  status  ==  PGRES_NONFATAL_ERROR  ) ) {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
		_PQclear(res);
	} else {
		switch	(status) {
		  case	PGRES_TUPLES_OK:
			if		(  ( n = PQntuples(res) )  >  0  ) {
				rc = MCP_OK;
			} else {
				rc = MCP_EOF;
				_PQclear(res);
			}
			break;
		  case	PGRES_COMMAND_OK:
		  case	PGRES_COPY_OUT:
		  case	PGRES_COPY_IN:
			rc = MCP_OK;
			break;
		  case	PGRES_EMPTY_QUERY:
		  case	PGRES_NONFATAL_ERROR:
		  default:
			rc = MCP_NONFATAL;
			_PQclear(res);
			break;
		}
	}
	return	(rc);
}

static	void
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	char	*host;
	char	*port
	,		*user
	,		*dbname
	,		*pass;
	PGconn	*conn;

dbgmsg(">_DBOPEN");
	if		(  DB_Host  !=  NULL  ) {
		host = DB_Host;
	} else {
		if		(  dbg->port  ==  NULL  ) {
			host = NULL;
		} else {
			host = dbg->port->host;
		}
	}
	if		(  DB_Port  !=  NULL  ) {
		port = DB_Port;
	} else {
		port = dbg->port->port;
	}
	user =  ( DB_User != NULL ) ? DB_User : dbg->user;
	dbname = ( DB_Name != NULL ) ? DB_Name : dbg->dbname;
	pass = ( DB_Pass != NULL ) ? DB_Pass : dbg->pass;
	conn = PQsetdbLogin(host,port,NULL,NULL,dbname,user,pass);
	if		(  PQstatus(conn)  !=  CONNECTION_OK  ) {
		fprintf(stderr,"Connection to database \"%s\" failed.\n",dbname);
		fprintf(stderr,"%s.\n",PQerrorMessage(conn));
		exit(1);
	}
	dbg->conn = (void *)conn;
	OpenDB_RedirectPort(dbg);
	dbg->fConnect = TRUE;
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
	}
dbgmsg("<_DBOPEN");
}

static	void
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
dbgmsg(">_DBDISCONNECT");
	if		(  dbg->fConnect  ) { 
		PQfinish((PGconn *)dbg->conn);
		CloseDB_RedirectPort(dbg);
		dbg->fConnect = FALSE;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
dbgmsg("<_DBDISCONNECT");
}

static	void
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	PGresult	*res;
	int			rc;

dbgmsg(">_DBSTART");
	BeginDB_Redirect(dbg); 
	res = _PQexec(dbg,"begin",FALSE);
	if		(	(  res ==  NULL  )
			||	(  PQresultStatus(res)  !=  PGRES_COMMAND_OK  ) ) {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
	} else {
		dbgmsg("OK");
		rc = MCP_OK;
	}
	_PQclear(res);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
dbgmsg("<_DBSTART");
}

static	void
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	PGresult	*res;
	int			rc;

dbgmsg(">_DBCOMMIT");
	res = _PQexec(dbg,"commit work",FALSE);
	if		(	(  res ==  NULL  )
			||	(  PQresultStatus(res)  !=  PGRES_COMMAND_OK  ) ) {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
	} else {
		dbgmsg("OK");
		rc = MCP_OK;
	}
	_PQclear(res);
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
dbgmsg("<_DBCOMMIT");
}

static	void
_DBSELECT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DB_Struct	*db;
	PathStruct	*path;
	LargeByteString	*src;

dbgmsg(">_DBSELECT");

	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_OK;
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_SELECT]->proc;
		ExecPGSQL(dbg,ctrl,rec,src,args);
	}
dbgmsg("<_DBSELECT");
}

static	void
_DBFETCH(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	char	sql[SIZE_SQL+1]
	,		*p;
	DB_Struct	*db;
	PathStruct	*path;
	PGresult	*res;
	int			n;
	LargeByteString	*src;

dbgmsg(">_DBFETCH");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_FETCH]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,rec,src,args);
		} else {
			p = sql;
			p += sprintf(p,"fetch from %s_%s_csr",rec->name,path->name);
			res = _PQexec(dbg,sql,TRUE);
			if		(	(  res ==  NULL  )
					||	(  PQresultStatus(res)  !=  PGRES_TUPLES_OK  ) ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				ctrl->rc = MCP_OK;
				if		(  ( n = PQntuples(res) )  >  0  ) {
					dbgmsg("OK");
					level = 0;
					alevel = 0;
					GetTable(dbg,res,args);
					ctrl->rc = MCP_OK;
				} else {
					dbgmsg("EOF");
					ctrl->rc = MCP_EOF;
				}
			}
			_PQclear(res);
		}
	}
dbgmsg("<_DBFETCH");
}

static	void
_DBUPDATE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	char	sql[SIZE_SQL+1]
	,		*p;
	char	name[SIZE_NAME+1]
	,		*q;
	DB_Struct	*db;
	char	***item
	,		**pk;
	PGresult	*res;
	PathStruct	*path;
	LargeByteString	*src;

dbgmsg(">_DBUPDATE");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_UPDATE]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,rec,src,args);
		} else {
			p = sql;
			p += sprintf(p,"UPDATE %s\tSET ",rec->name);
			level = 0;
			alevel = 0;
			fInArray = FALSE;
			p = UpdateValue(dbg,p,args);

			p += sprintf(p,"WHERE\t");
			item = db->pkey->item;
			while	(  *item  !=  NULL  ) {
				pk = *item;
				q = name;
				while	(  *pk  !=  NULL  ) {
					q += sprintf(q,"%s",*pk);
					pk ++;
					if		(  *pk  !=  NULL  ) {
						q += sprintf(q,"_");
					}
				}
				p += sprintf(p,"%s.%s = %s ",rec->name,name,KeyValue(dbg,args,*item));
				item ++;
				if		(  *item  !=  NULL  ) {
					p += sprintf(p,"and\t");
				}
			}
			res = _PQexec(dbg,sql,TRUE);
			if		(	(  res ==  NULL  )
						||	(  PQresultStatus(res)  !=  PGRES_COMMAND_OK  ) ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				dbgmsg("OK");
				ctrl->rc = MCP_OK;
			}
			_PQclear(res);
		}
	}
dbgmsg("<_DBUPDATE");
}

static	void
_DBDELETE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	char	sql[SIZE_SQL+1]
	,		*p;
	char	name[SIZE_NAME+1]
	,		*q;
	DB_Struct	*db;
	char	***item
	,		**pk;
	PGresult	*res;
	PathStruct	*path;
	LargeByteString	*src;

dbgmsg(">_DBDELETE");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_DELETE]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,rec,src,args);
		} else {
			p = sql;
			p += sprintf(p,"delete\tfrom\t%s ",rec->name);
			p += sprintf(p,"where\t");
			item = db->pkey->item;
			while	(  *item  !=  NULL  ) {
				pk = *item;
				q = name;
				while	(  *pk  !=  NULL  ) {
					q += sprintf(q,"%s",*pk);
					pk ++;
					if		(  *pk  !=  NULL  ) {
						q += sprintf(q,"_");
					}
				}
				p += sprintf(p,"%s.%s = %s ",rec->name,name,
							 KeyValue(dbg,args,*item));
				item ++;
				if		(  *item  !=  NULL  ) {
					p += sprintf(p,"and\t");
				}
			}
			res = _PQexec(dbg,sql,TRUE);
			if		(	(  res ==  NULL  )
					||	(  PQresultStatus(res)  !=  PGRES_COMMAND_OK  ) ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				dbgmsg("OK");
				ctrl->rc = MCP_OK;
			}
			_PQclear(res);
		}
	}
dbgmsg("<_DBDELETE");
}

static	void
_DBINSERT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	char	sql[SIZE_SQL+1]
	,		*p;
	DB_Struct	*db;
	PGresult	*res;
	PathStruct	*path;
	LargeByteString	*src;

dbgmsg(">_DBINSERT");
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_INSERT]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,rec,src,args);
		} else {
			p = sql;
			p += sprintf(p,"INSERT\tINTO\t%s (",rec->name);

			level = 0;
			alevel = 0;
			p = InsertNames(p,args);
			p += sprintf(p,") ");
			p += sprintf(p,"VALUES\t(");
			fInArray = FALSE;
			p = InsertValues(dbg,p,args);
			p += sprintf(p,") ");
			res = _PQexec(dbg,sql,TRUE);
			if		(	(  res ==  NULL  )
					||	(  PQresultStatus(res)  !=  PGRES_COMMAND_OK  ) ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				dbgmsg("OK");
				ctrl->rc = MCP_OK;
			}
			_PQclear(res);
		}
	}
dbgmsg("<_DBINSERT");
}

static	Bool
_DBACCESS(
	DBG_Struct		*dbg,
	char			*name,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	DB_Struct	*db;
	PathStruct	*path;
	LargeByteString	*src;
	int		ix;
	Bool	rc;

dbgmsg(">_DBACCESS");
#ifdef	TRACE
	printf("[%s]\n",name); 
#endif
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
		rc = TRUE;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		if		(  ( ix = (int)g_hash_table_lookup(path->opHash,name) )  ==  0  ) {
			rc = FALSE;
		} else {
			src = path->ops[ix-1]->proc;
			if		(  src  !=  NULL  ) {
				ctrl->rc = MCP_OK;
				ExecPGSQL(dbg,ctrl,rec,src,args);
				rc = TRUE;
			} else {
				rc = FALSE;
			}
		}
	}
dbgmsg("<_DBACCESS");
	return	(rc);
}

DB_OPS	PostgresOperations[] = {
	{	"exec",			(DB_FUNC)_EXEC	},
	/*	DB operations		*/
	{	"access",		(DB_FUNC)_DBACCESS	},
	{	"DBOPEN",		(DB_FUNC)_DBOPEN },
	{	"DBDISCONNECT",	(DB_FUNC)_DBDISCONNECT	},
	{	"DBSTART",		(DB_FUNC)_DBSTART },
	{	"DBCOMMIT",		(DB_FUNC)_DBCOMMIT },
	/*	table operations	*/
	{	"DBSELECT",		_DBSELECT },
	{	"DBFETCH",		_DBFETCH },
	{	"DBUPDATE",		_DBUPDATE },
	{	"DBDELETE",		_DBDELETE },
	{	"DBINSERT",		_DBINSERT },

	{	NULL,			NULL }
};

extern	DB_Func	*
InitPostgreSQL(void)
{
	return	(EnterDB_Function("PostgreSQL",PostgresOperations,"/*","*/\t"));
}

