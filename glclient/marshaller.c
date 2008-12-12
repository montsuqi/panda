/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<errno.h>

#include	"types.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"net.h"
#include	"comm.h"
#include	"protocol.h"
#include	"marshaller.h"
#include	"interface.h"
#include	"debug.h"

static	void
FreeBool(
	gpointer	data,
	gpointer	user_data)
{
	xfree(data);
}

static	void
FreeBoolList(
	GList	*list)
{
	g_list_foreach(list,(GFunc)FreeBool,NULL);
	g_list_free(list);
}

static	void
FreeString(
	gpointer	data,
	gpointer	user_data)
{
	xfree(data);
}

static	void
FreeStringList(
	GList	*list)
{
	g_list_foreach(list,(GFunc)FreeString,NULL);
	g_list_free(list);
}

static	void
FreeRow(
	gpointer	data,
	gpointer	user_data)
{
	g_list_foreach((GList *)data,(GFunc)FreeString,NULL);
	g_list_free((GList *)data);
}

static	void
FreeStringTable(
	GList	*list)
{
	g_list_foreach(list,(GFunc)FreeRow,NULL);
	g_list_free(list);
}

/******************************************************************************/
/* Gtk+ marshaller                                                            */
/******************************************************************************/

static	Bool
RecvEntry(
	WidgetData *data,
	NETFILE	*fp)
{
	Bool	ret;
	char	buff[SIZE_BUFF]
	,		name[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;
	_Entry	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Entry *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Entry, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->text);
		g_free(attrs->text_name);
	}

	if (GL_RecvDataType(fp)  ==  GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else {
				attrs->ptype = RecvStringData(fp,buff,SIZE_BUFF);
				attrs->text = strdup(buff);
				attrs->text_name = strdup(name);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendEntry(
	WidgetData	*data,
	NETFILE	*fp)
{
	char		iname[SIZE_BUFF];
	_Entry 		*attrs;

ENTER_FUNC;
	attrs = (_Entry *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s", data->name, attrs->text_name);
	GL_SendName(fp,iname);
	SendStringData(fp,attrs->ptype, attrs->text);
LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvLabel(
	WidgetData	*data,
	NETFILE		*fp)
{
	Bool	ret;
	char	buff[SIZE_BUFF]
	,		name[SIZE_BUFF];
	int		nitem
	,		i;
	_Label	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Label *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Label, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->text);
		g_free(attrs->text_name);
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else {
				attrs->ptype = RecvStringData(fp,buff,SIZE_BUFF);
				attrs->text = strdup(buff);
				attrs->text_name = strdup(name);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return	ret;
}

static	Bool
SendText(
	WidgetData *data,
	NETFILE	*fp)
{
	char		iname[SIZE_BUFF];
	_Text		*attrs;

ENTER_FUNC;
	attrs = (_Text *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s", data->name, attrs->text_name);
	GL_SendName(fp,iname);
	SendStringData(fp, attrs->ptype, attrs->text);
LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvText(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		buff[SIZE_BUFF]
	,			name[SIZE_BUFF];
	int			nitem
	,			i;
	int			state;
	_Text		*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Text *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Text, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->text);
		g_free(attrs->text_name);
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else {
				attrs->ptype = RecvStringData(fp,buff,SIZE_BUFF);
				attrs->text = strdup(buff);
				attrs->text_name = strdup(name);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendButton(
	WidgetData	*data,
	NETFILE	*fp)
{
	char		iname[SIZE_BUFF];
	_Button		*attrs;

ENTER_FUNC;
	attrs = (_Button *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s", data->name, attrs->button_state_name);
	GL_SendName(fp,iname);
	SendBoolData(fp, attrs->ptype, attrs->button_state);
LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvButton(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	Bool		fActive;
	char		name[SIZE_BUFF]
	,			buff[SIZE_BUFF];
	int			nitem
	,			i;
	int			state;
	_Button		*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Button *)data->attrs;
	if (data->attrs == NULL){
		// new data
		attrs = g_new0(_Button, 1);
		attrs->have_button_state = FALSE;
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->label);
		g_free(attrs->button_state_name);
		attrs->have_button_state = FALSE;
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"label")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->label = strdup(buff);
			} else {
				attrs->ptype = RecvBoolData(fp,&fActive);
				attrs->have_button_state = TRUE;
				attrs->button_state = fActive;
				attrs->button_state_name = strdup(name);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendCalendar(
	WidgetData	*data,
	NETFILE		*fp)
{
	char		iname[SIZE_BUFF];
	_Calendar	*attrs;

ENTER_FUNC;
	attrs = (_Calendar *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.year", data->name);
	GL_SendName(fp,(char *)iname);
	SendIntegerData(fp,GL_TYPE_INT, attrs->year);

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.month", data->name);
	GL_SendName(fp,(char *)iname);
	SendIntegerData(fp,GL_TYPE_INT, attrs->month + 1);

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.day", data->name);
	GL_SendName(fp,(char *)iname);
	SendIntegerData(fp,GL_TYPE_INT, attrs->day);
LEAVE_FUNC;
	return	(TRUE);
}

static	Bool
RecvCalendar(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF]
	,			buff[SIZE_BUFF];
	int			nitem
	,			i;
	int			year
	,			month
	,			day;
	int			state;
	_Calendar	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Calendar *)data->attrs;
	if (data->attrs == NULL){
		// new data
		attrs = g_new0(_Calendar, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"year")  ) {
				RecvIntegerData(fp,&year);
				attrs->year = year;
			} else
			if		(  !stricmp(name,"month")  ) {
				RecvIntegerData(fp,&month);
				attrs->month = month;
			} else
			if		(  !stricmp(name,"day")  ) {
				RecvIntegerData(fp,&day);
				attrs->day = day;
			} else {
				/*	fatal error	*/
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret; 
}

static	Bool
SendNotebook(
	WidgetData	*data,
	NETFILE		*fp)
{
	char 		iname[SIZE_BUFF];
	_Notebook	*attrs;

ENTER_FUNC;
	attrs = (_Notebook *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.pageno", data->name);
	GL_SendName(fp,iname);
	SendIntegerData(fp, attrs->ptype, attrs->pageno);
LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvNotebook(
	WidgetData	*data,
	NETFILE		*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF]
	,			subname[SIZE_BUFF]
	,			buff[SIZE_BUFF];
	int			nitem
	,			i;
	int			page;
	int			state;
	_Notebook	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Notebook *)data->attrs;
	if (data->attrs == NULL){
		// new data
		attrs = g_new0(_Notebook, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->subname);
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"pageno")  ) {
				attrs->ptype = RecvIntegerData(fp,&page);
				attrs->pageno = page;
			} else {
				sprintf(subname,"%s.%s", data->name, name);
				RecvValue(fp, subname);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendProgressBar(
	WidgetData	*data,
	NETFILE		*fp)
{
	char			iname[SIZE_BUFF];
	_ProgressBar	*attrs;

ENTER_FUNC;
	attrs = (_ProgressBar *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.value", data->name);
	GL_SendName(fp,iname);
	SendIntegerData(fp, attrs->ptype, attrs->value);
LEAVE_FUNC;
	return	(TRUE);
}

static	Bool
RecvProgressBar(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool			ret;
	char			name[SIZE_BUFF]
	,				buff[SIZE_BUFF];
	int				nitem
	,				i;
	int				value;
	int				state;
	_ProgressBar	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_ProgressBar *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_ProgressBar, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"value")  ) {
				attrs->ptype = RecvIntegerData(fp,&value);
				attrs->value = value;
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
RecvWindow(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF]
	,			longname[SIZE_BUFF]
	,			buff[SIZE_BUFF];
	int			i
	,			nitem
	,			state;
	_Window		*attrs;

ENTER_FUNC;
	ret = TRUE;
	attrs = (_Window *)data->attrs;
	if (data->attrs == NULL){
		// new data
		attrs = g_new0(_Window, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->title);
		g_free(attrs->style);
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"title")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->title = strdup(buff);
			} else {
				sprintf(longname,"%s.%s", data->name, name);
				RecvValue(fp, longname);
			}
		}
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
RecvFrame(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool	ret;
	char	name[SIZE_BUFF]
	,		subname[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;
	_Frame	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Frame *)data->attrs;
	if (data->attrs == NULL){
		// new data
		attrs = g_new0(_Frame, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->label);
		g_free(attrs->subname);
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"label")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->label = strdup(buff);
			} else {
				sprintf(subname,"%s.%s", data->name, name);
				RecvValue(fp, subname);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendScrolledWindow(
	WidgetData	*data,
	NETFILE		*fp)
{
	char			iname[SIZE_BUFF];
	_ScrolledWindow	*attrs;

ENTER_FUNC;
	attrs = (_ScrolledWindow *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.hpos", data->name);
	GL_SendName(fp,iname);
	SendIntegerData(fp,attrs->ptype, attrs->hpos);

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.vpos", data->name);
	GL_SendName(fp,iname);
	SendIntegerData(fp,attrs->ptype, attrs->vpos);
LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvScrolledWindow(
	WidgetData	*data,
	NETFILE	*fp)
{
	char			name[SIZE_LONGNAME+1]
	,				subname[SIZE_BUFF]
	,				buff[SIZE_BUFF];
	int				nitem
	,				i;
	int				vpos
	,				hpos;
	int				state;
	_ScrolledWindow	*attrs;

ENTER_FUNC;
	attrs = (_ScrolledWindow *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_ScrolledWindow, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->subname);
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if (!stricmp(name,"state")) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if (!stricmp(name,"style")) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if (!stricmp(name,"vpos")) {
				attrs->ptype = RecvIntegerData(fp,&vpos);
				attrs->vpos = vpos;
			} else
			if (!stricmp(name,"hpos")) {
				attrs->hpos = RecvIntegerData(fp,&hpos);
				attrs->hpos = hpos;
			} else {
				sprintf(subname,"%s.%s",data->name, name);
				RecvValue(fp, subname);
			}
		}
	}
LEAVE_FUNC;
	return TRUE;
}

/******************************************************************************/
/* gtk+panada marshaller                                                      */
/******************************************************************************/

static	Bool
RecvPandaPreview(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF];
	int			nitem
	,			i;
	_PREVIEW	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_PREVIEW *)data->attrs;
	if (data->attrs == NULL){
		// new data
		attrs = g_new0(_PREVIEW, 1);
		data->attrs = attrs;
	} else {
		// reset data
		FreeLBS(attrs->binary);
	}

	if (GL_RecvDataType(fp)  ==  GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			attrs->binary = NewLBS();
			RecvBinaryData(fp, attrs->binary);
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return	ret;
}

static	Bool
RecvPandaTimer(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF];
	int			nitem
	,			i;
	_Timer		*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Timer *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Timer, 1);
		data->attrs = attrs;
	} else {
		// reset data
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"duration")  ) {
				attrs->ptype = RecvIntegerData(fp,&(attrs->duration));
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendPandaTimer(
	WidgetData	*data,
	NETFILE	*fp)
{
	char		iname[SIZE_BUFF];
	_Timer		*attrs;

ENTER_FUNC;
	attrs = (_Timer *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.duration", data->name);
	GL_SendName(fp,iname);
	SendIntegerData(fp, attrs->ptype , attrs->duration);
LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvNumberEntry(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool			ret;
	char			buff[SIZE_BUFF]
	,				name[SIZE_BUFF];
	int				nitem
	,				i;
	int				state;
	_NumberEntry	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_NumberEntry *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_NumberEntry, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		FreeFixed(attrs->fixed);
		g_free(attrs->fixed_name);
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else {
				attrs->ptype = RecvFixedData(fp,&(attrs->fixed));
				attrs->fixed_name = strdup(name);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendNumberEntry(
	WidgetData	*data,
	NETFILE	*fp)
{
	char			iname[SIZE_BUFF];
	_NumberEntry	*attrs;

ENTER_FUNC;
	attrs = (_NumberEntry *)data->attrs;
	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s", data->name, attrs->fixed_name);
	GL_SendName(fp,iname);
	SendFixedData(fp, attrs->ptype, attrs->fixed);
LEAVE_FUNC;
	return	(TRUE);
}

static	Bool
RecvPandaCombo(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF]
	,			buff[SIZE_BUFF];
	int			count
	,			nitem
	,			num
	,			i
	,			j;
	int			state;
	_Combo		*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Combo *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Combo, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->subname);
		FreeStringList(attrs->item_list);
	}

	if (GL_RecvDataType(fp)	== GL_TYPE_RECORD) {
		nitem = GL_RecvInt(fp);
		count = 0;
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"count")  ) {
				RecvIntegerData(fp,&count);
				attrs->count = count;
			} else
			if		(  !stricmp(name,"item")  ) {
				attrs->item_list = g_list_append(NULL,StrDup(""));
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				num = GL_RecvInt(fp);
				for	( j = 0 ; j < num ; j ++ ) {
					RecvStringData(fp,buff,SIZE_BUFF);
					if (buff != NULL && j < count) {
						attrs->item_list = g_list_append(attrs->item_list,
							StrDup(buff));
					}
				}
			} else {
				sprintf(buff,"%s.%s", data->name, name);
				attrs->subname = strdup(buff);
				RecvWidgetData(buff,fp);
			}
		}
		ret = TRUE;
    }
LEAVE_FUNC;
	return ret;
}

static	Bool
SendPandaCList(
	WidgetData	*data,
	NETFILE	*fp)
{
	int			i;
	char		iname[SIZE_BUFF];
	Bool		row_state;
	 _CList		*attrs;

ENTER_FUNC;
	attrs = (_CList *)data->attrs;
	for	( i = 0; i < g_list_length(attrs->state_list); i ++ ) {
		sprintf(iname, "%s.%s[%d]", 
			data->name, attrs->state_list_name, i + attrs->from);
		GL_SendPacketClass(fp,GL_ScreenData);
		GL_SendName(fp,iname);
		row_state = *((Bool *)g_list_nth_data(attrs->state_list,i));
		SendBoolData(fp, GL_TYPE_BOOL, row_state);
	}

	sprintf(iname,"%s.row", data->name);
	GL_SendPacketClass(fp,GL_ScreenData);
	GL_SendName(fp,iname);
	SendIntegerData(fp, GL_TYPE_INT, attrs->row);

LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvPandaCList(
	WidgetData	*data,
	NETFILE		*fp)
{
	Bool	ret;
	char	name[SIZE_BUFF]
	,		iname[SIZE_BUFF]
	,		subname[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		count
	,		nitem
	,		num
	,		rnum
	,		from
	,		row
	,		column
	,		rowattr
	,		i
	,		j
	,		k;
	Bool	*fActive;
	int		state;
	gfloat	rowattrw;
	GList	*row_items;
	_CList	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_CList *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_CList, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->style);
		g_free(attrs->state_list_name);
		FreeStringTable(attrs->item_list);
		FreeBoolList(attrs->state_list);
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {

		nitem = GL_RecvInt(fp);
		count = -1;
		from = 0;
		row = 0;
		rowattrw = 0.0;

		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			sprintf(subname,"%s.%s",data->name, name);
			if (UI_IsWidgetName(subname)) {
				RecvWidgetData(subname,fp);
			} else
			if		(  !stricmp(name,"count")  ) {
				RecvIntegerData(fp,&count);
				attrs->count = count;
			} else
			if		(  !stricmp(name,"from")  ) {
				RecvIntegerData(fp,&from);
				attrs->from = from;
			} else
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				attrs->state = state;
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->style = strdup(buff);
			} else
			if		(  !stricmp(name,"row")  ) {
				RecvIntegerData(fp,&row);
				attrs->row = row;
			} else
			if		(  !stricmp(name,"rowattr")  ) {
				RecvIntegerData(fp,&rowattr);
				switch	(rowattr) {
				  case	1: /* DOWN */
					attrs->rowattr = 1.0;
					break;
				  case	2: /* MIDDLE */
					attrs->rowattr = 0.5;
					break;
				  default: /* [0] TOP */
					attrs->rowattr = 0.0;
					break;
				}
			} else
			if		(  !stricmp(name,"column")  ) {
				RecvIntegerData(fp,&column);
				attrs->column = column;
			} else
			if		(  !stricmp(name,"item")  ) {
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				num = GL_RecvInt(fp);
				if		(  count  <  0  ) {
					count = num;
				}
				attrs->item_list = NULL;
				for	( j = 0 ; j < num ; j ++ ) {
					GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
					rnum = GL_RecvInt(fp);
					row_items = NULL;
					for	( k = 0 ; k < rnum ; k ++ ) {
						GL_RecvName(fp, sizeof(iname), iname);
						(void)RecvStringData(fp,buff,SIZE_BUFF);
						row_items = g_list_append(row_items, StrDup(buff));
					}
					attrs->item_list = g_list_append(attrs->item_list, 
						row_items);
				}
			} else {
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				attrs->state_list_name = strdup(name);
				num = GL_RecvInt(fp);
				if		(  count  <  0  ) {
					count = num;
				}
				attrs->state_list = NULL;
				for	( j = 0 ; j < num ; j ++ ) {
					fActive = g_new0(Bool, 1);
					RecvBoolData(fp,fActive);
					attrs->state_list = g_list_append(attrs->state_list, 
						fActive);
				}
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
RecvPandaHTML(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool	ret;
	char	name[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	_HTML	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_HTML *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_HTML, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free(attrs->uri);
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			RecvStringData(fp,buff,SIZE_BUFF);
			attrs->uri = strdup(buff);
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

/*****************************************************************************/
/* Gnome marshaller                                                          */
/*****************************************************************************/

static	Bool
RecvPixmap(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF];
	int			nitem
	,			i;
	_Pixmap 	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Pixmap *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Pixmap, 1);
		data->attrs = attrs;
	} else {
		// reset data
		FreeLBS(attrs->binary);
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			attrs->binary = NewLBS();
			RecvBinaryData(fp, attrs->binary);
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
RecvFileEntry(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF]
	,			subname[SIZE_BUFF];
	int			nitem
	,			i;
	_FileEntry	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_FileEntry *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_FileEntry, 1);
		attrs->binary = NewLBS();
		data->attrs = attrs;
	} else {
		// reset data
		FreeLBS(attrs->binary);
		attrs->binary = NewLBS();
		g_free(attrs->subname);
	}


	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if (!stricmp(name,"objectdata")) {
				RecvBinaryData(fp, attrs->binary);
			} else {
				sprintf(subname,"%s.%s", data->name, name);
				attrs->subname = strdup(subname);
				RecvWidgetData(subname,fp);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendFileEntry(
	WidgetData	*data,
	NETFILE		*netfp)
{
	char			iname[SIZE_BUFF];
	FILE			*fp;
	struct stat		st;
	_FileEntry		*attrs;
	LargeByteString	*binary;

ENTER_FUNC;
	attrs = (_FileEntry *)data->attrs;
	if (attrs->path == NULL || strlen(attrs->path) <= 0){
		return TRUE;
	}
	if ( stat(attrs->path,&st) ){
		return TRUE;
	}
	if ( (fp = fopen(attrs->path,"r")) != NULL) {
		binary = NewLBS();
		LBS_ReserveSize(binary,st.st_size,FALSE);
		fread(LBS_Body(binary),st.st_size,1,fp);
		fclose(fp);
	} else {
		return TRUE;
	}
	
	GL_SendPacketClass(netfp,GL_ScreenData);
	sprintf(iname,"%s.objectdata", data->name);
	GL_SendName(netfp,iname);
	SendBinaryData(netfp, GL_TYPE_OBJECT, binary);
	FreeLBS(binary);
LEAVE_FUNC;
	return TRUE;
}

/******************************************************************************/
/* API                                                                        */
/******************************************************************************/

extern	Bool
RecvWidgetData(
	char			*widgetName,
	NETFILE			*fp)
{
	Bool		ret;
	WidgetType	type;
	WidgetData	*data;

	ENTER_FUNC;

	type = UI_GetWidgetType(ThisWindowName, widgetName);
	data = (WidgetData *)g_hash_table_lookup(WidgetDataTable, widgetName);
	if (data == NULL){
		// new data
		data = g_new0(WidgetData, 1);
		data->type = type;
		data->name = strdup(widgetName);
		data->window = g_hash_table_lookup(WindowTable, ThisWindowName);
		g_hash_table_insert(WidgetDataTable, strdup(widgetName), data);
	}
	EnQueue(data->window->UpdateWidgetQueue, data);

	switch(type) {
// gtk+panda
	case WIDGET_TYPE_NUMBER_ENTRY:
		ret = RecvNumberEntry(data, fp); break;
	case WIDGET_TYPE_PANDA_COMBO:
		ret = RecvPandaCombo(data, fp); break;
	case WIDGET_TYPE_PANDA_CLIST:
		ret = RecvPandaCList(data, fp); break;
	case WIDGET_TYPE_PANDA_ENTRY:
		ret = RecvEntry(data, fp); break;
	case WIDGET_TYPE_PANDA_TEXT:
		ret = RecvText(data, fp); break;
	case WIDGET_TYPE_PANDA_PREVIEW:
		ret = RecvPandaPreview(data, fp); break;
	case WIDGET_TYPE_PANDA_TIMER:
		ret = RecvPandaTimer(data, fp); break;
	case WIDGET_TYPE_PANDA_HTML:
		ret = RecvPandaHTML(data, fp); break;
// gtk+
	case WIDGET_TYPE_ENTRY:
		ret = RecvEntry(data, fp); break;
	case WIDGET_TYPE_TEXT:
		ret = RecvText(data, fp); break;
	case WIDGET_TYPE_LABEL: 
		ret = RecvLabel(data, fp); break;
	case WIDGET_TYPE_BUTTON:
	case WIDGET_TYPE_TOGGLE_BUTTON:
	case WIDGET_TYPE_CHECK_BUTTON:
	case WIDGET_TYPE_RADIO_BUTTON:
		ret = RecvButton(data, fp); break;
	case WIDGET_TYPE_CALENDAR:
		ret = RecvCalendar(data, fp); break;
	case WIDGET_TYPE_NOTEBOOK:
		ret = RecvNotebook(data, fp); break;
	case WIDGET_TYPE_PROGRESS_BAR:
		ret = RecvProgressBar(data, fp); break;
	case WIDGET_TYPE_WINDOW:
		ret = RecvWindow(data, fp); break;
	case WIDGET_TYPE_FRAME:
		ret = RecvFrame(data, fp); break;
	case WIDGET_TYPE_SCROLLED_WINDOW:
		ret = RecvScrolledWindow(data, fp); break;
// gnome
	case WIDGET_TYPE_FILE_ENTRY:
		ret = RecvFileEntry(data, fp); break;
	case WIDGET_TYPE_PIXMAP:
		ret = RecvPixmap(data, fp); break;
	default:
		ret = FALSE; break;
	}
LEAVE_FUNC;
	return (ret);
}

extern	void
SendWidgetData(
	gpointer	key,
	gpointer	value,
	gpointer	user_data)
{
	char		*widgetName;
	NETFILE		*fp;
	WidgetData	*data;

ENTER_FUNC;
	widgetName = (char *)key;
	fp = (NETFILE *)user_data;
	data = (WidgetData *)g_hash_table_lookup(WidgetDataTable, widgetName);
	if (data == NULL) {
		MessageLogPrintf("widget data [%s] is not found",widgetName);
		return;
	}

	UI_GetWidgetData(data);

	switch(data->type) {
// gtk+panda
	case WIDGET_TYPE_NUMBER_ENTRY:
		SendNumberEntry(data, fp); break;
	case WIDGET_TYPE_PANDA_CLIST:
		SendPandaCList(data, fp); break;
	case WIDGET_TYPE_PANDA_ENTRY:
		SendEntry(data, fp); break;
	case WIDGET_TYPE_PANDA_TEXT:
		SendText(data, fp); break;
	case WIDGET_TYPE_PANDA_TIMER:
		SendPandaTimer(data, fp); break;
// gtk+
	case WIDGET_TYPE_ENTRY:
		SendEntry(data, fp); break;
	case WIDGET_TYPE_TEXT:
		SendText(data, fp); break;
	case WIDGET_TYPE_TOGGLE_BUTTON:
	case WIDGET_TYPE_CHECK_BUTTON:
	case WIDGET_TYPE_RADIO_BUTTON:
		SendButton(data, fp); break;
	case WIDGET_TYPE_CALENDAR:
		SendCalendar(data, fp); break;
	case WIDGET_TYPE_NOTEBOOK:
		SendNotebook(data, fp); break;
	case WIDGET_TYPE_PROGRESS_BAR:
		SendProgressBar(data, fp); break;
	case WIDGET_TYPE_SCROLLED_WINDOW:
		SendScrolledWindow(data, fp); break;
// gnome
	case WIDGET_TYPE_FILE_ENTRY:
		SendFileEntry(data, fp); break;
	default:
		break;
	}
LEAVE_FUNC;
}
