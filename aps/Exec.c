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

#define	DEBUG
#define	TRACE
/*
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<iconv.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>
#include	<setjmp.h>
#include	<signal.h>
#include	"const.h"
#include	"types.h"
#include	"misc.h"
#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"directory.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"queue.h"
#include	"driver.h"
#include	"message.h"
#include	"debug.h"

static	CONVOPT		*ExecConv;
static	char		*ExecPath;
static	NETFILE		*fpDBR
		,			*fpDBW;
static	RecordStruct	*recDBCTRL;

static	pthread_t		_DB_Thread;
static	pthread_cond_t	condDB;
static	pthread_mutex_t	lockDB;

static	LargeByteString	*iobuff;

static	void
DumpNode(
	ProcessNode	*node)
{
#ifdef	DEBUG
dbgmsg(">DumpNode");
 printf("node = %p\n",node); 
//	DumpValueStruct(node->mcprec->value); 
dbgmsg("<DumpNode");
#endif
}

static	CONVOPT			*DbConv;
static	LargeByteString	*dbbuff;
static	void
ExecuteDB_Server(
	void	*para)
{
	RecordStruct	*rec;
	size_t			size;
	char			buff[SIZE_ARG];
	int				rno
	,				pno;
	DBCOMM_CTRL		ctrl;
	char			*rname
	,				*pname
	,				*func;

dbgmsg(">ExecuteDB_Server");
	while	(TRUE) {
		dbgmsg("read");
		LBS_EmitStart(dbbuff);
		RecvLargeString(fpDBR,dbbuff);		ON_IO_ERROR(fpDBR,badio);
		ConvSetRecName(DbConv,recDBCTRL->name);
		InitializeValue(recDBCTRL->value);
		CGI_UnPackValue(DbConv,LBS_Body(dbbuff),recDBCTRL->value);
		rname = ValueString(GetItemLongName(recDBCTRL->value,"rname"));
		if		(	(  rname  !=  NULL  ) 
				&&	(  ( rno = (int)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) ) {
			ctrl.rno = rno - 1;
			rec = ThisDB[ctrl.rno]; 
			pname = ValueString(GetItemLongName(recDBCTRL->value,"pname"));
			if		(  ( pno = (int)g_hash_table_lookup(rec->opt.db->paths,
														pname) )  !=  0  ) {
				ctrl.pno = pno - 1;
			} else {
				ctrl.pno = 0;
			}
			ConvSetRecName(DbConv,rec->name);
			InitializeValue(rec->value);
			CGI_UnPackValue(DbConv,LBS_Body(dbbuff), rec->value);
		} else {
			rec = NULL;
		}
		func = ValueString(GetItemLongName(recDBCTRL->value,"func"));
		if		(  *func  !=  0  ) {
			strcpy(ctrl.func,ValueString(GetItemLongName(recDBCTRL->value,"func")));
			ExecDB_Process(&ctrl,rec);
		} else {
			ctrl.rc = 0;
		}
		dbgmsg("write");
		sprintf(buff,"%s.rc=%d",recDBCTRL->name,ctrl.rc);
		Send(fpDBW,buff,strlen(buff));	ON_IO_ERROR(fpDBW,badio);
		if		(  rec  !=  NULL  ) {
			Send(fpDBW,"&",1);				ON_IO_ERROR(fpDBW,badio);
			LBS_EmitStart(dbbuff);
			ConvSetRecName(DbConv,rec->name);
			size = CGI_SizeValue(DbConv,rec->value);
			LBS_ReserveSize(dbbuff,size,FALSE);
			CGI_PackValue(DbConv,LBS_Body(dbbuff), rec->value);
			LBS_EmitEnd(dbbuff);
			SendLargeString(fpDBW,dbbuff);	ON_IO_ERROR(fpDBW,badio);
		}
		Send(fpDBW,"\n",1);				ON_IO_ERROR(fpDBW,badio);
	}
  badio:
dbgmsg("<ExecuteDB_Server");
}

static	RecordStruct	*
BuildDBCTRL(void)
{
	RecordStruct	*rec;
	FILE			*fp;

	if		(  ( fp = tmpfile() )  ==  NULL  ) {
		fprintf(stderr,"tempfile can not make.\n");
		exit(1);
	}
	fprintf(fp,	"dbctrl	{");
	fprintf(fp,		"rc int;");
	fprintf(fp,		"func	varchar(%d);",SIZE_FUNC);
	fprintf(fp,		"rname	varchar(%d);",SIZE_NAME);
	fprintf(fp,		"pname	varchar(%d);",SIZE_NAME);
	fprintf(fp,	"};");
	rewind(fp);
	rec = DD_Parse(fp,"");
	fclose(fp);

	return	(rec);
}

static	void
StartDB(void)
{
dbgmsg(">StartDB");
	pthread_create(&_DB_Thread,NULL,(void *(*)(void *))ExecuteDB_Server,NULL);
dbgmsg("<StartDB");
}

static	void
CancelDB(void)
{
dbgmsg(">CancelDB");
	if		(  pthread_kill(_DB_Thread,0)  ==  0  ) {
		pthread_cancel(_DB_Thread);
		pthread_join(_DB_Thread,NULL);
	}
dbgmsg("<CancelDB");
}

static	void
PutApplication(
	NETFILE		*fp,
	ProcessNode	*node)
{
	int		i;
	size_t	size;

dbgmsg(">PutApplication");
	DumpNode(node);

	ConvSetRecName(ExecConv,node->mcprec->name);
	size = CGI_SizeValue(ExecConv,node->mcprec->value);
	LBS_EmitStart(iobuff);
	LBS_ReserveSize(iobuff,size,FALSE);
	CGI_PackValue(ExecConv,LBS_Body(iobuff),node->mcprec->value);
	LBS_EmitEnd(iobuff);
	SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
	Send(fp,"&",1);					ON_IO_ERROR(fp,badio);

	ConvSetRecName(ExecConv,node->linkrec->name);
	size = CGI_SizeValue(ExecConv,node->linkrec->value);
	LBS_EmitStart(iobuff);
	LBS_ReserveSize(iobuff,size,FALSE);
	CGI_PackValue(ExecConv,LBS_Body(iobuff),node->linkrec->value);
	LBS_EmitEnd(iobuff);
	SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
	Send(fp,"&",1);					ON_IO_ERROR(fp,badio);

	ConvSetRecName(ExecConv,node->sparec->name);
	size = CGI_SizeValue(ExecConv,node->sparec->value);
	LBS_EmitStart(iobuff);
	LBS_ReserveSize(iobuff,size,FALSE);
	CGI_PackValue(ExecConv,LBS_Body(iobuff),node->sparec->value);
	LBS_EmitEnd(iobuff);
	SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);

	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		LBS_EmitStart(iobuff);
		if		(  node->scrrec[i]  !=  NULL  ) {
			ConvSetRecName(ExecConv,node->scrrec[i]->name);
			size = CGI_SizeValue(ExecConv,node->scrrec[i]->value);
			if		(  size  >  0  ) {
				Send(fp,"&",1);				ON_IO_ERROR(fp,badio);
			}
			LBS_ReserveSize(iobuff,size,FALSE);
			CGI_PackValue(ExecConv,LBS_Body(iobuff),node->scrrec[i]->value);
		}
		LBS_EmitEnd(iobuff);
		SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
	}
	Send(fp,"\n",1);		ON_IO_ERROR(fp,badio);
  badio:
dbgmsg("<PutApplication");
}

static	void
GetApplication(
	NETFILE		*fp,
	ProcessNode	*node)
{
	int		i;

dbgmsg(">GetApplication");
	LBS_EmitStart(iobuff);
	RecvLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);

	//dbgprintf("[%s]\n",LBS_Body(iobuff));
	ConvSetRecName(ExecConv,node->mcprec->name);
	CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->mcprec->value);

	ConvSetRecName(ExecConv,node->linkrec->name);
	CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->linkrec->value);

	ConvSetRecName(ExecConv,node->sparec->name);
	CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->sparec->value);

	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		if		(  node->scrrec[i]  !=  NULL  ) {
			ConvSetRecName(ExecConv,node->scrrec[i]->name);
			CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->scrrec[i]->value);
		}
	}
	DumpNode(node);
  badio:
dbgmsg("<GetApplication");
}

static	jmp_buf	SubError;

static	void
SignalHandler(
	int		dummy)
{
	longjmp(SubError,1);
}

#define	DBIN_FILENO		3
#define	DBOUT_FILENO	4

static	Bool
_ExecuteProcess(
	ProcessNode	*node)
{
	char	*module;
	int		pid;
	int		pAPR[2]
	,		pAPW[2]
	,		pDBR[2]
	,		pDBW[2];
	char	buff[SIZE_LONGNAME+1];
	Bool	rc;
	NETFILE	*fpAPR
	,		*fpAPW;


dbgmsg(">ExecuteProcess");
	signal(SIGPIPE, SignalHandler);
	module = ValueString(GetItemLongName(node->mcprec->value,"dc.module"));
	sprintf(buff,"%s/%s",ExecPath,module);
	if		(  pipe(pAPR)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	if		(  pipe(pAPW)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	if		(  pipe(pDBR)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	if		(  pipe(pDBW)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	if		(  setjmp(SubError)  ==  0  ) {
		if		(  ( pid = fork() )  ==  0  ) {
			dup2(pAPW[0],STDIN_FILENO);
			dup2(pAPR[1],STDOUT_FILENO);
			close(pAPW[0]);
			close(pAPW[1]);
			close(pAPR[0]);
			close(pAPR[1]);
			dup2(pDBW[0],DBIN_FILENO);
			dup2(pDBR[1],DBOUT_FILENO);
			close(pDBW[0]);
			close(pDBW[1]);
			close(pDBR[0]);
			close(pDBR[1]);
			execl(buff,buff,NULL);
		} else {
			fpAPR = FileToNet(pAPR[0]);
			close(pAPR[1]);
			fpAPW = FileToNet(pAPW[1]);
			close(pAPW[0]);
			fpDBR = FileToNet(pDBR[0]);
			close(pDBR[1]);
			fpDBW = FileToNet(pDBW[1]);
			close(pDBW[0]);
			StartDB();
		}
		PutApplication(fpAPW,node);
		GetApplication(fpAPR,node);
		(void)wait(&pid);
		CancelDB();
		signal(SIGPIPE, SIG_DFL);
		CloseNet(fpAPW);
		CloseNet(fpAPR);
		CloseNet(fpDBW);
		CloseNet(fpDBR);
		fpDBR = NULL;
		rc = TRUE;
	} else {
		rc = FALSE;
	}
dbgmsg("<ExecuteProcess");
	return	(rc); 
}

static	void
_StopDB(void)
{
}

static	void
_StopDC(void)
{
dbgmsg(">StopDC");
	if		(  ThisLD->cDB  >  0  ) {
		_StopDB();
	}
dbgmsg("<StopDC");
}

static	void
_ReadyDC(void)
{
dbgmsg(">ReadyApplication");
	if		(  LibPath  ==  NULL  ) { 
		ExecPath = getenv("APS_EXEC_PATH");
	} else {
		ExecPath = LibPath;
	}
	ExecConv = NewConvOpt();
	iobuff = NewLBS();
dbgmsg("<ReadyApplication");
}

static	void
_CleanUpDB(void)
{
	_StopDB();
}

static	void
_CleanUpDC(void)
{
}

static	void
_ReadyDB(void)
{
	recDBCTRL = BuildDBCTRL();
	DbConv = NewConvOpt();
	dbbuff = NewLBS();
}

static	int
_StartBatch(
	char	*name,
	char	*param)
{
	int		pid;
	int		pDBR[2]
	,		pDBW[2];
	char	line[SIZE_BUFF]
	,		**cmd;
	int		rc;

dbgmsg(">StartBatch");
	signal(SIGPIPE, SignalHandler);
	if		(  LibPath  ==  NULL  ) { 
		ExecPath = getenv("APS_EXEC_PATH");
	} else {
		ExecPath = LibPath;
	}
	if		(  pipe(pDBR)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	if		(  pipe(pDBW)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	sprintf(line,"%s/%s %s",ExecPath, name, param);
	cmd = ParCommandLine(line);

	if		(  setjmp(SubError)  ==  0  ) {
		if		(  ( pid = fork() )  ==  0  ) {
			dup2(pDBW[0],DBIN_FILENO);
			dup2(pDBR[1],DBOUT_FILENO);
			close(pDBW[0]);
			close(pDBW[1]);
			close(pDBR[0]);
			close(pDBR[1]);
			execv(cmd[0],cmd);
		} else {
			fpDBR = FileToNet(pDBR[0]);
			close(pDBR[1]);
			fpDBW = FileToNet(pDBW[1]);
			close(pDBW[0]);
			pthread_cond_signal(&condDB);
			(void)wait(&pid);
			CloseNet(fpDBW);
			CloseNet(fpDBR);
			fpDBR = NULL;
		}
		rc = TRUE;
	} else {
		rc = FALSE;
	}
dbgmsg("<StartBatch");
	return	(rc);
}

static	MessageHandlerClass	Handler = {
	"Exec",
	FALSE,
	_ExecuteProcess,
	_StartBatch,
	_ReadyDC,
	_StopDC,
	NULL,
	_ReadyDB,
	_StopDB,
	NULL
};

extern	MessageHandlerClass	*
Exec(void)
{
	return	(&Handler);
}
