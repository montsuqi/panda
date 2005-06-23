/*
PANDA -- a simple transaction monitor
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include	"redirect.h"
#include	"debug.h"

static	int		level;
static	char	*rname[SIZE_RNAME];
static	int		alevel;
static	int		Dim[SIZE_RNAME];

#define	OS_CONN(dbg)		((OsekiClientSession *)(dbg)->conn)

static	void
ValueToSQL(
	DBG_Struct	*dbg,
	LargeByteString	*lbs,
	ValueStruct	*val)
{
	size_t	size;

	if		(  val  ==  NULL  )	return;
	size = SQL_SizeValue(NULL,val);
	LBS_ReserveSize(lbs,size,TRUE);
	SQL_PackValue(NULL,LBS_Ptr(lbs),val);
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
        LBS_EmitString(lbs," = null ");
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
	size_t	size
		,	i;
	byte	*buff
		,	*p;

ENTER_FUNC;
	if		(  val  ==  NULL  )	return;
	size = SQL_SizeValue(NULL,val);
	buff = (byte *)xmalloc(size);
	SQL_PackValue(NULL,buff,val);
	p = buff;
	//while	(  *p  !=  0  )		{
	for	( i = 0 ; i < size ; i ++ ) {
		LBS_Emit(lbs,*p);
		p ++;
	}
	xfree(buff);
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
	printf("%s\n",sql);fflush(stdout);
#endif
	res = OsekiSendCommand(OS_CONN(dbg),sql);
	if		(  fRed  ) {
		PutDB_Redirect(dbg,sql);
	}
LEAVE_FUNC;
	return	(res);
}

static	void
GetTable(
	DBG_Struct	*dbg,
	ValueStruct	**tuple,
	int			items)
{
	byte	*p;
	int		i;
	OsekiClientSession	*ses = OS_CONN(dbg);

ENTER_FUNC;
	if		(  OsekiReadData(ses,OsekiResult(ses),NULL,NULL)  ==  0  ) {
		p = LBS_Body(OsekiBuff(ses));
		for	( i = 0 ; i < items ; i ++ ) {
			p += SQL_UnPackValue(NULL,p,tuple[i]);
		}
	}
LEAVE_FUNC;
}

static	void
ExecOseki(
	DBG_Struct		*dbg,
	DBCOMM_CTRL		*ctrl,
	RecordStruct	*rec,
	LargeByteString	*src,
	ValueStruct		*args)
{
    LargeByteString *sql;
	int		c;
	ValueStruct	*val;
	int			res;
	ValueStruct	**tuple;
	int		n
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
				LBS_EmitChar(sql,';');
                LBS_EmitEnd(sql);
				res = _SendCommand(dbg,LBS_Body(sql),TRUE);
                LBS_Clear(sql);
				if		(  res <  0  ) {
					dbgmsg("NG");
					ctrl->rc = res;
					break;
				} else {
					dbgmsg("OK");
					if		(  OsekiIsData(OS_CONN(dbg))  )	{
						GetTable(dbg,tuple,items);
						ctrl->rc += MCP_OK;
					} else {
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
	char	*host;
	char	*port
	,		*user
	,		*dbname
	,		*pass;
	OsekiClientSession	*ses;

ENTER_FUNC;
	if		(  DB_Host  !=  NULL  ) {
		host = DB_Host;
	} else {
		if		(  dbg->port  ==  NULL  ) {
			host = NULL;
		} else {
			host = IP_HOST(dbg->port);
		}
	}
	if		(  DB_Port  !=  NULL  ) {
		port = DB_Port;
	} else {
		port = IP_PORT(dbg->port);
	}
	user =  ( DB_User != NULL ) ? DB_User : dbg->user;
	dbname = ( DB_Name != NULL ) ? DB_Name : dbg->dbname;
	pass = ( DB_Pass != NULL ) ? DB_Pass : dbg->pass;

	ses = ConnectOseki(host,port,user,pass,"SQL");
	if		(  ses  ==  NULL  ) {
		fprintf(stderr,"Connection to database failed.\n");
		exit(1);
	}
	OpenDB_RedirectPort(dbg);
	dbg->conn = (void *)ses;
	dbg->fConnect = TRUE;
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
	if		(  dbg->fConnect  ) { 
		DisConnectOseki(OS_CONN(dbg));
		CloseDB_RedirectPort(dbg);
		dbg->fConnect = FALSE;
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
					if		(  OsekiReadData(ses,OsekiResult(ses),NULL,NULL)  ==  0  ) {
						SQL_UnPackValue(NULL,LBS_Body(OsekiBuff(ses)),args);
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
	DB_Struct	*db;
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
			ExecOseki(dbg,ctrl,rec,src,args);
		} else {
			ctrl->rc = MCP_BAD_OTHER;
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
	DB_Struct	*db;
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
			ctrl->rc = MCP_BAD_OTHER;
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
};

extern	DB_Func	*
InitOseki(void)
{
	return	(EnterDB_Function("Oseki",Operations,&Core,"/*","*/\t"));
}

#endif /* #ifdef HAVE_OSEKI */
