/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2001-2008 Ogochan & JMA (Japan Medical Association).
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

#ifdef	HAVE_OPENCOBOL
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
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
#include	"message.h"
#include	"debug.h"

static	void	*McpData;
static	void	*LinkData;
static	void	*SpaData;
static	void	*ScrData;

static	char	*ModuleLoadPath;

static	void
PutApplication(
	ProcessNode	*node)
{
	int		i;
	char	*p;

ENTER_FUNC;
	if		(  node->mcprec  !=  NULL  ) {
		OpenCOBOL_PackValue(OpenCOBOL_Conv,McpData,node->mcprec->value);
	}
	if		(  node->linkrec  !=  NULL  ) {
		OpenCOBOL_PackValue(OpenCOBOL_Conv,LinkData,node->linkrec->value);
	}
	if		(  node->sparec  !=  NULL  ) {
		OpenCOBOL_PackValue(OpenCOBOL_Conv,SpaData,node->sparec->value);
	}
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		if		(  node->scrrec[i]  !=  NULL  ) {
			p += OpenCOBOL_PackValue(OpenCOBOL_Conv,p,node->scrrec[i]->value);
		}
	}
LEAVE_FUNC;
}

static	void
GetApplication(
	ProcessNode	*node)
{
	char	*p;
	int		i;

ENTER_FUNC;
	if		(  node->mcprec  !=  NULL  ) {
		OpenCOBOL_UnPackValue(OpenCOBOL_Conv,McpData,node->mcprec->value);
	}
	if		(  node->linkrec  !=  NULL  ) {
		OpenCOBOL_UnPackValue(OpenCOBOL_Conv,LinkData,node->linkrec->value);
	}
	if		(  node->sparec  !=  NULL  ) {
		OpenCOBOL_UnPackValue(OpenCOBOL_Conv,SpaData,node->sparec->value);
	}
	for	( i = 0 , p = (char *)ScrData ; i < node->cWindow ; i ++ ) {
		if		(  node->scrrec[i]  !=  NULL  ) {
			p += OpenCOBOL_UnPackValue(OpenCOBOL_Conv,p,node->scrrec[i]->value);
		}
	}
LEAVE_FUNC;
}

static	Bool
_ExecuteProcess(
	MessageHandler	*handler,
	ProcessNode	*node)
{
	int		(*apl)(char *, char *, char *, char *);
	char	*module;
	Bool	rc;

ENTER_FUNC;
	module = ValueStringPointer(GetItemLongName(node->mcprec->value,"dc.module"));
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
		Warning("%s - %s is not found.",cob_resolve_error(),module);
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc); 
}

static	void
_ReadyDC(
	MessageHandler	*handler)
{
	int		i;
	size_t	scrsize;

ENTER_FUNC;
	OpenCOBOL_Conv = NewConvOpt();
	ConvSetSize(OpenCOBOL_Conv,ThisLD->textsize,ThisLD->arraysize);
	ConvSetCodeset(OpenCOBOL_Conv,ConvCodeset(handler->conv));
	OpenCOBOL_Conv->fBigEndian = handler->conv->fBigEndian;

	if		(  ThisEnv->mcprec  !=  NULL  ) {
		McpData = xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisEnv->mcprec->value));
	} else {
		McpData = NULL;
	}
	if		(  ThisEnv->linkrec  !=  NULL  ) {
		LinkData = xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisEnv->linkrec->value));
	} else {
		LinkData = NULL;
	}
	if		(  ThisLD->sparec  !=  NULL  ) {
		SpaData = xmalloc(OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisLD->sparec->value));
	}
	scrsize = 0;
	for	( i = 0 ; i < ThisLD->cWindow ; i ++ ) {
		if		(  ThisLD->windows[i]  !=  NULL  ) {
			scrsize += OpenCOBOL_SizeValue(OpenCOBOL_Conv,ThisLD->windows[i]->value);
		}
	}
	ScrData = xmalloc(scrsize);
LEAVE_FUNC;
}

static	void
_StopDC(
	MessageHandler	*handler)
{
	xfree(McpData);
	xfree(LinkData);
	xfree(SpaData);
	xfree(ScrData);
}

static	void
_StopDB(
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
	int		(*apl)(char *);
	int		rc;
	char	*arg;

ENTER_FUNC;
	OpenCOBOL_Conv = NewConvOpt();
	ConvSetSize(OpenCOBOL_Conv,ThisBD->textsize,ThisBD->arraysize);
	ConvSetCodeset(OpenCOBOL_Conv,ConvCodeset(handler->conv));
	OpenCOBOL_Conv->fBigEndian = handler->conv->fBigEndian;

#ifdef	DEBUG
	printf("starting [%s][%s]\n",name,param);
#endif
	if		(  ( apl = cob_resolve(name) )  !=  NULL  ) {
		arg = (char *)xmalloc(ThisBD->textsize);
		memclear(arg,ThisBD->textsize);
		strncpy(arg,param,ThisBD->textsize+1);
		arg[ThisBD->textsize] = 0;
		StringC2Cobol(arg,ThisBD->textsize);
		rc = apl(arg);
	} else {
		Warning( "%s - %s is not found.", cob_resolve_error(),name);
		rc = -1;
	}
LEAVE_FUNC;
	return	(rc); 
}

static	void
_ReadyExecute(
	MessageHandler	*handler,
	char	*loadpath)
{
ENTER_FUNC;

	if		(  LibPath  ==  NULL  ) { 
		if		(  ( ModuleLoadPath = getenv("COB_LIBRARY_PATH") )  ==  NULL  ) {
			if		(  loadpath  !=  NULL  ) {
				ModuleLoadPath = loadpath;
			} else {
				ModuleLoadPath = MONTSUQI_LOAD_PATH;
			}
		}
	} else {
		ModuleLoadPath = LibPath;
	}
	if		(  handler->loadpath  ==  NULL  ) {
		handler->loadpath = ModuleLoadPath;
	}
	cob_init(0,NULL);
	if		(  handler->loadpath  !=  NULL  ) {
		cob_set_library_path(handler->loadpath);
	}
LEAVE_FUNC;
}

static	MessageHandlerClass	Handler = {
	"OpenCOBOL",
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
OpenCOBOL(void)
{
ENTER_FUNC;
LEAVE_FUNC;
	return	(&Handler);
}
#endif
