/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2003-2009 Ogochan.
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

#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>
#include	<setjmp.h>
#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"directory.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"queue.h"
#include	"driver.h"
#include	"message.h"
#include	"debug.h"

#define	DBIN_FILENO		3
#define	DBOUT_FILENO	4

static	char			*ExecPath;
static	RecordStruct	*recDBCTRL;

static	NETFILE		*fpDBR
		,			*fpDBW;

static	pthread_t		_DB_Thread;

static	LargeByteString	*iobuff;
static	LargeByteString	*dbbuff;

static	void
DumpNode(
	ProcessNode	*node)
{
#ifdef	DEBUG
ENTER_FUNC;
	printf("node = %p\n",node); 
	printf("mcp = [%s]\n",node->mcprec->name);
	//	DumpValueStruct(node->mcprec->value); 
LEAVE_FUNC;
#endif
}

static	void
ExecuteDB_Server(
	MessageHandler	*handler)
{
	RecordStruct	*rec;
	ValueStruct		*value
		,			*ret;
	PathStruct		*path;
	DB_Operation	*op;
	size_t			size;
	int				rno
		,			pno
		,			ono;
	DBCOMM_CTRL		ctrl;
	char			*rname
	,				*pname
	,				*func;
	ConvFuncs		*conv;

ENTER_FUNC;
	conv = handler->serialize;
	while	(TRUE) {
		dbgmsg("read");
		LBS_EmitStart(dbbuff);
		RecvLargeString(fpDBR,dbbuff);		ON_IO_ERROR(fpDBR,badio);
		ConvSetRecName(handler->conv,recDBCTRL->name);
		InitializeValue(recDBCTRL->value);
		conv->UnPackValue(handler->conv,LBS_Body(dbbuff),recDBCTRL->value);
		rname = ValueStringPointer(GetItemLongName(recDBCTRL->value,"rname"));
		value = NULL;
		path = NULL;
		rec = NULL;
		ret = NULL;
		if		(	(  rname  !=  NULL  ) 
					&&	(  ( rno = (int)(long)g_hash_table_lookup(DB_Table,rname) )  !=  0  ) ) {
			ctrl.rno = rno - 1;
			rec = ThisDB[ctrl.rno];
			value = rec->value;
			pname = ValueStringPointer(GetItemLongName(recDBCTRL->value,"pname"));
			if		(  ( pno = (int)(long)g_hash_table_lookup(rec->opt.db->paths,
														pname) )  !=  0  ) {
				ctrl.pno = pno - 1;
				path = rec->opt.db->path[pno-1];
				value = ( path->args != NULL ) ? path->args : value;
			} else {
				ctrl.pno = 0;
			}
		}
		func = ValueStringPointer(GetItemLongName(recDBCTRL->value,"func"));
		if		(  *func  !=  0  ) {
			if		(  path  !=  NULL  ) {
				if		( ( ono = (int)(long)g_hash_table_lookup(path->opHash,func) )  !=  0  ) {
					op = path->ops[ono-1];
					value = ( op->args != NULL ) ? op->args : value;
				}
			}
			if		(  value  !=  NULL  ) {
				ConvSetRecName(handler->conv,rec->name);
				InitializeValue(value);
				conv->UnPackValue(handler->conv,LBS_Body(dbbuff), value);
			}
			ctrl.limit = 1;
			strcpy(ctrl.func,func);
			if		(  ( ctrl.rc = ValueInteger(GetItemLongName(recDBCTRL->value,"rc")) )
						 >=  0  ) {
				ret = ExecDB_Process(&ctrl,rec,value,ThisDB_Environment);
			}
		} else {
			ctrl.rc = 0;
		}
		dbgmsg("write");
		SetValueInteger(GetItemLongName(recDBCTRL->value,"rc"),ctrl.rc);
		ConvSetRecName(handler->conv,recDBCTRL->name);
		LBS_EmitStart(dbbuff);
		size = conv->SizeValue(handler->conv,recDBCTRL->value);
		LBS_ReserveSize(dbbuff,size,FALSE);
		conv->PackValue(handler->conv,LBS_Body(dbbuff), recDBCTRL->value);
		LBS_EmitEnd(dbbuff);
		SendLargeString(fpDBW,dbbuff);			ON_IO_ERROR(fpDBW,badio);
		if		(  ret == NULL && value != NULL ) {
			ret = DuplicateValue(value,TRUE);
		}
		if		(  ret  !=  NULL  ) {
			Send(fpDBW,conv->fsep,strlen(conv->fsep));		ON_IO_ERROR(fpDBW,badio);
			LBS_EmitStart(dbbuff);
			ConvSetRecName(handler->conv,rec->name);
			size = conv->SizeValue(handler->conv,ret);
			LBS_ReserveSize(dbbuff,size,FALSE);
			conv->PackValue(handler->conv,LBS_Body(dbbuff), ret);
			LBS_EmitEnd(dbbuff);
			SendLargeString(fpDBW,dbbuff);	ON_IO_ERROR(fpDBW,badio);
			FreeValueStruct(ret);
		}
		Send(fpDBW,conv->bsep,strlen(conv->bsep));		ON_IO_ERROR(fpDBW,badio);
	}
  badio:
LEAVE_FUNC;
}

static	void
StartDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
	pthread_create(&_DB_Thread,NULL,(void *(*)(void *))ExecuteDB_Server,handler);
LEAVE_FUNC;
}

static	void
CancelDB(void)
{
ENTER_FUNC;
	if		(  pthread_kill(_DB_Thread,0)  ==  0  ) {
		pthread_cancel(_DB_Thread);
		pthread_join(_DB_Thread,NULL);
	}
LEAVE_FUNC;
}

static	void
PutApplication(
	MessageHandler	*handler,
	NETFILE		*fp,
	ProcessNode	*node)
{
	int		i;
	size_t	size;
	ConvFuncs	*conv;

ENTER_FUNC;
	DumpNode(node);
	conv = handler->serialize;
	ConvSetRecName(handler->conv,node->mcprec->name);
	size = conv->SizeValue(handler->conv,node->mcprec->value);
	LBS_EmitStart(iobuff);
	LBS_ReserveSize(iobuff,size,FALSE);
	conv->PackValue(handler->conv,LBS_Body(iobuff),node->mcprec->value);
	LBS_EmitEnd(iobuff);
	SendLargeString(fp,iobuff);					ON_IO_ERROR(fp,badio);
	Send(fp,conv->fsep,strlen(conv->fsep));		ON_IO_ERROR(fp,badio);

	if		(  node->linkrec  !=  NULL  ) {
		ConvSetRecName(handler->conv,node->linkrec->name);
		size = conv->SizeValue(handler->conv,node->linkrec->value);
		LBS_EmitStart(iobuff);
		LBS_ReserveSize(iobuff,size,FALSE);
		conv->PackValue(handler->conv,LBS_Body(iobuff),node->linkrec->value);
		LBS_EmitEnd(iobuff);
		SendLargeString(fp,iobuff);					ON_IO_ERROR(fp,badio);
		Send(fp,conv->fsep,strlen(conv->fsep));		ON_IO_ERROR(fp,badio);
	}

	if		(  node->sparec  !=  NULL  ) {
		ConvSetRecName(handler->conv,node->sparec->name);
		size = conv->SizeValue(handler->conv,node->sparec->value);
		LBS_EmitStart(iobuff);
		LBS_ReserveSize(iobuff,size,FALSE);
		conv->PackValue(handler->conv,LBS_Body(iobuff),node->sparec->value);
		LBS_EmitEnd(iobuff);
		SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
	}

	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		LBS_EmitStart(iobuff);
		if		(  node->scrrec[i]  !=  NULL  ) {
			ConvSetRecName(handler->conv,node->scrrec[i]->name);
			size = conv->SizeValue(handler->conv,node->scrrec[i]->value);
			if		(  size  >  0  ) {
				Send(fp,conv->fsep,strlen(conv->fsep));		ON_IO_ERROR(fp,badio);
			}
			LBS_ReserveSize(iobuff,size,FALSE);
			conv->PackValue(handler->conv,LBS_Body(iobuff),node->scrrec[i]->value);
			LBS_EmitEnd(iobuff);
			SendLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);
		}
	}
	Send(fp,conv->bsep,strlen(conv->bsep));		ON_IO_ERROR(fp,badio);
  badio:
LEAVE_FUNC;
}

static	void
GetApplication(
	MessageHandler	*handler,
	NETFILE		*fp,
	ProcessNode	*node)
{
	int		i;
	ConvFuncs	*conv;

ENTER_FUNC;
	conv = handler->serialize;

	LBS_EmitStart(iobuff);
	RecvLargeString(fp,iobuff);		ON_IO_ERROR(fp,badio);

	ConvSetRecName(handler->conv,node->mcprec->name);
	conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->mcprec->value);

	if		(  node->linkrec  !=  NULL  ) {
		ConvSetRecName(handler->conv,node->linkrec->name);
		conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->linkrec->value);
	}

	if		(  node->sparec  !=  NULL  ) {
		ConvSetRecName(handler->conv,node->sparec->name);
		conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->sparec->value);
	}

	for	( i = 0 ; i < node->cWindow ; i ++ ) {
		if		(	(  node->scrrec[i]         !=  NULL  )
				&&	(  node->scrrec[i]->value  !=  NULL  ) ) {
			ConvSetRecName(handler->conv,node->scrrec[i]->name);
			conv->UnPackValue(handler->conv,LBS_Body(iobuff),node->scrrec[i]->value);
		}
	}
	DumpNode(node);
  badio:
LEAVE_FUNC;
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
	MessageHandler	*handler,
	ProcessNode	*node)
{
	char	*module;
	int		pid;
	int		pAPR[2]
	,		pAPW[2]
	,		pDBR[2]
	,		pDBW[2];
	Bool	rc;
	NETFILE	*fpAPR
	,		*fpAPW;
	char	line[SIZE_BUFF]
	,		**cmd;


ENTER_FUNC;
	if		(  handler->loadpath  ==  NULL  ) {
		handler->loadpath = ExecPath;
	}
	signal(SIGPIPE, SignalHandler);
	module = ValueToString(GetItemLongName(node->mcprec->value,"dc.module"),NULL);
	ExpandStart(line,handler->start,handler->loadpath,module,"");
	cmd = ParCommandLine(line);

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
			execv(cmd[0],cmd);
		} else {
			fpAPR = FileToNet(pAPR[0]);
			close(pAPR[1]);
			fpAPW = FileToNet(pAPW[1]);
			close(pAPW[0]);
			fpDBR = FileToNet(pDBR[0]);
			close(pDBR[1]);
			fpDBW = FileToNet(pDBW[1]);
			close(pDBW[0]);
			StartDB(handler);
			PutApplication(handler,fpAPW,node);
			GetApplication(handler,fpAPR,node);
			(void)wait(&pid);
			CancelDB();
			signal(SIGPIPE, SIG_DFL);
			CloseNet(fpAPW);
			CloseNet(fpAPR);
			CloseNet(fpDBW);
			CloseNet(fpDBR);
		}
		rc = TRUE;
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc); 
}

static	void
_StopDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
_StopDC(
	MessageHandler	*handler)
{
ENTER_FUNC;
	if		(  ThisLD->cDB  >  0  ) {
		_StopDB(handler);
	}
LEAVE_FUNC;
}

static	void
_ReadyDC(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}

static	void
_ReadyDB(
	MessageHandler	*handler)
{
ENTER_FUNC;
LEAVE_FUNC;
}


static	int
_StartBatch(
	MessageHandler	*handler,
	char	*name,
	char	*param)
{
	int		pid;
	int		pDBR[2]
	,		pDBW[2];
	char	line[SIZE_BUFF]
	,		**cmd;
	int		rc;
	int		pAPR[2]
	,		pAPW[2];

ENTER_FUNC;
	if		(  handler->loadpath  ==  NULL  ) {
		handler->loadpath = ExecPath;
	}
	signal(SIGPIPE, SignalHandler);
	if		(  LibPath  ==  NULL  ) { 
		ExecPath = getenv("APS_EXEC_PATH");
	} else {
		ExecPath = LibPath;
	}
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
	ExpandStart(line,handler->start,handler->loadpath,name,param);
	cmd = ParCommandLine(line);

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
			execv(cmd[0],cmd);
		} else {
			close(pAPR[1]);
			close(pAPW[0]);
			fpDBR = FileToNet(pDBR[0]);
			close(pDBR[1]);
			fpDBW = FileToNet(pDBW[1]);
			close(pDBW[0]);
			StartDB(handler);
		}
		(void)wait(&pid);
		CancelDB();
		CloseNet(fpDBW);
		CloseNet(fpDBR);
		rc = TRUE;
	} else {
		rc = FALSE;
	}
ENTER_FUNC;
	return	(rc);
}

static	void
_ReadyExecute(
	MessageHandler	*handler,
	char			*loadpath)
{
	if		(  LibPath  ==  NULL  ) { 
		if		(  ( ExecPath = getenv("APS_EXEC_PATH") )  ==  NULL  ) {
			if		(  loadpath  !=  NULL  ) {
				ExecPath = loadpath;
			} else {
				ExecPath = MONTSUQI_LOAD_PATH;
			}
		}
	} else {
		ExecPath = LibPath;
	}
	if		(  handler->loadpath  ==  NULL  ) {
		handler->loadpath = ExecPath;
	}
}

static	MessageHandlerClass	Handler = {
	"Exec",
	_ReadyExecute,
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
ENTER_FUNC;
	if		(  LibPath  ==  NULL  ) { 
		ExecPath = getenv("APS_EXEC_PATH");
	} else {
		ExecPath = LibPath;
	}
	iobuff = NewLBS();
	recDBCTRL = BuildDBCTRL();
	dbbuff = NewLBS();
LEAVE_FUNC;
	return	(&Handler);
}
