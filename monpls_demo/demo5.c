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
/*
*/

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
#include	"debug.h"

#define	SOURCE_LOCALE	"euc-jp"

extern	void
demo5Link(
	char		*arg)
{
ENTER_FUNC;
	printf("arg = [%s]\n",arg);
	(void)SetWindowName("deviceinscr");
	SetValueString(GetWindowValue("deviceinscr.swindow.hpos"),"50",NULL);
	SetValueString(GetWindowValue("deviceinscr.swindow.vpos"),"300",NULL);

	PutWindowByName("deviceinscr",SCREEN_NEW_WINDOW);
LEAVE_FUNC;
}

extern	void
demo5Main(
	char		*arg)
{
	WindowData	*win;

ENTER_FUNC;
	printf("arg = [%s]\n",arg);
	printf("window = [%s]\n",ThisWindow);
	printf("widget = [%s]\n",ThisWidget);
	printf("event  = [%s]\n",ThisEvent);

	printf("hpos = %d\n",ValueInteger(GetWindowValue("deviceinscr.swindow.hpos")));
	printf("vpos = %d\n",ValueInteger(GetWindowValue("deviceinscr.swindow.vpos")));

	if		(  !strcmp(ThisEvent,"Quit")  ) {
		win = SetWindowName("deviceinscr");
		PutWindow(win,SCREEN_END_SESSION);
	} else {
		win = SetWindowName("deviceinscr");
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
LEAVE_FUNC;
}

extern	void
demo5Init(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}
