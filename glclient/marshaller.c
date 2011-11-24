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

#define		MARSHALLER

#include	"glterm.h"
#include	"glclient.h"
#include	"net.h"
#include	"comm.h"
#include	"protocol.h"
#include	"marshaller.h"
#include	"interface.h"
#include	"printservice.h"
#include	"debug.h"

static	void
g_free_nullize(gpointer mem) {
	g_free(mem);
	mem = NULL;
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->text);
		g_free_nullize(attrs->text_name);
	}
	attrs->editable = TRUE;

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
			} else
			if		(  !stricmp(name,"editable")  ) {
				RecvBoolData(fp,&(attrs->editable));
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->text);
		g_free_nullize(attrs->text_name);
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->text);
		g_free_nullize(attrs->text_name);
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->label);
		g_free_nullize(attrs->button_state_name);
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
		g_free_nullize(attrs->style);
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->subname);
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
		g_free_nullize(attrs->style);
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->title);
		g_free_nullize(attrs->bgcolor);
		g_free_nullize(attrs->summary);
		g_free_nullize(attrs->body);
		g_free_nullize(attrs->icon);
 		attrs->timeout = 0;
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
			if		(  !stricmp(name,"bgcolor")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->bgcolor = strdup(buff);
			} else
			if		(  !stricmp(name,"title")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->title = strdup(buff);
			} else
			if		(  !stricmp(name,"popup_summary")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->summary = strdup(buff);
			} else
			if		(  !stricmp(name,"popup_body")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->body = strdup(buff);
			} else
			if		(  !stricmp(name,"popup_icon")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->icon = strdup(buff);
			} else
			if		(  !stricmp(name,"popup_timeout")  ) {
				RecvIntegerData(fp,&(attrs->timeout));
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->label);
		g_free_nullize(attrs->subname);
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->subname);
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
RecvPandaDownload(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool		ret;
	char		name[SIZE_BUFF]
	,			buff[SIZE_BUFF];
	int			nitem
	,			i;
	_Download	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Download *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Download, 1);
		attrs->binary = NewLBS();
		attrs->filename = NULL;
		data->attrs = attrs;
	} else {
		// reset data
		FreeLBS(attrs->binary);
		attrs->binary = NewLBS();
		g_free_nullize(attrs->filename);
	}

	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if (!stricmp(name,"objectdata")) {
				RecvBinaryData(fp, attrs->binary);
			} else if (!stricmp(name,"filename")) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->filename = strdup(buff);
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
RecvPandaPrint(
	WidgetData	*data,
	NETFILE	*fp)
{
	Bool			ret;
	PrintRequest	*req;
	char			name[SIZE_BUFF]
	,				path[SIZE_BUFF]
	,				title[SIZE_BUFF];
	int				nitem
	,				nitem2
	,				nitem3
	,				nretry
	,				showdialog
	,				i,j,k;

ENTER_FUNC;
	ret = FALSE;
	data->attrs = NULL;

	if	(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp, sizeof(name), name);
			if (!stricmp(name,"item")) {
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				nitem2 = GL_RecvInt(fp);
				for	( j = 0 ; j < nitem2 ; j ++ ) {
					GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
					nitem3 = GL_RecvInt(fp);
					path[0] = 0; title[0] = 0;
					nretry = 0;
					showdialog = 0;
					for	( k = 0 ; k < nitem3 ; k ++ ) {
						GL_RecvName(fp, sizeof(name), name);
						if (!stricmp(name,"path")) {
							RecvStringData(fp,path,SIZE_BUFF);
						} else if (!stricmp(name,"title")) {
							RecvStringData(fp,title,SIZE_BUFF);
						} else if (!stricmp(name,"nretry")) {
							RecvIntegerData(fp,&nretry);
						} else if (!stricmp(name,"showdialog")) {
							RecvIntegerData(fp,&showdialog);
						}
					}
					if (strlen(path) > 0 && strlen(title) > 0) {
						req = (PrintRequest*)xmalloc(sizeof(PrintRequest));
						req->path = StrDup(path);
						req->title = StrDup(title);
						req->nretry = nretry;
						req->showdialog = showdialog;
						PrintList = g_list_append(PrintList,req);
						MessageLogPrintf("add path[%s]\n",path);
					}
					path[0] = 0; title[0] = 0;
				}
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
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
		g_free_nullize(attrs->style);
		FreeFixed(attrs->fixed);
		g_free_nullize(attrs->fixed_name);
	}
	attrs->editable = TRUE;

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
			if		(  !stricmp(name,"editable")  ) {
				RecvBoolData(fp,&(attrs->editable));
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->subname);
		g_strfreev(attrs->itemdata);
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
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				num = GL_RecvInt(fp);
				attrs->itemdata = g_malloc(sizeof(gchar*)*(num+2));
				attrs->itemdata[num+1] = NULL;
				attrs->itemdata[0] = g_strdup("");
				for	( j = 1 ; j < num+1 ; j ++ ) {
					RecvStringData(fp,buff,SIZE_BUFF);
					if (buff != NULL && j - 1 < count) {
						attrs->itemdata[j] = g_strdup(buff);
					} else {
						attrs->itemdata[j] = NULL;
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
	for	( i = 0; attrs->states[i] != NULL; i ++ ) {
		sprintf(iname, "%s.%s[%d]", 
			data->name, attrs->states_name, i + attrs->from);
		GL_SendPacketClass(fp,GL_ScreenData);
		GL_SendName(fp,iname);
		if(*(attrs->states[i]) == 'T') {
			row_state = TRUE;
		} else {
			row_state = FALSE;
		}
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
	char	**rdata;
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
	Bool	fActive;
	int		state;
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
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->states_name);
		if (attrs->clistdata != NULL) {
			for(i=0;i<g_list_length(attrs->clistdata);i++) {
				g_strfreev(g_list_nth_data(attrs->clistdata,i));
			}
			g_list_free(attrs->clistdata);
			attrs->clistdata = NULL;
		}
		if (attrs->states != NULL) {
			g_strfreev(attrs->states);
		}
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {

		nitem = GL_RecvInt(fp);
		count = -1;
		from = 0;
		row = 0;

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
				  case	3: /* QUATER */
					attrs->rowattr = 0.25;
					break;
				  case	4: /* THREE QUATER */
					attrs->rowattr = 0.75;
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
				attrs->clistdata = NULL;
				for	( j = 0 ; j < num ; j ++ ) {
					GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
					rnum = GL_RecvInt(fp);
					rdata = g_malloc0(sizeof(gchar*)*(rnum+1));
					rdata[rnum] = NULL;
					for	( k = 0 ; k < rnum ; k ++ ) {
						GL_RecvName(fp, sizeof(iname), iname);
						(void)RecvStringData(fp,buff,SIZE_BUFF);
						rdata[k] = g_strdup(buff);
					}
					attrs->clistdata = g_list_append(attrs->clistdata,rdata);
				}
			} else {
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				attrs->states_name = strdup(name);
				num = GL_RecvInt(fp);
				if		(  count  <  0  ) {
					count = num;
				}
				attrs->states = g_malloc0(sizeof(gchar*)*(num+1));
				attrs->states[num] = NULL;
				for	( j = 0 ; j < num ; j ++ ) {
					RecvBoolData(fp,&fActive);
					if (fActive) {
						attrs->states[j] = g_strdup("T");
					} else {
						attrs->states[j] = g_strdup("F");
					}
				}
			}
		}
		ret = TRUE;
	}
LEAVE_FUNC;
	return ret;
}

static	Bool
SendPandaTable(
	WidgetData	*data,
	NETFILE	*fp)
{
	char		iname[SIZE_BUFF];
	 _Table		*attrs;

ENTER_FUNC;
	attrs = (_Table *)data->attrs;
	sprintf(iname,"%s.trow", data->name);
	GL_SendPacketClass(fp,GL_ScreenData);
	GL_SendName(fp,iname);
	SendIntegerData(fp, GL_TYPE_INT, attrs->trow+1);

	sprintf(iname,"%s.tcolumn", data->name);
	GL_SendPacketClass(fp,GL_ScreenData);
	GL_SendName(fp,iname);
	SendIntegerData(fp, GL_TYPE_INT, attrs->tcolumn+1);

	sprintf(iname,"%s.tvalue", data->name);
	GL_SendPacketClass(fp,GL_ScreenData);
	GL_SendName(fp,iname);
	SendStringData(fp,GL_TYPE_VARCHAR ,attrs->tvalue);
LEAVE_FUNC;
	return TRUE;
}

static	Bool
RecvPandaTable(
	WidgetData	*data,
	NETFILE		*fp)
{
	Bool	ret;
	gchar	name[SIZE_BUFF]
	,		iname[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	gchar	**rdata;
	gint	rowattr
	,		nitem
	,		num
	,		rnum
	,		state
	,		i
	,		j
	,		k;
	_Table	*attrs;

ENTER_FUNC;
	ret = FALSE;
	attrs = (_Table *)data->attrs;
	if (attrs == NULL){
		// new data
		attrs = g_new0(_Table, 1);
		data->attrs = attrs;
	} else {
		// reset data
		g_free_nullize(attrs->style);
		g_free_nullize(attrs->tvalue);
		if (attrs->bgcolors != NULL) {
			g_strfreev(attrs->bgcolors);
			attrs->bgcolors = NULL;
		}
		if (attrs->fgcolors != NULL) {
			g_strfreev(attrs->fgcolors);
			attrs->fgcolors = NULL;
		}
		if (attrs->tdata != NULL) {
			for(i=0;i<g_list_length(attrs->tdata);i++) {
				g_strfreev(g_list_nth_data(attrs->tdata,i));
			}
			g_list_free(attrs->tdata);
			attrs->tdata = NULL;
		}
	}

	if (GL_RecvDataType(fp) == GL_TYPE_RECORD) {

		nitem = GL_RecvInt(fp);
		attrs->trowattr = 0.0;

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
			if		(  !stricmp(name,"trow")  ) {
				RecvIntegerData(fp,&(attrs->trow));
				attrs->trow -= 1;
			} else
			if		(  !stricmp(name,"trowattr")  ) {
				RecvIntegerData(fp,&rowattr);
				switch	(rowattr) {
				  case	1: /* DOWN */
					attrs->trowattr = 1.0;
					break;
				  case	2: /* MIDDLE */
					attrs->trowattr = 0.5;
					break;
				  case	3: /* QUATER */
					attrs->trowattr = 0.25;
					break;
				  case	4: /* THREE QUATER */
					attrs->trowattr = 0.75;
					break;
				  default: /* [0] TOP */
					attrs->trowattr = 0.0;
					break;
				}
			} else
			if		(  !stricmp(name,"tcolumn")  ) {
				RecvIntegerData(fp,&(attrs->tcolumn));
				attrs->tcolumn -= 1;
			} else
			if		(  !stricmp(name,"tvalue")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				attrs->tvalue = strdup(buff);
			} else
			if		(  !stricmp(name,"fgcolors")  ) {
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				num = GL_RecvInt(fp);
				attrs->fgcolors = g_malloc(sizeof(gchar*)*(num+1));
				attrs->fgcolors[num] = NULL;
				for	( j = 0 ; j < num ; j ++ ) {
					RecvStringData(fp,buff,SIZE_BUFF);
					if (buff != NULL ) {
						attrs->fgcolors[j] = g_strdup(buff);
					} else {
						attrs->fgcolors[j] = g_strdup("black");
					}
				}
			} else
			if		(  !stricmp(name,"bgcolors")  ) {
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				num = GL_RecvInt(fp);
				attrs->bgcolors = g_malloc(sizeof(gchar*)*(num+1));
				attrs->bgcolors[num] = NULL;
				for	( j = 0 ; j < num ; j ++ ) {
					RecvStringData(fp,buff,SIZE_BUFF);
					if (buff != NULL ) {
						attrs->bgcolors[j] = g_strdup(buff);
					} else {
						attrs->bgcolors[j] = g_strdup("black");
					}
				}
			} else
			if		(  !stricmp(name,"tdata")  ) {
				GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
				num = GL_RecvInt(fp);
				attrs->tdata = NULL;
				for	( j = 0 ; j < num ; j ++ ) {
					GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
					rnum = GL_RecvInt(fp);
					rdata = g_malloc0(sizeof(gchar*)*(rnum+1));
                    rdata[rnum] = NULL;
					for	( k = 0 ; k < rnum ; k ++ ) {
						GL_RecvName(fp, sizeof(iname), iname);
						(void)RecvStringData(fp,buff,SIZE_BUFF);
						rdata[k] = g_strdup(buff);
					}
					attrs->tdata = g_list_append(attrs->tdata,rdata);
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
		g_free_nullize(attrs->uri);
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
		g_free_nullize(attrs->subname);
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
	_FileEntry		*attrs;

ENTER_FUNC;
	attrs = (_FileEntry *)data->attrs;
	if (attrs->path == NULL){
		return TRUE;
	}
	
	GL_SendPacketClass(netfp,GL_ScreenData);
	sprintf(iname,"%s.objectdata", data->name);
	GL_SendName(netfp,iname);
	SendBinaryData(netfp, GL_TYPE_OBJECT, attrs->binary);
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
	case WIDGET_TYPE_PANDA_DOWNLOAD:
		ret = RecvPandaDownload(data, fp); break;
	case WIDGET_TYPE_PANDA_PRINT:
		ret = RecvPandaPrint(data, fp); break;
	case WIDGET_TYPE_PANDA_HTML:
		ret = RecvPandaHTML(data, fp); break;
	case WIDGET_TYPE_PANDA_TABLE:
		ret = RecvPandaTable(data, fp); break;
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
	case WIDGET_TYPE_PANDA_TABLE:
		SendPandaTable(data, fp); break;
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
