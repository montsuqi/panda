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
#ifdef	HAVE_DOTCOBOL
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
#include	<sys/stat.h>		/*	for	mknod	*/
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"misc.h"
#include	"libmondai.h"
#include	"directory.h"
#include	"handler.h"
#include	"const.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"queue.h"
#include	"pty.h"
#include	"debug.h"


#ifdef	USE_PTY
static	pthread_t	_ConsoleThread;
static	int			fhSub;
#endif
static	pthread_t	_DB_Thread;

static	int		CobolPID;

static	FILE	*fpAPR
		,		*fpAPW;
static	int		fhAPR
		,		fhAPW;

static	size_t	McpSize
		,		LinkSize
		,		SpaSize
		,		ScrSize;
static	void	*McpData
		,		*LinkData
		,		*SpaData
		,		*ScrData;
static	char	*Command;
static	CONVOPT	*dotCOBOL_Conv;

static	void
DumpNode(
	ProcessNode	*node)
{
#ifdef	DEBUG
dbgmsg(">DumpNode");
	dbgprintf("mcpsize  = %d\n",node->mcpsize);
	dbgprintf("linksize = %d\n",node->linksize);
	dbgprintf("spasize  = %d\n",node->spasize);
	dbgprintf("scrsize  = %d\n",node->scrsize);
dbgmsg("<DumpNode");
#endif
}


static	void
PutApplication(
	FILE		*fp,
	ProcessNode	*node)
{
	size_t	size
	,		left;
	int		i;
	char	*p;

dbgmsg(">PutApplication");

	DumpNode(node); 

	dotCOBOL_PackValue(dotCOBOL_Conv,McpData,node->mcprec->value);
	dotCOBOL_PackValue(dotCOBOL_Conv,LinkData,node->linkrec->value);
	dotCOBOL_PackValue(dotCOBOL_Conv,SpaData,node->sparec->value);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = dotCOBOL_PackValue(dotCOBOL_Conv,p,node->scrrec[i]->value);
	}

	size  = fwrite(McpData,1,McpSize,fp);
	size += fwrite(SpaData,1,SpaSize,fp);
	size += fwrite(LinkData,1,LinkSize,fp);
	size += fwrite(ScrData,1,ScrSize,fp);
	left = SIZE_BLOCK - ( size % SIZE_BLOCK );
	for	( ; left > 0 ; left -- ) {
		putc(' ',fp);
	}
	fflush(fp);
dbgmsg("<PutApplication");
}

#if	0
static	Bool
IsSpaces(
	char	*buff,
	size_t	size)
{
	Bool	rc;

	for	( ; size > 0 ; size -- , buff ++ ) {
		if		(  *buff  !=  ' '  )	break;
	}
	rc = ( size == 0 ) ? TRUE : FALSE;
	return	(rc);
}
#endif
static	void
GetApplication(
	FILE		*fp,
	ProcessNode	*node)
{
	size_t	size
	,		left;
	int		i;
	char	*p;

dbgmsg(">GetApplication");
	fread(McpData,1,McpSize,fp);
	size  = McpSize;
	size += fread(SpaData,1,SpaSize,fp);
	size += fread(LinkData,1,LinkSize,fp);
	size += fread(ScrData,1,ScrSize,fp);
#ifdef	DEBUG
	printf("get size = %d\n",size);
#endif
	left = SIZE_BLOCK - ( size % SIZE_BLOCK );
	for	( ; left > 0 ; left -- ) {
		(void)getc(fp);
	}

	dotCOBOL_UnPackValue(dotCOBOL_Conv,McpData,node->mcprec->value);
	dotCOBOL_UnPackValue(dotCOBOL_Conv,LinkData,node->linkrec->value);
	dotCOBOL_UnPackValue(dotCOBOL_Conv,SpaData,node->sparec->value);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = dotCOBOL_UnPackValue(dotCOBOL_Conv,p,node->scrrec[i]->value);
	}


	DumpNode(node);

dbgmsg("<GetApplication");
}


static	Bool
_ExecuteProcess(
	MessageHandler	*handler,
	ProcessNode	*node)
{
	Bool	rc;

dbgmsg(">ExecuteProcess");
	PutApplication(fpAPW,node);
	GetApplication(fpAPR,node);
	if		(  ValueInteger(GetItemLongName(node->mcprec->value,"rc"))  <  0  ) {
		rc = FALSE;
	} else {
		rc = TRUE;
	}
dbgmsg("<ExecuteProcess");
	return	(rc); 
}


#ifdef	USE_PTY
static	void
PrintSJIS(
	char	*istr)
{
	char	*ob;
	char	obb[3];
	size_t	sib
	,		sob;
	iconv_t		cd;

	if		(  istr  ==  NULL  )	return;
	if		(  ( cd = iconv_open("EUC-JP","Shift-JIS") )  <  0  ) {
		printf("code set error\n");
		exit(1);
	}
	
	sib = strlen(istr);
	
	do {
		ob = obb;
		sob = 3;
		if		(  iconv(cd,&istr,&sib,&ob,&sob)  <  0  ) {
			printf("error\n");
			exit(1);
		}
		*ob = 0;
		printf("%s",obb);
	}	while	(  sib  >  0  );
	iconv_close(cd);
}

static	void
ConsoleThread(
	void	*para)
{
	int		i;
	Bool	fExit;
	int		fh = (int)para;
	char	buff[SIZE_BUFF+1];

	fExit = FALSE;
	while	( !fExit ) {
		dbgmsg("msg");
		if		(  ( i = read(fh,buff,SIZE_BUFF) )  >  0  ) {
			buff[i] = 0;
			PrintSJIS(buff);
			printf("\n");
			fflush(stdout);
		} else {
			fExit = TRUE;
		}
	}
	close(fh);
}

static	void
StartConsole(void)
{
	pthread_create(&_ConsoleThread,NULL,(void *(*)(void *))ConsoleThread,(void *)fhSub);
}
#else
static	void
StartConsole(void)
{
}
#endif

static	void
_StopDB(
	MessageHandler	*handler)
{
	if		(  pthread_kill(_DB_Thread,0)  ==  0  ) {
		pthread_cancel(_DB_Thread);
		pthread_join(_DB_Thread,NULL);
	}
}

static	void
_StopDC(
	MessageHandler	*handler)
{
dbgmsg(">StopDC");
	if		(  ThisLD->cDB  >  0  ) {
		_StopDB(handler);
	}
	xfree(McpData);
	xfree(LinkData);
	xfree(SpaData);
	xfree(ScrData);
#ifdef	USE_PTY
	if		(  pthread_kill(_ConsoleThread,0)  ==  0  ) {
		pthread_cancel(_ConsoleThread);
		pthread_join(_ConsoleThread,NULL);
	}
#endif
dbgmsg("<StopDC");
}

static	void
ReadyApplication(
	MessageHandler *handler)
{
	int		pid;
	int		slave;
	char	line[SIZE_BUFF]
	,		**cmd;

dbgmsg(">ReadyApplication");
#ifdef	USE_PTY
	if		(  ( fhSub = GetMasterPTY() )  <  0  ) {
		printf("error GetMasterPTY\n");
		exit(1);
	}
#endif
	ExpandStart(line,handler->start,handler->loadpath,"","");
	cmd = ParCommandLine(line);
	if		(  ( pid = fork() )  ==  0  ) {
#ifdef	USE_PTY
		close(fhSub);
		if		(  ( slave = GetSlavePTY(PTY_Name) )  <  0  ) {
			printf("open GetSlavePTY\n");
			exit(1);
		}
		ioctl(slave,TIOCSCTTY,NULL);
		StartConsole();
#else
		slave = open("fculog",O_RDWR|O_CREAT|O_TRUNC,0600);
#endif
		dup2(slave,STDIN_FILENO);
		dup2(slave,STDOUT_FILENO);
		dup2(slave,STDERR_FILENO);
		if		(  slave  >  2  ) {
			close(slave);
		}
		execv(cmd[0],cmd);
	} else {
		CobolPID = pid;
	}
	StartConsole();
dbgmsg("<ReadyApplication");
}

static	void
_ReadyDC(
	MessageHandler	*handler)
{
	int		i;
dbgmsg(">ReadyDC");
	unlink("dc.input");
	mknod("dc.input",S_IFIFO|0600,0);
	unlink("dc.output");
	mknod("dc.output",S_IFIFO|0600,0);

	fhAPR = open("dc.output",O_RDWR);
	if		(  ( fpAPR = fdopen(fhAPR,"r+") )  ==  NULL  ) {
		fprintf(stderr,"can not open input pipe\n");
		exit(1);
	}

	fhAPW = open("dc.input",O_RDWR);
	if		(  ( fpAPW = fdopen(fhAPW,"w+") )  ==  NULL  ) {
		fprintf(stderr,"can not open output pipe\n");
		exit(1);
	}
	ReadyApplication(handler);

	dotCOBOL_Conv = NewConvOpt();
	ConvSetSize(dotCOBOL_Conv,ThisLD->textsize,ThisLD->arraysize);
	ConvSetCoding(dotCOBOL_Conv,handler->conv->locale);

	McpSize = dotCOBOL_SizeValue(dotCOBOL_Conv,ThisEnv->mcprec->value);
	McpData = xmalloc(McpSize);
	LinkSize = dotCOBOL_SizeValue(dotCOBOL_Conv,ThisEnv->linkrec->value);
	LinkData = xmalloc(LinkSize);
	SpaSize = dotCOBOL_SizeValue(dotCOBOL_Conv,ThisLD->sparec->value);
	SpaData = xmalloc(SpaSize);
	ScrSize = 0;
	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		ScrSize += dotCOBOL_SizeValue(dotCOBOL_Conv,ThisLD->window[i]->rec->value);
	}
	ScrData = xmalloc(ScrSize);
dbgmsg("<ReadyDC");
}


static	void
_CleanUpDB(
	MessageHandler	*handler)
{
	_StopDB(handler);
	unlink("db.input");
	unlink("db.output");
}

static	void
_CleanUpDC(
	MessageHandler	*handler)
{
	fclose(fpAPW);
	fclose(fpAPR);
	(void)kill(CobolPID,SIGKILL);
	unlink("dc.input");
	unlink("dc.output");
}

static	void
ReadControl(
	DBCOMM_CTRL	*ctrl)
{
	StringCobol2C(ctrl->func,SIZE_FUNC);
	dotCOBOL_IntegerCobol2C(&ctrl->blocks);
	dotCOBOL_IntegerCobol2C(&ctrl->rc);
	dotCOBOL_IntegerCobol2C(&ctrl->rno);
	dotCOBOL_IntegerCobol2C(&ctrl->pno);
}

static	void
SetControl(
	DBCOMM_CTRL	*ctrl)
{
	StringC2Cobol(ctrl->func,SIZE_FUNC);
	dotCOBOL_IntegerC2Cobol(&ctrl->blocks);
	dotCOBOL_IntegerC2Cobol(&ctrl->rc);
	dotCOBOL_IntegerC2Cobol(&ctrl->rno);
	dotCOBOL_IntegerC2Cobol(&ctrl->pno);
}

static	Bool
ReadDB_Request(
	FILE	*fpDBR,
	char	**block,
	size_t	*bnum)
{
	DBCOMM_CTRL	*ctrl;
	size_t	size;
	Bool	rc;
	char		*p;
	int			i;

dbgmsg(">ReadDB_Request");
	if		(  ( size = fread(*block,1,SIZE_BLOCK,fpDBR) )  ==  SIZE_BLOCK  ) {
		ctrl = (DBCOMM_CTRL *)*block;
		ReadControl(ctrl);
		DumpDB_Node(ctrl);

		if		(  ctrl->blocks  >  *bnum  ) {
			p = (char *)xmalloc(SIZE_BLOCK * ctrl->blocks);
			memcpy(p,*block,(SIZE_BLOCK * *bnum));
			xfree(*block);
			*block = p;
			*bnum = ctrl->blocks;
		}
		p = *block;
		for	( i = 1 ; i < ctrl->blocks ; i ++ ) {
			p += SIZE_BLOCK;
			if		(  ( size = fread(p,1,SIZE_BLOCK,fpDBR) )  !=  SIZE_BLOCK  )
				break;
		}
	}
	if		(  size  !=  SIZE_BLOCK  )	{
		rc = FALSE;
	} else {
		rc = TRUE;
	}
dbgmsg("<ReadDB_Request");
	return	(rc);
}

static	void
WriteDB_Reply(
	FILE	*fpDBW,
	char	*block,
	size_t	bnum)
{
	DBCOMM_CTRL	*ctrl;
	size_t		blocks;

	ctrl = (DBCOMM_CTRL *)block;
	blocks = ctrl->blocks;
	SetControl(ctrl);
	fwrite(block,1,SIZE_BLOCK * blocks,fpDBW);
	fflush(fpDBW);
}

static	void
MakeDB_Pipe(void)
{
dbgmsg(">MakeDB_Pipe");
	unlink("db.input");
	mknod("db.input",S_IFIFO|0600,0);
	unlink("db.output");
	mknod("db.output",S_IFIFO|0600,0);
dbgmsg("<MakeDB_Pipe");
}

static	void
ExecuteDB_Server(
	void	*para)
{
	int			fhDBR
	,			fhDBW;
	FILE		*fpDBR
	,			*fpDBW;
	char		*block
	,			*data;
	DBCOMM_CTRL	*ctrl;
	size_t		bnum;
	RecordStruct	*rec;

dbgmsg(">ExecuteDB_Server");
	fhDBR = open("db.output",O_RDWR);
	if		(  ( fpDBR = fdopen(fhDBR,"r+") )  ==  NULL  ) {
		fprintf(stderr,"can not open input pipe\n");
		exit(1);
	}
	fhDBW = open("db.input",O_RDWR);
	if		(  ( fpDBW = fdopen(fhDBW,"w+") )  ==  NULL  ) {
		fprintf(stderr,"can not open output pipe\n");
		exit(1);
	}
	block = (char *)xmalloc(SIZE_BLOCK);
	bnum = 1;
	while	(TRUE) {
		dbgmsg("read");
		if		(  !ReadDB_Request(fpDBR,&block,&bnum)  )	break;
		data = block + sizeof(DBCOMM_CTRL);
		ctrl = (DBCOMM_CTRL *)block;
		if		(  ctrl->rno  >=  0  ) {
			rec = ThisDB[ctrl->rno]; 
			dotCOBOL_UnPackValue(dotCOBOL_Conv,data, rec->value);
		} else {
			rec = NULL;
		}
		ExecDB_Process(ctrl,rec);
		if		(  rec  !=  NULL  ) {
			dotCOBOL_PackValue(dotCOBOL_Conv,data, rec->value);
		}
		dbgmsg("write");
		WriteDB_Reply(fpDBW,block,bnum);
	}
dbgmsg("<ExecuteDB_Server");
}

static	void
StartDB(void)
{
	pthread_create(&_DB_Thread,NULL,(void *(*)(void *))ExecuteDB_Server,NULL);
}

static	void
_ReadyDB(
	MessageHandler	*handler)
{
	MakeDB_Pipe();
	StartDB();
}

static	int
_StartBatch(
	MessageHandler	*handler,
	char	*name,
	char	*param)
{
	int		pid;
	char	line[SIZE_BUFF]
	,		**cmd;

dbgmsg(">StartBatch");
	dotCOBOL_Conv = NewConvOpt();
	ConvSetSize(dotCOBOL_Conv,ThisLD->textsize,ThisLD->arraysize);
	ConvSetCoding(dotCOBOL_Conv,handler->conv->locale);

	ExpandStart(line,handler->start,handler->loadpath,name,param);
	cmd = ParCommandLine(line);
	if		(  ( pid = fork() )  ==  0  ) {
#if	0
		int		slave;
		slave = open("fculog",O_RDWR|O_CREAT|O_TRUNC,0600);
		dup2(slave,STDIN_FILENO);
		dup2(slave,STDOUT_FILENO);
		dup2(slave,STDERR_FILENO);
		close(slave);
#endif
		execv(cmd[0],cmd);
	} else {
		CobolPID = pid;
	}
	(void)wait(&pid);
dbgmsg("<StartBatch");
	return	(0); 
}

static	MessageHandlerClass	Handler = {
	"dotCOBOL",
	NULL,
	_ExecuteProcess,
	_StartBatch,
	_ReadyDC,
	_StopDC,
	_CleanUpDC,
	_ReadyDB,
	_StopDB,
	_CleanUpDB,
};

extern	MessageHandlerClass	*
dotCOBOL(void)
{
	if		(  ( Command = getenv("DOTCOBOL_LINE") )  ==  NULL  ) {
		Command = DOTCOBOL_COMMAND " %m";
	}
	return	(&Handler);
}
#endif
