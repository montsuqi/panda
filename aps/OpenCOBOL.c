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
#include	"const.h"
#include	"misc.h"
#include	"value.h"
#include	"comm.h"
#include	"dirs.h"
#include	"aps_main.h"
#include	"directory.h"
#include	"cobolvalue.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"wfc.h"
#include	"apsio.h"
#include	"dbgroup.h"
#include	"Postgres.h"
#include	"queue.h"
#include	"tcp.h"
#include	"driver.h"
#include	"libcob.h"
#include	"OpenCOBOL.h"
#include	"debug.h"

static	void	*McpData;
static	void	*LinkData;
static	void	*SpaData;
static	void	*ScrData;

static	void	_ReadyDC(void);
static	void	_StopDC(void);
static	void	_ExecuteProcess(ProcessNode *node);
static	void	_ReadyDB(void);
static	void	_StopDB(void);
static	int		_StartBatch(char *name, char *param);

static	MessageHandler	OpenCOBOL_Handler = {
	"OpenCOBOL",
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

static	void
IntegerCobol2C(
	int		*ival)
{
	/*	NOP	*/
}

/*
  unsigned	0123456789
  plus		0123456789
  minus		@ABCDEFGHI
*/

static	void
FixedCobol2C(
	char	*buff,
	size_t	size)
{
	buff[size] = 0;
	if		(	(  buff[size - 1]  >=  '@'  )
			&&	(  buff[size - 1]  <=  'I'  ) ) {
		buff[size - 1] = '0' + ( buff[size - 1] - '@' );
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
		buff[size - 1]  -= '0' - '@';
	}
}

extern	char	*
OpenCOBOL_UnPackValue(
	char	*p,
	ValueStruct	*value)
{
	int		i;
	char	buff[SIZE_BUFF];

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
				p = OpenCOBOL_UnPackValue(p,value->body.ArrayData.item[i]);
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
				p = OpenCOBOL_UnPackValue(p,value->body.RecordData.item[i]);
			}
			break;
		  default:
			break;
		}
	}
	return	(p);
}

extern	char	*
OpenCOBOL_PackValue(
	char	*p,
	ValueStruct	*value)
{
	int		i;

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
				p = OpenCOBOL_PackValue(p,value->body.ArrayData.item[i]);
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
				p = OpenCOBOL_PackValue(p,value->body.RecordData.item[i]);
			}
			break;
		  default:
			break;
		}
	}
	return	(p);
}

static	void
PutApplication(
	ProcessNode	*node)
{
	int		i;
	char	*p;

dbgmsg(">PutApplication");
	OpenCOBOL_PackValue(McpData,node->mcprec);
	OpenCOBOL_PackValue(LinkData,node->linkrec);
	OpenCOBOL_PackValue(SpaData,node->sparec);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = OpenCOBOL_PackValue(p,node->scrrec[i]);
	}
dbgmsg("<PutApplication");
}

static	void
GetApplication(
	ProcessNode	*node)
{
	char	*p;
	int		i;

dbgmsg(">GetApplication");
	OpenCOBOL_UnPackValue(McpData,node->mcprec);
	OpenCOBOL_UnPackValue(LinkData,node->linkrec);
	OpenCOBOL_UnPackValue(SpaData,node->sparec);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = OpenCOBOL_UnPackValue(p,node->scrrec[i]);
	}

dbgmsg("<GetApplication");
}

static	void
_ExecuteProcess(
	ProcessNode	*node)
{
	int		(*apl)(char *, char *, char *, char *);
	char	*module;

dbgmsg(">ExecuteProcess");
	module = ValueString(GetItemLongName(node->mcprec,"dc.module"));
	if		(  ( apl = cob_resolve(module) )  !=  NULL  ) {
		PutApplication(node);
		dbgmsg(">OpenCOBOL application");
		(void)apl(McpData,SpaData,LinkData,ScrData);
		dbgmsg("<OpenCOBOL application");
		GetApplication(node);
	} else {
		printf("%s\n%s is not found.\n",cob_resolve_error(),module);
	}
dbgmsg("<ExecuteProcess");
}

static	void
_ReadyDC(void)
{
	char	*path;
	WindowBind	*bind;
	int		i;
	size_t	scrsize;

dbgmsg(">ReadyDC");
	if		(  LibPath  ==  NULL  ) { 
		path = getenv("COB_LIBRARY_PATH");
	} else {
		path = LibPath;
	}
	cob_init(0,NULL);
	cob_set_library_path(path);

	McpData = xmalloc(SizeValue(ThisEnv->mcprec,ThisLD->arraysize,ThisLD->textsize));
	LinkData = xmalloc(SizeValue(ThisEnv->linkrec,ThisLD->arraysize,ThisLD->textsize));
	SpaData = xmalloc(SizeValue(ThisLD->sparec,ThisLD->arraysize,ThisLD->textsize));
	scrsize = 0;
	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		scrsize += SizeValue(ThisLD->window[i]->value,ThisLD->arraysize,ThisLD->textsize);
	}
	ScrData = xmalloc(scrsize);

	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		bind = ThisLD->window[i];
		if		(  bind->handler  ==  (void *)&OpenCOBOL_Handler  ) {
			printf("preload [%s]\n",bind->module);
			(void)cob_resolve(bind->module);
		}
	}
dbgmsg("<ReadyDC");
}

static	void
_StopDC(void)
{
	xfree(McpData);
	xfree(LinkData);
	xfree(SpaData);
	xfree(ScrData);
}

static	void
_StopDB(void)
{
dbgmsg(">StopDB");
	ExecDB_Function("DBDISCONNECT",NULL,NULL);
dbgmsg("<StopDB");
}

static	void
_ReadyDB(void)
{
	ExecDB_Function("DBOPEN",NULL,NULL);
}

static	int
_StartBatch(
	char	*name,
	char	*param)
{
	int		(*apl)(char *);
	int		rc;
	char	*path;

dbgmsg(">_StartBatch");
	if		(  LibPath  ==  NULL  ) { 
		path = getenv("COB_LIBRARY_PATH");
	} else {
		path = LibPath;
	}
	cob_init(0,NULL);
	cob_set_library_path(path);
#ifdef	DEBUG
	printf("starting [%s][%s]\n",name,param);
#endif
	if		(  ( apl = cob_resolve(name) )  !=  NULL  ) {
		rc = apl(param);
	} else {
		fprintf(stderr,"%s\n%s is not found.\n",cob_resolve_error(),name);
		rc = -1;
	}
dbgmsg("<_StartBatch");
 return	(rc); 
}

extern	void
Init_OpenCOBOL_Handler(void)
{
dbgmsg(">Init_OpenCOBOL_Handler");
	EnterHandler(&OpenCOBOL_Handler);
dbgmsg("<Init_OpenCOBOL_Handler");
}
