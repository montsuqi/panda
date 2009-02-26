/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2009 Ogochan & JMA (Japan Medical Association).
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

#define	MAIN
/*
*/
#define	DEBUG
#define	TRACE

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
#include	"RecParser.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	DBD_Struct	*ThisDBD;
static	sigset_t	hupset;
static	char		*PortNumber;
static	int			Back;
static	char		*Directory;
static	char		*AuthURL;
static	URL			Auth;

static	void
InitSystem(
	char	*name)
{
ENTER_FUNC;
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);

	InitDirectory();
	SetUpDirectory(Directory,"","",name,TRUE);
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
LEAVE_FUNC;
}

#define	COMM_STRING		1
#define	COMM_BINARY		2
#define	COMM_STRINGE	3

typedef	struct {
	char	user[SIZE_USER+1];
	int		type;
	Bool	fCount
	,		fLimit;
	DB_Environment	*env;
}	SessionNode;

static	void
FinishSession(
	SessionNode	*node)
{
ENTER_FUNC;
	xfree(node);
LEAVE_FUNC;
}

static	int
ParseVersion(
	char	*str)
{
	int		major;
	int		minor;
	int		micro;

	major = 0;
	minor = 0;
	micro = 0;

	while	(	(  *str  !=  0    )
			&&	(  isdigit(*str)  ) )	{
		major = major * 10 + ( *str - '0' );
		str ++;
	}
	if		(  *str  !=  0  )	str ++;
	while	(	(  *str  !=  0    )
			&&	(  isdigit(*str)  ) )	{
		minor = minor * 10 + ( *str - '0' );
		str ++;
	}
	if		(  *str  !=  0  )	str ++;
	while	(	(  *str  !=  0    )
			&&	(  isdigit(*str)  ) )	{
		micro = micro * 10 + ( *str - '0' );
		str ++;
	}
	dbgprintf("%d.%d.%d",major,minor,micro);
	return	(major * 10000 + minor * 100 + micro);
}

static	SessionNode	*
NewSessionNode(void)
{
	SessionNode	*ses;

	ses = New(SessionNode);
	*ses->user = 0;
	ses->type = COMM_STRING;
	ses->fCount = FALSE;
	ses->fLimit = FALSE;
	ses->env = NULL;

	return	(ses);
}

static	SessionNode	*
InitDBSSession(
	NETFILE	*fpComm)
{
	SessionNode	*ses;
	char	buff[SIZE_BUFF+1];
	char	*pass;
	char	*p
	,		*q;
	int		ver;

ENTER_FUNC;
	ses = NewSessionNode();
	/*
	 version user password type\n
	 */
	RecvStringDelim(fpComm,SIZE_BUFF,buff);
	p = buff;
	*(q = strchr(p,' ')) = 0;
	ver = ParseVersion(p);
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
	if		(  ver  <  10200  ) {
		SendStringDelim(fpComm,"Error: version\n");
 		Warning("reject client(invalid version %d)",ver);
		xfree(ses);
		ses = NULL;
	} else
	if		(  AuthUser(&Auth,ses->user,pass,NULL,NULL)  ) {
		SendStringDelim(fpComm,"Connect: OK\n");
		if		(  ver  >=  10403  ) {
			ses->fCount = TRUE;
			ses->fLimit = TRUE;
		}
	} else {
		SendStringDelim(fpComm,"Error: authentication\n");
		Warning("reject client(authentication error)");
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
	ValueStruct		*value;

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
	SessionNode	*ses,
	NETFILE		*fpComm,
	Bool		fType,
	DBCOMM_CTRL	*ctrl,
	ValueStruct	*args)
{
	char	name[SIZE_BUFF+1]
	,		rname[SIZE_BUFF+1]
	,		vname[SIZE_BUFF+1];
	char	buff[SIZE_BUFF+1];
	ValueStruct	*value
		,		*rec;
	char	*p
		,	*q;
	Bool	fName;
	int		ix
		,	i
		,	count;

ENTER_FUNC;
	SendStringDelim(fpComm,"Exec: ");
	if		(  ses->fCount  ) {
		sprintf(buff,"%d:%d\n",ctrl->rc,ctrl->count);
	} else {
		sprintf(buff,"%d\n",ctrl->rc);
		ctrl->count = 1;
	}
	SendStringDelim(fpComm,buff);
	dbgprintf("[%s]",buff);
	ix = 0;
	fName = FALSE;
	while	(  RecvStringDelim(fpComm,SIZE_BUFF,name)  ) {
		if		(  *name  ==  0  )	{
			ix ++;
			if		(  ix  >=  ctrl->count  )	break;
		}
		dbgprintf("name = [%s]",name);
		if		(  ( p = strchr(name,';') )  !=  NULL  ) {
			*p = 0;
			q = p + 1;
			count = atoi(q);
		} else {
			q = name;
			count = 1;
		}
		if		(  !ses->fCount  ) {
			count = 1;
		}
		if		(  ( p = strchr(q,':') )  !=  NULL  ) {
			*p = 0;
			fName = FALSE;
		} else {
			fName = TRUE;
		}
		dbgprintf("count = %d",count);
		dbgprintf("name = [%s]",name);
		DecodeName(rname,vname,name);
		if		(  count  >  1  ) {
			for	( i = 0 ; i < count ; i ++ ) {
				rec = ValueArrayItem(args,ix);
				if		(  *vname  !=  0  ) {
					value = GetItemLongName(rec,vname);
				} else {
					value = rec;
				}
				SetValueName(name);
				SendValueString(fpComm,value,NULL,fName,fType,NULL);
				if		(  fName  ) {
					SendStringDelim(fpComm,"\n");
				}
				ix ++;
				if		(  ix  ==  ctrl->count  )	break;
			}
		} else {
			if		(  *vname  !=  0  ) {
				value = GetItemLongName(args,vname);
			} else {
				value = args;
			}
#ifdef	DEBUG
			DumpValueStruct(value);
#endif
			if		(  value  !=  NULL  ) {
				SetValueName(name);
				dbgmsg("*");
				SendValueString(fpComm,value,NULL,fName,fType,NULL);
			}
		}
		if		(  fName  ) {
			SendStringDelim(fpComm,"\n");
		}
	}
LEAVE_FUNC;
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
		sprintf(buff,"char(%d)",(int)ValueStringLength(value));
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_VARCHAR:
		sprintf(buff,"varchar(%d)",(int)ValueStringLength(value));
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_DBCODE:
		sprintf(buff,"dbcode(%d)",(int)ValueStringLength(value));
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_NUMBER:
		if		(  ValueFixedSlen(value)  ==  0  ) {
			sprintf(buff,"number(%d)",(int)ValueFixedLength(value));
		} else {
			sprintf(buff,"number(%d,%d)",
					(int)ValueFixedLength(value),
					(int)ValueFixedSlen(value));
		}
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_TEXT:
		sprintf(buff,"text");
		SendStringDelim(fp,buff);
		break;
	  case	GL_TYPE_ARRAY:
		DumpItems(fp,ValueArrayItem(value,0));
		sprintf(buff,"[%d]",(int)ValueArraySize(value));
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
	SessionNode		*ses,
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
	ValueStruct		*value
		,			*arg;
	RecordStruct	*rec;
	char			func[SIZE_FUNC+1]
		,			rname[SIZE_RNAME+1]
		,			pname[SIZE_PNAME+1];
	char	*p
		,	*q;
	int		rno
		,	pno;
	Bool	fOn;

	if		(  strncmp(input,"Exec: ",6)  ==  0  ) {
		dbgmsg("exec");
		p = input + 6;
		ctrl.count = 0;
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
			p = q + 1;
			if		(  ( q = strchr(p,':') )  !=  NULL  ) {
				*q = 0;
				ctrl.limit = atoi(q+1);
			} else {
				ctrl.limit = 1;
			}
			if		(  !ses->fLimit  ) {
				ctrl.limit = 1;
			}
			DecodeStringURL(pname,p);
			rec = MakeCTRLbyName(&arg,&ctrl,rname,pname,func);
		} else {
			DecodeStringURL(func,p);
			ctrl.rno = 0;
			ctrl.pno = 0;
			ctrl.rc = 0;
			strcpy(ctrl.func,func);
			rec = NULL;
			value = NULL;
		}
		RecvData(fpComm,arg);
		value = NULL;
		dbgprintf("func = [%s]",func);
		if		(  stricmp(func,"DBOPEN")  ==  0  ) {
			ses->env = OpenAllDB();
		} else
		if		(  stricmp(func,"DBCLOSE")  ==  0  ) {
			ctrl.rc = CloseAllDB(ses->env);
		} else
		if		(  stricmp(func,"DBSTART")  ==  0  ) {
			ctrl.rc = TransactionAllStart(ses->env);
		} else
		if		(  strcmp(func,"DBCOMMIT")  ==  0  ) {
			ctrl.rc = TransactionAllEnd(ses->env);
		} else
		if		(  strcmp(func,"DBDISCONNECT")  ==  0  ) {
			ctrl.rc = CloseAllDB(ses->env);
		} else {
			value = ExecDB_Process(&ctrl,rec,arg,ses->env);
		}
		fType = ( ses->type == COMM_STRINGE ) ? TRUE : FALSE;
		WriteClientString(ses,fpComm,fType,&ctrl,value);
		FreeValueStruct(value);
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
			if		(  ( rno = (int)(long)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) {
				ctrl.rno = rno - 1;
				rec = ThisDB[ctrl.rno];
				if		(  ( pno = (int)(long)g_hash_table_lookup(rec->opt.db->paths,
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
			WriteClientStructure(ses,fpComm,rec);
			ret = TRUE;
		} else {
			ret = FALSE;
		}
	} else
	if		(  strncmp(input,"Feature: ",9)  ==  0  ) {
		dbgmsg("feature");
		p = input + 9;
		while	(  *p  !=  0  ) {
			while	(	(  *p  !=  0    )
					&&	(  isspace(*p)  ) ) p ++;
			if		(  *p  ==  0  )	break;
			if		(  ( q = strchr(p,',') )  !=  NULL  ) {
				*q = 0;
				q ++;
			} else {
				q = p + strlen(p);
			}
			if		(  *p  ==  '!'  ) {
				fOn = FALSE;
				p ++;
			} else {
				fOn = TRUE;
			}
			if		(  strcmp(p,"count")  ==  0  ) {
				ses->fCount = fOn;
			} else
			if		(  strcmp(p,"limit")  ==  0  ) {
				ses->fLimit = fOn;
			}
			p = q;
		}
		ret = TRUE;
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

ENTER_FUNC;
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
LEAVE_FUNC;
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

ENTER_FUNC;
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
LEAVE_FUNC;
}

static	void
StopProcess(
	int		ec)
{
ENTER_FUNC;
LEAVE_FUNC;
	exit(ec);
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"port number"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"connection waiting queue length" 				},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"base directory"				 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"record directory"				 				},
	{	"ddir",	STRING,			TRUE,	(void*)&D_Dir,
		"defines directory"				 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"config file name"		 						},

	{	"dbhost",	STRING,		TRUE,	(void*)&DB_Host,
		"DB host name(for override)"					},
	{	"dbport",	STRING,		TRUE,	(void*)&DB_Port,
		"DB port numver(for override)"					},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"DB name(for override)"							},
	{	"dbuser",	STRING,		TRUE,	(void*)&DB_User,
		"DB user name(for override)"					},
	{	"dbpass",	STRING,		TRUE,	(void*)&DB_Pass,
		"DB password(for override)"						},
	{	"dbsslmode",STRING,		TRUE,	(void*)&DB_Sslmode,
		"DB SSL mode(for override)"						},

	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		"authentication server"		 					},

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"no check dbredirector"							},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"no DB redirection"								},

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
	DB_Sslmode = NULL;
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

	(void)signal(SIGCHLD,SIG_IGN);
	(void)signal(SIGHUP,(void *)StopProcess);
	SetDefault();
	fl = GetOption(option,argc,argv,NULL);
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
		fprintf(stderr,"DBD is not specified\n");
	}
	exit(rc);
}
