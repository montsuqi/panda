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

#define	MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<ctype.h>

#include	<libgen.h>
#include    <sys/socket.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"enum.h"
#include	"dirs.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"authstub.h"
#include	"directory.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"front.h"
#include	"dbs_main.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	DBD_Struct	*ThisDBD;
static	sigset_t	hupset;
static	char		*PortNumber;
static	int			Back;
static	char	*Directory;
static	char		*AuthURL;
static	URL			Auth;

static	void
InitSystem(
	char	*name)
{
dbgmsg(">InitSystem");
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);

	InitDirectory();
	SetUpDirectory(Directory,"","",name);
	if		(  ( ThisDBD = GetDBD(name) )  ==  NULL  ) {
		fprintf(stderr,"DBD \"%s\" not found.\n",name);
		exit(1);
	}

	ThisDB = ThisDBD->db;
	DB_Table = ThisDBD->DBD_Table;
	CurrentProcess = NULL;

	if		(  ThisDBD->cDB  >  0  ) {
		InitDB_Process(NULL);
	}
dbgmsg("<InitSystem");
}

#define	COMM_STRING		1
#define	COMM_BINARY		2
#define	COMM_STRINGE	3

typedef	struct {
	char	user[SIZE_USER+1];
	int		type;
}	SessionNode;

static	void
FinishSession(
	SessionNode	*node)
{
dbgmsg(">FinishSession");
	xfree(node);
dbgmsg("<FinishSession");
}

static	SessionNode	*
InitDBSSession(
	NETFILE	*fpComm)
{
	SessionNode	*ses;
	char	buff[SIZE_BUFF+1];
	char	*pass;
	char	*ver;
	char	*p
	,		*q;

ENTER_FUNC;
	ses = New(SessionNode);
	/*
	 version user password type\n
	 */
	RecvStringDelim(fpComm,SIZE_BUFF,buff);
	p = buff;
	*(q = strchr(p,' ')) = 0;
	ver = p;
	p = q + 1;
	*(q = strchr(p,' ')) = 0;
	strcpy(ses->user,p);
	p = q + 1;
	*(q = strchr(p,' ')) = 0;
	pass = p;
	if		(  !stricmp(q+1,"binary")  ) {
		ses->type = COMM_BINARY;
	} else
	if		(  !stricmp(q+1,"string")  ) {
		ses->type = COMM_STRING;
	} else
	if		(  !stricmp(q+1,"stringe")  ) {
		ses->type = COMM_STRINGE;
	} else {
	}
	if		(  strcmp(ver,VERSION)  ) {
		SendStringDelim(fpComm,"Error: version\n");
		g_warning("reject client(invalid version)");
		xfree(ses);
		ses = NULL;
	} else
	if		(  AuthUser(&Auth,ses->user,pass,NULL)  ) {
		SendStringDelim(fpComm,"Connect: OK\n");
	} else {
		SendStringDelim(fpComm,"Error: authentication\n");
		g_warning("reject client(authentication error)");
		xfree(ses);
		ses = NULL;
	}
LEAVE_FUNC;
	return	(ses); 
}

static	void
DecodeName(
	char	*rname,
	char	*vname,
	char	*p)
{
	while	(	(  *p  !=  0    )
			&&	(  isspace(*p)  ) )	p ++;
	while	(	(  *p  !=  0     )
			&&	(  *p  !=  '.'   )
			&&	(  !isspace(*p)  ) ) {
		*rname ++ = *p ++;
	}
	*rname = 0;

	if		(  *p  !=  0  ) {
		p ++;
		while	(	(  *p  !=  0    )
				&&	(  isspace(*p)  ) )	p ++;
		while	(  *p  !=  0  ) {
			if		(  !isspace(*p)  ) {
				*vname ++ = *p;
			}
			p ++;
		}
	}
	*vname = 0;
}

static	void
RecvData(
	NETFILE		*fpComm,
	ValueStruct	*args)
{
	char	buff[SIZE_BUFF+1];
	char	vname[SIZE_BUFF+1]
	,		rname[SIZE_BUFF+1]
	,		str[SIZE_BUFF+1];
	char	*p;
	int		rno;
	ValueStruct		*value;
	RecordStruct	*rec;

	do {
		RecvStringDelim(fpComm,SIZE_BUFF,buff);
		if		(	(  *buff                     !=  0     )
				&&	(  ( p = strchr(buff,':') )  !=  NULL  ) ) {
			*p = 0;
			DecodeName(rname,vname,buff);
			DecodeStringURL(str,p+1);
			value = GetItemLongName(args,vname);
			ValueIsUpdate(value);
			SetValueString(value,str,DB_LOCALE);
		} else
			break;
	}	while	(TRUE);
}

static	void
WriteClientString(
	NETFILE		*fpComm,
	Bool		fType,
	DBCOMM_CTRL	*ctrl,
	ValueStruct	*args)
{
	char	name[SIZE_BUFF+1]
	,		rname[SIZE_BUFF+1]
	,		vname[SIZE_BUFF+1];
	char	buff[SIZE_BUFF+1];
	ValueStruct	*value;
	RecordStruct	*rec;
	char	*p;
	Bool	fName;
	int		rno;

dbgmsg(">WriteClientString");
	SendStringDelim(fpComm,"Exec: ");
	sprintf(buff,"%d\n",ctrl->rc);
	SendStringDelim(fpComm,buff);
	do {
		if		(  !RecvStringDelim(fpComm,SIZE_BUFF,name)  )	break;
		if		(  *name  ==  0  )	break;
		if		(  ( p = strchr(name,':') )  !=  NULL  ) {
			*p = 0;
			fName = FALSE;
		} else {
			fName = TRUE;
		}
		DecodeName(rname,vname,name);
		if		(  *vname  !=  0  ) {
			value = GetItemLongName(args,vname);
		} else {
			value = args;
		}
		SetValueName(name);
		SendValueString(fpComm,value,NULL,fName,fType);
		if		(  fName  ) {
			SendStringDelim(fpComm,"\n");
		}
	}	while	(TRUE);
dbgmsg("<WriteClientString");
}

static	void
DumpItems(
	NETFILE		*fp,
	ValueStruct	*value)
{
	char	buff[SIZE_LONGNAME];
	int		i;

	if		(  value  ==  NULL  )	return;
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		sprintf(buff,"int");
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_BOOL:
		sprintf(buff,"bool");
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_BYTE:
		sprintf(buff,"byte");
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_CHAR:
		sprintf(buff,"char(%d)",ValueStringLength(value));
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_VARCHAR:
		sprintf(buff,"varchar(%d)",ValueStringLength(value));
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_DBCODE:
		sprintf(buff,"dbcode(%d)",ValueStringLength(value));
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_NUMBER:
		if		(  ValueFixedSlen(value)  ==  0  ) {
			sprintf(buff,"number(%d)",ValueFixedLength(value));
		} else {
			sprintf(buff,"number(%d,%d)",
					ValueFixedLength(value),
					ValueFixedSlen(value));
		}
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_TEXT:
		sprintf(buff,"text");
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_ARRAY:
		DumpItems(fp,ValueArrayItem(value,0));
		sprintf(buff,"[%d]",ValueArraySize(value));
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_RECORD:
		SendStringDelim(fp,"{");
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			sprintf(buff,"%s ",ValueRecordName(value,i));
			SendStringDelim(fp,buff);
			DumpItems(fp,ValueRecordItem(value,i));
			SendStringDelim(fp,";");
		}
		SendStringDelim(fp,"}");
		break;
	  default:
		break;
	}
}

static	void
WriteClientStructure(
	NETFILE			*fpComm,
	RecordStruct	*rec)
{
ENTER_FUNC;
	SendStringDelim(fpComm,rec->name);
	DumpItems(fpComm,rec->value);
	SendStringDelim(fpComm,";\n");
LEAVE_FUNC;
}

static	Bool
do_String(
	NETFILE	*fpComm,
	char	*input,
	SessionNode	*ses)
{
	Bool	ret
	,		fType;
	DBCOMM_CTRL	ctrl;
	ValueStruct		*value;
	RecordStruct	*rec;
	PathStruct		*path;
	DB_Operation	*op;
	char			func[SIZE_FUNC+1]
		,			rname[SIZE_RNAME+1]
		,			pname[SIZE_PNAME+1];
	char	*p
		,	*q;
	int		rno
		,	pno
		,	ono;

	if		(  strncmp(input,"Exec: ",6)  ==  0  ) {
		dbgmsg("exec");
		p = input + 6;
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
			DecodeStringURL(func,p);
			p = q + 1;
			if		(  ( q = strchr(p,':') )  !=  NULL  ) {
				*q = 0;
				DecodeStringURL(rname,p);
				p = q + 1;
			} else {
				strcpy(rname,"");
			}
			DecodeStringURL(pname,p);
			value = NULL;
			if		(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) {
				ctrl.rno = rno - 1;
				rec = ThisDB[ctrl.rno];
				value = rec->value;
				if		(  ( pno = (int)g_hash_table_lookup(rec->opt.db->paths,
															pname) )  !=  0  ) {
					ctrl.pno = pno - 1;
					path = rec->opt.db->path[pno-1];
					value = ( path->args != NULL ) ? path->args : value;
					if		(  ( ono = (int)g_hash_table_lookup(path->opHash,func) )  !=  0  )	{
						op = path->ops[ono-1];
						value = ( op->args != NULL ) ? op->args : value;
					}
				} else {
					ctrl.pno = 0;
				}
			} else {
				ctrl.rno = 0;
				rec = NULL;
			}
		} else {
			DecodeStringURL(func,p);
			ctrl.rno = 0;
			ctrl.pno = 0;
			rec = NULL;
		}
		strcpy(ctrl.func,func);
		RecvData(fpComm,value);
		ctrl.rc = 0;
		ExecDB_Process(&ctrl,rec,value);
		fType = ( ses->type == COMM_STRINGE ) ? TRUE : FALSE;
		WriteClientString(fpComm,fType,&ctrl,value);
		ret = TRUE;
	} else
	if		(  strncmp(input,"Schema: ",8)  ==  0  ) {
		dbgmsg("schema");
		p = input + 8;
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
			DecodeStringURL(rname,p);
			p = q + 1;
			DecodeStringURL(pname,p);
			if		(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) {
				ctrl.rno = rno - 1;
				rec = ThisDB[ctrl.rno];
				if		(  ( pno = (int)g_hash_table_lookup(rec->opt.db->paths,
															pname) )  ==  0  ) {
					rec = NULL;
				}
			} else {
				rec = NULL;
			}
		} else {
			rec = NULL;
		}
		if		(  rec  !=  NULL  ) {
			WriteClientStructure(fpComm,rec);
			ret = TRUE;
		} else {
			ret = FALSE;
		}
	} else
	if		(  strncmp(input,"End",3)  ==  0  ) {
		dbgmsg("end");
		ret = FALSE;
	} else {
		printf("invalid message [%s]\n",input);
		ret = FALSE;
	}
	return	(ret);
}

static	Bool
MainLoop(
	NETFILE	*fpComm,
	SessionNode	*ses)
{
	char	buff[SIZE_BUFF+1];
	Bool	ret;

dbgmsg(">MainLoop");
	RecvStringDelim(fpComm,SIZE_BUFF,buff);
	switch	(ses->type) {
	  case	COMM_STRING:
	  case	COMM_STRINGE:
		ret = do_String(fpComm,buff,ses);
		break;
	  default:
		ret = FALSE;
		break;
	}
dbgmsg("<MainLoop");
	return	(ret);
}

static	void
ExecuteServer(void)
{
	int		fh
	,		_fh;
	NETFILE	*fp;
	int		pid;
	SessionNode	*ses;
	Port	*port;

dbgmsg(">ExecuteServer");
	port = ParPortName(PortNumber);
	_fh = InitServerPort(port,Back);
	while	(TRUE)	{
		if		(  ( fh = accept(_fh,0,0) )  <  0  )	{
			printf("_fh = %d\n",_fh);
			Error("INET Domain Accept");
		}

		if		(  ( pid = fork() )  >  0  )	{	/*	parent	*/
			close(fh);
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
			fp = SocketToNet(fh);
			close(_fh);
			if		(  ( ses = InitDBSSession(fp) )  !=  NULL  ) {
				while	(  MainLoop(fp,ses)  );
				FinishSession(ses);
			}
			CloseNet(fp);
			exit(0);
		}
	}
dbgmsg("<ExecuteServer");
}

static	void
StopProcess(
	int		ec)
{
dbgmsg(">StopProcess");
dbgmsg("<StopProcess");
	exit(ec);
}

static	char		*AuthURL;
static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"環境のベースディレクトリ"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"ddir",	STRING,			TRUE,	(void*)&D_Dir,
		"定義格納ディレクトリ"			 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"ディレクトリファイル"	 						},

	{	"dbhost",	STRING,		TRUE,	(void*)&DB_Host,
		"データベース稼働ホスト名"						},
	{	"dbport",	STRING,		TRUE,	(void*)&DB_Port,
		"データベース待機ポート番号"					},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"データベース名"								},
	{	"dbuser",	STRING,		TRUE,	(void*)&DB_User,
		"データベースのユーザ名"						},
	{	"dbpass",	STRING,		TRUE,	(void*)&DB_Pass,
		"データベースのパスワード"						},

	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		"認証サーバ"			 						},

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"dbredirectorの起動をチェックしない"			},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"dbredirectorを使わない"						},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = PORT_DBSERV;
	Back = 5;

	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	AuthURL = "glauth://localhost:8001";	/*	PORT_GLAUTH	*/

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;

	fNoCheck = FALSE;
	fNoRedirect = FALSE;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	int			rc;

	(void)signal(SIGHUP,(void *)StopProcess);
	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("dbs",NULL);

	ParseURL(&Auth,AuthURL,"file");

	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		InitNET();
		InitSystem(fl->name);
		ExecuteServer();
		StopProcess(0);
		rc = 0;
	} else {
		rc = -1;
		fprintf(stderr,"DBD名が指定されていません\n");
	}
	exit(rc);
}
