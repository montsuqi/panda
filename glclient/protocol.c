/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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
#include    <sys/types.h>
#include	<sys/stat.h>
#include    <unistd.h>
#include	<sys/time.h>
#include	<netinet/in.h>
#ifdef USE_GNOME
#	include	<gnome.h>
#else
#	include	<gtk/gtk.h>
#	include	"gettext.h"
#endif
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif

#include	"types.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"comm.h"
#include	"protocol.h"
#define		_PROTOCOL_C
#include	"action.h"
#include	"callbacks.h"
#include	"widgetOPS.h"
#include	"dialogs.h"
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

#if	0
static	GList		*ScreenStack;
static	void
InitScreenStack(void)
{
	ScreenStack = NULL;
}

static	void
PushScreenStack(
	char	*name)
{
	ScreenStack = g_list_append(ScreenStack,name);
}

static	char	*
BottomScreenStack(void)
{
	GList	*last;
	char	*ret;

	if		(  ScreenStack  !=  NULL  ) {
		last = g_list_last(ScreenStack);
		ret = (char *)last->data;
	} else {
		ret = NULL;
	}
	return	(ret);
}

static	void
PopScreenStack(void)
{
	GList	*last;

	last = g_list_last(ScreenStack);
	ScreenStack = g_list_remove_link(ScreenStack,last);
}
#endif

/*
 *	send/recv functions
 */

#define	SendChar(fp,c)	nputc((c),(fp))
#define	RecvChar(fp)	ngetc(fp)

static void
GL_Error(void)
{
	GLError(_("Connection lost\n"));
	if (gtk_main_level() > 0 ) 
		gtk_main_quit();
}

extern	void
GL_SendPacketClass(
	NETFILE	*fp,
	PacketClass	c)
{
	nputc(c,fp);
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
	DataType = c;
	return	(c);
}

extern	void
GL_SendInt(
	NETFILE	*fp,
	int		data)
{
	byte	buff[sizeof(int)];

	*(int *)buff = SEND32(data);
	Send(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

extern	int
GL_RecvInt(
	NETFILE	*fp)
{
	byte	buff[sizeof(int)];

	Recv(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	(RECV32(*(int *)buff));
}

static	void
GL_SendLength(
	NETFILE	*fp,
	size_t	data)
{
	byte	buff[sizeof(int)];

	*(int *)buff = SEND32(data);
	Send(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	size_t
GL_RecvLength(
	NETFILE	*fp)
{
	byte	buff[sizeof(int)];

	Recv(fp,buff,sizeof(int));
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	return	((size_t)RECV32(*(int *)buff));
}
#if	0
static	unsigned	int
GL_RecvUInt(
	NETFILE	*fp)
{
	byte	buff[sizeof(int)];

	Recv(fp,buff,sizeof(unsigned int));
	return	(RECV32(*(unsigned int *)buff));
}

static	void
GL_SendUInt(
	NETFILE	*fp,
	unsigned	int		data)
{
	byte	buff[sizeof(int)];

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
	size_t  size,
	char	*str)
{
	size_t	lsize;

ENTER_FUNC;
	lsize = GL_RecvLength(fp);
	if		(	size > lsize 	){
		size = lsize;
		Recv(fp,str,size);
		if		(  !CheckNetFile(fp)  ) {
			GL_Error();
		}
		str[size] = 0;
	} else {
		GLError(_("error size mismatch !"));
		exit(1);
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
	char	*str)
{
	size_t	size;

ENTER_FUNC;
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
	if		(  size  >  0  ) {
		Send(fp,str,size);
		if		(  !CheckNetFile(fp)  ) {
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
#if	0
static	void
GL_SendObject(
	NETFILE	*fp,
	MonObjectType	obj)
{
	GL_SendUInt(fp,(unsigned int)obj);
	if		(  !CheckNetFile(fp)  ) {
		GL_Error();
	}
}

static	MonObjectType
GL_RecvObject(
	NETFILE	*fp)
{
	return	((MonObjectType)GL_RecvUInt(fp));
}
#endif
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
	GL_RecvString(fp, (xval->flen + 1), xval->sval);
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
	NETFILE	*fpC,
	char	*name,
	char	*fname)
{
	FILE		*fp;
	size_t		size
	,			left;
	char		buff[SIZE_BUFF];
	Bool		ret;
	gchar 		*tmpfile, *dirname;
	int 		fd;

ENTER_FUNC;
	GL_SendPacketClass(fpC,GL_GetScreen);
	GL_SendString(fpC,name);
	if		(  fMlog  ) {
		sprintf(buff,"recv screen file [%s]\n",name);
		MessageLog(buff);
	}
	if		(  GL_RecvPacketClass(fpC)  ==  GL_ScreenDefine  ) {
		tmpfile = g_strconcat(fname, "gl_cache_XXXXXX", NULL);
		dirname = g_dirname(tmpfile);
		MakeCacheDir(dirname);
		g_free(dirname);
		if  ((fd = mkstemp(tmpfile)) == -1 ) {
			GLError(_("could not write tmp file"));
			exit(1);
		}
		if	((fp = fdopen(fd,"w")) == NULL) {
			GLError(_("could not write cache file"));
			exit(1);
		}
		fchmod(fileno(fp), 0600);
		left = (size_t)GL_RecvInt(fpC);
		do {
			if		(  left  >  SIZE_BUFF  ) {
				size = SIZE_BUFF;
			} else {
				size = left;
			}
			size = Recv(fpC,buff,size);
			if		(  size  >  0  ) {
				fwrite(buff,1,size,fp);
				left -= size;
			}
		}	while	(  left  >  0  );
		fclose(fp);
		rename(tmpfile, fname);
		unlink(tmpfile);
		g_free(tmpfile);
		ret = TRUE;
	} else {
		GLError(_("invalid protocol sequence"));
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
	char		sname[SIZE_BUFF];
	char		*fname;
	struct	stat	stbuf;
	time_t		stmtime
	,			stctime;
	off_t		stsize;
	PacketClass	klass;

ENTER_FUNC;
	while		(  ( klass = GL_RecvPacketClass(fp) )  ==  GL_QueryScreen  ) {
dbgmsg("*");
		GL_RecvString(fp, sizeof(sname), sname);
		stsize = (off_t)GL_RecvInt(fp);
		stmtime = (time_t)GL_RecvInt(fp);
		stctime = (time_t)GL_RecvInt(fp);
		fname = CacheFileName(sname);

		if		(	(  stat(fname,&stbuf)  !=  0        )
				 ||	(  stbuf.st_mtime      <   stmtime  )
				 ||	(  stbuf.st_ctime      <   stctime  )
				 ||	(  stbuf.st_size       !=  stsize   ) ) {
			RecvFile(fp, sname, fname);
		} else {
			GL_SendPacketClass(fp, GL_NOT);
		}
		if		(  fInit  ) {
			ShowWindow(sname,SCREEN_NEW_WINDOW);
			fInit = FALSE;
		}
	}
LEAVE_FUNC;
}

extern	Bool
RecvWidgetData(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	HandlerNode	*node;
	RecvHandler	rfunc;
	Bool		ret;

ENTER_FUNC;
	if		(  ( node = g_hash_table_lookup(ClassTable,
											(gconstpointer)(long)((GtkTypeObject *)widget)->klass->type) )  !=  NULL  ) {
		rfunc = node->rfunc;
		ret = (*rfunc)(widget,fp);
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

static	Bool
SendWidgetData(
	char		*name,
	GtkWidget	*widget,
	NETFILE		*fp)
{
	HandlerNode	*node;
	SendHandler	sfunc;
	Bool		ret;

ENTER_FUNC;
	if		(  ( node = g_hash_table_lookup(ClassTable,
											(gconstpointer)(long)((GtkTypeObject *)widget)->klass->type) )  !=  NULL  ) {
		sfunc = node->sfunc;
		ret = (*sfunc)(name,widget,fp);
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
RecvValueSkip(
	NETFILE			*fp,
	PacketDataType	type)
{
	char			name[SIZE_BUFF];
	char			buff[SIZE_BUFF];
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

extern	void
RecvValue(
	NETFILE		*fp,
	char		*longname)
{
	GtkWidget		*widget;
	Bool			fTrace;
	PacketDataType	type;
	int				count
	,				i;
	char			name[SIZE_BUFF]
	,				*dataname;
	Bool			fDone;

ENTER_FUNC;
	dbgprintf("WidgetName = [%s]\n",WidgetName);
	if		(  ThisXML  !=  NULL  ) {
		fDone = FALSE;
		fTrace = TRUE;
		if		(  Protocol1  ) {
			if		(  ( widget = glade_xml_get_widget_by_long_name(ThisXML,WidgetName) )
					   !=  NULL  ) {
				if		(  RecvWidgetData(widget,fp)  ) {
					fTrace = FALSE;
				}
				fDone = TRUE;
			} else {
				if		(  !Protocol2  ) {
					fTrace = FALSE;	/*	fatal error	*/
					fDone = TRUE;
					RecvValueSkip(fp,GL_TYPE_NULL);
				}
			}
		}
		if		(  Protocol2  ) {
			if		(  !fDone  ) {
				if		(  ( dataname = strchr(WidgetName,'.') )  !=  NULL  ) {
					dataname ++;
				}
				dbgprintf("dataname = [%s]\n",dataname);
				if		(  ( widget = glade_xml_get_widget(ThisXML,dataname) )
						   !=  NULL  ) {
					if		(  RecvWidgetData(widget,fp)  ) {
						fTrace = FALSE;
					}
					fDone = TRUE;
				}
			}
		}
		if		(  !fDone  ) {
			fTrace = TRUE;
		}
	} else {
		fTrace = FALSE;	/*	fatal error	*/
		RecvValueSkip(fp,GL_TYPE_NULL);
	}
	if		(  fTrace  ) {
		type = GL_RecvDataType(fp);
		switch	(type) {
		  case	GL_TYPE_RECORD:
			count = GL_RecvInt(fp);
			for	(  i = 0 ; i < count ; i ++ ) {
				GL_RecvString(fp, sizeof(name), name);
				sprintf(longname,".%s",name);
				RecvValue(fp,longname + strlen(name) + 1);
			}
			break;
		  case	GL_TYPE_ARRAY:
			count = GL_RecvInt(fp);
			for	(  i = 0 ; i < count ; i ++ ) {
				sprintf(name,"[%d]",i);
				sprintf(longname,"%s",name);
				RecvValue(fp,longname + strlen(name));
			}
			break;
		  default:
			RecvValueSkip(fp,type);
			break;
		}
	}
LEAVE_FUNC;
}

extern	Bool
GetScreenData(
	NETFILE		*fp)
{
	char		window[SIZE_BUFF]
	,			widgetName[SIZE_BUFF];
	PacketClass	c;
	Bool		fCancel;
	byte		type;
	XML_Node	*node;
	GtkWidget	*widget;
	char		buff[SIZE_BUFF];

ENTER_FUNC;
	fInRecv = TRUE; 
	CheckScreens(fp,FALSE);	 
	GL_SendPacketClass(fp,GL_GetData);
	GL_SendInt(fp,0);				/*	get all data	*/
	fCancel = FALSE;
	while	(  ( c = GL_RecvPacketClass(fp) )  ==  GL_WindowName  ) {
		GL_RecvString(fp, sizeof(window), window);
		if		(  fMlog  ) {
			sprintf(buff,"recv window [%s]\n",window);
			MessageLog(buff);
		}
		dbgprintf("[%s]\n",window);
		switch( type = (byte)GL_RecvInt(FPCOMM(glSession)) ) {
		  case	SCREEN_END_SESSION:
			fCancel= TRUE;
			break;
		  case	SCREEN_CLOSE_WINDOW:
		  case	SCREEN_JOIN_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			fCancel = TRUE;
			break;
		  case	SCREEN_CURRENT_WINDOW:
			break;
		  default:
			break;
		}
		if		(  ( node = ShowWindow(window,type) )  !=  NULL  ) {
			ThisXML = node->xml;
		}
		switch	(type) {
		  case	SCREEN_CURRENT_WINDOW:
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CHANGE_WINDOW:
			strcpy(WidgetName,window);
			if		(  ( c = GL_RecvPacketClass(fp) )  ==  GL_ScreenData  ) {
				RecvValue(fp,WidgetName + strlen(WidgetName));
			}
			break;
		  default:
			c = GL_RecvPacketClass(fp);
			break;
		}
		if		(  c  ==  GL_NOT  ) {
			/*	no screen data	*/
		} else {
			/*	fatal error	*/
		}
	}
	if		(  c  ==  GL_FocusName  ) {
		GL_RecvString(fp, sizeof(window), window);
		GL_RecvString(fp, sizeof(widgetName), widgetName);
		if		(	(  ( node = g_hash_table_lookup(WindowTable,window) )  !=  NULL  )
					&&	(  node->xml  !=  NULL  )
					&&	(  ( widget = glade_xml_get_widget(node->xml,widgetName) )
						   !=  NULL  ) ) {
			GrabFocus(widget);
		}
		c = GL_RecvPacketClass(fp);
	}
	/* reset GtkPandaTimer if exists */
	if		(  ( node = g_hash_table_lookup(WindowTable,window) )  !=  NULL  ) {
		ResetTimer(node->window);
	}
	fInRecv = FALSE;
LEAVE_FUNC;
	return	(fCancel);
}

static	void
GL_SendVersionString(
	NETFILE		*fp)
{
	char	*version;
	size_t	size;

#ifdef	NETWORK_ORDER
	version = "version:no:blob:expand:ps";
#else
	version = "version:blob:expand:ps";
#endif
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

extern	Bool
SendConnect(
	NETFILE		*fp,
	char		*apl)
{
	Bool	rc;
	PacketClass	pc;

ENTER_FUNC;
	if		(  fMlog  ) {
		MessageLog(_("connection start\n"));
	}
	GL_SendPacketClass(fp,GL_Connect);
	GL_SendVersionString(fp);
	GL_SendString(fp,User);
	GL_SendString(fp,Pass);
	GL_SendString(fp,apl);
	if		(  ( pc = GL_RecvPacketClass(fp) )   ==  GL_OK  ) {
		rc = TRUE;
	} else {
		rc = FALSE;
		switch	(pc) {
		  case	GL_NOT:
			GLError(_("can not connect server"));
			break;
		  case	GL_E_VERSION:
			GLError(_("can not connect server(version not match)"));
			break;
		  case	GL_E_AUTH:
#ifdef USE_SSL
			if 	(fSsl) {
				GLError(_("can not connect server(authentication error)"));
			} else {
				GLError(_("can not connect server(user or password is incorrect)"));
			}
#else
			GLError(_("can not connect server(user or password is incorrect)"));
#endif
			break;
		  case	GL_E_APPL:
			GLError(_("can not connect server(application name invalid)"));
			break;
		  default:
			dbgprintf("[%X]\n",pc);
			GLError(_("can not connect server(other protocol error)"));
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
	char		buff[SIZE_BUFF];

ENTER_FUNC;
	dbgprintf("window = [%s]",window); 
	dbgprintf("widget = [%s]",widget); 
	dbgprintf("event  = [%s]",event); 
	if		(  fMlog  ) {
		sprintf(buff,"send event  [%s:%s:%s]\n",window,widget,event);
		MessageLog(buff);
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
	WindowTable = NewNameHash();
	LargeBuff = NewLBS();
#if	0
	InitScreenStack();
#endif
	InitWidgetOperations();
LEAVE_FUNC;
}

static void
ghfunc_window_table_free (char *wname, XML_Node *node, gpointer user_data)
{
    gtk_object_unref (GTK_OBJECT (node->xml));
    xfree (node->name);
    if (node->UpdateWidget != NULL) { 
		g_hash_table_destroy (node->UpdateWidget);
	}
    xfree (node);
}

void
TermProtocol(void)
{
	if (WindowTable != NULL) {
		g_hash_table_foreach(WindowTable, (GHFunc) ghfunc_window_table_free, NULL);
        g_hash_table_destroy (WindowTable);
        WindowTable = NULL;
	}
}

static	void
_SendWindowData(
	char		*wname,
	XML_Node	*node,
	gpointer	user_data)
{
ENTER_FUNC;
	GL_SendPacketClass(FPCOMM(glSession),GL_WindowName);
	GL_SendString(FPCOMM(glSession),wname);
	g_hash_table_foreach(node->UpdateWidget,(GHFunc)SendWidgetData,FPCOMM(glSession));
	GL_SendPacketClass(FPCOMM(glSession),GL_END);
ENTER_FUNC;
}

extern	void
SendWindowData(void)
{
ENTER_FUNC;
	g_hash_table_foreach(WindowTable,(GHFunc)_SendWindowData,NULL);
	GL_SendPacketClass(FPCOMM(glSession),GL_END);
	ClearWindowTable();
ENTER_FUNC;
}

extern	Bool
RecvFixedData(
	NETFILE	*fp,
	Fixed	**xval)
{
	Bool	ret;
	
	DataType = GL_RecvDataType(fp);

	switch	(DataType) {
	  case	GL_TYPE_NUMBER:
		*xval = GL_RecvFixed(fp);
		ret = TRUE;
		break;
	  default:
		printf(_("invalid data conversion\n"));
		exit(1);
		ret = FALSE;
		break;
	}
	return	(ret);
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

extern	char	*
RecvStringData(
	NETFILE	*fp,
	char	*str,
	size_t	size)
{
	char	*ret;

ENTER_FUNC;
	DataType = GL_RecvDataType(fp);
	switch	(DataType) {
	  case	GL_TYPE_INT:
		dbgmsg("int");
		sprintf(str,"%d",GL_RecvInt(fp));
		ret = str;
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, size, str);
		ret = str;
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
		ret = str;
		break;
	  default:
		ret = NULL;
		break;
	}
LEAVE_FUNC;
	return	(ret);
}


extern	void
SendIntegerData(
	NETFILE			*fp,
	PacketDataType	type,
	int				val)
{
	char	buff[SIZE_BUFF];
	
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

extern	Bool
RecvIntegerData(
	NETFILE	*fp,
	int		*val)
{
	char	buff[SIZE_BUFF];
	Bool	ret;
	
	DataType = GL_RecvDataType(fp);

	switch	(DataType) {
	  case	GL_TYPE_INT:
		*val = GL_RecvInt(fp);
		ret = TRUE;
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = atoi(buff);
		ret = TRUE;
		break;
	  default:
		ret = 0;
		ret = FALSE;
		break;
	}
	return	(ret);
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

extern	Bool
RecvBoolData(
	NETFILE	*fp,
	Bool	*val)
{
	char	buff[SIZE_BUFF];
	Bool	ret;
	
	DataType = GL_RecvDataType(fp);

	ret = TRUE;
	switch	(DataType) {
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
	  default:
		ret = FALSE;
		break;
	}
	return	(ret);
}

extern	void
SendFloatData(
	NETFILE			*fp,
	PacketDataType	type,
	double			val)
{
	char	buff[SIZE_BUFF];
	
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

extern	Bool
RecvFloatData(
	NETFILE	*fp,
	double	*val)
{
	char	buff[SIZE_BUFF];
	Bool	ret;
	Fixed	*xval;

ENTER_FUNC;
	DataType = GL_RecvDataType(fp);

	switch	(DataType)	{
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp, sizeof(buff), buff);
		*val = atof(buff);
		ret = TRUE;
		break;
	  case	GL_TYPE_NUMBER:
		xval = GL_RecvFixed(fp);
		*val = atof(xval->sval) / pow(10.0, xval->slen);
		xfree(xval);
		ret = TRUE;
		break;
	  case	GL_TYPE_INT:
		*val = (double)GL_RecvInt(fp);
		ret = TRUE;
		break;
	  case	GL_TYPE_FLOAT:
		*val = GL_RecvFloat(fp);
		ret = TRUE;
		break;
	  default:
		ret = 0;
		ret = FALSE;
		break;
	}
LEAVE_FUNC;
	return	(ret);
}

extern void
RecvBinaryData(NETFILE *fp, LargeByteString *binary)
{
ENTER_FUNC;
    DataType = GL_RecvDataType(fp);
    switch (DataType) {
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
        Warning("unsupported data type: %d\n", DataType);
        break;
    }
LEAVE_FUNC;
}
