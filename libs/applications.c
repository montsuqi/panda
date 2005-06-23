/*
PANDA -- a simple transaction monitor
Copyright (C) 1998-1999 Ogochan.
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	<signal.h>
#include	<dlfcn.h>
#include	<fcntl.h>
#include    <sys/types.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>

#include	"defaults.h"
#include	"types.h"
#include	"socket.h"
#include	"libmondai.h"
#include	"glterm.h"
#include	"load.h"
#include	"applications.h"
#include	"debug.h"

typedef	void	(*APL_INIT)(int, char **);

static	GHashTable	*ApplicationTable;
static	int			Argc;
static	char		**Argv;
static	char		*MONPS_LoadPath;

static	ApplicationStruct	*
ApplicationsRegist(
	char	*name,
	APL_FUNC	link,
	APL_FUNC	main)
{
	ApplicationStruct	*func;

ENTER_FUNC;
	if		(  ( func = g_hash_table_lookup(ApplicationTable,name) )  ==  NULL  ) {
		func = New(ApplicationStruct); 
		func->name = StrDup(name);
		func->main = main;
		func->link = link;
		g_hash_table_insert(ApplicationTable,name,func);
	}
LEAVE_FUNC;
	return	(func); 
}

static	ApplicationStruct	*
ApplicationLoad(
	char	*name)
{
	char		funcname[SIZE_BUFF]
	,			filename[SIZE_BUFF];
	APL_FUNC	f_link
	,			f_main;
	APL_INIT	f_init;
	void		*handle;
	ApplicationStruct	*func;

ENTER_FUNC;
	if		(  ( func = (ApplicationStruct *)g_hash_table_lookup(ApplicationTable,name) )
			   ==  NULL  ) {
		sprintf(filename,"%s.so",name);
		dbgprintf("MONPS_LoadPath = [%s]\n",MONPS_LoadPath);
		if		(  ( handle = LoadFile(MONPS_LoadPath,filename) )  !=  NULL  ) {
			sprintf(funcname,"%sInit",name);
			if		(  ( f_init = (APL_INIT)dlsym(handle,funcname) )  !=  NULL  ) {
				f_init(Argc,Argv);
			}
			sprintf(funcname,"%sLink",name);
			f_link = dlsym(handle,funcname);
			sprintf(funcname,"%sMain",name);
			f_main = dlsym(handle,funcname);
			func = ApplicationsRegist(StrDup(name),f_link,f_main);
		}
	}
LEAVE_FUNC;
	return	(func);
}

extern	void
ApplicationsCall(
	int 		sts,
	ScreenData	*scr)
{
	ApplicationStruct	*apl;
	char	name[SIZE_NAME+1];
	char	*p
	,		*q;

ENTER_FUNC;
	ThisScreen = scr;
#ifdef	DEBUG
	printf("sts = %d\n",sts);
#endif
	for	( p = ThisScreen->cmd , q = name ;
			(	(  *p  !=  0     )
			&&	(  !isspace(*p)  )
			&&	(  *p  !=  ':'   ) ); p ++ , q ++ ) {
		*q = *p;
	}
	*q = 0;
	p ++;
	while	(  *p && isspace(*p)  )	p ++;
	apl = ApplicationLoad(name);
	if		(  apl  ==  NULL  ) {
		Warning("application not found [%s]\n",ThisScreen->cmd);
		scr->status = APL_SESSION_NULL;
	} else {
		switch	(sts) {
		  case	APL_SESSION_LINK:
			apl->link(p);
			break;
		  case	APL_SESSION_GET:
			apl->main(p);
			break;
		  default:
			Warning("invalid status [%s] %d\n",name,sts);
			break;
		}
	}
#if	0
	if		(  scr->Windows  !=  NULL  ) { 
		printf("----\n");
		g_hash_table_foreach(ThisScreen->Windows,(GHFunc)DumpWindow,NULL);
		printf("----\n");
	}
#endif
LEAVE_FUNC;
}

extern	void
ApplicationsInit(
	int		argc,
	char	**argv)
{
ENTER_FUNC;
	Argc = argc; 
	Argv = argv;

	ApplicationTable = NewNameHash();
	if		(  ( MONPS_LoadPath = getenv("MONPS_LOAD_PATH") )  ==  NULL  ) {
		MONPS_LoadPath = MONTSUQI_LOAD_PATH;
	}
LEAVE_FUNC;
}
