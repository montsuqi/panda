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
#include	"misc.h"
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
#include	"dbs_main.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	DBD_Struct	*ThisDBD;
static	sigset_t	hupset;
static	char		*PortNumber;
static	int			Back;
static	char	*Directory;

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

	if		(  ThisDBD->cDB  >  0  ) {
		InitDB_Process();
	}
dbgmsg("<InitSystem");
}

#define	COMM_STRING		1
#define	COMM_BINARY		2

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
InitSession(
	NETFILE	*fpComm)
{
	SessionNode	*ses;
	char	buff[SIZE_BUFF+1];
	char	*pass;
	char	*ver;
	char	*p
	,		*q;

dbgmsg(">InitSession");
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
	} else {
		ses->type = COMM_STRING;
	}
	if		(  strcmp(ver,VERSION)  ) {
		SendStringDelim(fpComm,"Error: version\n");
		g_warning("reject client(invalid version)");
		xfree(ses);
		ses = NULL;
	} else
	if		(  AuthUser(ses->user,pass,NULL)  ) {
		SendStringDelim(fpComm,"Connect: OK\n");
	} else {
		SendStringDelim(fpComm,"Error: authentication\n");
		g_warning("reject client(authentication error)");
		xfree(ses);
		ses = NULL;
	}
dbgmsg("<InitSession");
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
	NETFILE	*fpComm)
{
	char	buff[SIZE_BUFF+1];
	char	vname[SIZE_BUFF+1]
	,		rname[SIZE_BUFF+1]
	,		str[SIZE_BUFF+1];
	char	*p;
	int		rno;
	ValueStruct	*value;
	RecordStruct	*rec;

	do {
		RecvStringDelim(fpComm,SIZE_BUFF,buff);
		if		(	(  *buff                     !=  0     )
				&&	(  ( p = strchr(buff,':') )  !=  NULL  ) ) {
			*p = 0;
			DecodeName(rname,vname,buff);
			DecodeStringURL(str,p+1);
			if		(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) {
				if		(  ( rec = ThisDB[rno-1] )  !=  NULL  ) {
					value = GetItemLongName(rec->value,vname);
					value->fUpdate = TRUE;
					SetValueString(value,str,DB_LOCALE);
				}
			}
		} else
			break;
	}	while	(TRUE);
}

static	void
WriteClient(
	NETFILE		*fpComm,
	DBCOMM_CTRL		*ctrl)
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

dbgmsg(">WriteClient");
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
		if		(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) {
			if		(  ( rec = ThisDB[rno-1] )  !=  NULL  ) {
				if		(  *vname  !=  0  ) {
					value = GetItemLongName(rec->value,vname);
				} else {
					value = rec->value;
				}
				SetValueName(name);
				SendValueString(fpComm,value,NULL,fName);
				if		(  fName  ) {
					SendStringDelim(fpComm,"\n");
				}
			}
		}
	}	while	(TRUE);
dbgmsg("<WriteClient");
}

static	Bool
MainLoop(
	NETFILE	*fpComm,
	SessionNode	*ses)
{
	DBCOMM_CTRL	ctrl;
	RecordStruct	*rec;
	Bool	ret;
	char	buff[SIZE_BUFF+1]
	,		func[SIZE_FUNC+1]
	,		rname[SIZE_RNAME+1]
	,		pname[SIZE_PNAME+1];
	char	*p
	,		*q;
	int		rno
	,		pno;

dbgmsg(">MainLoop");
	if		(  ses->type  ==  COMM_STRING  ) {
		RecvStringDelim(fpComm,SIZE_BUFF,buff);
		if		(  strncmp(buff,"Exec: ",6)  ==  0  ) {
			/*
			 *	Exec: func:rec:path\n
			 */
			dbgmsg("exec");
			p = buff + 6;
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
				if		(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) {
					ctrl.rno = rno - 1;
					rec = ThisDB[ctrl.rno];
					if		(  ( pno = (int)g_hash_table_lookup(rec->opt.db->paths,
																pname) )  !=  0  ) {
						ctrl.pno = pno - 1;
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
			RecvData(fpComm);
			ExecDB_Process(&ctrl,rec);
			WriteClient(fpComm,&ctrl);
			ret = TRUE;
		} else
		if		(  strncmp(buff,"End",3)  ==  0  ) {
			dbgmsg("end");
			ret = FALSE;
		} else {
			printf("invalid message [%s]\n",buff);
			ret = FALSE;
		}
	} else {
		ret = FALSE;
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

dbgmsg(">ExecuteServer");
	_fh = InitServerPort(PortNumber,Back);
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
			if		(  ( ses = InitSession(fp) )  !=  NULL  ) {
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

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = IntStrDup(PORT_DBSERV);
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

	ParseURL(&Auth,AuthURL);

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
