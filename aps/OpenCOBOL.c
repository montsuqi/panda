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

#ifdef	HAVE_OPENCOBOL
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

#include	"types.h"
#include	"const.h"
#include	"misc.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"directory.h"
#include	"handler.h"
#include	"defaults.h"
#include	"enum.h"
#include	"dblib.h"
#include	"queue.h"
#include	"driver.h"
#include	"libcob.h"
#define		_OPENCOBOL
#include	"OpenCOBOL.h"
#include	"debug.h"

static	void	*McpData;
static	void	*LinkData;
static	void	*SpaData;
static	void	*ScrData;

static	void	_ReadyDC(void);
static	void	_StopDC(void);
static	Bool	_ExecuteProcess(ProcessNode *node);
static	void	_ReadyDB(void);
static	void	_StopDB(void);
static	int		_StartBatch(char *name, char *param);

static	MessageHandler	Handler = {
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

extern	MessageHandler	*
OpenCOBOL(void)
{
dbgmsg(">OpenCOBOL");
dbgmsg("<OpenCOBOL");
	return	(&Handler);
}

static	void
PutApplication(
	ProcessNode	*node)
{
	int		i;
	char	*p;

dbgmsg(">PutApplication");
	OpenCOBOL_PackValue(OpenCOBOL_Conv,McpData,node->mcprec->value);
	OpenCOBOL_PackValue(OpenCOBOL_Conv,LinkData,node->linkrec->value);
	OpenCOBOL_PackValue(OpenCOBOL_Conv,SpaData,node->sparec->value);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = OpenCOBOL_PackValue(OpenCOBOL_Conv,p,node->scrrec[i]->value);
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
	OpenCOBOL_UnPackValue(OpenCOBOL_Conv,McpData,node->mcprec->value);
	OpenCOBOL_UnPackValue(OpenCOBOL_Conv,LinkData,node->linkrec->value);
	OpenCOBOL_UnPackValue(OpenCOBOL_Conv,SpaData,node->sparec->value);
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		p = OpenCOBOL_UnPackValue(OpenCOBOL_Conv,p,node->scrrec[i]->value);
	}
dbgmsg("<GetApplication");
}

static	Bool
_ExecuteProcess(
	ProcessNode	*node)
{
	int		(*apl)(char *, char *, char *, char *);
	char	*module;
	Bool	rc;

dbgmsg(">_ExecuteProcess");
	module = ValueString(GetItemLongName(node->mcprec->value,"dc.module"));
	if		(  ( apl = cob_resolve(module) )  !=  NULL  ) {
		PutApplication(node);
		dbgmsg(">OpenCOBOL application");
		(void)apl(McpData,SpaData,LinkData,ScrData);
		dbgmsg("<OpenCOBOL application");
		GetApplication(node);
		if		(  ValueInteger(GetItemLongName(node->mcprec->value,"rc"))  <  0  ) {
			rc = FALSE;
		} else {
			rc = TRUE;
		}
	} else {
		printf("%s\n%s is not found.\n",cob_resolve_error(),module);
		rc = FALSE;
	}
dbgmsg("<_ExecuteProcess");
	return	(rc); 
}

static	void
_ReadyDC(void)
{
	char	*path;
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
	OpenCOBOL_Conv = NewConvOpt();
	ConvSetSize(OpenCOBOL_Conv,ThisLD->textsize,ThisLD->arraysize);

	McpData = xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisEnv->mcprec->value));
	LinkData = xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisEnv->linkrec->value));
	SpaData = xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisLD->sparec->value));
	scrsize = 0;
	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		scrsize += OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisLD->window[i]->rec->value);
	}
	ScrData = xmalloc(scrsize);

#if	0
	{
		WindowBind	*bind;
		for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
			bind = ThisLD->window[i];
			if		(  bind->handler  ==  (void *)&Handler  ) {
				printf("preload [%s]\n",bind->module);
				(void)cob_resolve(bind->module);
			}
		}
	}
#endif
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
dbgmsg(">_StopDB");
dbgmsg("<_StopDB");
}

static	void
_ReadyDB(void)
{
dbgmsg(">_ReadyDB");
dbgmsg("<_ReadyDB");
}

static	int
_StartBatch(
	char	*name,
	char	*param)
{
	int		(*apl)(char *);
	int		rc;
	char	*path;
	char	*arg;

dbgmsg(">_StartBatch");
	if		(  LibPath  ==  NULL  ) { 
		path = getenv("COB_LIBRARY_PATH");
	} else {
		path = LibPath;
	}
	cob_init(0,NULL);
	cob_set_library_path(path);

	OpenCOBOL_Conv = NewConvOpt();
	ConvSetSize(OpenCOBOL_Conv,ThisBD->textsize,ThisBD->arraysize);

#ifdef	DEBUG
	printf("starting [%s][%s]\n",name,param);
#endif
	if		(  ( apl = cob_resolve(name) )  !=  NULL  ) {
		arg = (char *)xmalloc(ThisBD->textsize);
		strncpy(arg,param,ThisBD->textsize+1);
		arg[ThisBD->textsize] = 0;
		StringC2Cobol(arg,ThisBD->textsize);
		rc = apl(arg);
dbgmsg("ret");
	} else {
		fprintf(stderr,"%s\n%s is not found.\n",cob_resolve_error(),name);
		rc = -1;
	}
dbgmsg("<_StartBatch");
 return	(rc); 
}
#endif
