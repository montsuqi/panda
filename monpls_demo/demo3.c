/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"applications.h"
#include	"driver.h"
#include	"misc.h"
#include	"debug.h"

extern	void
demo3Link(
	char		*arg)
{
dbgmsg(">demo3Link");
	printf("arg = [%s]\n",arg);
	(void)SetWindowName("project5");
	SetValueInteger(GetWindowValue("project5.vbox1.calendar1.year"),2001);
	SetValueInteger(GetWindowValue("project5.vbox1.calendar1.month"),4);
	SetValueInteger(GetWindowValue("project5.vbox1.calendar1.day"),1);

	PutWindowByName("project5",SCREEN_NEW_WINDOW);
dbgmsg("<demo3Link");
}

extern	void
demo3Main(
	char		*arg)
{
	WindowData	*win;
	ValueStruct	*e;

dbgmsg(">demo3Main");
	printf("arg = [%s]\n",arg);
	printf("window = [%s]\n",ThisWindow);
	printf("widget = [%s]\n",ThisWidget);
	printf("event  = [%s]\n",ThisEvent);

	if		(  !strcmp(ThisEvent,"Quit")  ) {
		win = SetWindowName("project5");
		e = GetWindowValue("project5.vbox1.calendar1.year");
		printf("year  [%d]\n",ValueInteger(e));
		e = GetWindowValue("project5.vbox1.calendar1.month");
		printf("month [%d]\n",ValueInteger(e));
		e = GetWindowValue("project5.vbox1.calendar1.day");
		printf("day   [%d]\n",ValueInteger(e));
		PutWindow(win,SCREEN_CLOSE_WINDOW);
		LinkModule("demo");
	} else {
		exit(1);
	}
dbgmsg("<demo3Main");
}

extern	void
demo3Init(void)
{
dbgmsg(">demo3Init");
dbgmsg("<demo3Init");
}
