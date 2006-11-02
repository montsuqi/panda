/*
 * PANDA -- a simple transaction monitor
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
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_OSEKI
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<numeric.h>

#include	"const.h"
#include	"types.h"
#include	"enum.h"
#include	"SQLparser.h"
#include	"libmondai.h"
#include	"dbgroup.h"
#include	"client.h"
#include	"directory.h"
#include	"redirect.h"
#include	"debug.h"

static	int		level;
static	char	*rname[SIZE_RNAME];
static	int		alevel;
static	int		Dim[SIZE_RNAME];

#define	OS_CONN(dbg)		((OsekiClientSession *)(dbg)->conn)

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
static	void
ValueToSQL(
	DBG_Struct	*dbg,
	LargeByteString	*lbs,
	ValueStruct	*val)
{
	char	buff[SIZE_LONGNAME+1];
	Numeric	nv;

ENTER_FUNC;
	if		(  IS_VALUE_NIL(val)  ) {
		LBS_EmitString(lbs,"null");
	} else
	switch	(ValueType(val)) {
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_TEXT:
		LBS_EmitChar(lbs, '\'');
		EscapeString(lbs, ValueToString(val,dbg->coding));
		LBS_EmitChar(lbs, '\'');
		break;
	  case	GL_TYPE_BINARY:
		LBS_EmitChar(lbs, '\'');
		EscapeBytea(lbs, ValueByte(val), ValueByteLength(val));
		LBS_EmitChar(lbs, '\'');
        break;
	  case	GL_TYPE_DBCODE:
		LBS_EmitString(lbs,ValueToString(val,dbg->coding));
		break;
	  default:
		SQL_PackValue(NULL,buff,val);
		LBS_EmitString(lbs,buff);
		break;
	}
LEAVE_FUNC;
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
	static	char	buff[SIZE_LONGNAME+1];
	char	*p;
	int		i;

	p = buff;
	if		(  level  >  1  ) {
		for	( i = 0 ; i < level - 1 ; i ++ ) {
			p += sprintf(p,"%s.",rname[i]);
		}
	}
	p += sprintf(p,"%s",rname[level-1]);
#ifdef	TRACE
	printf("item = [%s]\n",buff);
#endif
	return	(buff);
}

static	char	*
PutDim(void)
{
	static	char	buff[SIZE_LONGNAME+1];
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
#if	0
        LBS_EmitString(lbs,ItemName());
        LBS_EmitString(lbs,PutDim());
        LBS_EmitString(lbs," = null ");
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
	size_t	size;
	byte	*buff;

ENTER_FUNC;
	if		(  val  !=  NULL  ) {
		size = SQL_SizeValue(NULL,val);
		buff = (byte *)xmalloc(size+1);
		SQL_PackValue(NULL,buff,val);
		LBS_EmitString(lbs,buff);
		xfree(buff);
	}
LEAVE_FUNC;
}

static	void
ReadTuple(
	ValueStruct	*args,
	ValueStruct	*tuple)
{
	ValueStruct	*value
		,		*arg;
	char		*name;
	int			i;

ENTER_FUNC;
	if		(	(  args   !=  NULL  )
			&&	(  tuple  !=  NULL  ) ) {
		for	( i = 0 ; i < ValueRecordSize(tuple) ; i ++ ) {
			value = ValueRecordItem(tuple,i);
			name = ValueRecordName(tuple,i);
			dbgprintf("name = [%s]",name);
			arg = GetRecordItem(args,name);
			CopyValue(arg,value);
		}
	}
LEAVE_FUNC;
}

static	void
ReadItems(
	ValueStruct	**args,
	int			items,
	ValueStruct	**tuples)
{
	ValueStruct	*value;
	int			i
		,		j
		,		k;

ENTER_FUNC;
	if		(	(  args    !=  NULL  )
			&&	(  tuples  !=  NULL  ) ) {
		i = 0;
		k = 0;
		for	( j = 0 ; j < items ; j ++ , k ++ ) {
			if		(  k  ==  ValueArraySize(tuples[i])  ) {
				j ++;
				k = 0;
			}
			if		(  tuples[i]  ==  NULL  )	break;
			value = ValueArrayItem(tuples[i],k);
			CopyValue(args[j],value);
		}
	}
LEAVE_FUNC;
}

static	int
_SendCommand(
	DBG_Struct	*dbg,
	unsigned char	*sql,
	Bool	fRed)
{
	int		res;

ENTER_FUNC;
#ifdef	TRACE
	printf("[%s]\n",sql);fflush(stdout);
#endif
	res = OsekiSendCommand(OS_CONN(dbg),sql);
	if		(  fRed  ) {
		PutDB_Redirect(dbg,sql);
	}
LEAVE_FUNC;
	return	(res);
}

static	void
ExecOseki(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	LargeByteString	*src,
	ValueStruct		*args)
{
	OsekiClientSession	*ses = OS_CONN(dbg);
    LargeByteString *sql;
	int		c;
	ValueStruct	*val;
	int			res;
	ValueStruct	**tuple
		,		**input;
	int			n
		,		items;
	Bool	fIntoAster;

ENTER_FUNC;
	dbg =  rec->opt.db->dbg;
	sql = NewLBS();
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
            LBS_EmitChar(sql,c);
		} else {
			c = LBS_FetchByte(src);
			switch	(c) {
			  case	SQL_OP_SYMBOL:
				while	(  ( c = LBS_FetchByte(src) )  !=  ' ' ) {
					LBS_EmitChar(sql,c);
				}
				LBS_EmitChar(sql,c);
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
				res = _SendCommand(dbg,LBS_Body(sql),TRUE);
                LBS_Clear(sql);
				if		(  res <  0  ) {
					dbgmsg("NG");
					ctrl->rc = res;
				} else {
					if		(  OsekiIsData(dbg->conn)  ) {
						dbgmsg("OK");
						if		(  fIntoAster  ) {
							if		(  ( input = OsekiReadData(ses,OsekiResult(ses)) )
									   !=  NULL  ) {
								ReadTuple(args,input[0]);
								OsekiFreeTuple(input);
								ctrl->rc += MCP_OK;
							} else {
								ctrl->rc += MCP_EOF;
							}
						} else
						if		(	(  items  ==  0     )
								||	(  tuple  ==  NULL  ) ) {
							dbgmsg("no tuple");
							ctrl->rc = MCP_EOF;
						} else {
							dbgmsg("+");
							if		(  ( input = OsekiReadData(ses,OsekiResult(ses)) )
									   !=  NULL  ) {
								ReadItems(tuple,items,input);
								OsekiFreeTuple(input);
								ctrl->rc += MCP_OK;
							} else {
								ctrl->rc += MCP_EOF;
							}
						}
						if		(  tuple  !=  NULL  ) {
							xfree(tuple);
							tuple = NULL;
						}
						ctrl->rc += MCP_OK;
					} else {
						dbgmsg("EOF");
						ctrl->rc += MCP_EOF;
					}
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
    FreeLBS(sql);
LEAVE_FUNC;
}

static	int
_EXEC(
	DBG_Struct	*dbg,
	char		*sql,
	Bool		fRed)
{
	int			res;
	int			rc;

	res = _SendCommand(dbg,sql,fRed);
	if		(  res <  0  ) {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
	} else
	if		(  res  ==  0  ) {
		dbgmsg("OK");
		rc = MCP_OK;
	} else {
		dbgmsg("EOF");
		rc = MCP_EOF;
	}
	return	(rc);
}

static	void
_DBOPEN(
	DBG_Struct	*dbg,
	DBCOMM_CTRL	*ctrl)
{
	char	*port
	,		*user
	,		*dbname
	,		*pass;
	OsekiClientSession	*ses;

ENTER_FUNC;
	if ( dbg->fConnect == CONNECT ){
		Warning("database is already connected.");
	}

	port = StringPort(dbg->port);
	user =  GetDB_User(dbg);
	dbname = GetDB_DBname(dbg);
	pass = GetDB_Pass(dbg);

	dbgprintf("port = [%s]\n",port);
	dbgprintf("user = [%s]\n",user);
	dbgprintf("pass = [%s]\n",pass);

	ses = ConnectOseki(NULL,port,user,pass);
	if		(  ses  !=  NULL  ) {
		OpenDB_RedirectPort(dbg);
		dbg->conn = (void *)ses;
		dbg->fConnect = CONNECT;
		ctrl->rc = MCP_OK;
	} else {
		Message("Connection to database failed.\n");
		ctrl->rc = MCP_BAD_OTHER;
	}
	if		(  ctrl  !=  NULL  ) {
		ctrl->rc = MCP_OK;
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
		DisConnectOseki(OS_CONN(dbg));
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
	int			rc;

ENTER_FUNC;
	BeginDB_Redirect(dbg); 
	rc = OsekiStart(OS_CONN(dbg));
	if		(  rc  !=  0  ) {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
	} else {
		dbgmsg("OK");
		rc = MCP_OK;
	}
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
	int			rc;

ENTER_FUNC;
	CheckDB_Redirect(dbg);
	rc = OsekiCommit(OS_CONN(dbg));
	if		(  rc  !=  0  ) {
		dbgmsg("NG");
		rc = MCP_BAD_OTHER;
	} else {
		dbgmsg("OK");
		rc = MCP_OK;
	}
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
		ExecOseki(dbg,ctrl,rec,src,args);
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
	OsekiClientSession	*ses = OS_CONN(dbg);
	char	sql[SIZE_SQL+1]
	,		*p;
	DB_Struct	*db;
	PathStruct	*path;
	int			res;
	LargeByteString	*src;
	ValueStruct		**input;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_FETCH]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecOseki(dbg,ctrl,rec,src,args);
		} else {
			p = sql;
			p += sprintf(p,"FETCH FROM %s_%s_csr;",rec->name,path->name);
			res = _SendCommand(dbg,sql,TRUE);
			if		(  res  <  0  ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				if		(  OsekiIsData(ses)  ) {
					dbgmsg("OK");
					if		(  ( input = OsekiReadData(ses,OsekiResult(ses)) )  !=  NULL  ) {
						ReadTuple(args,input[0]);
						OsekiFreeTuple(input);
						ctrl->rc = MCP_OK;
					} else {
						ctrl->rc = MCP_EOF;
					}
				} else {
					dbgmsg("EOF");
					ctrl->rc = MCP_EOF;
				}
			}
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
	int			res;
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
			ExecOseki(dbg,ctrl,rec,src,args);
		} else {
			p = sql;
			p += sprintf(p,"CLOSE %s_%s_csr;",rec->name,path->name);
			res = _SendCommand(dbg,sql,TRUE);
			if		(  res !=  0  ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				ctrl->rc = MCP_OK;
				dbgmsg("OK");
			}
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
		,	**pk;
	PathStruct	*path;
	LargeByteString	*src;
	int			res;

ENTER_FUNC;
	if		(  rec->type  !=  RECORD_DB  ) {
		ctrl->rc = MCP_BAD_ARG;
	} else {
		db = rec->opt.db;
		path = db->path[ctrl->pno];
		src = path->ops[DBOP_UPDATE]->proc;
		if		(  src  !=  NULL  ) {
			ctrl->rc = MCP_OK;
			ExecOseki(dbg,ctrl,rec,src,args);
		} else {
            sql = NewLBS();
            LBS_EmitString(sql,"UPDATE ");
            LBS_EmitString(sql,rec->name);
            LBS_EmitString(sql,"\tSET ");
			level = 0;
			alevel = 0;
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
			res = _SendCommand(dbg,LBS_Body(sql),TRUE);
			if		(  res  <  0  ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				ctrl->rc = MCP_OK;
			}
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
	int			res;
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
			ExecOseki(dbg,ctrl,rec,src,args);
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
			res = _SendCommand(dbg,LBS_Body(sql),TRUE);
			if		(  res  <  0  ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				ctrl->rc = MCP_OK;
			}
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
	int			res;
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
			ExecOseki(dbg,ctrl,rec,src,args);
		} else {
            sql = NewLBS();
			LBS_EmitString(sql,"INSERT\tINTO\t");
			LBS_EmitString(sql,rec->name);
			LBS_EmitString(sql," (");

			level = 0;
			alevel = 0;
			InsertNames(sql,args);
			LBS_EmitString(sql,") VALUES\t(");
			InsertValues(dbg,sql,args);
			LBS_EmitString(sql,")");
			LBS_EmitChar(sql,';');
            LBS_EmitEnd(sql);
			res = _SendCommand(dbg,LBS_Body(sql),TRUE);
			if		(  res  !=  0  ) {
				dbgmsg("NG");
				ctrl->rc = MCP_BAD_OTHER;
			} else {
				dbgmsg("OK");
				ctrl->rc = MCP_OK;
			}
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
		if		(  ( ix = (int)g_hash_table_lookup(path->opHash,name) )  ==  0  ) {
			ctrl->rc = MCP_BAD_FUNC;
			rc = FALSE;
		} else {
			src = path->ops[ix-1]->proc;
			if		(  src  !=  NULL  ) {
				ctrl->rc = MCP_OK;
				ExecOseki(dbg,ctrl,rec,src,args);
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
InitOseki(void)
{
	return	(EnterDB_Function("Oseki",Operations,DB_PARSER_SQL,&Core,"/*","*/\t"));
}

#endif /* #ifdef HAVE_OSEKI */
