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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include    <sys/types.h>
#include	<sys/stat.h>
#include    <unistd.h>
#include	<sys/time.h>
#ifdef ENABLE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#endif
#include	<glade/glade.h>
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif

#include	"types.h"
#include	"misc.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"styleParser.h"
#include	"comm.h"
#include	"protocol.h"
#include	"widgetOPS.h"
#include	"debug.h"

static	GHashTable		*ValueTable;

typedef	struct {
	char	*name;
	PacketDataType	type;
	char	*vname;
	void	*opt;
}	ValueAttribute;

static	void
FreeStrings(
	gpointer	data,
	gpointer	user_data)
{
	xfree(data);
}

static	void
FreeStringList(
	GList	*list)
{
	g_list_foreach(list,(GFunc)FreeStrings,NULL);
	g_list_free(list);
}

static	void
RegistValue(
	char	*vname,
	void	*opt)
{
	ValueAttribute	*p;

#ifdef	TRACE
	printf("Regist = [%s]\n",WidgetName);
#endif
	if		(  ( p = (ValueAttribute *)g_hash_table_lookup(ValueTable,WidgetName) )
			   ==  NULL  ) {
		p = New(ValueAttribute);
		p->name = StrDup(WidgetName);
		p->type = DataType;
		p->vname = StrDup(vname);
		g_hash_table_insert(ValueTable,p->name,p);
	} else {
		p->type = DataType;
		xfree(p->vname);
		p->vname = StrDup(vname);
	}
	p->opt = opt;
}

static	ValueAttribute	*
GetValue(
	char	*name)
{
	ValueAttribute	*p;

#ifdef	TRACE
	printf("Remain = [%s]\n",name);
#endif
	p = g_hash_table_lookup(ValueTable,name);
	return	(p);
}

static	void
SetState(
	GtkWidget	*widget,
	GtkStateType	state)
{
	if (state != GTK_STATE_INSENSITIVE)
		gtk_widget_set_sensitive(widget,TRUE);
	gtk_widget_set_state(widget,state);
}

static	Bool
RecvEntry(
	GtkWidget	*widget,
	FILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_NAME+1];
	int		nitem
	,		i;
	int		state;

dbgmsg(">RecvEntry");
	if		(  RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			RecvString(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				RecvStringData(fp,buff);
				RegistValue(name,NULL);
				gtk_entry_set_text(GTK_ENTRY(widget),buff);
			}
		}
	}
dbgmsg("<RecvEntry");
	return	(TRUE);
}


static	Bool
SendEntry(
	char	*name,
	GtkWidget	*widget,
	FILE	*fp)
{
	char	*p;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];

dbgmsg(">SendEntry");
	p = (char *)gtk_entry_get_text(GTK_ENTRY(widget));
	SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",name,v->vname);
	SendString(fp,iname);
	SendStringData(fp,v->type,(char *)p);
dbgmsg("<SendEntry");
	return	(TRUE);
}

#ifdef	USE_PANDA
static	Bool
RecvNumberEntry(
	GtkWidget	*widget,
	FILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_NAME+1];
	int		nitem
	,		i;
	int		state;
	Fixed	*xval;
	Numeric	value;

dbgmsg(">RecvNumberEntry");
	if		(  RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			RecvString(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				if		(  ( xval = (Fixed *)GetValue(name) )  !=  NULL  ) {
					xfree(xval->sval);
					xfree(xval);
				}
				RecvFixedData(fp,&xval);
				RegistValue(name,xval);
				value = FixedToNumeric(xval);
				gtk_number_entry_set_value(GTK_NUMBER_ENTRY(widget),value);
				NumericFree(value);
			}
		}
	}
dbgmsg("<RecvNumberEntry");
	return	(TRUE);
}


static	Bool
SendNumberEntry(
	char	*name,
	GtkWidget	*widget,
	FILE	*fp)
{
	Numeric	value;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];
	Fixed	*xval;

dbgmsg(">SendNumberEntry");
	value = gtk_number_entry_get_value(GTK_NUMBER_ENTRY(widget));
	SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",name,v->vname);
	SendString(fp,iname);
	xval = (Fixed *)v->opt;
	xval->sval = NumericToFixed(value,xval->flen,xval->slen);
	NumericFree(value);
	SendFixedData(fp,v->type,xval);
dbgmsg("<SendNumberEntry");
	return	(TRUE);
}
#endif

static	Bool
RecvLabel(
	GtkWidget	*widget,
	FILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_NAME+1];
	int		nitem
	,		i;

dbgmsg(">RecvLabel");
	if		(  RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			RecvString(fp,name);
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				RecvStringData(fp,buff);
				RegistValue(name,NULL);
				gtk_label_set(GTK_LABEL(widget),buff);
			}
		}
	}
dbgmsg("<RecvLabel");
	return	(TRUE);
}

static	Bool
SendText(
	char	*name,
	GtkWidget	*widget,
	FILE	*fp)
{
	char	*p;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];

dbgmsg(">SendText");
	p = gtk_editable_get_chars(GTK_EDITABLE(widget),0,-1); 
	SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",name,v->vname);
	SendString(fp,iname);
	SendStringData(fp,v->type,(char *)p);
	g_free(p);
dbgmsg("<SendText");
	return	(TRUE);
}

static	Bool
RecvText(
	GtkWidget	*widget,
	FILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_NAME+1];
	int		nitem
	,		i;
	int		state;

dbgmsg(">RecvText");
	if		(  RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			RecvString(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				RecvStringData(fp,buff);
				RegistValue(name,NULL);
				gtk_text_freeze(GTK_TEXT(widget));
				gtk_text_set_point(GTK_TEXT(widget), 0);
				gtk_text_forward_delete(GTK_TEXT(widget),
										gtk_text_get_length((GTK_TEXT(widget))));
				gtk_text_insert(GTK_TEXT(widget),NULL,NULL,NULL,buff,strlen(buff));
				gtk_text_thaw(GTK_TEXT(widget));
			}
		}
	}
dbgmsg("<RecvText");
	return	(TRUE);
}

static	Bool
SendButton(
	char	*name,
	GtkWidget	*widget,
	FILE	*fp)
{
	Bool	fActive;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];

dbgmsg(">SendButton");
	fActive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",name,v->vname);
	SendString(fp,iname);
	SendBoolData(fp,v->type,fActive);
dbgmsg("<SendButton");
	return	(TRUE);
}

static	void
SetLabel(
	GtkWidget	*widget,
	char		*label)
{
	if		(  GTK_IS_LABEL(widget)  ) {
		gtk_label_set(GTK_LABEL(widget),label);
	} else {
		gtk_container_foreach(GTK_CONTAINER(widget),(GtkCallback)SetLabel,label);
	}
}

static	void
SetStyle(
	GtkWidget	*widget,
	GtkStyle	*style)
{
	if		(  GTK_IS_LABEL(widget)  ) {
		gtk_widget_set_style(widget,style);
	} else {
		gtk_container_foreach(GTK_CONTAINER(widget),(GtkCallback)SetStyle,style);
	}
}

static	Bool
RecvButton(
	GtkWidget	*widget,
	FILE	*fp)
{
	Bool	fActive;
	char	name[SIZE_NAME+1]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;

dbgmsg(">RecvButton");
	if		(  RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			RecvString(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff);
				SetStyle(widget,GetStyle(buff));
			} else
			if		(  !stricmp(name,"label")  ) {
				RecvStringData(fp,buff);
				SetLabel(widget,buff);
			} else {
				RecvBoolData(fp,&fActive);
				RegistValue(name,NULL);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), fActive);
			}
		}
	}
dbgmsg("<RecvButton");
	return	(TRUE);
}

static	Bool
RecvCombo(
	GtkWidget	*widget,
	FILE	*fp)
{
	char	name[SIZE_NAME+1]
	,		buff[SIZE_BUFF]
	,		*longname;
	int		count
	,		nitem
	,		num
	,		i
	,		j;
	GtkWidget	*subWidget;
	GList		*list;
	int		state;
	GtkCombo	*combo = GTK_COMBO(widget);

dbgmsg(">RecvCombo");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	longname = WidgetName + strlen(WidgetName);
	nitem = RecvInt(fp);
	count = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		RecvString(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"item")  ) {
			list = g_list_append(NULL,StrDup(""));
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = RecvInt(fp);
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvStringData(fp,buff)  ) {
					if		(  j  <  count  ) {
						list = g_list_append(list,StrDup(buff));
					}
				}
			}
			gtk_signal_handler_block (GTK_OBJECT (combo->list), combo->list_change_id);
			gtk_combo_set_popdown_strings(combo,list);
			FreeStringList(list);
			gtk_signal_handler_unblock (GTK_OBJECT (combo->list), combo->list_change_id);
		} else {
			sprintf(longname,".%s",name);
			if		(  ( subWidget = glade_xml_get_widget_by_long_name(ThisXML,WidgetName) )
			   !=  NULL  ) {
				RecvEntry(subWidget,fp);
			} else {
				printf("sub widget not found\n");
				/*	fatal error	*/
			}
		}
	}
dbgmsg("<RecvCombo");
	return	(TRUE);
}

#ifdef	USE_PANDA
static	Bool
RecvPandaCombo(
	GtkWidget	*widget,
	FILE	*fp)
{
	char	name[SIZE_NAME+1]
	,		buff[SIZE_BUFF]
	,		*longname;
	int		count
	,		nitem
	,		num
	,		i
	,		j;
	GtkWidget	*subWidget;
	GList		*list;
	int		state;
	GtkPandaCombo	*combo = GTK_PANDA_COMBO(widget);

dbgmsg(">RecvPandaCombo");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	longname = WidgetName + strlen(WidgetName);
	nitem = RecvInt(fp);
	count = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		printf("%s\n", name);
		RecvString(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"item")  ) {
			list = g_list_append(NULL,StrDup(""));
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = RecvInt(fp);
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvStringData(fp,buff)  ) {
					if		(  j  <  count  ) {
						list = g_list_append(list,StrDup(buff));
					}
				}
			}
			gtk_signal_handler_block (GTK_OBJECT (combo->list), combo->list_change_id);
			gtkpanda_combo_set_popdown_strings(combo,list);
			FreeStringList(list);
			gtk_signal_handler_unblock (GTK_OBJECT (combo->list), combo->list_change_id);
			} else {
				sprintf(longname,".%s",name);
				if		(  ( subWidget = glade_xml_get_widget_by_long_name(ThisXML,WidgetName) )
						   !=  NULL  ) {
					RecvEntry(subWidget,fp);
				} else {
					printf("sub widget not found\n");
					/*	fatal error	*/
				}
			}
	}
dbgmsg("<RecvPandaCombo");
	return	(TRUE);
}

static	Bool
SendPandaCList(
	char	*name,
	GtkWidget	*widget,
	FILE	*fp)
{
	GList			*children;
	GtkStateType	state;
	GtkPandaCListRow	*clist_row;
	int				i;
	char			iname[SIZE_BUFF];
	ValueAttribute	*v;

dbgmsg(">SendCList");
	v = GetValue(name);
	for	( children = GTK_PANDA_CLIST(widget)->row_list , i = 0 ;
		  children  !=  NULL ; children = children->next , i ++ ) {
		if		(  ( clist_row = GTK_PANDA_CLIST_ROW(children) )  !=  NULL  ) {
			state = clist_row->state;
			sprintf(iname,"%s.%s[%d]",name,v->vname,i + (int)v->opt);
			SendPacketClass(fp,GL_ScreenData);
			SendString(fp,iname);
			SendDataType(fp,GL_TYPE_BOOL);
			if		(  state  ==  GTK_STATE_SELECTED  ) {
				SendBool(fp,TRUE);
			} else {
				SendBool(fp,FALSE);
			}
		}
	}
dbgmsg("<SendPandaCList");
	return	(TRUE);
}

static	Bool
RecvPandaCList(
	GtkWidget	*widget,
	FILE		*fp)
{
	char	name[SIZE_NAME+1]
	,		iname[SIZE_NAME+1]
	,		label[SIZE_BUFF+1]
	,		buff[SIZE_BUFF]
	,		*longname;
	int		count
	,		nitem
	,		num
	,		rnum
	,		from
	,		i
	,		j
	,		k;
	GtkWidget	*subWidget;
	char	**rdata;
	Bool	fActive;
	int		state;

dbgmsg(">RecvPandaCList");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	strcpy(label,WidgetName);
	longname = label + strlen(label);
	nitem = RecvInt(fp);
	count = -1;
	rdata = NULL;
	from = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		RecvString(fp,name);
		sprintf(longname,".%s",name);
		if		(  ( subWidget = glade_xml_get_widget_by_long_name(ThisXML,label) )
				   !=  NULL  ) {
			RecvWidgetData(subWidget,fp);
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"from")  ) {
			RecvIntegerData(fp,&from);
		} else
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"row")  ) {
			/*	NOP	*/
		} else
		if		(  !stricmp(name,"column")  ) {
			/*	NOP	*/
		} else
		if		(  !stricmp(name,"item")  ) {
			gtkpanda_clist_freeze(GTK_PANDA_CLIST(widget));
			gtkpanda_clist_clear(GTK_PANDA_CLIST(widget));
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
				rnum = RecvInt(fp);
				if		(  rdata  ==  NULL  ) {
					rdata = (char **)xmalloc(sizeof(char *)*rnum);
				}
				for	( k = 0 ; k < rnum ; k ++ ) {
					RecvString(fp,iname);
					(void)RecvStringData(fp,buff);
					rdata[k] = StrDup(buff);
				}
				if		(	(  j             >=  from   )
						&&	(  ( j - from )  <   count  ) ) {
					gtkpanda_clist_append(GTK_PANDA_CLIST(widget), rdata);
				}
				for	( k = 0 ; k < rnum ; k ++ ) {
					xfree(rdata[k]);
				}
			}
			xfree(rdata);
			gtkpanda_clist_thaw(GTK_PANDA_CLIST(widget));
		} else {
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			RegistValue(name,(void*)from);
			num = RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvBoolData(fp,&fActive)  ) {
					if		(	(  j             >=  from   )
							&&	(  ( j - from )  <   count  ) ) {
						if		(  fActive  ) {
							gtkpanda_clist_select_row(GTK_PANDA_CLIST(widget),( j - from ),0);
						} else {
							gtkpanda_clist_unselect_row(GTK_PANDA_CLIST(widget),( j - from ),0);
						}
					}
				}
			}
		}
	}
dbgmsg("<RecvPandaCList");
	return	(TRUE);
}
#endif

static	Bool
SendCList(
	char	*name,
	GtkWidget	*widget,
	FILE	*fp)
{
	GList			*children;
	GtkStateType	state;
	GtkCListRow		*clist_row;
	int				i;
	char			iname[SIZE_BUFF];
	ValueAttribute	*v;

dbgmsg(">SendCList");
	v = GetValue(name);
	for	( children = GTK_CLIST(widget)->row_list , i = 0 ;
		  children  !=  NULL ; children = children->next , i ++ ) {
		if		(  ( clist_row = GTK_CLIST_ROW(children) )  !=  NULL  ) {
			state = clist_row->state;
			sprintf(iname,"%s.%s[%d]",name,v->vname,i + (int)v->opt);
			SendPacketClass(fp,GL_ScreenData);
			SendString(fp,iname);
			SendDataType(fp,GL_TYPE_BOOL);
			if		(  state  ==  GTK_STATE_SELECTED  ) {
				SendBool(fp,TRUE);
			} else {
				SendBool(fp,FALSE);
			}
		}
	}
dbgmsg("<SendCList");
	return	(TRUE);
}

static	Bool
RecvCList(
	GtkWidget	*widget,
	FILE		*fp)
{
	char	name[SIZE_NAME+1]
	,		iname[SIZE_NAME+1]
	,		label[SIZE_BUFF+1]
	,		buff[SIZE_BUFF]
	,		*longname;
	int		count
	,		nitem
	,		num
	,		rnum
	,		from
	,		i
	,		j
	,		k;
	GtkWidget	*subWidget;
	char	**rdata;
	Bool	fActive;
	int		state;

dbgmsg(">RecvCList");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	strcpy(label,WidgetName);
	longname = label + strlen(label);
	nitem = RecvInt(fp);
	count = -1;
	rdata = NULL;
	from = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		RecvString(fp,name);
		sprintf(longname,".%s",name);
		if		(  ( subWidget = glade_xml_get_widget_by_long_name(ThisXML,label) )
			   !=  NULL  ) {
			RecvWidgetData(subWidget,fp);
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"from")  ) {
			RecvIntegerData(fp,&from);
		} else
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"row")  ) {
			/*	NOP	*/
		} else
		if		(  !stricmp(name,"column")  ) {
			/*	NOP	*/
		} else
		if		(  !stricmp(name,"item")  ) {
			gtk_clist_freeze(GTK_CLIST(widget));
			gtk_clist_clear(GTK_CLIST(widget));
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
				rnum = RecvInt(fp);
				if		(  rdata  ==  NULL  ) {
					rdata = (char **)xmalloc(sizeof(char *)*rnum);
				}
				for	( k = 0 ; k < rnum ; k ++ ) {
					RecvString(fp,iname);
					(void)RecvStringData(fp,buff);
					rdata[k] = StrDup(buff);
				}
				if		(	(  j             >=  from   )
						&&	(  ( j - from )  <   count  ) ) {
					gtk_clist_append(GTK_CLIST(widget), rdata);
				}
				for	( k = 0 ; k < rnum ; k ++ ) {
					xfree(rdata[k]);
				}
			}
			xfree(rdata);
			gtk_clist_thaw(GTK_CLIST(widget));
		} else {
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			RegistValue(name,(void*)from);
			num = RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvBoolData(fp,&fActive)  ) {
					if		(	(  j             >=  from   )
							&&	(  ( j - from )  <   count  ) ) {
						if		(  fActive  ) {
							gtk_clist_select_row(GTK_CLIST(widget),( j - from ),0);
						} else {
							gtk_clist_unselect_row(GTK_CLIST(widget),( j - from ),0);
						}
					}
				}
			}
		}
	}
dbgmsg("<RecvCList");
	return	(TRUE);
}


static	Bool
SendList(
	char	*name,
	GtkWidget	*widget,
	FILE	*fp)
{
	GtkWidget		*child;
	GList			*children;
	GtkStateType	state;
	int				i;
	char			iname[SIZE_BUFF];
	ValueAttribute	*v;

dbgmsg(">SendList");
	v = GetValue(name);
	for	( children = GTK_LIST(widget)->children , i = 0 ;
		  children  !=  NULL ; children = children->next , i ++ ) {
		child = children->data;
		state = GTK_WIDGET_STATE(child);
		sprintf(iname,"%s.%s[%d]",name,v->vname,i + (int)v->opt);
		SendPacketClass(fp,GL_ScreenData);
		SendString(fp,iname);
		SendDataType(fp,GL_TYPE_BOOL);
		if		(  state  ==  GTK_STATE_SELECTED  ) {
			SendBool(fp,TRUE);
		} else {
			SendBool(fp,FALSE);
		}
	}
dbgmsg("<SendList");
	return	(TRUE);
}

static	Bool
RecvList(
	GtkWidget	*widget,
	FILE	*fp)
{
	char	name[SIZE_NAME+1]
	,		buff[SIZE_BUFF]
	,		*longname;
	int		count
	,		nitem
	,		num
	,		from
	,		i
	,		j;
	GtkWidget	*item;
	Bool	fActive;
	int		state;
	//gfloat	pos;

dbgmsg(">RecvList");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	longname = WidgetName + strlen(WidgetName);
	nitem = RecvInt(fp);
	count = -1;
	from = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		RecvString(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"from")  ) {
			RecvIntegerData(fp,&from);
		} else
		if		(  !stricmp(name,"item")  ) {
			gtk_list_clear_items(GTK_LIST(widget),0,-1);
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvStringData(fp,buff)  ) {
					if		(	(  j             >=  from   )
							&&	(  ( j - from )  <   count  ) ) {
						item = gtk_list_item_new_with_label(buff);
						gtk_widget_show(item);
						gtk_container_add(GTK_CONTAINER(widget),item);
					}
				}
			}
		} else {
			DataType = RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			RegistValue(name,(void *)from);
			num = RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvBoolData(fp,&fActive)  ) {
					if		(	(  j             >=  from   )
							&&	(  ( j - from )  <   count  ) ) {
						if		(  fActive  ) {
							gtk_list_select_item(GTK_LIST(widget),j);
						} else {
							gtk_list_unselect_item(GTK_LIST(widget),j);
						}
					}
				}
			}
		}
	}
#if	0
	pos = (gfloat)from / (gfloat)count;
printf("pos = %f\n",(double)pos);
	gtk_list_scroll_vertical(GTK_LIST(widget),GTK_SCROLL_JUMP,pos);
#endif
dbgmsg("<RecvList");
	return	(TRUE);
}

#include	<gtk/gtkcalendar.h>
static	Bool
SendCalendar(
	char		*name,
	GtkWidget	*calendar,
	FILE		*fp)
{
	char	iname[SIZE_BUFF];
	int		year
	,		month
	,		day;

dbgmsg(">SendCaleandar");
	gtk_calendar_get_date(GTK_CALENDAR(calendar),&year,&month,&day);

	SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.year",name);
	SendString(fp,(char *)iname);
	SendDataType(fp,GL_TYPE_INT);
	SendInt(fp,year);

	SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.month",name);
	SendString(fp,(char *)iname);
	SendDataType(fp,GL_TYPE_INT);
	SendInt(fp,month+1);

	SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.day",name);
	SendString(fp,(char *)iname);
	SendDataType(fp,GL_TYPE_INT);
	SendInt(fp,day);
dbgmsg("<SendCaleandar");
	return	(TRUE);
}

static	Bool
RecvCalendar(
	GtkWidget	*widget,
	FILE		*fp)
{
	char	name[SIZE_NAME+1]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		year
	,		month
	,		day;
	int		state;

dbgmsg(">RecvCaleandar");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	RegistValue("",NULL);
	nitem = RecvInt(fp);
	for	( i = 0 ; i < nitem ; i ++ ) {
		RecvString(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"year")  ) {
			RecvIntegerData(fp,&year);
		} else
		if		(  !stricmp(name,"month")  ) {
			RecvIntegerData(fp,&month);
		} else
		if		(  !stricmp(name,"day")  ) {
			RecvIntegerData(fp,&day);
		} else {
			/*	fatal error	*/
		}
	}
	gtk_calendar_select_month(GTK_CALENDAR(widget),month-1,year);
	gtk_calendar_select_day(GTK_CALENDAR(widget),day);
dbgmsg("<RecvCaleandar");
	return	(TRUE); 
}

static	Bool
SendNotebook(
	char		*name,
	GtkWidget	*notebook,
	FILE		*fp)
{
	char	iname[SIZE_BUFF];
	ValueAttribute	*v;
	int		page;

dbgmsg(">SendNotebook");
	v = GetValue(name);
	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s",name,v->vname);
#ifdef	TRACE
	printf("iname = [%s] value = %d\n",iname,page);
#endif
	SendString(fp,iname);
	SendIntegerData(fp,v->type,page);
dbgmsg("<SendNotebook");
	return	(TRUE);
}

static	Bool
RecvNotebook(
	GtkWidget	*widget,
	FILE		*fp)
{
	char	name[SIZE_NAME+1]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		page;
	int		state;
	char	*longname;

dbgmsg(">RecvNotebook");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	nitem = RecvInt(fp);
	longname = WidgetName + strlen(WidgetName);
	for	( i = 0 ; i < nitem ; i ++ ) {
		RecvString(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"pageno")  ) {
			RecvIntegerData(fp,&page);
			RegistValue(name,NULL);
		} else {
			sprintf(longname,".%s",name);
			RecvValue(fp,longname + strlen(name) + 1);
		}
	}
	gtk_notebook_set_page(GTK_NOTEBOOK(widget),page);
dbgmsg("<RecvNotebook");
	return	(TRUE);
}

static	Bool
SendProgressBar(
	char		*name,
	GtkWidget	*progress,
	FILE		*fp)
{
	char	iname[SIZE_BUFF];
	ValueAttribute	*v;
	int		value;

dbgmsg(">SendProgress");
	v = GetValue(name);
	value = gtk_progress_get_value(GTK_PROGRESS(progress));

	SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s",name,v->vname);
#ifdef	TRACE
	printf("iname = [%s] value = %d\n",iname,value);
#endif
	SendString(fp,iname);
	SendIntegerData(fp,v->type,value);
dbgmsg("<SendProgress");
	return	(TRUE);
}

static	Bool
RecvProgressBar(
	GtkWidget	*widget,
	FILE		*fp)
{
	char	name[SIZE_NAME+1]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		value;
	int		state;
	char	*longname;

dbgmsg(">RecvProgress");
	DataType = RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	nitem = RecvInt(fp);
	longname = WidgetName + strlen(WidgetName);
	for	( i = 0 ; i < nitem ; i ++ ) {
		RecvString(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"value")  ) {
			RecvIntegerData(fp,&value);
			RegistValue(name,NULL);
			gtk_progress_set_value(GTK_PROGRESS(widget),value);
		}
	}
dbgmsg("<RecvProgress");
	return	(TRUE);
}

extern	void
InitWidgetOperations(void)
{
	ValueTable = NewNameHash();

	AddClass(GTK_TYPE_ENTRY,RecvEntry,SendEntry);
#ifdef	USE_PANDA
#ifdef	GTK_TYPE_NUMBER_ENTRY
	AddClass(GTK_TYPE_NUMBER_ENTRY,RecvNumberEntry,SendNumberEntry);
#endif
#ifdef	GTK_PANDA_TYPE_COMBO
	AddClass(GTK_PANDA_TYPE_COMBO,RecvPandaCombo,NULL);
#endif
#ifdef	GTK_PANDA_TYPE_CLIST
	AddClass(GTK_PANDA_TYPE_CLIST,RecvPandaCList,SendPandaCList);
#endif
#ifdef	GTK_PANDA_TYPE_ENTRY
	AddClass(GTK_PANDA_TYPE_ENTRY,RecvEntry,SendEntry);
#endif
#endif
	AddClass(GTK_TYPE_TEXT,RecvText,SendText);
	AddClass(GTK_TYPE_LABEL,RecvLabel,NULL);
	AddClass(gtk_combo_get_type (),RecvCombo,NULL);
	AddClass(GTK_TYPE_CLIST,RecvCList,SendCList);
	AddClass(GTK_TYPE_BUTTON,RecvButton,NULL);
	AddClass(GTK_TYPE_TOGGLE_BUTTON,RecvButton,SendButton);
	AddClass(GTK_TYPE_CHECK_BUTTON,RecvButton,SendButton);
	AddClass(GTK_TYPE_RADIO_BUTTON,RecvButton,SendButton);
	AddClass(GTK_TYPE_LIST,RecvList,SendList);
	AddClass(GTK_TYPE_CALENDAR,RecvCalendar,SendCalendar);
	AddClass(GTK_TYPE_NOTEBOOK,RecvNotebook,SendNotebook);
	AddClass(GTK_TYPE_PROGRESS_BAR,RecvProgressBar,SendProgressBar);
}
