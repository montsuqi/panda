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
#include	"HTClex.h"
#include	"HTCparser.h"
#include	"mon.h"
#include	"tags.h"
#include	"exec.h"
#include	"debug.h"

#define	SIZE_ASTACK		30
static	size_t	AStack[SIZE_ASTACK];
static	size_t	pAStack;

#define	Push(v)		do { \
    if (pAStack == SIZE_ASTACK) { \
        HTC_Error("stack level too deep\n"); \
        exit(1); \
    } \
    AStack[pAStack ++] = (v); \
} while (0)
#define	Pop			(AStack[-- pAStack])
#define	TOP(n)		AStack[pAStack-(n)]

static char *enctype_urlencoded = "application/x-www-form-urlencoded";
static char *enctype_multipart = "multipart/form-data";

static char *ScriptName;

#define IsTrue(s)  (*s == 'T' || *s == 't')

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
EmitAttributeValue(
	HTCInfo	*htc,
	char	*str,
	Bool	fQuote,
	Bool	fEncodeURL)
{
	if		(  fQuote  ) {
		LBS_EmitString(htc->code,"\"");
	}
	switch	(*str) {
        case '$':
            EmitCode(htc,OPC_NAME);
            LBS_EmitPointer(htc->code,StrDup(str+1));
            EmitCode(htc,OPC_HSNAME);
            if (fEncodeURL) {
                EmitCode(htc,OPC_URLENC);
            }
            EmitCode(htc,OPC_REFSTR);
            break;
        default:
            EmitCode(htc,OPC_NAME);
            LBS_EmitPointer(htc->code,StrDup(str));
            if (fEncodeURL) {
                EmitCode(htc,OPC_URLENC);
            }
            EmitCode(htc,OPC_REFSTR);
            break;
	}
	if		(  fQuote  ) {
		LBS_EmitString(htc->code,"\"");
	}
}

static void
JavaScriptEvent(HTCInfo *htc, Tag *tag, char *event)
{
    char *value;
    char buf[SIZE_BUFF];

    if ((value = GetArg(tag, event, 0)) == NULL)
        return;
        
    snprintf(buf, SIZE_BUFF,
             " %s=\""
             "document.forms[%d].elements[0].name='_event';"
             "document.forms[%d].elements[0].value='%s';"
             "document.forms[%d].submit();\"",
             event, htc->FormNo, htc->FormNo, value, htc->FormNo);
    LBS_EmitString(htc->code, buf);
}

static void
JavaScriptKeyEvent(HTCInfo *htc, Tag *tag, char *event)
{
    char *value;
    char buf[SIZE_BUFF];
    char *key, *p;

    if ((value = GetArg(tag, event, 0)) == NULL)
        return;
    p = value;
    while (isspace(*p)) p++;
    if (strncmp(p, "key=", 4) != 0) {
        HTC_Error("missing `key=' parameter in %s event of <%s>\n",
                  event, tag->name);
        return;
    }
    key = p + 4;
    if ((p = strchr(key, ';')) == NULL) {
        HTC_Error("missing `;' in %s event of <%s>\n",
                  event, tag->name);
        return;
    }
    *p++ = '\0';
    while (isspace(*p)) p++;
    snprintf(buf, SIZE_BUFF,
             " %s=\"if (event.keyCode == %s) { "
             "document.forms[%d].elements[0].name='_event';"
             "document.forms[%d].elements[0].value='%s';"
             "document.forms[%d].submit(); }\"",
             event, key, htc->FormNo, htc->FormNo, p, htc->FormNo);
    LBS_EmitString(htc->code, buf);
}

static	void
Style(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*id
	,		*klass;

	if		(  ( id = GetArg(tag,"id",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," id=");
		EmitAttributeValue(htc,id,TRUE,FALSE);
	}
	if		(  ( klass = GetArg(tag,"class",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," class=");
		EmitAttributeValue(htc,klass,TRUE,FALSE);
	}
}

static	void
_Entry(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name
	,		*size
	,		*maxlength
	,		*visible;

dbgmsg(">_Entry");
	if		(  ( name = GetArg(tag,"name",0) )  !=  NULL  ) {
        if ((visible = GetArg(tag, "visible", 0)) != NULL &&
            !IsTrue(visible)) {
            LBS_EmitString(htc->code,"<input type=\"password\" name=\"");
        }
        else {
            LBS_EmitString(htc->code,"<input type=\"text\" name=\"");
        }

		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_REFSTR);

		LBS_EmitString(htc->code,"\" value=\"");

		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_EHSNAME);

		LBS_EmitString(htc->code,"\"");
		if		(  ( size = GetArg(tag,"size",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code," size=\"");

			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(size));
			EmitCode(htc,OPC_REFSTR);
			LBS_EmitString(htc->code,"\"");

		}
		if		(  ( maxlength = GetArg(tag,"maxlength",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code," maxlength=\"");

			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(maxlength));
			EmitCode(htc,OPC_REFSTR);
			LBS_EmitString(htc->code,"\"");

		}
        JavaScriptEvent(htc, tag, "onchange");
        JavaScriptKeyEvent(htc, tag, "onkeydown");
        JavaScriptKeyEvent(htc, tag, "onkeyup");
		Style(htc,tag);
		LBS_EmitString(htc->code,">\n");
	}
dbgmsg("<_Entry");
}

static	void
_Fixed(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name;
dbgmsg(">_Fixed");
	if		(  ( name = GetArg(tag,"name",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code,"<span");
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
        EmitCode(htc,OPC_NAME);
        LBS_EmitPointer(htc->code,StrDup(name));
        EmitCode(htc,OPC_HSNAME);
        EmitCode(htc,OPC_EMITSTR);
		LBS_EmitString(htc->code,"</span>");
	}
dbgmsg("<_Fixed");
}

static	void
_Combo(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	name[SIZE_ARG];
	char	*size, *onchange;
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
		LBS_EmitString(htc->code,"\"");
	}
    JavaScriptEvent(htc, tag, "onchange");
	Style(htc,tag);
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
	EmitCode(htc,OPC_EHSNAME);

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
	char	*name, *defaultevent;

dbgmsg(">_Form");
	htc->FormNo++;
	LBS_EmitString(htc->code,"<form action=\"");
    LBS_EmitString(htc->code,ScriptName);
    LBS_EmitString(htc->code,"\" method=\"");
	if		(  fGet  ) {
		LBS_EmitString(htc->code,"get");
	} else {
		LBS_EmitString(htc->code,"post");
	}
	LBS_EmitString(htc->code,"\"");
    LBS_EmitString(htc->code, " enctype=\"");
    EmitCode(htc, OPC_SCONST);
    htc->EnctypePos = LBS_GetPos(htc->code);
    LBS_EmitPointer(htc->code, enctype_urlencoded);
    EmitCode(htc, OPC_REFSTR);
    LBS_EmitString(htc->code, "\"");
	Style(htc,tag);
	if		(  ( name = GetArg(tag,"name",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," name=\"");
		LBS_EmitString(htc->code,name);
		LBS_EmitString(htc->code,"\"");
	}
	LBS_EmitString(htc->code,">\n");

    /* document.forms[htc->FormNo].elements[0] for JavaScript */
	LBS_EmitString(htc->code,"\n<input type=\"hidden\" name=\"_\" value=\"\">");

	LBS_EmitString(htc->code,"\n<input type=\"hidden\" name=\"_name\" value=\"");
	EmitGetValue(htc,"_name");
	LBS_EmitString(htc->code,"\">");
	if		(  !fCookie  ) {
		LBS_EmitString(htc->code,"\n<input type=\"hidden\" name=\"_sesid\" value=\"");
		EmitGetValue(htc,"_sesid");
		LBS_EmitString(htc->code,"\">");
	}

    if ((defaultevent = GetArg(tag, "defaultevent", 0)) != NULL) {
        htc->DefaultEvent = StrDup(defaultevent);
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
	EmitCode(htc,OPC_EHSNAME);
	LBS_EmitString(htc->code,"\"");
	if		(  ( cols = GetArg(tag,"cols",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," cols=\"");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(cols));
		EmitCode(htc,OPC_REFSTR);
		LBS_EmitString(htc->code,"\"");
	}
	if		(  ( rows = GetArg(tag,"rows",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," rows=\"");
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(rows));
		EmitCode(htc,OPC_REFSTR);
		LBS_EmitString(htc->code,"\"");
	}
    JavaScriptEvent(htc, tag, "onchange");
    JavaScriptKeyEvent(htc, tag, "onkeydown");
    JavaScriptKeyEvent(htc, tag, "onkeyup");
	Style(htc,tag);
	LBS_EmitString(htc->code,">\n");

	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,name);
	EmitCode(htc,OPC_EHSNAME);
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
	if ((event = GetArg(tag, "event", 0)) == NULL) {
        HTC_Error("`event' attribute is required for <%s>\n", tag->name);
        return;
	}
	if ((face = GetArg(tag, "face", 0)) == NULL) {
		face = event;
	}
    if (htc->DefaultEvent == NULL)
        htc->DefaultEvent = event;
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

	Style(htc,tag);
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
	Style(htc,tag);
	LBS_EmitString(htc->code,">");

	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"label",0)));
	EmitCode(htc,OPC_EHSNAME);
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
	Style(htc,tag);
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
	Style(htc,tag);
	LBS_EmitString(htc->code,">");
	LBS_EmitString(htc->code,GetArg(tag,"label",0));
	g_hash_table_insert(htc->Radio,StrDup(group),(void*)1);
dbgmsg("<_RadioButton");
}

static void
_List(HTCInfo *htc, Tag *tag)
{
	char	buf[SIZE_ARG];
	char	*name, *label, *count, *size, *multiple, *onchange;
	size_t	pos;

dbgmsg(">_List");
    if ((name = GetArg(tag,"name",0)) != NULL &&
        (label = GetArg(tag,"label",0)) != NULL &&
        (count = GetArg(tag,"count",0)) != NULL) {
        LBS_EmitString(htc->code,"<select name=\"");
        EmitCode(htc,OPC_NAME);
        LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
        EmitCode(htc,OPC_REFSTR);
        LBS_EmitString(htc->code,"\"");
        if ((size = GetArg(tag,"size",0)) != NULL) {
            LBS_EmitString(htc->code," size=");
            EmitCode(htc,OPC_NAME);
            LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"size",0)));
            EmitCode(htc,OPC_REFSTR);
            LBS_EmitString(htc->code,"\"");
        }
        if ((multiple = GetArg(tag,"multiple",0)) != NULL &&
            IsTrue(multiple)) {
            LBS_EmitString(htc->code," multiple");
        }
        JavaScriptEvent(htc, tag, "onchange");
        Style(htc,tag);
        LBS_EmitString(htc->code,">\n");
        EmitCode(htc,OPC_VAR);
        LBS_EmitPointer(htc->code,NULL);					/*	3	var		*/
        EmitCode(htc,OPC_ICONST);
        LBS_EmitInt(htc->code,0);
        EmitCode(htc,OPC_STORE);
        EmitCode(htc,OPC_HIVAR);							/*	2	limit	*/
        LBS_EmitPointer(htc->code,StrDup(count));
        EmitCode(htc,OPC_ICONST);							/*	1	step	*/
        LBS_EmitInt(htc->code,1);
        Push(LBS_GetPos(htc->code));
        EmitCode(htc,OPC_BREAK);
        LBS_EmitInt(htc->code,0);

        LBS_EmitString(htc->code,"<option value=\"");
        EmitCode(htc,OPC_LDVAR);
        LBS_EmitPointer(htc->code,"");
        EmitCode(htc,OPC_REFINT);
        LBS_EmitString(htc->code,"\"");
        EmitCode(htc,OPC_NAME);
        sprintf(buf,"%s[#]",name);
        LBS_EmitPointer(htc->code,StrDup(buf));
        EmitCode(htc,OPC_HBES);
        LBS_EmitPointer(htc->code," selected");
        LBS_EmitString(htc->code,">");
        EmitCode(htc,OPC_NAME);
        sprintf(buf,"%s[#]",label);
        LBS_EmitPointer(htc->code,StrDup(buf));
        EmitCode(htc,OPC_EHSNAME);
        LBS_EmitString(htc->code,"</option>\n");

        EmitCode(htc,OPC_LEND);
        LBS_EmitInt(htc->code,Pop);
        pos = LBS_GetPos(htc->code);
        LBS_SetPos(htc->code,TOP(0));
        EmitCode(htc,OPC_BREAK);
        LBS_EmitInt(htc->code,pos);
        LBS_SetPos(htc->code,pos);
        LBS_EmitString(htc->code,"</select>");
    }
dbgmsg("<_List");
}

static	void
_FileSelection(HTCInfo *htc, Tag *tag)
{
	char *name, *filename, *size, *maxlength;

dbgmsg(">_FileSelection");
    if ((name = GetArg(tag, "name", 0)) != NULL &&
        (filename = GetArg(tag, "filename", 0)) != NULL) {
		LBS_EmitString(htc->code, "<input type=\"file\" name=\"");
		EmitCode(htc, OPC_NAME);
		LBS_EmitPointer(htc->code, StrDup(name));
		EmitCode(htc, OPC_REFSTR);

		LBS_EmitString(htc->code, "\" value=\"");
		EmitCode(htc, OPC_NAME);
		LBS_EmitPointer(htc->code, StrDup(filename));
		EmitCode(htc, OPC_EHSNAME);
        LBS_EmitString(htc->code, "\"");

		if ((size = GetArg(tag, "size", 0)) != NULL) {
			LBS_EmitString(htc->code, " size=\"");
			EmitCode(htc, OPC_NAME);
			LBS_EmitPointer(htc->code, StrDup(size));
			EmitCode(htc, OPC_REFSTR);
			LBS_EmitString(htc->code, "\"");
		}

        if ((maxlength = GetArg(tag, "maxlength", 0)) != NULL) {
			LBS_EmitString(htc->code, " maxlength=\"");
			EmitCode(htc, OPC_NAME);
			LBS_EmitPointer(htc->code, StrDup(maxlength));
			EmitCode(htc, OPC_REFSTR);
			LBS_EmitString(htc->code, "\"");
		}

        Style(htc, tag);
		LBS_EmitString(htc->code, ">\n");

        g_hash_table_insert(htc->FileSelection,
                            StrDup(name), StrDup(filename));

        if (htc->EnctypePos != 0) {
            size_t pos = LBS_GetPos(htc->code);
            LBS_SetPos(htc->code, htc->EnctypePos);
            LBS_EmitPointer(htc->code, enctype_multipart);
            LBS_SetPos(htc->code, pos);
        }
    }
dbgmsg("<_FileSelection");
}

static void
_HyperLink(HTCInfo *htc, Tag *tag)
{
	char *event;
	char *name;
	char *value;
    char *file;
    char *filename;
    char *contenttype;

dbgmsg(">_HyperLink");
	if ((event = GetArg(tag, "event", 0)) != NULL) {
		LBS_EmitString(htc->code, "<a");
		Style(htc,tag);

		LBS_EmitString(htc->code, " href=\"");
        LBS_EmitString(htc->code,ScriptName);
		LBS_EmitString(htc->code, "?_name=");
		EmitGetValue(htc,"_name");
		LBS_EmitString(htc->code, "&amp;_event=");
		EmitCode(htc, OPC_NAME);
		LBS_EmitPointer(htc->code, StrDup(event));
		EmitCode(htc, OPC_REFSTR);
		if (!fCookie) {
			LBS_EmitString(htc->code, "&amp;_sesid=");
			EmitGetValue(htc,"_sesid");
		}
		if ((name = GetArg(tag, "name", 0)) != NULL &&
            (value = GetArg(tag, "value", 0)) != NULL) {
			LBS_EmitString(htc->code,"&amp;");
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(name));
			EmitCode(htc,OPC_REFSTR);
			LBS_EmitString(htc->code,"=");
            EmitAttributeValue(htc,value,FALSE,TRUE); 
		}
		if ((file = GetArg(tag, "file", 0)) != NULL) {
			LBS_EmitString(htc->code, "&amp;_file=");
            EmitCode(htc, OPC_NAME);
			LBS_EmitPointer(htc->code, StrDup(file));
            EmitCode(htc, OPC_REFSTR);
        }
		if ((filename = GetArg(tag, "filename", 0)) != NULL) {
			LBS_EmitString(htc->code, "&amp;_filename=");
            EmitCode(htc, OPC_NAME);
			LBS_EmitPointer(htc->code, StrDup(filename));
            EmitCode(htc, OPC_REFSTR);
        }
		if ((contenttype = GetArg(tag, "contenttype", 0)) != NULL) {
			LBS_EmitString(htc->code, "&amp;_contenttype=");
            EmitCode(htc, OPC_NAME);
			LBS_EmitPointer(htc->code, StrDup(contenttype));
            EmitCode(htc, OPC_REFSTR);
        }
		LBS_EmitString(htc->code,"\">");
	}
dbgmsg("<_HyperLink");
}

static void
_eHyperLink(HTCInfo *htc, Tag *tag)
{
dbgmsg(">_eHyperLink");
	LBS_EmitString(htc->code, "</a>");
dbgmsg("<_eHyperLink");
}

static	void
_Panel(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*visible;
    int		has_style;

dbgmsg(">_Panel");
    if ((visible = GetArg(tag, "visible", 0)) == NULL) {
        Push(0);
    }
    else {
        EmitCode(htc, OPC_NAME);
        LBS_EmitPointer(htc->code, StrDup(visible));
        EmitCode(htc, OPC_HBNAME);
        EmitCode(htc, OPC_JNZP);
        Push(LBS_GetPos(htc->code));
        LBS_EmitInt(htc->code, 0);
    }
    has_style =
        GetArg(tag, "id", 0) !=  NULL ||
        GetArg(tag, "class", 0) !=  NULL;
    if (has_style) {
        LBS_EmitString(htc->code,"<div");
    }
    Style(htc,tag);
    if (has_style) {
        LBS_EmitString(htc->code,">");
    }
    Push(has_style);
dbgmsg("<_Panel");
}

static void
_ePanel(HTCInfo *htc, Tag *tag)
{
    size_t jnzp_pos, pos;
dbgmsg(">_ePanel");
    if (Pop) { 
        LBS_EmitString(htc->code, "</div>");
    }
    if ((jnzp_pos = Pop) != 0) {
        pos = LBS_GetPos(htc->code);
        LBS_SetPos(htc->code, jnzp_pos);
        LBS_EmitInt(htc->code, pos);
        LBS_SetPos(htc->code, pos);
        EmitCode(htc, OPC_DROP);
    }
dbgmsg("<_ePanel");
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

static	void
_Htc(
	HTCInfo	*htc,
	Tag		*tag)
{
dbgmsg(">_Htc");
	Codeset = GetArg(tag,"coding",0); 
	HTCSetCodeset(Codeset);
dbgmsg("<_Htc");
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
TagsInit(char *script_name)
{
	Tag		*tag;

dbgmsg(">TagsInit");
    ScriptName = script_name; 
 
	pAStack = 0; 
	Tags = NewNameiHash();

	tag = NewTag("ENTRY",_Entry);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"maxlength",TRUE);
	AddArg(tag,"visible",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"onkeydown",TRUE);
	AddArg(tag,"onkeyup",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("COMBO",_Combo);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"item",TRUE);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("FIXED",_Fixed);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

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
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"onkeydown",TRUE);
	AddArg(tag,"onkeyup",TRUE);
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

	tag = NewTag("LIST",_List);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"multiple",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("FILESELECTION",_FileSelection);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"filename",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"maxlength",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);

	tag = NewTag("HYPERLINK",_HyperLink);
	AddArg(tag,"event",TRUE);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"value",TRUE);
	AddArg(tag,"file",TRUE);
	AddArg(tag,"filename",TRUE);
	AddArg(tag,"contenttype",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	tag = NewTag("/HYPERLINK",_eHyperLink);

	tag = NewTag("PANEL", _Panel);
	AddArg(tag, "visible", TRUE);
	AddArg(tag, "id", TRUE);
	AddArg(tag, "class", TRUE);
	tag = NewTag("/PANEL", _ePanel);

	tag = NewTag("CALENDAR",_Calendar);
	AddArg(tag,"year",TRUE);
	AddArg(tag,"month",TRUE);
	AddArg(tag,"day",TRUE);

	tag = NewTag("HTC",_Htc);
	AddArg(tag,"coding",TRUE);

	tag = NewTag("FORM",_Form);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"defaultevent",TRUE);
	tag = NewTag("HEAD",_Head);
	tag = NewTag("/BODY",_eBody);
	tag = NewTag("/HTML",_eHtml);
dbgmsg("<TagsInit");
}
