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

static	int		count;

extern	void
calLink(
	char		*arg)
{
	int			i;

	ValueStruct	*e
	,			*v;
	WindowData	*w;
	char		name[SIZE_BUFF]
	,			str[SIZE_BUFF];

dbgmsg(">calLink");
	count = 0; 
	printf("arg = [%s]\n",arg);
	(void)SetWindowName("calendar");

	SetValueInteger(GetWindowValue("calendar.note.pageno"),1);

	e = GetWindowValue("calendar.D.list");
	for	( i = 0 ; i < 24 ; i ++ ) {
		sprintf(name,"item[%d].value0",i);
		v = GetItemLongName(e,name);
		sprintf(str,"%02d:00",i);
		SetValueString(v,str);
		sprintf(name,"item[%d].value1",i);
		v = GetItemLongName(e,name);
		sprintf(str,"うひょひょ%d",i);
		SetValueString(v,str);
	}
	SetValueString(GetWindowValue("calendar.D.date.value"),"2003年04月30日");

	e = GetWindowValue("calendar.W.apo");
	for	( i = 0 ; i < 7 ; i ++ ) {
		sprintf(name,"[%d].value",i);
		v = GetItemLongName(e,name);
		sprintf(str,"%02d",i);
		SetValueString(v,str);
	}
	SetValueString(GetWindowValue("calendar.W.date.value"),"2003年04月");

	w = PutWindowByName("calendar",SCREEN_NEW_WINDOW);
	strcpy(ThisWindow,"calendar");
	strcpy(ThisWidget,"");
dbgmsg("<calLink");
}

extern	void
calMain(
	char		*arg)
{
	WindowData	*win;

dbgmsg(">calMain");
	printf("arg = [%s]\n",arg);
	printf("window = [%s]\n",ThisWindow);
	printf("widget = [%s]\n",ThisWidget);
	printf("event  = [%s]\n",ThisEvent);

	win = SetWindowName("calendar");
	PutWindow(win,SCREEN_CURRENT_WINDOW);
dbgmsg("<calMain");
}

extern	void
calInit(void)
{
dbgmsg(">calInit");
dbgmsg("<calInit");
}
