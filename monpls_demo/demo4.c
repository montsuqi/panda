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
#include	"libmondai.h"
#include	"applications.h"
#include	"driver.h"
#include	"debug.h"

#define	SOURCE_LOCALE	"utf-8"

static	int		count;

extern	void
demo4Link(
	char		*arg)
{
	WindowData	*w;

ENTER_FUNC;
	count = 0; 
	printf("arg = [%s]\n",arg);
	(void)SetWindowName("project6");
	
	/* page1 */
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.entry7.value"),"kida",SOURCE_LOCALE); 
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.text1.value"),"testtesttesttesttest",SOURCE_LOCALE);

	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.combo1.item[0]"),"test1",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.combo1.item[1]"),"test2",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.combo1.item[2]"),"test3",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.combo1.item[3]"),"test4",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.combo1.name.value"),"test2",SOURCE_LOCALE);
	SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed4.combo1.count"),4);
	SetValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.checkbutton2.value"),TRUE);
	SetValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.radiobutton2.value"),TRUE);

	/* page2 */

	SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.calendar1.year"),2001);
	SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.calendar1.month"),11);
	SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.calendar1.day"),11);


	/* page3 */

	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.item[0]"),"test1",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.item[1]"),"test2",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.item[2]"),"test3",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.item[3]"),"test4",SOURCE_LOCALE);
	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.combo_entry7.value"),"test2",SOURCE_LOCALE);
	SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.count"),4);

	w = PutWindowByName("project6",SCREEN_NEW_WINDOW);
	strcpy(ThisWindow,"project6");
LEAVE_FUNC;
}

extern	void
demo4Main(
	char		*arg)
{
	WindowData	*win;
	ValueStruct	*e1
	,			*e2
	,			*e3
	,			*e4
	,			*e5
	,			*e6
	,			*e7
	,			*e8
	,			*e9
	,			*e10
	,			*v
	,			*v1;


ENTER_FUNC;
	printf("arg = [%s]\n",arg);
	printf("window = [%s]\n",ThisWindow);
	printf("widget = [%s]\n",ThisWidget);
	printf("event  = [%s]\n",ThisEvent);

	if (  !strcmp(ThisEvent,"Close")  ) {
		win = SetWindowName("project6");
		PutWindow(win,SCREEN_CLOSE_WINDOW);
		LinkModule("demo1");
 	}
	if (  !strcmp(ThisEvent,"reset")  ) {
		win = SetWindowName("project6");
	   	SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.entry7.value"),"",SOURCE_LOCALE); 
		SetValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.checkbutton2.value"),FALSE);
		SetValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.checkbutton3.value"),FALSE);
		SetValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.radiobutton2.value"),TRUE);
		SetValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.radiobutton3.value"),FALSE);
		SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.text1.value"),"",SOURCE_LOCALE);
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
	if (  !strcmp(ThisEvent,"reset2")  ) {
		win = SetWindowName("project6");
		SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.pandaentry1.value"),"",SOURCE_LOCALE); 
		SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed3.numberentry2.value"),0);
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
	if (  !strcmp(ThisEvent,"insert")  ) {
		win = SetWindowName("project6"); 

  		e1 = GetWindowValue("project6.vbox1.notebook1.fixed4.text1.value");
		e2 = GetWindowValue("project6.vbox1.notebook1.fixed4.entry7.value");
		e3 = GetWindowValue("project6.vbox1.notebook1.fixed4.togglebutton2");
		v = GetWindowValue("project6.vbox1.notebook1.fixed4.clist3.item");
		v = GetArrayItem(v,count);

		SetValueString(GetRecordItem(v,"value1"),ValueStringPointer(e1),SOURCE_LOCALE);
		if ( GetRecordItem(e3,"value")->body.BoolData ) {
			if ( ValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.radiobutton2.value")) ){
    		    SetValueString(GetRecordItem(v,"value2"),"panda",SOURCE_LOCALE);
			} else {
				SetValueString(GetRecordItem(v,"value2"),"zebra",SOURCE_LOCALE);
			}		 
		} else {      
			if ( ValueBool(GetWindowValue("project6.vbox1.notebook1.fixed4.checkbutton2.value")) ) {
				SetValueString(GetRecordItem(v,"value2"),"check1",SOURCE_LOCALE);
			} else {
				SetValueString(GetRecordItem(v,"value2"),"check2",SOURCE_LOCALE);
			}
		}

		SetValueString(GetRecordItem(v,"value3"),ValueToString(e2,NULL),NULL);
		count ++;
		SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed4.clist3.count"),count);
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
	if ( !strcmp(ThisEvent,"insert2") ) {
		win = SetWindowName("project6");
		e4 = GetWindowValue("project6.vbox1.notebook1.fixed3.pandaentry1.value");
		e5 = GetWindowValue("project6.vbox1.notebook1.fixed3.numberentry2.value");
		e6 = GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.combo_entry7.value");	
		v1 = GetWindowValue("project6.vbox1.notebook1.fixed3.pandaclist1.item");
		v1 = GetArrayItem(v1,count);
		SetValueString(GetRecordItem(v1,"value1"),ValueStringPointer(e4),SOURCE_LOCALE);
		SetValueFixed(GetRecordItem(v1,"value2"),&ValueFixed(e5));
		SetValueString(GetRecordItem(v1,"value3"),ValueStringPointer(e6),SOURCE_LOCALE);

		count ++;
		SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed3.pandaclist1.count"),count);

		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
	if ( !strcmp(ThisEvent,"SELECT")){
		win = SetWindowName("project6");
 		SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.pandaentry1.value"),"",SOURCE_LOCALE);
 		SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed3.numberentry2.value"),"",SOURCE_LOCALE);
 		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
	if ( !strcmp(ThisEvent,"CHANGED")){
		win = SetWindowName("project6");
		e7 = GetWindowValue("project6.vbox1.notebook1.fixed4.entry7.value");
		SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.text1.value"),"",SOURCE_LOCALE);
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}       
	if ( !strcmp(ThisEvent,"SELECT2")){
		win = SetWindowName("project6");
		SetValueString(GetWindowValue("project6.vbox1.notebook1.fixed4.text1.value"),"",SOURCE_LOCALE);
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
	if ( !strcmp(ThisEvent,"CLICKED")){
		win = SetWindowName("project6");

		SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.entry4.value"),ValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.calendar1.year"))); 
		SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.entry5.value"),ValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.calendar1.month"))); 
		SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.entry6.value"),ValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed1.calendar1.day"))); 		
		
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
	if ( !strcmp(ThisEvent,"ACTIVATE")) {
		win = SetWindowName("project6");
		e8 = GetWindowValue("project6.vbox1.notebook1.fixed3.pandaentry1.value");
		e9 = GetWindowValue("project6.vbox1.notebook1.fixed3.numberentry2.value");
		e10 = GetWindowValue("project6.vbox1.notebook1.fixed3.pandacombo1.combo_entry7.value");	
		v1 = GetWindowValue("project6.vbox1.notebook1.fixed3.pandaclist1.item");
		v1 = GetArrayItem(v1,count);
		SetValueString(GetRecordItem(v1,"value1"),ValueStringPointer(e8),SOURCE_LOCALE);
		SetValueFixed(GetRecordItem(v1,"value2"),&ValueFixed(e9));
		SetValueString(GetRecordItem(v1,"value3"),ValueStringPointer(e10),SOURCE_LOCALE);
		count ++;
		SetValueInteger(GetWindowValue("project6.vbox1.notebook1.fixed3.pandaclist1.count"),count);
		PutWindow(win,SCREEN_CURRENT_WINDOW);
	}
LEAVE_FUNC;
}

extern	void
demo4Init(void)
{
ENTER_FUNC;
LEAVE_FUNC;
}
