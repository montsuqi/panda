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
/*
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include	<sys/stat.h>
#include    <unistd.h>
#include	<sys/time.h>
#ifdef ENABLE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif

#include	"types.h"
#include	"misc.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"comm.h"
#include	"protocol.h"
#define		_PROTOCOL_C
#include	"callbacks.h"
#include	"widgetOPS.h"
#include	"message.h"
#include	"debug.h"

static	GHashTable	*ClassTable;


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

dbgmsg(">RecvFile");
	SendPacketClass(fpC,GL_GetScreen);
	SendString(fpC,name);
	if		(  RecvPacketClass(fpC)  ==  GL_ScreenDefine  ) {
		fp = Fopen(fname,"w");
		left = (size_t)RecvLong(fpC);
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
		ret = TRUE;
	} else {
		g_warning("invalid protocol sequence");
		ret = FALSE;
	}
dbgmsg("<RecvFile");
	return	(ret);
}

extern	XML_Node	*
ShowWindow(
	char	*wname,
	int		type)
{
	char		*fname;
	XML_Node	*node;

dbgmsg(">ShowWindow");
	dbgprintf("ShowWindow [%s][%d]",wname,type);
	fname = CacheFileName(wname);

	if		(  ( node = g_hash_table_lookup(WindowTable,wname) )  ==  NULL  ) {
		/* Create new node */
		switch	(type) {
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CURRENT_WINDOW:
			node = New(XML_Node);
			node->xml = glade_xml_new(fname, NULL);
			node->window = GTK_WINDOW(glade_xml_get_widget(node->xml, wname));
			node->name = StrDup(wname);
			node->UpdateWidget = NewNameHash();
			glade_xml_signal_autoconnect(node->xml);
			g_hash_table_insert(WindowTable,node->name,node);
		}
	}

	if		(  node  !=  NULL  ) {
		switch	(type) {
		  case	SCREEN_NEW_WINDOW:
		  case	SCREEN_CURRENT_WINDOW:
			gtk_widget_show_all(GTK_WIDGET(node->window));
			break;
		  case	SCREEN_CLOSE_WINDOW:
			gtk_widget_hide_all(GTK_WIDGET(node->window));
			/* fall through */
		  default:
			node = NULL;
			break;
		}
	}

dbgmsg("<ShowWindow");
	return	(node);
}

static	void
DestroyWindow(
	char	*sname)
{
	XML_Node	*node;

	if		(  ( node = (XML_Node *)g_hash_table_lookup(WindowTable,sname) )
			   !=  NULL  ) {
		gtk_widget_destroy(GTK_WIDGET(node->window));
		gtk_object_destroy((GtkObject *)node->xml);
		if		(  node->UpdateWidget  !=  NULL  ) {
			g_hash_table_destroy(node->UpdateWidget);
		}
		xfree(node->name);
		xfree(node);
		g_hash_table_remove(WindowTable,sname);
	}
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

dbgmsg(">CheckScreens");
	while		(  ( klass = RecvPacketClass(fp) )  ==  GL_QueryScreen  ) {
		RecvString(fp,sname);
		stsize = (off_t)RecvLong(fp);
		stmtime = (time_t)RecvLong(fp);
		stctime = (time_t)RecvLong(fp);
		fname = CacheFileName(sname);

		if		(	(  stat(fname,&stbuf)  !=  0        )
				 ||	(  stbuf.st_mtime      <   stmtime  )
				 ||	(  stbuf.st_ctime      <   stctime  )
				 ||	(  stbuf.st_size       !=  stsize   ) ) {
			RecvFile(fp, sname, fname);
			/* Clear cache */
			DestroyWindow(sname);
		} else {
			SendPacketClass(fp, GL_NOT);
		}
		if		(  fInit  ) {
			ShowWindow(sname,SCREEN_NEW_WINDOW);
			fInit = FALSE;
		}
	}
dbgmsg("<CheckScreens");
}

extern	Bool
RecvWidgetData(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	HandlerNode	*node;
	RecvHandler	rfunc;
	Bool		ret;

dbgmsg(">RecvWidgetData");
	if		(  ( node = g_hash_table_lookup(ClassTable,
											(gconstpointer)((GtkTypeObject *)widget)->klass->type) )  !=  NULL  ) {
		rfunc = node->rfunc;
		ret = (*rfunc)(widget,fp);
	} else {
		ret = FALSE;
	}
dbgmsg("<RecvWidgetData");
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

dbgmsg(">SendWidgetData");
	if		(  ( node = g_hash_table_lookup(ClassTable,
											(gconstpointer)((GtkTypeObject *)widget)->klass->type) )  !=  NULL  ) {
		sfunc = node->sfunc;
		ret = (*sfunc)(name,widget,fp);
	} else {
		ret = FALSE;
	}
dbgmsg("<SendWidgetData");
	return	(ret);
}

static	void
RecvValueSkip(
	NETFILE	*fp)
{
	PacketDataType	type;
	char			name[SIZE_BUFF];
	char			buff[SIZE_BUFF];
	int				count
	,				i;

	type = RecvDataType(fp);
	switch	(type) {
	  case	GL_TYPE_INT:
		(void)RecvInt(fp);
		break;
	  case	GL_TYPE_BOOL:
		(void)RecvBool(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_NUMBER:
		RecvString(fp,buff);
		break;
	  case	GL_TYPE_ARRAY:
		count = RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			RecvValueSkip(fp);
		}
		break;
	  case	GL_TYPE_RECORD:
		count = RecvInt(fp);
		for	(  i = 0 ; i < count ; i ++ ) {
			RecvString(fp,name);
			RecvValueSkip(fp);
		}
		break;
	  default:
		break;
	}
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

dbgmsg(">RecvValue");
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
					RecvValueSkip(fp);
				}
			}
		}
		if		(  Protocol2  ) {
			if		(  !fDone  ) {
				if		(  ( dataname = strchr(WidgetName,'.') )  !=  NULL  ) {
					dataname ++;
				}
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
		RecvValueSkip(fp);
	}
	if		(  fTrace  ) {
		type = RecvDataType(fp);
		switch	(type) {
		  case	GL_TYPE_RECORD:
			count = RecvInt(fp);
			for	(  i = 0 ; i < count ; i ++ ) {
				RecvString(fp,name);
				sprintf(longname,".%s",name);
				RecvValue(fp,longname + strlen(name) + 1);
			}
			break;
		  case	GL_TYPE_ARRAY:
			count = RecvInt(fp);
			for	(  i = 0 ; i < count ; i ++ ) {
				sprintf(name,"[%d]",i);
				sprintf(longname,"%s",name);
				RecvValue(fp,longname + strlen(name));
			}
			break;
		  default:
			RecvValueSkip(fp);
			break;
		}
	}
dbgmsg("<RecvValue");
}

static	gint
_GrabFocus(gpointer data)
{
	gtk_widget_grab_focus(data);
	return FALSE;
}

static	void
GrabFocus(GtkWidget *widget)
{
	gtk_idle_add(_GrabFocus, widget);
}

static	void
_ResetTimer(
	    GtkWidget	*widget,
	    gpointer	data)
{
	if (GTK_IS_CONTAINER (widget))
		gtk_container_forall (GTK_CONTAINER (widget), _ResetTimer, NULL);
	else if (GTK_IS_PANDA_TIMER (widget))
		gtk_panda_timer_reset (GTK_PANDA_TIMER (widget));
}

extern	void
ResetTimer(
	GtkWindow	*window)
{
	gtk_container_forall (GTK_CONTAINER (window), _ResetTimer, NULL);
}

extern	Bool
GetScreenData(
	NETFILE		*fp)
{
	char		window[SIZE_BUFF]
	,			widgetName[SIZE_BUFF];
	PacketClass	c;
	Bool		fCancel;
	int			type;
	XML_Node	*node;
	GtkWidget	*widget;

dbgmsg(">GetScreenData");
	fInRecv = TRUE; 
	CheckScreens(fp,FALSE);	 
	SendPacketClass(fp,GL_GetData);
	SendLong(fp,0);					/*	get all data	*/
	fCancel = FALSE;
	while	(  ( c = RecvPacketClass(fp) )  ==  GL_WindowName  ) {
		RecvString(fp,window);
		switch( type = RecvInt(fpComm) ) {
		  case	SCREEN_END_SESSION:
			ExitSystem();
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
			if		(  ( c = RecvPacketClass(fp) )  ==  GL_ScreenData  ) {
				RecvValue(fp,WidgetName + strlen(WidgetName));
			}
			break;
		  default:
			c = RecvPacketClass(fp);
			break;
		}
		if		(  c  ==  GL_NOT  ) {
			/*	no screen data	*/
		} else {
			/*	fatal error	*/
		}
	}
	if		(  c  ==  GL_FocusName  ) {
		RecvString(fp,window);
		RecvString(fp,widgetName);
		if		(	(  ( node = g_hash_table_lookup(WindowTable,window) )  !=  NULL  )
					&&	(  node->xml  !=  NULL  )
					&&	(  ( widget = glade_xml_get_widget(node->xml,widgetName) )
						   !=  NULL  ) ) {
			GrabFocus(widget);
		}
		c = RecvPacketClass(fp);
	}
	/* reset GtkPandaTimer if exists */
	if		(  ( node = g_hash_table_lookup(WindowTable,window) )  !=  NULL  ) {
		ResetTimer(node->window);
	}
	fInRecv = FALSE;
dbgmsg("<GetScreenData");
	return	(fCancel);
}

extern	Bool
SendConnect(
	NETFILE		*fp,
	char		*apl)
{
	Bool	rc;
	PacketClass	pc;

dbgmsg(">SendConnect");
	SendPacketClass(fp,GL_Connect);
	SendString(fp,VERSION);
	SendString(fp,User);
	SendString(fp,Pass);
	SendString(fp,apl);
	if		(  ( pc = RecvPacketClass(fp) )   ==  GL_OK  ) {
		rc = TRUE;
	} else {
		rc = FALSE;
		switch	(pc) {
		  case	GL_NOT:
			g_warning("can not connect server");
			break;
		  case	GL_E_VERSION:
			g_warning("can not connect server(version not match)");
			break;
		  case	GL_E_AUTH:
			g_warning("can not connect server(authentication error)");
			break;
		  case	GL_E_APPL:
			g_warning("can not connect server(application name invalid)");
			break;
		  default:
			printf("[%X]\n",pc);
			g_warning("can not connect server(other protocol error)");
			break;
		}
	}
dbgmsg("<SendConnect");
	return	(rc);
}

extern	void
SendEvent(
	NETFILE		*fp,
	char		*window,
	char		*widget,
	char		*event)
{
dbgmsg(">SendEvent");
	dbgprintf("window = [%s]",window); 
	dbgprintf("widget = [%s]",widget); 
	dbgprintf("event  = [%s]",event); 

	SendPacketClass(fp,GL_Event);
	SendString(fp,window);
	SendString(fp,widget);
	SendString(fp,event);
dbgmsg("<SendEvent");
}

static	guint
ClassHash(
	gconstpointer	key)
{
	return	((guint)key);
}

static	gint
ClassCompare(
	gconstpointer	s1,
	gconstpointer	s2)
{
	return	(gtk_type_is_a((GtkType)s1,(GtkType)s2));
}

extern	void
AddClass(
	GtkType	type,
	RecvHandler	rfunc,
	SendHandler	sfunc)
{
	HandlerNode	*node;

	if		(  g_hash_table_lookup(ClassTable,(gconstpointer)type)  ==  NULL  ) {
		node = New(HandlerNode);
		node->type = type;
		node->rfunc = rfunc;
		node->sfunc = sfunc;
		g_hash_table_insert(ClassTable,(gpointer)type,node);
	}
}

extern	void
InitProtocol(void)
{
dbgmsg(">InitProtocol");
	ClassTable = g_hash_table_new((GHashFunc)ClassHash,(GCompareFunc)ClassCompare);
	WindowTable = NewNameHash();
//	InitScreenStack();
	InitWidgetOperations();
dbgmsg("<InitProtocol");
}

static	void
_SendWindowData(
	char		*wname,
	XML_Node	*node,
	gpointer	user_data)
{
dbgmsg(">SendWindowData");
	SendPacketClass(fpComm,GL_WindowName);
	SendString(fpComm,wname);
	g_hash_table_foreach(node->UpdateWidget,(GHFunc)SendWidgetData,fpComm);
	SendPacketClass(fpComm,GL_END);
dbgmsg("<SendWindowData");
}

extern	void
SendWindowData(void)
{
dbgmsg(">SendWindowData");
	g_hash_table_foreach(WindowTable,(GHFunc)_SendWindowData,NULL);
	SendPacketClass(fpComm,GL_END);
	ClearWindowTable();
dbgmsg("<SendWindowData");
}
