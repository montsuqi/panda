/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<glib.h>
#include	<time.h>
#include	<sys/time.h>
#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"HTCparser.h"
#include	"mon.h"
#include	"tags.h"
#include	"exec.h"
#include	"misc.h"
#include	"debug.h"

#define	SIZE_ASTACK		10
static	size_t	AStack[SIZE_ASTACK];
static	size_t	pAStack;

#define	Push(v)		AStack[pAStack ++] = (v)
#define	Pop			(AStack[-- pAStack])
#define	TOP(n)		AStack[pAStack-(n)]


static	char	*
GetArg(
	Tag		*tag,
	char	*name,
	int		i)
{
	TagType	*type;
	char	*ret;

dbgmsg(">GetArg");
	if		(  ( type = g_hash_table_lookup(tag->args,name) )  !=  NULL  ) {
		if		(  type->fPara  ) {
			if		(  i  <  type->nPara  ) {
				ret = type->Para[i];
			} else {
				ret = NULL;
			}
		} else {
			ret = "";
		}
	} else {
		ret = NULL;
	}
#ifdef	DEBUG
	if		(  ret  ==  NULL  ) {
		printf("%s = NULL\n",name);
	} else {
		printf("%s = [%s]\n",name,ret);
	}
#endif
dbgmsg("<GetArg");
	return	(ret);
}

static	void
EmitCode(
	HTCInfo	*htc,
	byte	code)
{
	LBS_EmitByte(htc->code,0x01);
	LBS_EmitByte(htc->code,code);
}

static	void
EmitGetValue(
	HTCInfo	*htc,
	char	*name)
{
	EmitCode(htc,OPC_REF);
	LBS_EmitPointer(htc->code,name);
}

static	void
Attribute(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*id
	,		*klass;

	if		(  ( id = GetArg(tag,"id",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," id=\"");
		LBS_EmitString(htc->code,id);
		LBS_EmitString(htc->code,"\"");
	}
	if		(  ( klass = GetArg(tag,"class",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," class=\"");
		LBS_EmitString(htc->code,klass);
		LBS_EmitString(htc->code,"\"");
	}
}

static	void
_Entry(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*size
	,		*maxlength;

dbgmsg(">_Entry");
	LBS_EmitString(htc->code,"<input type=\"text\" name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\" value=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HSNAME);
	LBS_EmitString(htc->code,"\"");
	if		(  ( size = GetArg(tag,"size",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," size=");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(size));
		EmitCode(htc,OPC_REFSTR);
	}
	if		(  ( maxlength = GetArg(tag,"maxlength",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," maxlength=");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(maxlength));
		EmitCode(htc,OPC_REFSTR);
	}
	Attribute(htc,tag);
	LBS_EmitString(htc->code,">\n");
dbgmsg("<_Entry");
}

static	void
_Fixed(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name;
dbgmsg(">_Fixed");
	name = StrDup(GetArg(tag,"name",0));
	switch	(*name) {
	  case	'$':
		EmitCode(htc,OPC_VAR);
		LBS_EmitPointer(htc->code,name+1);
		EmitCode(htc,OPC_REFSTR);
		break;
	  case	'#':
		EmitCode(htc,OPC_VAR);
		LBS_EmitPointer(htc->code,name+1);
		EmitCode(htc,OPC_REFINT);
		break;
	  default:
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,name);
		EmitCode(htc,OPC_HSNAME);
		break;
	}
dbgmsg("<_Fixed");
}

static	void
_Combo(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	name[SIZE_ARG];
	char	*size;
	size_t	pos;

dbgmsg(">_Combo");
	LBS_EmitString(htc->code,"<select name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_REFSTR);
	size = GetArg(tag,"size",0);
	if		(  size  ==  NULL  ) {
		LBS_EmitString(htc->code,"\"");
	} else {
		LBS_EmitString(htc->code,"\" size=");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"size",0)));
		EmitCode(htc,OPC_REFSTR);
		LBS_EmitString(htc->code,">\n");
	}
	Attribute(htc,tag);
	LBS_EmitString(htc->code,">\n");
	EmitCode(htc,OPC_VAR);
	LBS_EmitPointer(htc->code,NULL);					/*	3	var		*/
	EmitCode(htc,OPC_ICONST);
	LBS_EmitInt(htc->code,0);
	EmitCode(htc,OPC_STORE);
	EmitCode(htc,OPC_HIVAR);							/*	2	limit	*/
	LBS_EmitPointer(htc->code,GetArg(tag,"count",0));
	EmitCode(htc,OPC_ICONST);							/*	1	step	*/
	LBS_EmitInt(htc->code,1);
	Push(LBS_GetPos(htc->code));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,0);

	LBS_EmitString(htc->code,"<option>\n");
	EmitCode(htc,OPC_NAME);
	sprintf(name,"%s[#]",GetArg(tag,"item",0));
	LBS_EmitPointer(htc->code,StrDup(name));
	EmitCode(htc,OPC_HSNAME);

	EmitCode(htc,OPC_LEND);
	LBS_EmitInt(htc->code,Pop);
	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,TOP(0));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
	LBS_EmitString(htc->code,"</select>");
dbgmsg("<_Combo");
}

static	void
_Form(
	HTCInfo	*htc,
	Tag		*tag)
{
dbgmsg(">_Form");
	LBS_EmitString(htc->code,"<form action=\"mon.cgi\" method=\"");
	if		(  fGet  ) {
		LBS_EmitString(htc->code,"get");
	} else {
		LBS_EmitString(htc->code,"post");
	}
	LBS_EmitString(htc->code,"\"");
	Attribute(htc,tag);
	LBS_EmitString(htc->code,">\n");

	LBS_EmitString(htc->code,"\n<input type=\"hidden\" name=\"_name\" value=\"");
	EmitGetValue(htc,"_name");
	LBS_EmitString(htc->code,"\">");
	if		(  !fCookie  ) {
		LBS_EmitString(htc->code,"\n<input type=\"hidden\" name=\"_sesid\" value=\"");
		EmitGetValue(htc,"_sesid");
		LBS_EmitString(htc->code,"\">");
	}
dbgmsg("<_Form");
}

static	void
_Text(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name
	,		*cols
	,		*rows;
dbgmsg(">_Text");
	LBS_EmitString(htc->code,"<textarea name=\"");
	EmitCode(htc,OPC_NAME);
	name = StrDup(GetArg(tag,"name",0));
	LBS_EmitPointer(htc->code,name);
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\" value=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HSNAME);
	if		(  ( cols = GetArg(tag,"cols",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code,"\" cols=");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(cols));
		EmitCode(htc,OPC_REFSTR);
		LBS_EmitString(htc->code," ");
	}
	if		(  ( rows = GetArg(tag,"rows",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code,"\" rows=");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(rows));
		EmitCode(htc,OPC_REFSTR);
	}

	LBS_EmitString(htc->code,"\"");
	Attribute(htc,tag);
	LBS_EmitString(htc->code,">\n");

	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,name);
	EmitCode(htc,OPC_HSNAME);
	LBS_EmitString(htc->code,"</textarea>\n");
dbgmsg("<_Text");
}

static	void
_Head(
	HTCInfo	*htc,
	Tag		*tag)
{
dbgmsg(">_Head");
	LBS_EmitString(htc->code,
				   "<head>\n<meta http-equiv=\"Pragma\" content=\"no-cache\">");
dbgmsg("<_Head");
}	

static	void
_eBody(
	HTCInfo	*htc,
	Tag		*tag)
{
dbgmsg(">_eBody");
	if		(  !fDump  ) {
		LBS_EmitString(htc->code,"</body>");
	}
dbgmsg("<_eBody");
}	

static	void
_eHtml(
	HTCInfo	*htc,
	Tag		*tag)
{
dbgmsg(">_eHtml");
	if		(  !fDump  ) {
		LBS_EmitString(htc->code,"</html>");
	}
	LBS_EmitEnd(htc->code);
dbgmsg("<_eHtml");
}

static	void
_Count(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*from
	,		*to
	,		*step;

dbgmsg(">_Count");
	EmitCode(htc,OPC_VAR);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"var",0)));		/*	3	var		*/
	if		(  ( from = GetArg(tag,"from",0) )  !=  NULL  ) {
		if		(  isdigit(*from)  ) {
			EmitCode(htc,OPC_ICONST);
			LBS_EmitInt(htc->code,atoi(from));
		} else {
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(from));
			EmitCode(htc,OPC_HINAME);
		}
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,0);
	}
	EmitCode(htc,OPC_STORE);

	if		(  ( to = GetArg(tag,"to",0) )  !=  NULL  ) {
		if		(  isdigit(*to)  ) {
			EmitCode(htc,OPC_ICONST);
			LBS_EmitInt(htc->code,atoi(to));
		} else {
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(to));
			EmitCode(htc,OPC_HINAME);
		}
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,0);
	}													/*	2	limit	*/
	if		(  ( step = GetArg(tag,"step",0) )  !=  NULL  ) {
		if		(  isdigit(*step)  ) {
			EmitCode(htc,OPC_ICONST);
			LBS_EmitInt(htc->code,atoi(step));
		} else {
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(step));
			EmitCode(htc,OPC_HINAME);
		}
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,1);
	}													/*	1	step	*/
	Push(LBS_GetPos(htc->code));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,0);
dbgmsg("<_Count");
}

static	void
_eCount(
	HTCInfo	*htc,
	Tag		*tag)
{
	size_t	pos;
dbgmsg(">_eCount");
	EmitCode(htc,OPC_LEND);
	LBS_EmitInt(htc->code,Pop);
	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,TOP(0));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
dbgmsg("<_eCount");
}

static	void
_Button(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*face
	,		*event
	,		*size;

dbgmsg(">_Button");
	if		(  ( face = GetArg(tag,"face",0) )  ==  NULL  ) {
		face = GetArg(tag,"event",0);
	}
	if		(  ( event = GetArg(tag,"event",0) )  ==  NULL  ) {
		event = GetArg(tag,"face",0);
	}
	g_hash_table_insert(htc->Trans,StrDup(face),StrDup(event));
	LBS_EmitString(htc->code,"<input type=\"submit\" name=\"_event\" value=\"");
	LBS_EmitString(htc->code,face);
	LBS_EmitString(htc->code,"\"");
	if		(  ( size = GetArg(tag,"size",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," size=");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(size));
		EmitCode(htc,OPC_REFSTR);
	}

	Attribute(htc,tag);
	LBS_EmitString(htc->code,">");
dbgmsg("<_Button");
}

static	void
_ToggleButton(
	HTCInfo	*htc,
	Tag		*tag)
{
dbgmsg(">_ToggleButton");
	LBS_EmitString(htc->code,"<input type=\"checkbox\" name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HBES);
	LBS_EmitPointer(htc->code," checked ");
	LBS_EmitString(htc->code," value=\"TRUE\"");
	Attribute(htc,tag);
	LBS_EmitString(htc->code,">");

	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"label",0)));
	EmitCode(htc,OPC_HSNAME);
dbgmsg("<_ToggleButton");
}

static	void
_CheckButton(
	HTCInfo	*htc,
	Tag		*tag)
{
dbgmsg(">_CheckButton");
	LBS_EmitString(htc->code,"<input type=\"checkbox\" name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HBES);
	LBS_EmitPointer(htc->code," checked");
	LBS_EmitString(htc->code," value=\"TRUE\"");
	Attribute(htc,tag);
	LBS_EmitString(htc->code,">");
	LBS_EmitString(htc->code,GetArg(tag,"label",0));
dbgmsg("<_CheckButton");
}

static	void
_RadioButton(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*group
	,		*name;

dbgmsg(">_RadioButton");
	group = GetArg(tag,"group",0); 
	name = GetArg(tag,"name",0); 
	LBS_EmitString(htc->code,"<input type=\"radio\" name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(group));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\"");

	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HBES);
	LBS_EmitPointer(htc->code," checked ");

	LBS_EmitString(htc->code,"value=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(name));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\"");
	Attribute(htc,tag);
	LBS_EmitString(htc->code,">");
	LBS_EmitString(htc->code,GetArg(tag,"label",0));
	g_hash_table_insert(htc->Radio,StrDup(group),(void*)1);
dbgmsg("<_RadioButton");
}


static	void
_Calendar(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*year
	,		*month
	,		*day;
	time_t		nowtime;
	struct	tm	*Now;
	int		this_yy
	,		this_mm
	,		this_dd;
	size_t	pos;

dbgmsg(">_Calendar");
	time(&nowtime);
	Now = localtime(&nowtime);
	this_yy = Now->tm_year + 1900;
	this_mm = Now->tm_mon + 1;
	this_dd = Now->tm_mday;
	if		(  ( year = GetArg(tag,"year",0) )  ==  NULL  ) {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_yy);
	} else {
		year = StrDup(year);
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,year);
		EmitCode(htc,OPC_HINAME);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);
		EmitCode(htc,OPC_DROP);
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_yy);
		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);
	}
	if		(  ( month = GetArg(tag,"month",0) )  ==  NULL  ) {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_mm);
	} else {
		month = StrDup(month);
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,month);
		EmitCode(htc,OPC_HINAME);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);
		EmitCode(htc,OPC_DROP);
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_mm);
		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);
	}
	if		(  ( day = GetArg(tag,"day",0) )  ==  NULL  ) {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_dd);
	} else {
		day = StrDup(day);
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,day);
		EmitCode(htc,OPC_HINAME);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);
		EmitCode(htc,OPC_DROP);
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_dd);
		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);
	}
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,year);
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,month);
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,day);
	EmitCode(htc,OPC_CALENDAR);
dbgmsg("<_Calendar");
}

static	Tag		*
NewTag(
	char	*name,
	void	(*emit)(HTCInfo *, struct _Tag *))
{
	Tag		*tag;

	tag = New(Tag);
	tag->name = name;
	tag->emit = emit;
	tag->args = NewNameiHash();
	g_hash_table_insert(Tags,tag->name,tag);
	return	(tag);
}

static	void
AddArg(
	Tag		*tag,
	char	*name,
	Bool	fPara)
{
	TagType	*type;

	type = New(TagType);
	type->name = name;
	type->fPara = fPara;
	type->nPara = 0;
	type->Para = NULL;
	g_hash_table_insert(tag->args,type->name,type);
}

extern	void
TagsInit(void)
{
	Tag		*tag;

dbgmsg(">TagsInit");
	pAStack = 0; 
	Tags = NewNameiHash();

	tag = NewTag("ENTRY",_Entry);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"maxlength",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("COMBO",_Combo);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"item",TRUE);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("FIXED",_Fixed);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);

	tag = NewTag("COUNT",_Count);
	AddArg(tag,"var",TRUE);
	AddArg(tag,"from",TRUE);
	AddArg(tag,"to",TRUE);
	AddArg(tag,"step",TRUE);
	tag = NewTag("/COUNT",_eCount);

	tag = NewTag("TEXT",_Text);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"rows",TRUE);
	AddArg(tag,"cols",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("BUTTON",_Button);
	AddArg(tag,"event",TRUE);
	AddArg(tag,"face",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("TOGGLEBUTTON",_ToggleButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("CHECKBUTTON",_CheckButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("RADIOBUTTON",_RadioButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"group",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("CALENDAR",_Calendar);
	AddArg(tag,"year",TRUE);
	AddArg(tag,"month",TRUE);
	AddArg(tag,"day",TRUE);

	tag = NewTag("FORM",_Form);
	tag = NewTag("HEAD",_Head);
	tag = NewTag("/BODY",_eBody);
	tag = NewTag("/HTML",_eHtml);
dbgmsg("<TagsInit");
}
