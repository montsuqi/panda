/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include    <sys/types.h>
#include	<unistd.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"libmondai.h"
#include	"RecParser.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"dirs.h"
#include	"wfcdata.h"
#include	"dbgroup.h"
#include	"controlthread.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

#include	"socket.h"

#include	"directory.h"

static	char	*ControlPort;
static	int		Back;
static	char	*Directory;

static	Bool
Auth(
	NETFILE	*fp)
{
	Bool	ret;

ENTER_FUNC;
	SendStringDelim(fp,ThisEnv->name);
	SendStringDelim(fp,"\n");
	dbgprintf("name = [%s]\n",ThisEnv->name);
	switch	(RecvPacketClass(fp)) {
	  case	WFCCONTROL_OK:
		ret = TRUE;
		break;
	  default:
		ret = FALSE;
		break;
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
MainProc(
	char	*comm)
{
	NETFILE	*fp;

ENTER_FUNC;
	if		(  ( fp = OpenPort(ControlPort,0) )  !=  NULL  ) {
		if		(  Auth(fp)  ) {
			if		(  stricmp(comm,"stop")  ==  0  ) {
				SendPacketClass(fp,WFCCONTROL_STOP);
			} else {
				printf("command error\n");
			}
		} else {
			printf("auth fail\n");
		}
		CloseNet(fp);
	} else {
		fprintf(stderr,"invalid control port\n");
	}
LEAVE_FUNC;
}

static	void
InitSystem(void)
{
ENTER_FUNC;
	InitDirectory();
	SetUpDirectory(Directory,NULL,"","",FALSE);
	InitNET();
LEAVE_FUNC;
}

static	void
CleanUp(void)
{
}

static	ARG_TABLE	option[] = {
	{	"control",	STRING,		TRUE,	(void*)&ControlPort,
		"control port"									},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"base directory of configurations"				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"record directory"				 				},
	{	"ddir",		STRING,		TRUE,	(void*)&D_Dir,
		"definition directory"		 					},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"configuration file"	 						},

	{	NULL,		0,			TRUE,	NULL		 	}
};

static	void
SetDefault(void)
{
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	ControlPort = CONTROL_PORT;
}

extern	int
main(
	int		argc,
	char	**argv)
{

	FILE_LIST	*fl;
	char		*command;

	SetDefault();
	fl = GetOption(option,argc,argv,NULL);

	InitMessage("wfccontrol",NULL);

	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		command = fl->name;
	} else {
		command = "stop";
	}

	InitSystem();
	MainProc(command);
	CleanUp();
	return	(0);
}
