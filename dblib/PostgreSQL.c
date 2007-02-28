/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
*/
#define	DEBUG
#define	TRACE

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_POSTGRES
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<numeric.h>

#include	<libpq-fe.h>
#include	<libpq/libpq-fs.h>

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"SQLparser.h"
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"directory.h"
#include	"redirect.h"
#include	"debug.h"

static	int		level;
static	char	*rname[SIZE_RNAME];
static	int		alevel;
static	int		Dim[SIZE_RNAME];
static	Bool	fInArray;

#define	PGCONN(dbg)		((PGconn *)(dbg)->conn)

/*	This code depends on sizeof(Oid).	*/
static	void
SetValueOid(
	ValueStruct	*value,
	DBG_Struct	*dbg,
	Oid			id)
{
	ValueObjectId(value) = (uint64_t)id;
}

static	Oid
ValueOid(
	MonObjectType	obj)
{
	Oid	id;

	id = (Oid)obj;
	return	(id);
}
/**/

static	void
EscapeString(LargeByteString *lbs, char *s)
{
	unsigned char *sp;
	unsigned char *dp;
	size_t len;
    size_t old_size;

	len = 0;
	for (sp = s; *sp != '\0'; sp++) {
		switch(*sp) {
          case '\\':
          case '\'':
            len += 2;
            break;
          default:
			len++;
            break;
        }
	}

    old_size = LBS_Size(lbs);
    LBS_ReserveSize(lbs, old_size + len, TRUE);
    dp = LBS_Body(lbs) + old_size;

	for (sp = s; *sp != '\0'; sp++) {
		switch(*sp) {
          case '\\':
            *dp++ = '\\';
            *dp++ = '\\';
            break;
          case '\'':
            *dp++ = '\'';
            *dp++ = '\'';
            break;
          default:
			*dp++ = *sp;
            break;
        }
	}
    LBS_SetPos(lbs, old_size + len);
}

static	void
EscapeStringInArray(LargeByteString *lbs, char *s)
{
	unsigned char *sp;
	unsigned char *dp;
	size_t len;
    size_t old_size;

	len = 0;
	for (sp = s; *sp != '\0'; sp++) {
		switch(*sp) {
          case '\\':
            len += 4;
            break;
          case '"':
            len += 3;
            break;
          case '\'':
            len += 2;
            break;
          default:
			len++;
            break;
        }
	}

    old_size = LBS_Size(lbs);
    LBS_ReserveSize(lbs, old_size + len, TRUE);
    dp = LBS_Body(lbs) + old_size;

	for (sp = s; *sp != '\0'; sp++) {
		switch(*sp) {
          case '\\':
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            break;
          case '"':
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '"';
            break;
          case '\'':
            *dp++ = '\'';
            *dp++ = '\'';
            break;
          default:
			*dp++ = *sp;
            break;
        }
	}
    LBS_SetPos(lbs, old_size + len);
}

static void
EscapeBytea(LargeByteString *lbs, unsigned char *bintext, size_t binlen)
{
    unsigned char *sp, *spend = bintext + binlen;
    unsigned char *dp;
    size_t len;
    size_t old_size;

    len = 0;
    for (sp = bintext; sp < spend; sp++) {
        if (*sp < 0x20 || *sp > 0x7e) {
            len += 5;
        }
        else if (*sp == '\'') {
            len += 2;
        }
        else if (*sp == '\\') {
            len += 4;
        }
        else {
            len++;
        }
    }

    old_size = LBS_Size(lbs);
    LBS_ReserveSize(lbs, old_size + len, TRUE);
    dp = LBS_Body(lbs) + old_size;

    for (sp = bintext; sp < spend; sp++) {
        if (*sp < 0x20 || *sp > 0x7e) {
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '0' + ((*sp >> 6) & 7);
            *dp++ = '0' + ((*sp >> 3) & 7);
            *dp++ = '0' + (*sp & 7);
        }
        else if (*sp == '\'') {
            *dp++ = '\\';
            *dp++ = '\'';
        }
        else if (*sp == '\\') {
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
        }
        else {
            *dp++ = *sp;
        }
    }
    LBS_SetPos(lbs, old_size + len);
}

static void
EscapeByteaInArray(LargeByteString *lbs, unsigned char *bintext, size_t binlen)
{
    unsigned char *sp, *spend = bintext + binlen;
    unsigned char *dp;
    size_t len;
    size_t old_size;

    len = 0;
    for (sp = bintext; sp < spend; sp++) {
        if (*sp < 0x20 || *sp > 0x7e) {
            len += 7;
        }
        else if (*sp == '\'') {
            len += 2;
        }
        else if (*sp == '"') {
            len += 3;
        }
        else if (*sp == '\\') {
            len += 8;
        }
        else {
            len++;
        }
    }

    old_size = LBS_Size(lbs);
    LBS_ReserveSize(lbs, old_size + len, TRUE);
    dp = LBS_Body(lbs) + old_size;

    for (sp = bintext; sp < spend; sp++) {
        if (*sp < 0x20 || *sp > 0x7e) {
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '0' + ((*sp >> 6) & 7);
            *dp++ = '0' + ((*sp >> 3) & 7);
            *dp++ = '0' + (*sp & 7);
        }
        else if (*sp == '\'') {
            *dp++ = '\\';
            *dp++ = '\'';
        }
        else if (*sp == '"') {
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '"';
        }
        else if (*sp == '\\') {
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
            *dp++ = '\\';
        }
        else {
            *dp++ = *sp;
        }
    }
    LBS_SetPos(lbs, old_size + len);
}

static void
NoticeMessage(
	void * arg,
	const char * message)
{
	Warning("%s", message);
}

void
AddConninfo(
	LargeByteString *conninfo,
	char *item,
	char *value)
{
	if ( value ) {
		LBS_EmitString(conninfo, item);
		LBS_EmitString(conninfo, "='");
		LBS_EmitString(conninfo, value);
		LBS_EmitString(conninfo, "'");
		LBS_EmitSpace(conninfo);
	}
}

LargeByteString *
CreateConninfo(
	DBG_Struct	*dbg)
{
	static LargeByteString *conninfo;

	conninfo = NewLBS();
	AddConninfo(conninfo, "host", GetDB_Host(dbg));
	AddConninfo(conninfo, "port", GetDB_Port(dbg));
	AddConninfo(conninfo, "dbname", GetDB_DBname(dbg));
	AddConninfo(conninfo, "user", GetDB_User(dbg));
	AddConninfo(conninfo, "password", GetDB_Pass(dbg));
	LBS_EmitEnd(conninfo);

	return conninfo;
}

static	void
ValueToSQL(
	DBG_Struct	*dbg,
	LargeByteString	*lbs,
	ValueStruct	*val)
{
	static	char	buff[SIZE_BUFF];
	Numeric	nv;
	Oid		id;

	if		(  IS_VALUE_NIL(val)  ) {
		LBS_EmitString(lbs,"null");
	} else
	switch	(ValueType(val)) {
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_TEXT:
		if		(  fInArray  ) {
            LBS_EmitChar(lbs, '"');
            EscapeStringInArray(lbs, ValueToString(val,dbg->coding));
            LBS_EmitChar(lbs, '"');
		} else {
            LBS_EmitChar(lbs, '\'');
            EscapeString(lbs, ValueToString(val,dbg->coding));
            LBS_EmitChar(lbs, '\'');
		}
		break;
	  case	GL_TYPE_BINARY:
		if		(  fInArray  ) {
            LBS_EmitChar(lbs, '"');
            EscapeByteaInArray(lbs, ValueByte(val), ValueByteLength(val));
            LBS_EmitChar(lbs, '"');
		} else {
            LBS_EmitChar(lbs, '\'');
            EscapeBytea(lbs, ValueByte(val), ValueByteLength(val));
            LBS_EmitChar(lbs, '\'');
		}
        break;
	  case	GL_TYPE_DBCODE:
		LBS_EmitString(lbs,ValueToString(val,dbg->coding));
		break;
	  case	GL_TYPE_NUMBER:
		nv = FixedToNumeric(&ValueFixed(val));
		LBS_EmitString(lbs,NumericOutput(nv));
		NumericFree(nv);
		break;
	  case	GL_TYPE_INT:
		sprintf(buff,"%d",ValueToInteger(val));
		LBS_EmitString(lbs,buff);
		break;
	  case	GL_TYPE_FLOAT:
		sprintf(buff,"%g",ValueToFloat(val));
		LBS_EmitString(lbs,buff);
		break;
	  case	GL_TYPE_BOOL:
		sprintf(buff,"'%s'",ValueToBool(val) ? "t" : "f");
		LBS_EmitString(lbs,buff);
		break;
	  case	GL_TYPE_OBJECT:
		id = ValueOid(ValueObjectId(val));
		sprintf(buff,"%u",id);
		LBS_EmitString(lbs,buff);
		break;
	  default:
		break;
	}
}

static	void
KeyValue(
	DBG_Struct	*dbg,
	LargeByteString	*lbs,
	ValueStruct	*args,
	char		**pk)
{
	ValueStruct	*val;

ENTER_FUNC;
	val = args;
	while	(  *pk  !=  NULL  ) {
		val = GetRecordItem(args,*pk);
		pk ++;
	}
	ValueToSQL(dbg,lbs,val);
LEAVE_FUNC;
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
				SetValueOid(item,dbg,id);
				break;
			  case	GL_TYPE_BOOL:
				SetValueBool(item,*p == 't');
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
              case	GL_TYPE_BINARY:
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
                SetValueBinary(item, qq, q - qq);
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

ENTER_FUNC;
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
			SetValueBool(val,*(char *)PQgetvalue(res,0,fnum) == 't');
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
	  case	GL_TYPE_BINARY:
		dbgmsg("byte");
		fnum = PQfnumber(res,ItemName());
		if		(  fnum  <  0  ) {
			if		(  !IS_VALUE_VIRTUAL(val)  ) {
				ValueIsNil(val);
			}
		} else {
            SetValueBinary(val, PQgetvalue(res,0,fnum),
                           PQgetlength(res,0,fnum));
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
			str = NumericOutput(nv);
			SetValueString(val,str,NULL);
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
			SetValueOid(val,dbg,id);
		}
		break;
	  case	GL_TYPE_ALIAS:
		Warning("invalid data type");
		break;
	  default:
		break;
	}
LEAVE_FUNC;
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

static	void
UpdateValue(
	DBG_Struct	*dbg,
	LargeByteString	*lbs,
	ValueStruct	*val)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;

	if		(  val  ==  NULL  )	return;
	if		(  IS_VALUE_NIL(val)  ) {
        LBS_EmitString(lbs,ItemName());
        LBS_EmitString(lbs,PutDim());
#if	1
        LBS_EmitString(lbs," = null ");
#else
        LBS_EmitString(lbs," is null ");
#endif
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
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_OBJECT:
        LBS_EmitString(lbs,ItemName());
        LBS_EmitString(lbs,PutDim());
        LBS_EmitString(lbs," = ");
        ValueToSQL(dbg,lbs,val);
        LBS_EmitString(lbs," ");
		break;
	  case	GL_TYPE_ARRAY:
		fComm = FALSE;
		for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
			tmp = ValueArrayItem(val,i);
			if		(  ( tmp->attr & GL_ATTR_VIRTUAL )  !=  GL_ATTR_VIRTUAL  ) {
				if		(  fComm  ) {
                    LBS_EmitChar(lbs,',');
				}
				fComm = TRUE;
				Dim[alevel] = i;
				alevel ++;
				UpdateValue(dbg,lbs,tmp);
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
                    LBS_EmitChar(lbs,',');
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				UpdateValue(dbg,lbs,tmp);
			}
		}
		level --;
		break;
	  default:
		break;
	}
}

static	void
InsertNames(
	LargeByteString	*lbs,
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
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_OBJECT:
	  case	GL_TYPE_ARRAY:
        LBS_EmitString(lbs,ItemName());
        LBS_EmitString(lbs,PutDim());
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			if		(  ( tmp->attr & GL_ATTR_VIRTUAL )  !=  GL_ATTR_VIRTUAL  ) {
				if		(  fComm  ) {
                    LBS_EmitChar(lbs,',');
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				InsertNames(lbs,tmp);
			}
		}
		level --;
		break;
	  default:
		break;
	}
}

static	void
InsertValues(
	DBG_Struct	*dbg,
	LargeByteString		*lbs,
	ValueStruct	*val)
{
	int		i;
	ValueStruct	*tmp;
	Bool	fComm;

	if		(  val  ==  NULL  )	return;
	if		(  IS_VALUE_NIL(val)  ) {
        LBS_EmitString(lbs, "null");
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
	  case	GL_TYPE_BINARY:
		ValueToSQL(dbg,lbs,val);
		break;
	  case	GL_TYPE_ARRAY:
        LBS_EmitString(lbs, "'{");
		fInArray = TRUE;
		fComm = FALSE;
		for	( i = 0 ; i < ValueArraySize(val) ; i ++ ) {
			tmp = ValueArrayItem(val,i);
			if		(  !IS_VALUE_VIRTUAL(tmp)  )	{
				if		(  fComm  ) {
                    LBS_EmitChar(lbs,',');
				}
				fComm = TRUE;
				Dim[alevel] = i;
				alevel ++;
				InsertValues(dbg,lbs,tmp);
				alevel --;
			}
		}
		fInArray = FALSE;
        LBS_EmitString(lbs,"}' ");
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		fComm = FALSE;
		for	( i = 0 ; i < ValueRecordSize(val) ; i ++ ) {
			tmp = ValueRecordItem(val,i);
			if		(  !IS_VALUE_VIRTUAL(tmp)  )	{
				if		(  fComm  ) {
                    LBS_EmitString(lbs,", ");
				}
				fComm = TRUE;
				rname[level-1] = ValueRecordName(val,i);
				InsertValues(dbg,lbs,tmp);
			}
		}
		level --;
		break;
	  default:
		break;
	}
}

static	void
_PQclear(
	PGresult	*res)
{
	if		(  res  !=  NULL  ) {
		PQclear(res);
	}
}

static Bool
IsRedirectQuery(
	PGresult	*res)
{
	Bool rc = FALSE;

	if ( (strncmp(PQcmdStatus(res), "INSERT ", 7) == 0)
		 ||(strncmp(PQcmdStatus(res), "UPDATE ", 7) == 0)
		 ||(strncmp(PQcmdStatus(res), "DELETE ", 7) == 0) ){
		rc = TRUE;
	}
	return rc;
}

static	PGresult	*
_PQexec(
	DBG_Struct	*dbg,
	char	*sql,
	Bool	fRed)
{
	PGresult	*res;

ENTER_FUNC;
#ifdef	TRACE
	printf("%s;\n",sql);fflush(stdout);
#endif
	res = PQexec(PGCONN(dbg),sql);
	if		(  fRed  && IsRedirectQuery(res) ){
		PutDB_Redirect(dbg,sql);
		PutDB_Redirect(dbg,";");
		PutCheckDataDB_Redirect(dbg, PQcmdTuples(res));
		PutCheckDataDB_Redirect(dbg, ":");
	}
LEAVE_FUNC;
	return	(res);
}

static int
_PQsendQuery(
	DBG_Struct	*dbg,
	char	*sql)
{
#ifdef	TRACE
	printf("%s;\n",sql);fflush(stdout);
#endif
	return PQsendQuery(PGCONN(dbg),sql);
}

static	PGresult	*
_PQgetResult(
	DBG_Struct	*dbg)
{
	return PQgetResult(PGCONN(dbg));
}

static int
CheckResult(
	DBG_Struct	*dbg,
	PGresult	*res,
	int status)
{
	int rc;

	if ( PQstatus(PGCONN(dbg)) != CONNECTION_OK ){
		dbgmsg("NG");
		rc = MCP_BAD_CONN;
	} else 
	if ( res && (PQresultStatus(res) == status)) {
		dbgmsg("OK");
		rc = MCP_OK;
	} else {
		dbgmsg("NG");
		Warning("%s",PQerrorMessage(PGCONN(dbg)));
		rc = MCP_BAD_OTHER;
		AbortDB_Redirect(dbg); 
	}
	return rc;
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

ENTER_FUNC;
	if		(  PQgetisnull(res,tnum,fnum)  ==  1  ) { 	/*	null	*/
		ValueIsNil(val);
	} else {
		ValueIsNonNil(val);
		switch	(ValueType(val)) {
		  case	GL_TYPE_INT:
			SetValueInteger(val,atoi((char *)PQgetvalue(res,tnum,fnum)));
			break;
		  case	GL_TYPE_OBJECT:
			SetValueOid(val,dbg,(Oid)atol((char *)PQgetvalue(res,tnum,fnum)));
			break;
		  case	GL_TYPE_BOOL:
			SetValueBool(val,*(char *)PQgetvalue(res,tnum,fnum) == 't');
			break;
		  case	GL_TYPE_BYTE:
		  case	GL_TYPE_CHAR:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
		  case	GL_TYPE_TEXT:
			SetValueString(val,(char *)PQgetvalue(res,tnum,fnum),dbg->coding);
			break;
          case	GL_TYPE_BINARY:
            SetValueBinary(val, PQgetvalue(res,tnum,fnum),
                           PQgetlength(res,tnum,fnum));
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
LEAVE_FUNC;
}

static	void
ExecPGSQL(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	LargeByteString	*src,
	ValueStruct		*args)
{
    LargeByteString *sql;
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
	Bool	fIntoAster
		,	fDot;
	char	buff[SIZE_LONGNAME+1]
		,	*p
		,	*q;

ENTER_FUNC;
	sql = NewLBS();
	if	(  src  ==  NULL )	{
		Error("function \"%s\" is not found.",ctrl->func);
	}
	RewindLBS(src);
	items = 0;
	tuple = NULL;
	fIntoAster = FALSE;
	while	(  ( c = LBS_FetchByte(src) )  >=  0  ) {
		if		(  c  !=  SQL_OP_ESC  ) {
            LBS_EmitChar(sql,c);
		} else {
			c = LBS_FetchByte(src);
			switch	(c) {
			  case	SQL_OP_SYMBOL:
				p = buff;
				while	(  ( c = LBS_FetchByte(src) )  !=  ' ' ) {
					*p ++ = c;
				}
				*p = 0;
				fDot = FALSE;
				p = buff;
				while	(  ( q = strchr(p,'.') )  !=  NULL  ) {
					*q = 0;
					LBS_EmitString(sql,p);
					p = q + 1;
					if		(  !fDot  ) {
						LBS_EmitChar(sql,'.');
						fDot = TRUE;
					} else {
						LBS_EmitChar(sql,'_');
					}
				}
				LBS_EmitString(sql,p);
				LBS_EmitSpace(sql);
				break;
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
				InsertValues(dbg,sql,val);
				break;
			  case	SQL_OP_VCHAR:
				break;
			  case	SQL_OP_EOL:
                LBS_EmitEnd(sql);
				res = _PQexec(dbg,LBS_Body(sql),TRUE);
                LBS_Clear(sql);
				status = PGRES_FATAL_ERROR;
				if		(	(  res ==  NULL  )
						||	(  ( status = PQresultStatus(res) )
							           ==  PGRES_BAD_RESPONSE    )
						||	(  status  ==  PGRES_FATAL_ERROR     )
						||	(  status  ==  PGRES_NONFATAL_ERROR  ) ) {
					dbgmsg("NG");
					Warning("%s",PQerrorMessage(PGCONN(dbg)));
					ctrl->rc = - status;
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
				}
				_PQclear(res);
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
    FreeLBS(sql);
LEAVE_FUNC;
}

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed)
{
	PGresult	*res;
	int			rc = MCP_OK;

	LBS_EmitStart(dbg->checkData);
	if	( _PQsendQuery(dbg,sql) == TRUE ) {
		while ( (res = _PQgetResult(dbg)) != NULL ){
			rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
			if ( rc == MCP_OK ) {
				PutCheckDataDB_Redirect(dbg, PQcmdTuples(res));
				PutCheckDataDB_Redirect(dbg, ":");
			} else {
				_PQclear(res);
				break;
			}
			_PQclear(res);
		}
	} else {
		Warning("%s",PQerrorMessage(PGCONN(dbg)));
		rc = MCP_BAD_OTHER;
	}
	LBS_EmitEnd(dbg->checkData);
	return	rc;
}

static	void
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	int		rc;
	LargeByteString *conninfo;
	PGconn	*conn;
	
ENTER_FUNC;
	if ( dbg->fConnect == CONNECT ){
		Warning("database is already connected.");
	}
	conninfo = CreateConninfo(dbg);
	conn = PQconnectdb(LBS_Body(conninfo));
	FreeLBS(conninfo);
	
	if		(  PQstatus(conn)  ==  CONNECTION_OK  ) {
		PQsetNoticeProcessor((conn), NoticeMessage, NULL);
		OpenDB_RedirectPort(dbg);
		dbg->conn = (void *)conn;
		dbg->fConnect = CONNECT;
		rc = MCP_OK;
	} else {
		Message("Connection to database \"%s\" failed.", GetDB_DBname(dbg));
		Message("%s", PQerrorMessage(conn));
		PQfinish(conn);
		rc = MCP_BAD_CONN;
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
}

static	void
_DBDISCONNECT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
ENTER_FUNC;
	if		(  dbg->fConnect == CONNECT ) { 
		PQfinish(PGCONN(dbg));
		CloseDB_RedirectPort(dbg);
		dbg->fConnect = DISCONNECT;
		if		(  ctrl  !=  NULL  ) {
			ctrl->rc = MCP_OK;
		}
	}
LEAVE_FUNC;
}

static	void
_DBSTART(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	PGresult	*res;
	int			rc;

ENTER_FUNC;
	BeginDB_Redirect(dbg); 
	res = _PQexec(dbg,"BEGIN",FALSE);
	rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
	_PQclear(res);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
}

static	void
_DBCOMMIT(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	PGresult	*res;
	int			rc;

ENTER_FUNC;
	CheckDB_Redirect(dbg);
	res = _PQexec(dbg,"COMMIT WORK",FALSE);
	rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
	_PQclear(res);
	CommitDB_Redirect(dbg);
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = rc;
	}
LEAVE_FUNC;
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

ENTER_FUNC;

	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		ctrl->rc = MCP_OK;
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_SELECT]->proc;
		ExecPGSQL(dbg,ctrl,src,args);
	}
LEAVE_FUNC;
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

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_FETCH]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,src,args);
		} else {
			p = sql;
			p += sprintf(p,"FETCH FROM %s_%s_csr",rec->name,path->name);
			res = _PQexec(dbg,sql,TRUE);
			ctrl->rc = CheckResult(dbg, res, PGRES_TUPLES_OK);
			if ( ctrl->rc == MCP_OK ) {
				if		(  ( n = PQntuples(res) )  >  0  ) {
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
LEAVE_FUNC;
}

static	void
_DBCLOSECURSOR(
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
	LargeByteString	*src;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_CLOSE]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,src,args);
		} else {
			p = sql;
			p += sprintf(p,"CLOSE %s_%s_csr",rec->name,path->name);
			res = _PQexec(dbg,sql,TRUE);
			ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
			_PQclear(res);
		}
	}
LEAVE_FUNC;
}

static	void
_DBUPDATE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
	LargeByteString	*sql;
	DB_Struct	*db;
	char	***item
	,		**pk;
	PGresult	*res;
	PathStruct	*path;
	LargeByteString	*src;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_UPDATE]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,src,args);
		} else {
            sql = NewLBS();
            LBS_EmitString(sql,"UPDATE ");
            LBS_EmitString(sql,rec->name);
            LBS_EmitString(sql,"\tSET ");
			level = 0;
			alevel = 0;
			fInArray = FALSE;
			UpdateValue(dbg,sql,args);

			LBS_EmitString(sql,"WHERE\t");
			item = db->pkey->item;
			while	(  *item  !=  NULL  ) {
                LBS_EmitString(sql,rec->name);
                LBS_EmitChar(sql,'.');
				pk = *item;
				while	(  *pk  !=  NULL  ) {
                    LBS_EmitString(sql,*pk);
					pk ++;
					if		(  *pk  !=  NULL  ) {
                        LBS_EmitChar(sql,'_');
					}
				}
                LBS_EmitString(sql," = ");
                KeyValue(dbg,sql,args,*item);
                LBS_EmitChar(sql,' ');
				item ++;
				if		(  *item  !=  NULL  ) {
                    LBS_EmitString(sql,"AND\t");
				}
			}
            LBS_EmitEnd(sql);
			res = _PQexec(dbg,LBS_Body(sql),TRUE);
			ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
			_PQclear(res);
            FreeLBS(sql);
		}
	}
LEAVE_FUNC;
}

static	void
_DBDELETE(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
    LargeByteString	*sql;
	DB_Struct	*db;
	char	***item
	,		**pk;
	PGresult	*res;
	PathStruct	*path;
	LargeByteString	*src;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_DELETE]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,src,args);
		} else {
			sql = NewLBS();
			LBS_EmitString(sql,"DELETE\tFROM\t");
			LBS_EmitString(sql,rec->name);
			LBS_EmitString(sql," WHERE\t");
			item = db->pkey->item;
			while	(  *item  !=  NULL  ) {
				pk = *item;
                LBS_EmitString(sql,rec->name);
                LBS_EmitChar(sql,'.');
				while	(  *pk  !=  NULL  ) {
                    LBS_EmitString(sql,*pk);
					pk ++;
					if		(  *pk  !=  NULL  ) {
                        LBS_EmitChar(sql,' ');
					}
				}
                LBS_EmitString(sql," = ");
                KeyValue(dbg,sql,args,*item);
                LBS_EmitChar(sql,' ');
				item ++;
				if		(  *item  !=  NULL  ) {
                    LBS_EmitString(sql,"AND\t");
				}
			}
            LBS_EmitEnd(sql);
			res = _PQexec(dbg,LBS_Body(sql),TRUE);
			ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
			_PQclear(res);
            FreeLBS(sql);
		}
	}
LEAVE_FUNC;
}

static	void
_DBINSERT(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	ValueStruct		*args)
{
    LargeByteString	*sql;
	DB_Struct	*db;
	PGresult	*res;
	PathStruct	*path;
	LargeByteString	*src;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_INSERT]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecPGSQL(dbg,ctrl,src,args);
		} else {
			sql = NewLBS();
			LBS_EmitString(sql,"INSERT\tINTO\t");
			LBS_EmitString(sql,rec->name);
			LBS_EmitString(sql," (");

			level = 0;
			alevel = 0;
			InsertNames(sql,args);
			LBS_EmitString(sql,") VALUES\t(");
			fInArray = FALSE;
			InsertValues(dbg,sql,args);
			LBS_EmitString(sql,") ");
            LBS_EmitEnd(sql);
			res = _PQexec(dbg,LBS_Body(sql),TRUE);
			ctrl->rc = CheckResult(dbg, res, PGRES_COMMAND_OK);
			_PQclear(res);
            FreeLBS(sql);
		}
	}
LEAVE_FUNC;
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

ENTER_FUNC;
#ifdef	TRACE
	printf("[%s]\n",name); 
#endif
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
		rc = TRUE;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		if		(  ( ix = (int)(long)g_hash_table_lookup(path->opHash,name) )  ==  0  ) {
			ctrl->rc = MCP_BAD_FUNC;
			rc = FALSE;
		} else {
			src = path->ops[ix-1]->proc;
			if		(  src  !=  NULL  ) {
				ctrl->rc = MCP_OK;
				ExecPGSQL(dbg,ctrl,src,args);
				rc = TRUE;
			} else {
				ctrl->rc = MCP_BAD_OTHER;
				rc = FALSE;
			}
		}
	}
LEAVE_FUNC;
	return	(rc);
}

static	DB_OPS	Operations[] = {
	/*	DB operations		*/
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
	{	"DBCLOSECURSOR",_DBCLOSECURSOR },

	{	NULL,			NULL }
};

static	DB_Primitives	Core = {
	_EXEC,
	_DBACCESS,
	NULL,
};

extern	DB_Func	*
InitPostgreSQL(void)
{
	return	(EnterDB_Function("PostgreSQL",Operations,DB_PARSER_SQL,&Core,"/*","*/\t"));
}

#endif /* #ifdef HAVE_POSTGRES */
