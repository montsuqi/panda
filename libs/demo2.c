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
#include	"value.h"
#include	"applications.h"
#include	"driver.h"
#include	"misc.h"
#include	"debug.h"

static	void
Demo2Link(
	char		*arg)
{
dbgmsg(">Demo2Link");
	printf("arg = [%s]\n",arg);
	(void)SetWindowName("project3");

	SetValueString(GetWindowValue("project3.vbox1.swin1.text1.style"),"green");
	SetValueString(GetWindowValue("project3.vbox1.swin1.text1.value"),"漢字を入れてみた");

	PutWindowByName("project3",SCREEN_NEW_WINDOW);
dbgmsg("<Demo2Link");
}

static	void
Demo2Main(
	char		*arg)
{
	WindowData	*win;
	ValueStruct	*e;

dbgmsg(">Demo2Main");
	printf("arg = [%s]\n",arg);
	printf("window = [%s]\n",ThisWindow);
	printf("widget = [%s]\n",ThisWidget);
	printf("event  = [%s]\n",ThisEvent);

	if		(  !strcmp(ThisEvent,"Quit")  ) {
		win = SetWindowName("project3");
		e = GetWindowValue("project3.vbox1.swin1.text1.value");
		printf("text [%s]\n",ValueString(e));
		PutWindow(win,SCREEN_CLOSE_WINDOW);
		LinkModule("demo");
	} else {
		exit(1);
	}
dbgmsg("<Demo2Main");
}

extern	void
Demo2Init(void)
{
dbgmsg(">Demo2Init");
	ApplicationsRegist("demo2",Demo2Link,Demo2Main);
dbgmsg("<Demo2Init");
}
