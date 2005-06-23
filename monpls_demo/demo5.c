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
