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
#include	"glterm.h"
#include	"glclient.h"
#include	"styleParser.h"
#include	"net.h"
#include	"comm.h"
#include	"protocol.h"
#include	"widgetOPS.h"
#include	"debug.h"

static	GHashTable		*ValueTable;

typedef	struct {
	char	*key;
	PacketDataType	type;
	char	*NameSuffix;
	char	*ValueName;
	int		optype;
#define	OPT_TYPE_NULL	0
#define	OPT_TYPE_FIXED	1
#define	OPT_TYPE_INT	2
	union	{
		Fixed	*xval;
		int		ival;
	}	opt;
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
	GtkWidget	*widget,
	char	*vname,
	int		optype,
	void	*opt)
{
	ValueAttribute	*p;
	const	char	*rname;

	rname = glade_get_widget_long_name(widget);
#ifdef	TRACE
	printf("Regist = [%s:%s]\n",WidgetName,rname);
#endif
	if		(  ( p = (ValueAttribute *)g_hash_table_lookup(ValueTable,rname) )
			   ==  NULL  ) {
		p = New(ValueAttribute);
		p->key = StrDup((char *)rname);
		p->NameSuffix = StrDup(vname);
		p->ValueName = StrDup(WidgetName);
		g_hash_table_insert(ValueTable,p->key,p);
	} else {
		if		(  p->optype  ==  OPT_TYPE_FIXED  ) {
			xfree(p->opt.xval->sval);
			xfree(p->opt.xval);
		}
		xfree(p->NameSuffix);
		p->NameSuffix = StrDup(vname);
	}
	p->type = DataType;
	p->optype = optype;
	switch	(optype) {
	  case	OPT_TYPE_FIXED:
		p->opt.xval = (Fixed *)opt;
		break;
	  case	OPT_TYPE_INT:
		p->opt.ival = (int)opt;
		break;
	  default:
		break;
	}
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

static	void
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
		g_hash_table_insert(ClassTable,(gpointer)node->type,node);
	}
}

//////////////////////////////////////////////////////////////////////
static	Bool
RecvEntry(
	GtkWidget	*widget,
	NETFILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;

dbgmsg(">RecvEntry");
	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				RecvStringData(fp,buff,SIZE_BUFF);
				RegistValue(widget,name,OPT_TYPE_NULL,NULL);
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
	NETFILE	*fp)
{
	char	*p;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];

dbgmsg(">SendEntry");
	p = (char *)gtk_entry_get_text(GTK_ENTRY(widget));
	GL_SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
	GL_SendName(fp,iname);
	SendStringData(fp,v->type,(char *)p);
dbgmsg("<SendEntry");
	return	(TRUE);
}

#ifdef	USE_PANDA
static	Bool
RecvPS(
	GtkWidget	*widget,
	NETFILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_BUFF];
	int		nitem
	,		i;

dbgmsg(">RecvPS");
	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp,name);
			RecvStringData(fp,buff,SIZE_BUFF);
			RegistValue(widget,name,OPT_TYPE_NULL,NULL);
			gtk_panda_ps_load(GTK_PANDA_PS(widget),buff);
		}
	}
dbgmsg("<RecvPS");
	return	(TRUE);
}


static	Bool
SendPS(
	char	*name,
	GtkWidget	*widget,
	NETFILE	*fp)
{
dbgmsg(">SendPS");
dbgmsg("<SendPS");
	return	(TRUE);
}
#endif

#ifdef	USE_PANDA
static	Bool
RecvTimer(
	GtkWidget	*widget,
	NETFILE	*fp)
{
	char		name[SIZE_BUFF];
	int		duration;
	int		nitem
	,		i;

dbgmsg(">RecvTimer");
	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp,name);
			if		(  !stricmp(name,"duration")  ) {
				RecvIntegerData(fp,&duration);
				gtk_panda_timer_set(GTK_PANDA_TIMER(widget),
						    duration);
			}
		}
	}
dbgmsg("<RecvTimer");
	return	(TRUE);
}


static	Bool
SendTimer(
	char	*name,
	GtkWidget	*widget,
	NETFILE	*fp)
{
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];
	Fixed	*xval;

dbgmsg(">SendTimer");
	GL_SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
	GL_SendName(fp,iname);
	SendIntegerData(fp,v->type,GTK_PANDA_TIMER(widget)->duration / 1000);
dbgmsg("<SendTimer");
	return	(TRUE);
}
#endif

#ifdef	USE_PANDA
static	Bool
RecvNumberEntry(
	GtkWidget	*widget,
	NETFILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;
	Fixed	*xval;
	Numeric	value;

ENTER_FUNC;
	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				sprintf(buff,"%s.%s",WidgetName,name); 
				if		(  ( xval = (Fixed *)GetValue(buff) )  !=  NULL  ) {
					xfree(xval->sval);
					xfree(xval);
				}
				RecvFixedData(fp,&xval);
				RegistValue(widget,name,OPT_TYPE_FIXED,xval);
				value = FixedToNumeric(xval);
				gtk_number_entry_set_value(GTK_NUMBER_ENTRY(widget),value);
				NumericFree(value);
			}
		}
	}
LEAVE_FUNC;
	return	(TRUE);
}


static	Bool
SendNumberEntry(
	char	*name,
	GtkWidget	*widget,
	NETFILE	*fp)
{
	Numeric	value;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];
	Fixed	*xval;

dbgmsg(">SendNumberEntry");
	value = gtk_number_entry_get_value(GTK_NUMBER_ENTRY(widget));
	GL_SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
	GL_SendName(fp,iname);
	xval = v->opt.xval;
	xfree(xval->sval);
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
	NETFILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_BUFF];
	int		nitem
	,		i;

dbgmsg(">RecvLabel");
	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp,name);
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				RecvStringData(fp,buff,SIZE_BUFF);
				RegistValue(widget,name,OPT_TYPE_NULL,NULL);
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
	NETFILE	*fp)
{
	char	*p;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];

dbgmsg(">SendText");
	p = gtk_editable_get_chars(GTK_EDITABLE(widget),0,-1); 
	GL_SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
	GL_SendName(fp,iname);
	SendStringData(fp,v->type,(char *)p);
	g_free(p);
dbgmsg("<SendText");
	return	(TRUE);
}

static	Bool
RecvText(
	GtkWidget	*widget,
	NETFILE	*fp)
{
	char	buff[SIZE_BUFF]
	,		name[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;
	MonObjectType	obj;

dbgmsg(">RecvText");
	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				gtk_widget_set_style(widget,GetStyle(buff));
			} else {
				RecvStringData(fp,buff,SIZE_BUFF);
				RegistValue(widget,name,OPT_TYPE_NULL,NULL);
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
	NETFILE	*fp)
{
	Bool	fActive;
	ValueAttribute	*v;
	char	iname[SIZE_BUFF];

dbgmsg(">SendButton");
	fActive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	GL_SendPacketClass(fp,GL_ScreenData);
	v = GetValue(name);
	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
	GL_SendName(fp,iname);
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
	NETFILE	*fp)
{
	Bool	fActive;
	char	name[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;

dbgmsg(">RecvButton");
	if		(  GL_RecvDataType(fp)  ==  GL_TYPE_RECORD  ) {
		nitem = GL_RecvInt(fp);
		for	( i = 0 ; i < nitem ; i ++ ) {
			GL_RecvName(fp,name);
			if		(  !stricmp(name,"state")  ) {
				RecvIntegerData(fp,&state);
				SetState(widget,(GtkStateType)state);
			} else
			if		(  !stricmp(name,"style")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				SetStyle(widget,GetStyle(buff));
			} else
			if		(  !stricmp(name,"label")  ) {
				RecvStringData(fp,buff,SIZE_BUFF);
				SetLabel(widget,buff);
			} else {
				RecvBoolData(fp,&fActive);
				RegistValue(widget,name,OPT_TYPE_NULL,NULL);
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
	NETFILE	*fp)
{
	char	name[SIZE_BUFF]
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
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	longname = WidgetName + strlen(WidgetName);
	nitem = GL_RecvInt(fp);
	count = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"item")  ) {
			list = g_list_append(NULL,StrDup(""));
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = GL_RecvInt(fp);
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvStringData(fp,buff,SIZE_BUFF)  !=  NULL  ) {
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
				printf("sub widget not found: %s\n", longname);
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
	NETFILE	*fp)
{
	char	name[SIZE_BUFF]
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
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	longname = WidgetName + strlen(WidgetName);
	nitem = GL_RecvInt(fp);
	count = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"item")  ) {
			list = g_list_append(NULL,StrDup(""));
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = GL_RecvInt(fp);
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvStringData(fp,buff,SIZE_BUFF)  !=  NULL  ) {
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
	NETFILE	*fp)
{
	GList			*children;
	GtkStateType	state;
	GtkPandaCListRow	*clist_row;
	int				i;
	char			iname[SIZE_BUFF];
	ValueAttribute	*v;
	GtkVisibility	visi;
	gboolean	fVisibleRow;

dbgmsg(">SendPandaCList");
	v = GetValue(name);
	fVisibleRow = FALSE;
	for	( children = GTK_PANDA_CLIST(widget)->row_list , i = 0 ;
		  children  !=  NULL ; children = children->next , i ++ ) {
		if		(  ( clist_row = GTK_PANDA_CLIST_ROW(children) )  !=  NULL  ) {
			state = clist_row->state;
			sprintf(iname,"%s.%s[%d]",v->ValueName,v->NameSuffix,i + v->opt.ival);
			GL_SendPacketClass(fp,GL_ScreenData);
			GL_SendName(fp,iname);
			SendBoolData(fp,GL_TYPE_BOOL,((state == GTK_STATE_SELECTED) ? TRUE : FALSE));
		}
		if	( !fVisibleRow ) {
			if	( visi = gtkpanda_clist_row_is_visible(GTK_PANDA_CLIST(widget),i) == GTK_VISIBILITY_FULL ) {
				sprintf(iname,"%s.row",v->ValueName);
				GL_SendPacketClass(fp,GL_ScreenData);
				GL_SendName(fp,iname);
				SendIntegerData(fp,GL_TYPE_INT,(i + 1));
				fVisibleRow = TRUE;
			}
		} 
	}
dbgmsg("<SendPandaCList");
	return	(TRUE);
}

static	Bool
RecvPandaCList(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	char	name[SIZE_BUFF]
	,		iname[SIZE_BUFF]
	,		label[SIZE_BUFF]
	,		buff[SIZE_BUFF]
	,		*longname;
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
	GtkWidget	*subWidget;
	char	**rdata;
	Bool	fActive;
	int		state;
	gfloat		rowattrw;

dbgmsg(">RecvPandaCList");
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	strcpy(label,WidgetName);
	longname = label + strlen(label);
	nitem = GL_RecvInt(fp);
	count = -1;
	rdata = NULL;
	from = 0;
	row = 0;
	rowattrw = 0.0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
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
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"row")  ) {
			RecvIntegerData(fp,&row);
		} else
		if		(  !stricmp(name,"rowattr")  ) {
			RecvIntegerData(fp,&rowattr);
			switch	(rowattr) {
			  case	1: /* DOWN */
				rowattrw = 1.0;
				break;
			  case	2: /* MIDDLE */
				rowattrw = 0.5;
				break;
			  default: /* [0] TOP */
				break;
			}
		} else
		if		(  !stricmp(name,"column")  ) {
			RecvIntegerData(fp,&column);
		} else
		if		(  !stricmp(name,"item")  ) {
			gtkpanda_clist_freeze(GTK_PANDA_CLIST(widget));
			gtkpanda_clist_clear(GTK_PANDA_CLIST(widget));
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = GL_RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
				rnum = GL_RecvInt(fp);
				if		(  rdata  ==  NULL  ) {
					rdata = (char **)xmalloc(sizeof(char *)*rnum);
				}
				for	( k = 0 ; k < rnum ; k ++ ) {
					GL_RecvName(fp,iname);
					(void)RecvStringData(fp,buff,SIZE_BUFF);
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
			gtkpanda_clist_moveto(GTK_PANDA_CLIST(widget), row - 1, 0, rowattrw, 0.0);
		} else {
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			RegistValue(widget,name,OPT_TYPE_INT,(void*)from);
			num = GL_RecvInt(fp);
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
	fInRecv = FALSE;
	UpdateWidget((GtkWidget *)widget,NULL);
	fInRecv = TRUE;
dbgmsg("<RecvPandaCList");
	return	(TRUE);
}
#endif

static	Bool
SendCList(
	char	*name,
	GtkWidget	*widget,
	NETFILE	*fp)
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
			sprintf(iname,"%s.%s[%d]",v->ValueName,v->NameSuffix,i + v->opt.ival);
			GL_SendPacketClass(fp,GL_ScreenData);
			GL_SendName(fp,iname);
			SendBoolData(fp,GL_TYPE_BOOL,((state == GTK_STATE_SELECTED) ? TRUE : FALSE));
		}
	}
dbgmsg("<SendCList");
	return	(TRUE);
}

static	Bool
RecvCList(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	char	name[SIZE_BUFF]
	,		iname[SIZE_BUFF]
	,		label[SIZE_BUFF]
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
	int		row
		,	column;

dbgmsg(">RecvCList");
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	strcpy(label,WidgetName);
	longname = label + strlen(label);
	nitem = GL_RecvInt(fp);
	count = -1;
	rdata = NULL;
	from = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
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
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"row")  ) {
			RecvIntegerData(fp,&row);
		} else
		if		(  !stricmp(name,"column")  ) {
			RecvIntegerData(fp,&column);
		} else
		if		(  !stricmp(name,"item")  ) {
			gtk_clist_freeze(GTK_CLIST(widget));
			gtk_clist_clear(GTK_CLIST(widget));
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = GL_RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
				rnum = GL_RecvInt(fp);
				if		(  rdata  ==  NULL  ) {
					rdata = (char **)xmalloc(sizeof(char *)*rnum);
				}
				for	( k = 0 ; k < rnum ; k ++ ) {
					GL_RecvName(fp,iname);
					(void)RecvStringData(fp,buff,SIZE_BUFF);
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
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			RegistValue(widget,name,OPT_TYPE_INT,(void*)from);
			num = GL_RecvInt(fp);
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
	NETFILE	*fp)
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
		sprintf(iname,"%s.%s[%d]",v->ValueName,v->NameSuffix,i + v->opt.ival);
		GL_SendPacketClass(fp,GL_ScreenData);
		GL_SendName(fp,iname);
		SendBoolData(fp,GL_TYPE_BOOL,((state == GTK_STATE_SELECTED) ? TRUE : FALSE));
	}
dbgmsg("<SendList");
	return	(TRUE);
}

static	Bool
RecvList(
	GtkWidget	*widget,
	NETFILE	*fp)
{
	char	name[SIZE_BUFF]
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
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	longname = WidgetName + strlen(WidgetName);
	nitem = GL_RecvInt(fp);
	count = -1;
	from = 0;
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
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
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = GL_RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvStringData(fp,buff,SIZE_BUFF)  !=  NULL  ) {
					if		(	(  j             >=  from   )
							&&	(  ( j - from )  <   count  ) ) {
						item = gtk_list_item_new_with_label(buff);
						gtk_widget_show(item);
						gtk_container_add(GTK_CONTAINER(widget),item);
					}
				}
			}
		} else {
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			RegistValue(widget,name,OPT_TYPE_INT,(void *)from);
			num = GL_RecvInt(fp);
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
	NETFILE		*fp)
{
	char	iname[SIZE_BUFF];
	int		year
	,		month
	,		day;
	ValueAttribute	*v;

dbgmsg(">SendCaleandar");
	gtk_calendar_get_date(GTK_CALENDAR(calendar),&year,&month,&day);
	v = GetValue(name);

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.year",v->ValueName);
	GL_SendName(fp,(char *)iname);
	SendIntegerData(fp,GL_TYPE_INT,year);

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.month",v->ValueName);
	GL_SendName(fp,(char *)iname);
	SendIntegerData(fp,GL_TYPE_INT,(month+1));

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.day",v->ValueName);
	GL_SendName(fp,(char *)iname);
	SendIntegerData(fp,GL_TYPE_INT,day);
dbgmsg("<SendCaleandar");
	return	(TRUE);
}

static	Bool
RecvCalendar(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	char	name[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		year
	,		month
	,		day;
	int		state;

dbgmsg(">RecvCaleandar");
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	RegistValue(widget,"",OPT_TYPE_NULL,NULL);
	nitem = GL_RecvInt(fp);
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
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
	if		(  year  >  0  ) {
		gtk_calendar_select_month(GTK_CALENDAR(widget),month-1,year);
		gtk_calendar_select_day(GTK_CALENDAR(widget),day);
	}
dbgmsg("<RecvCaleandar");
	return	(TRUE); 
}

static	Bool
SendNotebook(
	char		*name,
	GtkWidget	*notebook,
	NETFILE		*fp)
{
	char	iname[SIZE_BUFF];
	ValueAttribute	*v;
	int		page;

dbgmsg(">SendNotebook");
	v = GetValue(name);
	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
#ifdef	TRACE
	printf("iname = [%s] value = %d\n",iname,page);
#endif
	GL_SendName(fp,iname);
	SendIntegerData(fp,v->type,page);
dbgmsg("<SendNotebook");
	return	(TRUE);
}

static	Bool
RecvNotebook(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	char	name[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		page;
	int		state;
	char	*longname;

dbgmsg(">RecvNotebook");
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	nitem = GL_RecvInt(fp);
	longname = WidgetName + strlen(WidgetName);
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"pageno")  ) {
			RecvIntegerData(fp,&page);
			RegistValue(widget,name,OPT_TYPE_NULL,NULL);
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
	NETFILE		*fp)
{
	char	iname[SIZE_BUFF];
	ValueAttribute	*v;
	int		value;

dbgmsg(">SendProgress");
	v = GetValue(name);
	value = gtk_progress_get_value(GTK_PROGRESS(progress));

	GL_SendPacketClass(fp,GL_ScreenData);
	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
#ifdef	TRACE
	printf("iname = [%s] value = %d\n",iname,value);
#endif
	GL_SendName(fp,iname);
	SendIntegerData(fp,v->type,value);
dbgmsg("<SendProgress");
	return	(TRUE);
}

static	Bool
RecvProgressBar(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	char	name[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		value;
	int		state;
	char	*longname;

dbgmsg(">RecvProgress");
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	nitem = GL_RecvInt(fp);
	longname = WidgetName + strlen(WidgetName);
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"value")  ) {
			RecvIntegerData(fp,&value);
			RegistValue(widget,name,OPT_TYPE_NULL,NULL);
			gtk_progress_set_value(GTK_PROGRESS(widget),value);
		}
	}
dbgmsg("<RecvProgress");
	return	(TRUE);
}

static	Bool
RecvFrame(
	GtkWidget	*widget,
	NETFILE		*fp)
{
	char	name[SIZE_BUFF]
	,		buff[SIZE_BUFF];
	int		nitem
	,		i;
	int		state;
	char	*longname;

ENTER_FUNC;
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/
	nitem = GL_RecvInt(fp);
	longname = WidgetName + strlen(WidgetName);
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"label")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_frame_set_label(GTK_FRAME(widget),buff);
		} else {
			sprintf(longname,".%s",name);
			RecvValue(fp,longname + strlen(name) + 1);
		}
	}
LEAVE_FUNC;
	return	(TRUE);
}

static	Bool
SendOption(
	char	*name,
	GtkWidget	*widget,
	NETFILE	*fp)
{
    GtkWidget *menu;
	char			iname[SIZE_BUFF];
	ValueAttribute	*v;

ENTER_FUNC;
	v = GetValue(name);

	menu= gtk_option_menu_get_menu(GTK_OPTION_MENU(widget));

	sprintf(iname,"%s.%s",v->ValueName,v->NameSuffix);
	GL_SendPacketClass(fp,GL_ScreenData);
	GL_SendName(fp,iname);
	SendIntegerData(fp,v->type,g_list_index(GTK_MENU_SHELL(menu)->children,
									GTK_OPTION_MENU(widget)->menu_item));
LEAVE_FUNC;
	return	(TRUE);
}

static	Bool
RecvOption(
	GtkWidget	*widget,
	NETFILE	*fp)
{
    GtkWidget *menu;

	char	name[SIZE_BUFF]
	,		buff[SIZE_BUFF]
	,		*longname;
	int		count
		,	choice
		,	nitem
		,	num
		,	i
		,	j;
	GtkWidget	*item;
	int		state;

ENTER_FUNC;
	DataType = GL_RecvDataType(fp);	/*	GL_TYPE_RECORD	*/

	longname = WidgetName + strlen(WidgetName);
	nitem = GL_RecvInt(fp);
	count = -1;
	for	( i = 0 ; i < nitem ; i ++ ) {
		GL_RecvName(fp,name);
		if		(  !stricmp(name,"state")  ) {
			RecvIntegerData(fp,&state);
			SetState(widget,(GtkStateType)state);
		} else
		if		(  !stricmp(name,"style")  ) {
			RecvStringData(fp,buff,SIZE_BUFF);
			gtk_widget_set_style(widget,GetStyle(buff));
		} else
		if		(  !stricmp(name,"count")  ) {
			RecvIntegerData(fp,&count);
		} else
		if		(  !stricmp(name,"select")  ) {
			RegistValue(widget,name,OPT_TYPE_NULL,NULL);
			RecvIntegerData(fp,&choice);
		} else
		if		(  !stricmp(name,"item")  ) {
			if		(  gtk_option_menu_get_menu(GTK_OPTION_MENU(widget))  !=  NULL  ) {
				gtk_option_menu_remove_menu(GTK_OPTION_MENU(widget));
			}
			menu = gtk_menu_new();
			DataType = GL_RecvDataType(fp);	/*	GL_TYPE_ARRAY	*/
			num = GL_RecvInt(fp);
			if		(  count  <  0  ) {
				count = num;
			}
			for	( j = 0 ; j < num ; j ++ ) {
				if		(  RecvStringData(fp,buff,SIZE_BUFF)  !=  NULL  ) {
					if		(  j  <  count  ) {
						item = gtk_menu_item_new_with_label(buff);
						gtk_widget_show(item);
						gtk_menu_append (GTK_MENU(menu), item);
					}
				}
			}
			gtk_option_menu_set_menu(GTK_OPTION_MENU(widget), menu);
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget), choice);
		} else {
			sprintf(longname,".%s",name);
			RecvValue(fp,longname + strlen(name) + 1);
		}
	}
	UpdateWidget(widget,NULL);
LEAVE_FUNC;
	return	(TRUE);
}

extern	void
InitWidgetOperations(void)
{
	ValueTable = NewNameHash();

	GTK_PANDA_TYPE_CLIST;	/*	for gtk+panda bug	*/

	ClassTable = g_hash_table_new((GHashFunc)ClassHash,(GCompareFunc)ClassCompare);

#ifdef	USE_PANDA
	AddClass(GTK_TYPE_NUMBER_ENTRY,RecvNumberEntry,SendNumberEntry);
	AddClass(GTK_PANDA_TYPE_COMBO,RecvPandaCombo,NULL);
	AddClass(GTK_PANDA_TYPE_CLIST,RecvPandaCList,SendPandaCList);
	AddClass(GTK_PANDA_TYPE_ENTRY,RecvEntry,SendEntry);
	AddClass(GTK_PANDA_TYPE_TEXT,RecvText,SendText);
	AddClass(GTK_PANDA_TYPE_PS,RecvPS,SendPS);
	AddClass(GTK_PANDA_TYPE_TIMER,RecvTimer,SendTimer);
#endif
	AddClass(GTK_TYPE_ENTRY,RecvEntry,SendEntry);
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

	AddClass(GTK_TYPE_FRAME,RecvFrame,NULL);
	AddClass(GTK_TYPE_OPTION_MENU,RecvOption,SendOption);
}
