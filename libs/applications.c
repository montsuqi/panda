/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<signal.h>
#include	<termio.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>

#include	"types.h"
#include	"enum.h"
#include	"misc.h"
#include	"tcp.h"
#include	"value.h"
#include	"glterm.h"
#include	"applications.h"
#include	"driver.h"
#include	"debug.h"

static	GHashTable	*ApplicationTable;

extern	void
ApplicationsRegist(
	char	*name,
	APL_FUNC	link,
	APL_FUNC	main)
{
	ApplicationStruct	*func;

dbgmsg(">ApplicationsRegist");
	if		(  g_hash_table_lookup(ApplicationTable,name)  ==  NULL  ) {
		func = New(ApplicationStruct); 
		func->name = StrDup(name);
		func->main = main;
		func->link = link;
		g_hash_table_insert(ApplicationTable,name,func);
	}
dbgmsg("<ApplicationsRegist");
}

#ifdef	DEBUG
static	void
DumpWindow(
	gpointer	key,
	gpointer	value,
	gpointer	user_data)
{
	WindowData	*win = (WindowData *)value;

	printf("Window = [%s]\n",(char *)key);
	DumpValueStruct(win->Value);
}
#endif

extern	void
ApplicationsCall(
	int 		sts,
	ScreenData	*scr)
{
	ApplicationStruct	*apl;
	char	name[SIZE_NAME+1];
	char	*p
	,		*q;

dbgmsg(">ApplicationsCall");
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
	if		(  ( apl = (ApplicationStruct *)g_hash_table_lookup(ApplicationTable,name) )
			   !=  NULL  ) {
		switch	(sts) {
		  case	APL_SESSION_LINK:
			apl->link(p);
			break;
		  case	APL_SESSION_GET:
			apl->main(p);
			break;
		  default:
			fprintf(stderr,"invalid status [%s] %d\n",name,sts);
			break;
		}
	} else {
		fprintf(stderr,"application not found [%s]\n",ThisScreen->cmd);
		scr->status = APL_SESSION_NULL;
	}
#if	0
	if		(  scr->Windows  !=  NULL  ) { 
		printf("----\n");
		g_hash_table_foreach(ThisScreen->Windows,(GHFunc)DumpWindow,NULL);
		printf("----\n");
	}
#endif
dbgmsg("<ApplicationsCall");
}

#include	"pandaIO.h"
#include	"demo.h"

extern	void
ApplicationsInit(void)
{
dbgmsg(">ApplicationsInit");
	ApplicationTable = NewNameHash();
	DemoInit();
	Demo2Init();
	Demo3Init();
	Demo4Init();
	PandaIO_Init();
dbgmsg("<ApplicationsInit");
}

