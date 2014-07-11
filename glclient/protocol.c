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

/*
#define	NETWORK_ORDER
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<netinet/in.h>
#include	<iconv.h>

#include	"glterm.h"
#include	"glclient.h"
#include	"comm.h"
#include	"protocol.h"
#define		_PROTOCOL_C
#include	"marshaller.h"
#include	"gettext.h"
#include	"const.h"
#include	"notify.h"
#include	"dialogs.h"
#include	"widgetOPS.h"
#include	"action.h"
#include	"print.h"
#include	"download.h"
#include	"message.h"
#include	"debug.h"

#ifdef	NETWORK_ORDER
#define	RECV32(v)	ntohl(v)
#define	RECV16(v)	ntohs(v)
#define	SEND32(v)	htonl(v)
#define	SEND16(v)	htons(v)
#else
#define	RECV32(v)	(v)
#define	RECV16(v)	(v)
#define	SEND32(v)	(v)
#define	SEND16(v)	(v)
#endif

static	LargeByteString	*LargeBuff;

/*
 *	send/recv functions
 */

#define	SendChar(fp,c)	nputc((c),(fp))
#define	RecvChar(fp)	ngetc(fp)

static void
GL_Error(void)
{
	ShowErrorDialog(_("Connection lost\n"));
}

extern	void
GL_SendPacketClass(
	NETFILE	*fp,
	PacketClass	c)
{
	nputc(c,fp);
	Flush(fp);
}

extern	PacketClass
GL_RecvPacketClass(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

extern	PacketDataType
GL_RecvDataType(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

extern	void
GL_SendInt(
	NETFILE	*fp,
	int		data)
{
	unsigned char	buff[sizeof(int)];

	data = SEND32(data);
	memcpy(buff,&data,sizeof(int));
	Send(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

extern	int
GL_RecvInt(
	NETFILE	*fp)
{
	unsigned char	buff[sizeof(int)];
	int data;

	Recv(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	memcpy(&data,buff,sizeof(int));
	data = RECV32(data);
	return data;
}

static	void
GL_SendLength(
	NETFILE	*fp,
	size_t	data)
{
	GL_SendInt(fp,data);
}

static	size_t
GL_RecvLength(
	NETFILE	*fp)
{
	return (size_t)GL_RecvInt(fp);
}
#if	0
static	unsigned	int
GL_RecvUInt(
	NETFILE	*fp)
{
	unsigned char	buff[sizeof(int)];

	Recv(fp,buff,sizeof(unsigned int));
	return	(RECV32(*(unsigned int *)buff));
}

static	void
GL_SendUInt(
	NETFILE	*fp,
	unsigned	int		data)
{
	unsigned char	buff[sizeof(int)];

	*(unsigned int *)buff = SEND32(data);
	Send(fp,buff,sizeof(unsigned int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

#endif

static	void
GL_RecvString(
	NETFILE	*fp,
	size_t  maxsize,
	char	*str)
{
	size_t		lsize;
ENTER_FUNC;
	lsize = GL_RecvLength(fp);
	if		(	maxsize >= lsize 	){
		Recv(fp,str,lsize);
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
		str[lsize] = 0;
	} else {
		ShowErrorDialog(
		_("GL_RecvString invalid size %ld > %ld(max size)"),
		(unsigned long)lsize,(unsigned long)maxsize);
	}
LEAVE_FUNC;
}

extern	void
GL_RecvName(
	NETFILE	*fp,
	size_t  size,
	char	*name)
{
ENTER_FUNC;
	GL_RecvString( fp, size, name);
LEAVE_FUNC;
}

static	void
GL_SendString(
	NETFILE	*fp,
	const char	*str)
{
	size_t		size;

ENTER_FUNC;
	if (str != NULL) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size);
	if (!CheckNetFile(fp)) {
		GL_Error();
	}
	if (size > 0) {
		Send(fp,(void*)str,size);
		if (!CheckNetFile(fp)) {
			GL_Error();
		}
	}
LEAVE_FUNC;
}

extern	void
GL_SendName(
	NETFILE	*fp,
	char	*name)
{
	size_t	size;

ENTER_FUNC;
	if		(   name  !=  NULL  ) { 
		size = strlen(name);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size);
	if		(  size  >  0  ) {
		Send(fp,name,size);
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
	}
LEAVE_FUNC;
}

extern	void
GL_SendDataType(
	NETFILE	*fp,
	PacketClass	c)
{
	nputc(c,fp);
}

static	void
GL_SendFixed(
	NETFILE	*fp,
	Fixed	*xval)
{
	GL_SendLength(fp,xval->flen);
	GL_SendLength(fp,xval->slen);
	GL_SendString(fp,xval->sval);
}

static	Fixed	*
GL_RecvFixed(
	NETFILE	*fp)
{
	Fixed	*xval;

ENTER_FUNC;
	xval = New(Fixed);
	xval->flen = GL_RecvLength(fp);
	xval->slen = GL_RecvLength(fp);
	xval->sval = (char *)xmalloc(xval->flen+1);
	GL_RecvString(fp, xval->flen, xval->sval);
LEAVE_FUNC;
	return	(xval); 
}

static	double
GL_RecvFloat(
	NETFILE	*fp)
{
	double	data;

	Recv(fp,&data,sizeof(data));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	(data);
}

static	void
GL_SendFloat(
	NETFILE	*fp,
	double	data)
{
	Send(fp,&data,sizeof(data));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	Bool
GL_RecvBool(
	NETFILE	*fp)
{
	char	buf[1];

	Recv(fp,buf,1);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

static	void
GL_SendBool(
	NETFILE	*fp,
	Bool	data)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	void
GL_RecvLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
	size_t	size;
ENTER_FUNC;
	size = GL_RecvLength(fp);
	LBS_ReserveSize(lbs,size,FALSE);
	if		(  size  >  0  ) {
		Recv(fp,LBS_Body(lbs),size);
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
	} else {
		dbgmsg("Recv LBS 0 size.");
	}
LEAVE_FUNC;
}

static	void
GL_SendLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
	GL_SendLength(fp,LBS_Size(lbs));
	if		(  LBS_Size(lbs)  >  0  ) {
		Send(fp,LBS_Body(lbs),LBS_Size(lbs));
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
	}
}

//////////////////////////////////////////////////////////////////////

static	Bool
RecvFile(
	NETFILE	*fp,
	char	*name)
{
	char 		*buff;
	Bool		ret;

ENTER_FUNC;
	GL_SendString(fp,name);
	if (fMlog) {
		MessageLogPrintf("recv screen file [%s]\n",name);
	}
	if (GL_RecvPacketClass(fp) == GL_ScreenDefine) {
		// download
		buff = xmalloc(SIZE_LARGE_BUFF);
		GL_RecvString(fp, SIZE_LARGE_BUFF, buff);
		CreateWindow(name,strlen(buff),buff);
		xfree(buff);
		ret = TRUE;
	} else {
		ShowErrorDialog(_("invalid protocol sequence"));
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
CheckScreens(
	NETFILE		*fp,
	Bool		fInit)
{	
	char		sname[SIZE_NAME];
	PacketClass	klass;

ENTER_FUNC;
	while ((klass = GL_RecvPacketClass(fp)) == GL_QueryScreen) {
		GL_RecvString(fp, sizeof(sname), sname);
		GL_RecvInt(fp);  /*stsize*/
		GL_RecvInt(fp);  /*stctime*/
		GL_RecvInt(fp);  /*stmtime*/

		if (GetWindowData(sname) == NULL) {
			GL_SendPacketClass(fp,GL_GetScreen);
			RecvFile(fp, sname);
		} else {
			GL_SendPacketClass(fp, GL_NOT);
		}
		if (fInit) {
			ShowWindow(sname);
			fInit = FALSE;
		}
	}
LEAVE_FUNC;
}

static	void
RecvValueSkip(
	NETFILE			*fp,
	PacketDataType	type)
{
	char			name[CLIENT_SIZE_BUFF];
	char			buff[CLIENT_SIZE_BUFF];
	int				count
	,				i;

ENTER_FUNC;
	if		(  type  ==  GL_TYPE_NULL  ) {
		type = GL_RecvDataType(fp);
	}
	switch	(type) {
	  case	GL_TYPE_INT:
		(void)GL_RecvInt(fp);
		break;
	  case	GL_TYPE_BOOL:
		(void)GL_RecvBool(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_OBJECT:
		GL_RecvLBS(fp, LargeBuff);
		break;
	  case	GL_TYPE_NUMBER:
		FreeFixed(GL_RecvFixed(fp));
		break;
	  case	GL_TYPE_ARRAY:
		count = GL_RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			RecvValueSkip(fp,GL_TYPE_NULL);
		}
		break;
	  case	GL_TYPE_RECORD:
		count = GL_RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			GL_RecvString(fp, sizeof(name), name);
			RecvValueSkip(fp,GL_TYPE_NULL);
		}
		break;
	  default:
		break;
	}
LEAVE_FUNC;
}

static	void
RecvNodeValue(
	NETFILE		*fp,
	char		*widgetName)
{
	PacketDataType	type;
	int				count
	,				i;
	char			name[CLIENT_SIZE_BUFF]
	,				childWidgetName[CLIENT_SIZE_BUFF];

ENTER_FUNC;
	type = GL_RecvDataType(fp);
	switch	(type) {
	  case	GL_TYPE_RECORD:
		count = GL_RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			GL_RecvString(fp, sizeof(name), name);
			sprintf(childWidgetName,"%s.%s",widgetName,name);
			RecvValue(fp,childWidgetName);
		}
		break;
	  case	GL_TYPE_ARRAY:
		count = GL_RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			sprintf(childWidgetName, "%s[%d]",widgetName, i);
			RecvValue(fp,childWidgetName);
		}
		break;
	  default:
		RecvValueSkip(fp,type);
		break;
	}
LEAVE_FUNC;
}

extern	void
RecvValue(
	NETFILE		*fp,
	char		*widgetName)
{
ENTER_FUNC;
	dbgprintf("WidgetName = [%s]\n",widgetName);
	if (IsWidgetName(THISWINDOW(Session))) {
		if (IsWidgetName(widgetName)) {
			if (RecvWidgetData(widgetName,fp)) {
			} else {
				RecvNodeValue(fp,widgetName);
			}
		} else {
			RecvValueSkip(fp,GL_TYPE_NULL);
		}
	} else {
		RecvValueSkip(fp,GL_TYPE_NULL);
	}
LEAVE_FUNC;
}

extern	Bool
GetScreenData(
	NETFILE		*fp)
{
	char			window[SIZE_NAME+1];
	char			widgetName[SIZE_LONGNAME+1];
	PacketClass		c;
	unsigned char	type;

ENTER_FUNC;
	CheckScreens(fp,FALSE);	 
	GL_SendPacketClass(fp,GL_GetData);
	GL_SendInt(fp,0);/*get all data*/
	if (fMlog) {
		MessageLog("====");
	}
	while ((c = GL_RecvPacketClass(fp)) == GL_WindowName) {
		GL_RecvString(fp, sizeof(window), window);
		dbgprintf("[%s]\n",window);
		type = (unsigned char)GL_RecvInt(FPCOMM(Session)); 
		if		(  fMlog  ) {
			switch	(type) {
			  case	SCREEN_NEW_WINDOW:
				MessageLogPrintf("new window [%s]\n",window);break;
			  case	SCREEN_CHANGE_WINDOW:
				MessageLogPrintf("change window [%s]\n",window);break;
			  case	SCREEN_CURRENT_WINDOW:
				MessageLogPrintf("current window [%s]\n",window);break;
			  case	SCREEN_CLOSE_WINDOW:
				MessageLogPrintf("close window [%s]\n",window);break;
			  case	SCREEN_JOIN_WINDOW:
				MessageLogPrintf("join window [%s]\n",window);break;
			}
		}
		switch	(type) {
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
		  case	SCREEN_CURRENT_WINDOW:
			if ((c = GL_RecvPacketClass(fp)) == GL_ScreenData) {
				if (THISWINDOW(Session) != NULL) {
					free(THISWINDOW(Session));
				}
				THISWINDOW(Session) = strdup(window);
				RecvValue(fp,window);
				ShowWindow(window);
				UpdateWindow(window);
				ResetTimer(window);
			}
			break;
		  case	SCREEN_CLOSE_WINDOW:
			CloseWindow(window);
			c = GL_RecvPacketClass(fp);
			break;
		  default:
			CloseWindow(window);
			c = GL_RecvPacketClass(fp);
			break;
		}
		if		(  c  ==  GL_NOT  ) {
			/*	no screen data	*/
		} else {
			/*	fatal error	*/
		}
	}

	if (c == GL_FocusName) {
		GL_RecvString(fp, sizeof(window), window);
		GL_RecvString(fp, sizeof(widgetName), widgetName);
		GrabFocus(window, widgetName);
		PandaTableFocusCell(widgetName);
		c = GL_RecvPacketClass(fp);
	}
LEAVE_FUNC;
	return TRUE;
}

static	void
GL_SendVersionString(
	NETFILE		*fp)
{
	char	version[256];
	char	buff[256];
	size_t	size;

	sprintf(version,"version:blob:expand:pdf:download:negotiation:v47:v48:i18n");
#ifdef	NETWORK_ORDER
	strcat(version, ":no");
#endif

	sprintf(buff,":agent=glclient2/%s",PACKAGE_VERSION);
	strcat(version, buff);

	size = strlen(version);
	SendChar(fp,(size&0xFF));
	SendChar(fp,0);
	SendChar(fp,0);
	SendChar(fp,0);
	Send(fp,version,size);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static LargeByteString*
RecvBLOB(
	NETFILE *fp,
	const char *oid)
{
	LargeByteString *lbs;

	lbs = NewLBS();
	GL_SendPacketClass(fp,GL_GetBLOB);ON_IO_ERROR(fp,badio);
	GL_SendString(fp,oid);ON_IO_ERROR(fp,badio);
	GL_RecvLBS(fp,lbs);
badio:
	return lbs;
}

static void
PrintReport(
	NETFILE *fp,
	json_object *obj)
{
	json_object *child;
	char *printer,*oid,*title;
	gboolean showdialog;
	LargeByteString *lbs;

	printer = NULL;
	title = "";
	showdialog = FALSE;

	child = json_object_object_get(obj,"object_id");
	if (!CheckJSONObject(child,json_type_string)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	child = json_object_object_get(obj,"printer");
	if (CheckJSONObject(child,json_type_string)) {
		printer = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"title");
	if (CheckJSONObject(child,json_type_string)) {
		title = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"showdialog");
	if (CheckJSONObject(child,json_type_boolean)) {
		showdialog = json_object_get_boolean(child);
	}
	lbs = RecvBLOB(fp,oid);
	if (LBS_Size(lbs) == 0) {
		FreeLBS(lbs);
		return;
	}
	if (showdialog) {
		ShowPrintDialog(title,lbs);
	} else {
		Print(title,printer,lbs);
	}
	FreeLBS(lbs);
}

static void
DownloadFile(
	NETFILE *fp,
	json_object *obj)
{
	json_object *child;
	char *oid,*filename,*desc;
	LargeByteString *lbs;

	filename = "foo.dat";
	desc = "";

	child = json_object_object_get(obj,"object_id");
	if (!CheckJSONObject(child,json_type_string)) {
		return;
	}
	oid = (char*)json_object_get_string(child);

	child = json_object_object_get(obj,"filename");
	if (CheckJSONObject(child,json_type_string)) {
		filename = (char*)json_object_get_string(child);
	}
	child = json_object_object_get(obj,"description");
	if (CheckJSONObject(child,json_type_string)) {
		desc = (char*)json_object_get_string(child);
	}
	lbs = RecvBLOB(fp,oid);
	if (LBS_Size(lbs) == 0) {
		FreeLBS(lbs);
		return;
	}
	ShowDownloadDialog(NULL,filename,desc,lbs);
	FreeLBS(lbs);
}

static void
CheckDownloads(
	NETFILE *fp)
{
	char buff[CLIENT_SIZE_BUFF];
	json_object *obj,*result,*child,*type;
	int i;

	GL_SendPacketClass(fp,GL_ListDownloads);ON_IO_ERROR(fp,badio);
	GL_RecvString(fp,sizeof(buff),buff);ON_IO_ERROR(fp,badio);
	obj = json_tokener_parse(buff);
	if (is_error(obj)) {
		return;
	}
	result = json_object_object_get(obj,"result");
	if (!CheckJSONObject(result,json_type_array)) {
		json_object_put(obj);
		return;
	}
	for(i=0;i<json_object_array_length(result);i++) {
		child = json_object_array_get_idx(result,i);
		if (!CheckJSONObject(child,json_type_object)) {
			continue;
		}
		type = json_object_object_get(child,"type");
		if (!CheckJSONObject(type,json_type_string)) {
			continue;
		}
		if (!strcmp(json_object_get_string(type),"report")) {
			PrintReport(fp,child);
		} else {
			DownloadFile(fp,child);
		}
	}
	json_object_put(obj);
badio:
	return;
}

static gint
PingTimerFunc(gpointer data)
{
	NETFILE *fp = (NETFILE *)data;
	PacketClass	c;
	char buff[CLIENT_SIZE_BUFF];

	if (ISRECV(Session)) {
		return 1;
	}
	ISRECV(Session) = TRUE;
	fp = (NETFILE *)data;
	if (fV47) {
		GL_SendPacketClass(fp,GL_Ping);ON_IO_ERROR(fp,badio);
		c = GL_RecvPacketClass(fp);ON_IO_ERROR(fp,badio);
		switch (c) {
		case GL_Pong_Dialog:
			GL_RecvString(fp, sizeof(buff), buff);ON_IO_ERROR(fp,badio);
			ISRECV(Session) = FALSE;
			ShowInfoDialog(buff);
			break;
		case GL_Pong_Popup:
			GL_RecvString(fp, sizeof(buff), buff);ON_IO_ERROR(fp,badio);
			ISRECV(Session) = FALSE;
			Notify(_("glclient message notify"),buff,"gtk-dialog-info",0);
			break;
		case GL_Pong_Abort:
			GL_RecvString(fp, sizeof(buff), buff);ON_IO_ERROR(fp,badio);
			ShowInfoDialog(buff);
			exit(1);
			break;
		case GL_Pong:
			ISRECV(Session) = FALSE;
			break;
		default:
			ShowErrorDialog(_("connection error(invalid pong packet)"));
			break;
		}
	}
	CheckDownloads(fp);
	ISRECV(Session) = FALSE;
	return 1;
badio:
	ShowErrorDialog(_("connection error(server doesn't reply ping)"));
	return 1;
}

extern	Bool
SendConnect(
	NETFILE		*fp,
	char		*apl)
{
	Bool		rc;
	PacketClass	pc;
	char		ver[16];

ENTER_FUNC;
	rc = TRUE;
	if		(  fMlog  ) {
		MessageLog(_("connection start\n"));
	}
	GL_SendPacketClass(fp,GL_Connect);
	GL_SendVersionString(fp);
	GL_SendString(fp,User);
	GL_SendString(fp,Pass);
	GL_SendString(fp,apl);
	pc = GL_RecvPacketClass(fp);
	if		(  pc  ==  GL_OK  ) {
	} else if (pc == GL_ServerVersion) {
		GL_RecvString(fp, sizeof(ver), ver);
		if (strcmp(ver, "1.4.4.01") >= 0) {
			SetPingTimerFunc(PingTimerFunc, fp);
		}
		if (strcmp(ver, "1.4.6.99") >= 0) {
			fV47 = TRUE;
		} else {
			fV47 = FALSE;
		}
	} else {
		rc = FALSE;
		switch	(pc) {
		  case	GL_NOT:
			ShowErrorDialog(_("can not connect server"));
			break;
		  case	GL_E_VERSION:
			ShowErrorDialog(_("can not connect server(version not match)"));
			break;
		  case	GL_E_AUTH:
#ifdef USE_SSL
			if 	(fSsl) {
				ShowErrorDialog(
					_("can not connect server(authentication error)"));
			} else {
				ShowErrorDialog(
					_("can not connect server(user or password is incorrect)"));
			}
#else
			ShowErrorDialog(
				_("can not connect server(user or password is incorrect)"));
#endif
			break;
		  case	GL_E_APPL:
			ShowErrorDialog(
				_("can not connect server(application name invalid)"));
			break;
		  default:
			dbgprintf("[%X]\n",pc);
			ShowErrorDialog(_("can not connect server(other protocol error)"));
			break;
		}
	}
LEAVE_FUNC;
	return	(rc);
}

extern	void
SendEvent(
	NETFILE		*fp,
	char		*window,
	char		*widget,
	char		*event)
{
ENTER_FUNC;
	dbgprintf("window = [%s]",window); 
	dbgprintf("widget = [%s]",widget); 
	dbgprintf("event  = [%s]",event);
	if		(  fMlog  ) {
		MessageLogPrintf("send event  [%s:%s:%s]\n",window,widget,event);
	}
	GL_SendPacketClass(fp,GL_Event);
	GL_SendString(fp,window);
	GL_SendString(fp,widget);
	GL_SendString(fp,event);
LEAVE_FUNC;
}

extern	void
InitProtocol(void)
{
ENTER_FUNC;
	LargeBuff = NewLBS();
	THISWINDOW(Session) = NULL;
	WINDOWTABLE(Session) = NewNameHash();
	WIDGETTABLE(Session) = NewNameHash();
LEAVE_FUNC;
}

static gboolean RemoveChangedWidget(gpointer	key,
	gpointer	value,
	gpointer	user_data) 
{
	g_free((char *)key);
	return TRUE;
}

static	void
_SendWindowData(
	char		*wname,
	WindowData	*wdata,
	gpointer	user_data)
{
ENTER_FUNC;
	GL_SendPacketClass(FPCOMM(Session),GL_WindowName);
	GL_SendString(FPCOMM(Session),wname);
	g_hash_table_foreach(wdata->ChangedWidgetTable,
		(GHFunc)SendWidgetData,FPCOMM(Session));
	GL_SendPacketClass(FPCOMM(Session),GL_END);

	if (wdata->ChangedWidgetTable != NULL) {
		g_hash_table_foreach_remove(wdata->ChangedWidgetTable, 
			RemoveChangedWidget, NULL);
		g_hash_table_destroy(wdata->ChangedWidgetTable);
	}
    wdata->ChangedWidgetTable = NewNameHash();
LEAVE_FUNC;
}

extern	void
SendWindowData(void)
{
ENTER_FUNC;
	g_hash_table_foreach(WINDOWTABLE(Session),(GHFunc)_SendWindowData,NULL);
	GL_SendPacketClass(FPCOMM(Session),GL_END);
LEAVE_FUNC;
}

extern	PacketDataType
RecvFixedData(
	NETFILE	*fp,
	Fixed	**xval)
{
	PacketDataType	type;
	
	type = GL_RecvDataType(fp);

	switch	(type) {
	  case	GL_TYPE_NUMBER:
		*xval = GL_RecvFixed(fp);
		break;
	  default:
		printf(_("invalid data conversion\n"));
		exit(1);
		break;
	}
	return type;
}

extern	void
SendFixedData(
	NETFILE			*fp,
	PacketDataType	type,
	Fixed			*xval)
{
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,(char *)xval->sval);
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFixed(fp,xval);
		break;
	  default:
		printf(_("invalid data conversion\n"));
		exit(1);
		break;
	}
}

extern	void
SendStringData(
	NETFILE			*fp,
	PacketDataType	type,
	char			*str)
{
ENTER_FUNC;
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,(char *)str);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_OBJECT:
		LBS_ReserveSize(LargeBuff,strlen(str)+1,FALSE);
		strcpy(LBS_Body(LargeBuff),str);
		GL_SendLBS(fp,LargeBuff);
		break;
	  case	GL_TYPE_INT:
		GL_SendInt(fp,atoi(str));
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,atof(str));
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,(( *str == 'T' ) ? TRUE: FALSE ));
		break;
	  default:
		break;
	}
LEAVE_FUNC;
}

extern	PacketDataType
RecvStringData(
	NETFILE	*fp,
	char	*str,
	size_t	size)
{
	PacketDataType	type;

ENTER_FUNC;
	type = GL_RecvDataType(fp);
	switch	(type) {
	  case	GL_TYPE_INT:
		dbgmsg("int");
		sprintf(str,"%d",GL_RecvInt(fp));
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, size, str);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_OBJECT:
		dbgmsg("LBS");
		GL_RecvLBS(fp,LargeBuff);
		if		(  LBS_Size(LargeBuff)  >  0  ) {
			memcpy(str,LBS_Body(LargeBuff),LBS_Size(LargeBuff));
			str[LBS_Size(LargeBuff)] = 0;
		} else {
			*str = 0;
		}
		break;
	}
LEAVE_FUNC;
	return type;
}


extern	void
SendIntegerData(
	NETFILE			*fp,
	PacketDataType	type,
	int				val)
{
	char	buff[CLIENT_SIZE_BUFF];
	
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(buff,"%d",val);
		GL_SendString(fp,(char *)buff);
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_INT:
		GL_SendInt(fp,val);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,(( val == 0 ) ? FALSE : TRUE ));
		break;
	  default:
		break;
	}
}

extern	PacketDataType
RecvIntegerData(
	NETFILE	*fp,
	int		*val)
{
	PacketDataType type;
	char	buff[CLIENT_SIZE_BUFF];
	
	type = GL_RecvDataType(fp);

	switch	(type) {
	  case	GL_TYPE_INT:
		*val = GL_RecvInt(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = atoi(buff);
		break;
	}
	return type;
}

extern	void
SendBoolData(
	NETFILE			*fp,
	PacketDataType	type,
	Bool			val)
{
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,( val ? "T" : "F" ));
		break;
	  case	GL_TYPE_INT:
		GL_SendInt(fp,(int)val);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,val);
		break;
	  default:
		break;
	}
}

extern	PacketDataType
RecvBoolData(
	NETFILE	*fp,
	Bool	*val)
{
	PacketDataType	type;
	char	buff[CLIENT_SIZE_BUFF];
	
	type = GL_RecvDataType(fp);

	switch	(type) {
	  case	GL_TYPE_INT:
		*val = ( GL_RecvInt(fp) == 0 ) ? FALSE : TRUE;
		break;
	  case	GL_TYPE_BOOL:
		*val = GL_RecvBool(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = (  buff[0]  ==  'T'  ) ? TRUE : FALSE;
		break;
	}
	return	type;
}

extern	void
SendFloatData(
	NETFILE			*fp,
	PacketDataType	type,
	double			val)
{
	char	buff[CLIENT_SIZE_BUFF];
	
	GL_SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(buff,"%g",val);
		GL_SendString(fp,(char *)buff);
		break;
#if	0
	  case	GL_TYPE_NUMBER:
		xval = DoubleToFixed(val);
		GL_SendFixed(fp,xval);
		FreeFixed(xval);
		break;
#endif
	  case	GL_TYPE_INT:
		GL_SendInt(fp,(int)val);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,val);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,(( val == 0 ) ? FALSE : TRUE ));
		break;
	  default:
		printf(_("invalid data conversion\n"));
		exit(1);
		break;
	}
}

extern	PacketDataType
RecvFloatData(
	NETFILE	*fp,
	double	*val)
{
	PacketDataType	type;
	char	buff[CLIENT_SIZE_BUFF];
	Fixed	*xval;

ENTER_FUNC;
	type = GL_RecvDataType(fp);

	switch	(type)	{
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = atof(buff);
		break;
	  case	GL_TYPE_NUMBER:
		xval = GL_RecvFixed(fp);
		*val = atof(xval->sval) / pow(10.0, xval->slen);
		xfree(xval);
		break;
	  case	GL_TYPE_INT:
		*val = (double)GL_RecvInt(fp);
		break;
	  case	GL_TYPE_FLOAT:
		*val = GL_RecvFloat(fp);
		break;
	}
LEAVE_FUNC;
	return type;
}

extern PacketDataType
RecvBinaryData(NETFILE *fp, LargeByteString *binary)
{
	PacketDataType	type;

ENTER_FUNC;
    type = GL_RecvDataType(fp);
    switch (type) {
    case  GL_TYPE_CHAR:
    case  GL_TYPE_VARCHAR:
    case  GL_TYPE_DBCODE:
    case  GL_TYPE_TEXT:
    case  GL_TYPE_BINARY:
    case  GL_TYPE_BYTE:
    case  GL_TYPE_OBJECT:
        GL_RecvLBS(fp, binary);
        break;
    default:
        Warning("unsupported data type: %d\n", type);
        break;
    }
LEAVE_FUNC;
	return type;
}

extern void
SendBinaryData(
	NETFILE 		*fp, 
	PacketDataType 	type, 
	LargeByteString	*binary)
{
ENTER_FUNC;
    switch (type) {
    case  GL_TYPE_BINARY:
    case  GL_TYPE_BYTE:
    case  GL_TYPE_OBJECT:
		GL_SendDataType(fp, GL_TYPE_OBJECT);
        GL_SendLBS(fp, binary);
        break;
    default:
        Warning("unsupported data type: %d\n", type);
        break;
    }
LEAVE_FUNC;
}
