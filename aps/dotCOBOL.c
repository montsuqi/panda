/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
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
#include	"value.h"
#include	"comm.h"
#include	"dirs.h"
#include	"aps_main.h"
#include	"directory.h"
#include	"cobolvalue.h"
#include	"handler.h"
#include	"const.h"
#include	"defaults.h"
#include	"enum.h"
#include	"wfc.h"
#include	"apsio.h"
#include	"dbgroup.h"
#include	"Postgres.h"
#include	"i18n_code.h"
#include	"queue.h"
#include	"tcp.h"
#include	"pty.h"
#include	"driver.h"
#include	"dotCOBOL.h"
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

static	void
IntegerCobol2C(
	int		*ival)
{
	char	*p
	,		c;
	p = (char *)ival;
	c = p[3];
	p[3] = p[0];
	p[0] = c;
	c = p[2];
	p[2] = p[1];
	p[1] = c;
}

static	void
FixedCobol2C(
	char	*buff,
	size_t	size)
{
	buff[size] = 0;
	if		(  buff[size - 1]  >=  0x70  ) {
		buff[size - 1] ^= 0x40;
		*buff |= 0x40;
	}
}

static	void
FixedC2Cobol(
	char	*buff,
	size_t	size)
{
	if		(  *buff  >=  0x70  ) {
		*buff ^= 0x40;
		buff[size - 1] |= 0x40;
	}
}

static	char	*
UnPackValueCobol(
	char	*p,
	ValueStruct	*value)
{
	int		i;
	char	buff[SIZE_BUFF];

dbgmsg(">UnPackValueCobol");
	if		(  value  !=  NULL  ) {
		switch	(value->type) {
		  case	GL_TYPE_INT:
			value->body.IntegerData = *(int *)p;
			IntegerCobol2C(&value->body.IntegerData);
			p += sizeof(int);
			break;
		  case	GL_TYPE_BOOL:
			value->body.BoolData = ( *(char *)p == 'T' ) ? TRUE : FALSE;
			p ++;
			break;
		  case	GL_TYPE_BYTE:
			memcpy(value->body.CharData.sval,p,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_CHAR:
			memcpy(value->body.CharData.sval,p,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_TEXT:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
			memcpy(value->body.CharData.sval,p,value->body.CharData.len);
			p += value->body.CharData.len;
			StringCobol2C(value->body.CharData.sval,value->body.CharData.len);
			break;
		  case	GL_TYPE_NUMBER:
			memcpy(buff,p,ValueFixed(value)->flen);
			FixedCobol2C(buff,ValueFixed(value)->flen);
			strcpy(ValueFixed(value)->sval,buff);
			p += value->body.FixedData.flen;
			break;
		  case	GL_TYPE_ARRAY:
			for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
				p = UnPackValueCobol(p,value->body.ArrayData.item[i]);
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
				p = UnPackValueCobol(p,value->body.RecordData.item[i]);
			}
			break;
		  default:
			break;
		}
	}
dbgmsg("<UnPackValueCobol");
	return	(p);
}

static	char	*
PackValueCobol(
	char	*p,
	ValueStruct	*value)
{
	int		i;

dbgmsg(">PackValueCobol");
	if		(  value  !=  NULL  ) {
		switch	(value->type) {
		  case	GL_TYPE_INT:
			*(int *)p = value->body.IntegerData;
			IntegerC2Cobol((int *)p);
			p += sizeof(int);
			break;
		  case	GL_TYPE_BOOL:
			*(char *)p = value->body.BoolData ? 'T' : 'F';
			p ++;
			break;
		  case	GL_TYPE_BYTE:
			memcpy(p,value->body.CharData.sval,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_CHAR:
			memcpy(p,value->body.CharData.sval,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_TEXT:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
			StringC2Cobol(value->body.CharData.sval,value->body.CharData.len);
			memcpy(p,value->body.CharData.sval,value->body.CharData.len);
			p += value->body.CharData.len;
			break;
		  case	GL_TYPE_NUMBER:
			memcpy(p,ValueFixed(value)->sval,ValueFixed(value)->flen);
			FixedC2Cobol(p,ValueFixed(value)->flen);
			p += value->body.FixedData.flen;
			break;
		  case	GL_TYPE_ARRAY:
			for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
				p = PackValueCobol(p,value->body.ArrayData.item[i]);
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
				p = PackValueCobol(p,value->body.RecordData.item[i]);
			}
			break;
		  default:
			break;
		}
	}
dbgmsg("<PackValueCobol");
	return	(p);
}

static	void
DumpNode(
	ProcessNode	*node)
{
#ifdef	DEBUG
dbgmsg(">DumpNode");
	printf("mcpsize  = %d\n",node->mcpsize);
	printf("linksize = %d\n",node->linksize);
	printf("spasize  = %d\n",node->spasize);
	printf("scrsize  = %d\n",node->scrsize);
dbgmsg("<DumpNode");
#endif
}

static	void
DumpDB_Node(
	DBCOMM_CTRL	*ctrl)
{
#ifdef	DEBUG
	printf("func   = [%s]\n",ctrl->func);
	printf("blocks = %d\n",ctrl->blocks);
	printf("rno    = %d\n",ctrl->rno);
	printf("pno    = %d\n",ctrl->pno);
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

	PackValueCobol(McpData,node->mcprec);
	PackValueCobol(LinkData,node->linkrec);
	PackValueCobol(SpaData,node->sparec);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = PackValueCobol(p,node->scrrec[i]);
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

	UnPackValueCobol(McpData,node->mcprec);
	UnPackValueCobol(LinkData,node->linkrec);
	UnPackValueCobol(SpaData,node->sparec);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = UnPackValueCobol(p,node->scrrec[i]);
	}


	DumpNode(node);

dbgmsg("<GetApplication");
}


static	void
_ExecuteProcess(
	ProcessNode	*node)
{
dbgmsg(">ExecuteProcess");
	PutApplication(fpAPW,node);
	GetApplication(fpAPR,node);
dbgmsg("<ExecuteProcess");
}


#ifdef	USE_PTY
static	void
ConsoleThread(
	void	*para)
{
	int		i;
	STREAM	*is;
	Char	c;
	Bool	fExit;
	int		fh = (int)para;
	char	buff[SIZE_BUFF+1];

	fExit = FALSE;
	CodeInit(ENCODE_EUC);
	while	( !fExit ) {
		dbgmsg("msg");
		if		(  ( i = read(fh,buff,SIZE_BUFF) )  >  0  ) {
			buff[i] = 0;
			is = CodeOpenString(buff);
			CodeSetEncode(is,ENCODE_SJIS);
			while	(  ( c = CodeGetChar(is) )  !=  Char_EOF  )	{
				CodePutChar(STDOUT,c);
			}
			printf("\n");
			fflush(stdout);
			CodeClose(is);
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
ReadyApplication(void)
{
	int		pid;
	int		slave;
	char	*line
	,		**cmd;

dbgmsg(">ReadyApplication");
#ifdef	USE_PTY
	if		(  ( fhSub = GetMasterPTY() )  <  0  ) {
		printf("error GetMasterPTY\n");
		exit(1);
	}
#endif
	if		(  ( line = getenv("DOTCOBOL_LINE") )  ==  NULL  ) {
		line = DOTCOBOL_COMMAND " ./MCPMAIN";
	}
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
_ReadyDC(void)
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
	ReadyApplication();

	McpSize = SizeValue(ThisEnv->mcprec,ThisLD->arraysize,ThisLD->textsize);
	McpData = xmalloc(McpSize);
	LinkSize = SizeValue(ThisEnv->linkrec,ThisLD->arraysize,ThisLD->textsize);
	LinkData = xmalloc(LinkSize);
	SpaSize = SizeValue(ThisLD->sparec,ThisLD->arraysize,ThisLD->textsize);
	SpaData = xmalloc(SpaSize);
	ScrSize = 0;
	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		ScrSize += SizeValue(ThisLD->window[i]->value,ThisLD->arraysize,ThisLD->textsize);
	}
	ScrData = xmalloc(ScrSize);
dbgmsg("<ReadyDC");
}


static	void
_CleanUpDB(void)
{
	_StopDB();
	unlink("db.input");
	unlink("db.output");
}

static	void
_CleanUpDC(void)
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
	IntegerCobol2C(&ctrl->blocks);
	IntegerCobol2C(&ctrl->rc);
	IntegerCobol2C(&ctrl->rno);
	IntegerCobol2C(&ctrl->pno);
}

static	void
SetControl(
	DBCOMM_CTRL	*ctrl)
{
	StringC2Cobol(ctrl->func,SIZE_FUNC);
	IntegerC2Cobol(&ctrl->blocks);
	IntegerC2Cobol(&ctrl->rc);
	IntegerC2Cobol(&ctrl->rno);
	IntegerC2Cobol(&ctrl->pno);
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
			UnPackValueCobol(data, rec->rec);
		} else {
			rec = NULL;
		}
		ExecDB_Process(ctrl,rec);
		if		(  rec  !=  NULL  ) {
			PackValueCobol(data, rec->rec);
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
_ReadyDB(void)
{
	MakeDB_Pipe();
	StartDB();
}

static	int
_StartBatch(
	char	*name,
	char	*param)
{
	int		pid;
	char	line[SIZE_BUFF]
	,		**cmd;

dbgmsg(">StartBatch");
	sprintf(line,"%s %s",DOTCOBOL_COMMAND, name);
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

static	MessageHandler	dotCOBOL_Handler = {
	"dotCOBOL",
	FALSE,
	_ExecuteProcess,
	_StartBatch,
	_ReadyDC,
	_StopDC,
	_CleanUpDC,
	_ReadyDB,
	_StopDB,
	_CleanUpDB,
};

extern	void
Init_dotCOBOL_Handler(void)
{
	EnterHandler(&dotCOBOL_Handler);
}

