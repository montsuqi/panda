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

static	pthread_t	_DB_Thread;
static	CONVOPT		*ExecConv;
static	char		*ExecPath;

static	NETFILE	*fpAPR
		,		*fpAPW;

static	LargeByteString	*iobuff;

static	void
DumpNode(
	ProcessNode	*node)
{
#ifdef	DEBUG
dbgmsg(">DumpNode");
dbgmsg("<DumpNode");
#endif
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
dbgmsg("*");
	SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
dbgmsg("**");
	Send(fp,"&",1);					ON_IO_ERROR(fp,badio);
dbgmsg("***");

	ConvSetRecName(ExecConv,node->linkrec->name);
	size = CGI_SizeValue(ExecConv,node->linkrec->value);
	LBS_EmitStart(iobuff);
	LBS_ReserveSize(iobuff,size,FALSE);
	CGI_PackValue(ExecConv,LBS_Body(iobuff),node->linkrec->value);
	LBS_EmitEnd(iobuff);
	SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
	Send(fp,"&",1);					ON_IO_ERROR(fp,badio);
dbgmsg("****");

	ConvSetRecName(ExecConv,node->sparec->name);
	size = CGI_SizeValue(ExecConv,node->sparec->value);
	LBS_EmitStart(iobuff);
	LBS_ReserveSize(iobuff,size,FALSE);
	CGI_PackValue(ExecConv,LBS_Body(iobuff),node->sparec->value);
	LBS_EmitEnd(iobuff);
	SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
dbgmsg("*****");

	for	( i = 0 ; i < node->cWindow ; i ++ ) {
dbgmsg("-");
		LBS_EmitStart(iobuff);
dbgmsg("--");
		if		(  node->scrrec[i]  !=  NULL  ) {
dbgmsg("---");
			ConvSetRecName(ExecConv,node->scrrec[i]->name);
dbgmsg("----");
			size = CGI_SizeValue(ExecConv,node->scrrec[i]->value);
dbgmsg("-----");
			if		(  size  >  0  ) {
dbgmsg("------");
				Send(fp,"&",1);				ON_IO_ERROR(fp,badio);
dbgmsg("-------");
			}
			LBS_ReserveSize(iobuff,size,FALSE);
dbgmsg("--------");
			CGI_PackValue(ExecConv,LBS_Body(iobuff),node->scrrec[i]->value);
dbgmsg("---------");
		}
		LBS_EmitEnd(iobuff);
dbgmsg("----------");
		SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
	}
dbgmsg("******");
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

printf("*\n");
	ConvSetRecName(ExecConv,node->mcprec->name);
	CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->mcprec->value);
printf("**\n");

	ConvSetRecName(ExecConv,node->linkrec->name);
	CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->linkrec->value);
printf("***\n");

	ConvSetRecName(ExecConv,node->sparec->name);
	CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->sparec->value);
printf("****\n");

	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		if		(  node->scrrec[i]  !=  NULL  ) {
			ConvSetRecName(ExecConv,node->scrrec[i]->name);
			CGI_UnPackValue(ExecConv,LBS_Body(iobuff),node->scrrec[i]->value);
printf("*****\n");
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

static	Bool
_ExecuteProcess(
	ProcessNode	*node)
{
	char	*module;
	int		pid;
	int		ofiledes[2]
	,		ifiledes[2];
	char	buff[SIZE_LONGNAME+1];
	Bool	rc;

dbgmsg(">ExecuteProcess");
	signal(SIGPIPE, SignalHandler);
	module = ValueString(GetItemLongName(node->mcprec->value,"dc.module"));
	sprintf(buff,"%s/%s",ExecPath,module);
	if		(  pipe(ofiledes)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	if		(  pipe(ifiledes)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	if		(  setjmp(SubError)  ==  0  ) {
		if		(  ( pid = fork() )  ==  0  ) {
			dup2(ifiledes[0],STDIN_FILENO);
			dup2(ofiledes[1],STDOUT_FILENO);
			close(ifiledes[0]);
			close(ifiledes[1]);
			close(ofiledes[0]);
			close(ofiledes[1]);
			execl(buff,buff,NULL);
		} else {
			fpAPR = FileToNet(ofiledes[0]);
			close(ofiledes[1]);
			fpAPW = FileToNet(ifiledes[1]);
			close(ifiledes[0]);
		}
		PutApplication(fpAPW,node);
		GetApplication(fpAPR,node);
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	signal(SIGPIPE, SIG_DFL);
	CloseNet(fpAPW);
	CloseNet(fpAPR);
dbgmsg("<ExecuteProcess");
	return	(rc); 
}

static	void
_StopDB(void)
{
	if		(  pthread_kill(_DB_Thread,0)  ==  0  ) {
		pthread_cancel(_DB_Thread);
		pthread_join(_DB_Thread,NULL);
	}
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
	ConvSetSize(ExecConv,ThisLD->textsize,ThisLD->arraysize);
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
	CloseNet(fpAPW);
	CloseNet(fpAPR);
}

static	void
ExecuteDB_Server(
	void	*para)
{
dbgmsg(">ExecuteDB_Server");
dbgmsg("<ExecuteDB_Server");
}

static	void
StartDB(void)
{
	pthread_create(&_DB_Thread,NULL,(void *(*)(void *))ExecuteDB_Server,NULL);
}

static	void
_ReadyDB(void)
{
	StartDB();
}

static	int
_StartBatch(
	char	*name,
	char	*param)
{
	int		pid;
	int		filedes[2];

dbgmsg(">StartBatch");
	if		(  LibPath  ==  NULL  ) { 
		ExecPath = getenv("APS_EXEC_PATH");
	} else {
		ExecPath = LibPath;
	}

	if		(  pipe(filedes)  !=  0  ) {
		perror("pipe");
		exit(1);
	}
	ExecConv = NewConvOpt();
	ConvSetSize(ExecConv,ThisBD->textsize,ThisBD->arraysize);
	if		(  ( pid = fork() )  ==  0  ) {
		dup2(filedes[0],STDIN_FILENO);
		dup2(filedes[1],STDOUT_FILENO);
		dup2(filedes[1],STDERR_FILENO);
		close(filedes[0]);
		close(filedes[1]);
		execl(name,name,NULL);
	}
	(void)wait(&pid);
dbgmsg("<StartBatch");
	return	(0); 
}

static	MessageHandler	Handler = {
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

extern	MessageHandler	*
Exec(void)
{
	return	(&Handler);
}
