/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"applications.h"
#include	"driver.h"
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
