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

#define	SOURCE_LOCALE	"euc-jp"

static	int		count;

extern	void
demoLink(
	char		*arg)
{
	ValueStruct	*e;
	WindowData	*w;

dbgmsg(">demoLink");
	count = 0; 
	printf("arg = [%s]\n",arg);
	(void)SetWindowName("project1");

	SetValueString(GetWindowValue("project1.vbox1.entry1.value"),"漢字を入れてみた",SOURCE_LOCALE);

//	SetValueInteger(GetWindowValue("project1.vbox1.entry1.state"),WIDGET_STATE_INSENSITIVE);

	SetValueInteger(GetWindowValue("project1.vbox1.entry2.value"),-12345);
	SetValueString(GetWindowValue("project1.vbox1.entry1.style"),"green",NULL);
	SetValueString(GetWindowValue("project1.vbox1.entry2.style"),"red",NULL);
	SetValueString(GetWindowValue("project1.vbox1.entry3.style"),"blue",NULL);

	SetValueString(GetWindowValue("project1.vbox1.combo1.item[0]"),"おごちゃん",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.combo1.item[1]"),"じゅんちゃ",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.combo1.item[2]"),"えりさ",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.combo1.item[3]"),"ques",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.combo1.name.value"),"えりさ",SOURCE_LOCALE);
	SetValueInteger(GetWindowValue("project1.vbox1.combo1.count"),4);
	SetValueString(GetWindowValue("project1.vbox1.combo1.name.style"),"blue",NULL);

	SetValueString(GetWindowValue("project1.vbox1.swin2.vp1.list1.item[0]"),"おごちゃん",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin2.vp1.list1.item[1]"),"じゅんちゃ",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin2.vp1.list1.item[2]"),"えりさ",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin2.vp1.list1.item[3]"),"ques",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin2.vp1.list1.item[4]"),"void_No2",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin2.vp1.list1.item[5]"),"Mul6",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin2.vp1.list1.item[6]"),"末広",SOURCE_LOCALE);
	SetValueInteger(GetWindowValue("project1.vbox1.swin2.vp1.list1.count"),7);
	SetValueInteger(GetWindowValue("project1.vbox1.swin2.vp1.list1.from"),5);
	SetValueBool(GetWindowValue("project1.vbox1.swin2.vp1.list1.select[1]"),TRUE);

	SetValueString(GetWindowValue("project1.vbox1.swin1.clist1.label1.value"),"その１",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin1.clist1.label1.style"),"blue",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin1.clist1.label2.value"),"その２",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin1.clist1.label2.style"),"red",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin1.clist1.label3.value"),"その３",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.swin1.clist1.label3.style"),"green",SOURCE_LOCALE);
	SetValueInteger(GetWindowValue("project1.vbox1.swin1.clist1.from"),2);

	SetValueBool(GetWindowValue("project1.vbox1.togglebutton1.value"),FALSE);
	SetValueString(GetWindowValue("project1.vbox1.togglebutton1.label"),"まだ",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.togglebutton1.style"),"green",SOURCE_LOCALE);

	SetValueBool(GetWindowValue("project1.vbox1.hbox5.checkleft.value"),FALSE);
	SetValueBool(GetWindowValue("project1.vbox1.hbox5.checkright.value"),FALSE);
	SetValueBool(GetWindowValue("project1.vbox1.hbox5.checknone"),FALSE);

	SetValueBool(GetWindowValue("project1.vbox1.hbox6.radioleft.value"),FALSE);
	SetValueBool(GetWindowValue("project1.vbox1.hbox6.radioright.value"),FALSE);
	SetValueBool(GetWindowValue("project1.vbox1.hbox6.radionone.value"),FALSE);

	SetValueInteger(GetWindowValue("project1.vbox1.notebook1.pageno"),3);
	SetValueString(GetWindowValue("project1.vbox1.notebook1.entry4.value"),"123456",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.notebook1.swin3.text1.value"),"ノートブックの中身",SOURCE_LOCALE);

	SetValueString(GetWindowValue("project1.vbox1.notebook1.swin4.clist2.label14.value"),"名前",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.notebook1.swin4.clist2.label15.value"),"電話番号",SOURCE_LOCALE);

	SetValueString(GetWindowValue("project1.vbox1.notebook1.swin4.clist2.item[0].value1"),"おごちゃん",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project1.vbox1.notebook1.swin4.clist2.item[0].value2"),"070-6163-7932",NULL);
	SetValueInteger(GetWindowValue("project1.vbox1.notebook1.swin4.clist2.count"),1);

	SetValueInteger(GetWindowValue("project1.vbox1.notebook1.table1.progressbar1.value"),10);

dbgmsg("*");

	w = PutWindowByName("project1",SCREEN_NEW_WINDOW);
dbgmsg("*");
	strcpy(ThisWindow,"project1");
	strcpy(ThisWidget,"entry2");
dbgmsg("<demoLink");
}

extern	void
demoMain(
	char		*arg)
{
	char		buff[SIZE_BUFF];
	char		buff2[SIZE_BUFF];
	WindowData	*win;
	ValueStruct	*e1
	,			*e2
	,			*e3
	,			*e
	,			*v;
	int			i;

dbgmsg(">demoMain");
	printf("arg = [%s]\n",arg);

	printf("window = [%s]\n",ThisWindow);
	printf("widget = [%s]\n",ThisWidget);
	printf("event  = [%s]\n",ThisEvent);

	printf("switch = [%d]\n",ValueInteger(GetWindowValue("project1.vbox1.notebook1.pageno")));

	if		(  !strcmp(ThisEvent,"OpenCalc")  ) {
		win = SetWindowName("project2");
		PutWindow(win,SCREEN_NEW_WINDOW);
	} else
	if		(  !strcmp(ThisEvent,"CloseCalc")  ) {
		win = SetWindowName("project2");
		PutWindow(win,SCREEN_CLOSE_WINDOW);
	} else
	if		(  !strcmp(ThisEvent,"OpenCalendar")  ) {
		win = SetWindowName("project1");
		PutWindow(win,SCREEN_CLOSE_WINDOW);
		LinkModule("demo3");
	} else
	if		(  !strcmp(ThisEvent,"PutData")  ) {
		win = SetWindowName("project1");

		e = GetWindowValue("project1.vbox1.togglebutton1");

		if		(  GetRecordItem(e,"value")->body.BoolData  ) {
			SetValueString(GetRecordItem(e,"label"),"選んだ",SOURCE_LOCALE);
			e1 = GetWindowValue("project1.vbox1.entry1.value");
			e2 = GetWindowValue("project1.vbox1.entry2.value");
			e3 = GetWindowValue("project1.vbox1.combo1.name.value");
			v = GetWindowValue("project1.vbox1.swin1.clist1.item");
			v = GetArrayItem(v,count);
			SetValueString(GetRecordItem(v,"value1"),ValueStringPointer(e1),SOURCE_LOCALE);
			SetValueFixed(GetRecordItem(v,"value2"),&ValueFixed(e2));
			SetValueString(GetRecordItem(v,"value3"),ValueStringPointer(e3),SOURCE_LOCALE);
#if	0
			SetValueBool(GetWindowValue("project1.vbox1.swin1.clist1.select[0]")
						 ,TRUE);
			SetValueBool(GetWindowValue("project1.vbox1.swin1.clist1.select[1]")
						 ,TRUE);
#endif
			count ++;
			SetValueInteger(GetWindowValue("project1.vbox1.swin1.clist1.count")
							,count);
		} else {
			SetValueString(GetRecordItem(e,"label"),"選んでない",SOURCE_LOCALE);
		}
		for	( i = 0 ; i < 20 ; i ++ ) {
			v = GetWindowValue("project1.vbox1.swin1.clist1.select");
			v = GetArrayItem(v,i);
			if		(  ValueBool(v)  ) {
				e = GetWindowValue("project1.vbox1.swin1.clist1.item");
				e = GetArrayItem(e,i);
				SetValueString(GetRecordItem(e,"value1"),"よい子",SOURCE_LOCALE);
//				ValueBool(v) = FALSE;
			}
		}
		memclear(buff,SIZE_BUFF);
		v = GetWindowValue("project1.vbox1.hbox5.checknone.value");
		if		(  ValueBool(v)  ) {
			strcat(buff,"なし");
		}
		v = GetWindowValue("project1.vbox1.hbox5.checkleft.value");
		if		(  ValueBool(v)  ) {
			strcat(buff,"左");
		}
		v = GetWindowValue("project1.vbox1.hbox5.checkright.value");
		if		(  ValueBool(v)  ) {
			strcat(buff,"右");
		}

		if		(  ValueBool(GetWindowValue("project1.vbox1.hbox6.radioleft.value")) ) {
			e1 = GetWindowValue("project1.vbox1.entry1.value");
			e2 = GetWindowValue("project1.vbox1.entry2.value");
			sprintf(buff2,"%s%s",
					ValueStringPointer(e1),
					ValueStringPointer(e2));
			strcat(buff,buff2);
		} else
		if		(  ValueBool(GetWindowValue("project1.vbox1.hbox6.radioright.value")) ) {
			e1 = GetWindowValue("project1.vbox1.entry1.value");
			e2 = GetWindowValue("project1.vbox1.entry2.value");
			sprintf(buff2,"%s%s",
					ValueStringPointer(e2),
					ValueStringPointer(e1));
			strcat(buff,buff2);
		} else {
		}

		for	( i = 0 ; i < 20 ; i ++ ) {
			v = GetWindowValue("project1.vbox1.swin2.vp1.list1.select");
			v = GetArrayItem(v,i);
			if		(  ValueBool(v)  ) {
				e = GetWindowValue("project1.vbox1.swin2.vp1.list1.item");
				e = GetArrayItem(e,i);
				SetValueString(e,"よい子",SOURCE_LOCALE);
				ValueBool(v) = FALSE;
			}
		}

		SetValueString(GetWindowValue("project1.vbox1.entry3.value"),buff,SOURCE_LOCALE);

		if		(  count  <  20  ) {
			PutWindow(win,SCREEN_CURRENT_WINDOW);
		} else {
			PutWindow(win,SCREEN_END_SESSION);
		}
	} else
	if		(  !strcmp(ThisEvent,"Left")  ) {
		win = SetWindowName("project1");

		e1 = GetItemLongName(win->rec->value,"vbox1.entry1.value");
		e2 = GetItemLongName(win->rec->value,"vbox1.entry2.value");
		sprintf(buff,"%s%s",
				ValueStringPointer(e1),
				ValueStringPointer(e2));
		SetValueString(GetItemLongName(win->rec->value,"vbox1.entry3.value"),buff,NULL);
		SetValueBool(GetWindowValue("project1.vbox1.hbox5.checkleft"),FALSE);
		SetValueInteger(GetWindowValue("project1.vbox1.notebook1.table1.progressbar1.value"),
						ValueToInteger(GetWindowValue("project1.vbox1.notebook1.table1.progressbar1.value"))-10);
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	} else
	if		(  !strcmp(ThisEvent,"Right")  ) {
		win = SetWindowName("project1");
		e1 = GetItemLongName(win->rec->value,"vbox1.entry1.value");
		e2 = GetItemLongName(win->rec->value,"vbox1.entry2.value");
		sprintf(buff,"%s%s",
				ValueStringPointer(e2),
				ValueStringPointer(e1));
		SetValueString(GetItemLongName(win->rec->value,"vbox1.entry3.value"),buff,NULL);
		SetValueBool(GetWindowValue("project1.vbox1.hbox5.checkright"),FALSE);
		SetValueInteger(GetWindowValue("project1.vbox1.notebook1.table1.progressbar1.value"),
						ValueToInteger(GetWindowValue("project1.vbox1.notebook1.table1.progressbar1.value"))+10);

		PutWindow(win,SCREEN_CURRENT_WINDOW);
	} else {
		win = SetWindowName("project1");
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
dbgmsg("<demoMain");
}

extern	void
demoInit(void)
{
dbgmsg(">demoInit");
dbgmsg("<demoInit");
}
