/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2005 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2006-2008 Ogochan & JMA (Japan Medical Association)
 *                                 & JFBA (Japan Federation of Bar Association)
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
#include	"cgi.h"
#include	"enum.h"
#include	"tags.h"
#include	"exec.h"
#include	"debug.h"

#define	SIZE_ASTACK		30
static	size_t	AStack[SIZE_ASTACK];
static	size_t	pAStack;

#ifndef	_XOPEN_SOURCE
extern	char *strptime(const char *s, const char *format, struct tm *tm);
#endif

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

static	Bool	fButton;
static	Bool	fHead;

#define IsTrue(s)  (*(s) == 'T' || *(s) == 't')

static	char	*
HTCGetProp(
	Tag		*tag,
	char	*name,
	int		i)
{
	TagType	*type;
	char	*ret;

ENTER_FUNC;
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
LEAVE_FUNC;
	return	(ret);
}

extern	void
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
	Bool	fEncodeURL,
    Bool	fEscapeJavaScriptString)
{
	if		(  fQuote  ) {
		LBS_EmitString(htc->code,"\"");
	}
	switch	(*str) {
	  case '$':
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(str+1));
		EmitCode(htc,OPC_PHSTR);
		if (fEncodeURL) {
			EmitCode(htc,OPC_LOCURI);
		}
		if (fEscapeJavaScriptString) {
			EmitCode(htc,OPC_ESCJSS);
		}
		EmitCode(htc,OPC_EMITSTR);
		break;
	  default:
		LBS_EmitString(htc->code,str);
		break;
	}
	if		(  fQuote  ) {
		LBS_EmitString(htc->code,"\"");
	}
}

static	void
EmitAttribute(
	HTCInfo	*htc,
	Tag		*tag,
	char	*name)
{
	char	*attr;

	if		(  ( attr = HTCGetProp(tag,name,0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," ");
		LBS_EmitString(htc->code,name);
		LBS_EmitString(htc->code,"=");
		EmitAttributeValue(htc,attr,TRUE,FALSE,FALSE);
	}
}

extern	void
ExpandAttributeString(
	HTCInfo	*htc,
	char	*para)
{
	Bool	fQuote;

	fQuote = ( strchr(para,'"')  !=  NULL );
	if		(  fQuote  ) {
		LBS_EmitChar(htc->code,'\'');
	} else {
		LBS_EmitChar(htc->code,'"');
	}
	switch	(*para) {
	  case	'$':
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(para+1));
		EmitCode(htc,OPC_PHSTR);
		EmitCode(htc,OPC_EMITSTR);
		break;
	  case	'#':
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(para));
		EmitCode(htc,OPC_EMITSTR);
		break;
	  case	'\\':
		LBS_EmitString(htc->code,para+1);
		break;
	  default:
		LBS_EmitString(htc->code,para);
		break;
	}
	if		(  fQuote  ) {
		LBS_EmitChar(htc->code,'\'');
	} else {
		LBS_EmitChar(htc->code,'"');
	}
}

static void
JavaScriptEvent(HTCInfo *htc, Tag *tag, char *event)
{
    char 	*value
		,	*p;
    char buf[SIZE_BUFF];
	size_t	len;

	if		(  !fJavaScript  )
		return;
    if ((value = HTCGetProp(tag, event, 0)) == NULL)
        return;

    LBS_EmitChar(htc->code, ' ');
	LBS_EmitString(htc->code, event);
	if		(  *value  ==  '?'  ) {
		InvokeJs("send_event");
		snprintf(buf, SIZE_BUFF,
				 "=\""
				 "send_event(%d,'%s');\"",htc->FormNo,value+1);
		LBS_EmitString(htc->code, buf);
	} else {
		LBS_EmitString(htc->code, "=\"");
		while	(  *value  !=  0  ) {
			switch	(*value) {
			  case	'$':
				value ++;
				/*	path through	*/
			  case	'#':
				p = buf;
				len = 0;
				while	(	(  isalpha(*value) )
						||	(  isdigit(*value) )
						||	(  *value  ==  '#' )
						||	(  *value  ==  '$' )
						||	(  *value  ==  '.' )
						||	(  *value  ==  '_' )
						||	(  *value  ==  '-' )
						||	(  *value  ==  ':' ) ) {
					*p = *value;
					if		(  len  <  SIZE_BUFF  ) {
						p ++;
						len ++;
					}
					value ++;
					if		(  *value  ==  0  )	break;
				}
				*p = 0;
				if		(	(  buf[0]  ==  '$'  )
						||	(  buf[0]  ==  '#'  ) ) {
					EmitCode(htc,OPC_EVAL);
					LBS_EmitPointer(htc->code,StrDup(buf));
					EmitCode(htc,OPC_EMITSTR);
				} else {
					EmitCode(htc,OPC_EVAL);
					LBS_EmitPointer(htc->code,StrDup(buf));
					EmitCode(htc,OPC_PHSTR);
					EmitCode(htc,OPC_EMITSTR);
				}
				break;
			  case	'\\':
				value ++;
				LBS_EmitChar(htc->code,*value);
				value ++;
				break;
			  default:
				LBS_EmitChar(htc->code,*value);
				value ++;
				break;
			}
		}
		LBS_EmitChar(htc->code,'"');
	}
}

static void
JavaScriptKeyEvent(HTCInfo *htc, Tag *tag, char *event)
{
    char *value;
    char buf[SIZE_BUFF];
    char *key, *p;

	if		(  !fJavaScript  )
		return;
    if ((value = HTCGetProp(tag, event, 0)) == NULL)
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
    LBS_EmitChar(htc->code, ' ');
    LBS_EmitString(htc->code, event);
	InvokeJs("send_event");
    snprintf(buf, SIZE_BUFF,
             "=\"if (event.keyCode == %s) { "
			 "send_event(%d,'%s');}\"",key,htc->FormNo,StrDup(p));
    LBS_EmitString(htc->code, buf);
}

static	void
Style(
	HTCInfo	*htc,
	Tag		*tag)
{
  	EmitAttribute(htc,tag,"class");
	EmitAttribute(htc,tag,"dir");
	EmitAttribute(htc,tag,"id");
	EmitAttribute(htc,tag,"lang");
	EmitAttribute(htc,tag,"style");
	EmitAttribute(htc,tag,"title");
}

static	void
_Include(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*type;
ENTER_FUNC;
	if		(	(  ( type = HTCGetProp(tag,"type",0) )  ==  NULL  )
			||	(  strlicmp(type,"html")          ==  0     ) ) {
	}
LEAVE_FUNC;
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

ENTER_FUNC;
	if		(  ( name = HTCGetProp(tag,"name",0) )  !=  NULL  ) {
        if ((visible = HTCGetProp(tag, "visible", 0)) != NULL &&
            !IsTrue(visible)) {
            LBS_EmitString(htc->code,"<input type=\"password\" name=\"");
        }
        else {
            LBS_EmitString(htc->code,"<input type=\"text\" name=\"");
        }

		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_EMITSTR);

		LBS_EmitString(htc->code,"\" value=\"");

		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_PHSTR);
		EmitCode(htc,OPC_EMITSAFE);

		LBS_EmitString(htc->code,"\"");
		if		(  ( size = HTCGetProp(tag,"size",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code," size=\"");
			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(size));
			EmitCode(htc,OPC_EMITSAFE);
			LBS_EmitString(htc->code,"\"");

		}
		if		(  ( maxlength = HTCGetProp(tag,"maxlength",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code," maxlength=\"");

			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(maxlength));
			EmitCode(htc,OPC_EMITSTR);
			LBS_EmitString(htc->code,"\"");

		}
        JavaScriptEvent(htc, tag, "onchange");
        JavaScriptKeyEvent(htc, tag, "onkeydown");
        JavaScriptKeyEvent(htc, tag, "onkeyup");
		Style(htc,tag);
		LBS_EmitString(htc->code,">\n");
	}
LEAVE_FUNC;
}

static	void
_Variable(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name
		,	*type
		,	*expire
		,	*secure
		,	*value;
	CookieEntry	*ent;
	struct	tm	tm_exp;
	time_t	time_exp;

ENTER_FUNC;
	if		(  ( name = HTCGetProp(tag,"name",0) )  !=  NULL  ) {
		if		(	(  ( type = HTCGetProp(tag,"type",0) )  ==  NULL  )
				||	(  stricmp(type,"form")                 ==  0     ) ) {
			LBS_EmitString(htc->code,"<input type=\"hidden\" name=\"");
			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(name));
			EmitCode(htc,OPC_EMITSTR);

			LBS_EmitString(htc->code,"\" value=\"");

			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(name));
			EmitCode(htc,OPC_PHSTR);
			EmitCode(htc,OPC_EMITRAW);

			LBS_EmitString(htc->code,"\">\n");
		} else
		if		(  strlicmp(type,"cookie")  ==  0  ) {
			if		(  ( ent = g_hash_table_lookup(htc->Cookie,name) )  ==  NULL  ) {
				ent = New(CookieEntry);
				ent->name = StrDup(name);
				ent->expire = NULL;
				ent->domain = NULL;
				ent->value = NULL;
				ent->path = NULL;
				ent->fSecure = FALSE;
				g_hash_table_insert(htc->Cookie,ent->name,ent);
			}
			if		(  ( value = HTCGetProp(tag,"value",0) )  !=  NULL  ) {
				if		(  ent->value  !=  NULL  ) {
					xfree(ent->value);
				}
				ent->value = StrDup(value);
			}
			ent->domain = HTCGetProp(tag,"domain",0);
			ent->path = HTCGetProp(tag,"path",0);
			if		(	(  ( secure = HTCGetProp(tag,"secure",0) )  !=  NULL  )
					&&	(  stricmp(secure,"true")                   ==  0  ) ) {
				ent->fSecure = TRUE;
			} else {
				ent->fSecure = FALSE;
			}
			if		(  ( expire = HTCGetProp(tag,"expires",0) )  !=  NULL  ) {
				strptime(expire,"%Y-%m-%d %H:%M:%S",&tm_exp);
				tm_exp.tm_year += 1900;
				time_exp = mktime(&tm_exp);
				ent->expire = New(time_t);
				*ent->expire = time_exp;
			} else {
				ent->expire = NULL;
			}
		}
	}
LEAVE_FUNC;
}

static	void
_Fixed(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name
		,	*link
		,	*target
		,	*value
		,	*type;
	int		dtype;

#define	DTYPE_RAW		0
#define	DTYPE_DIV		1
#define	DTYPE_SPAN		2
#define	DTYPE_PRE		3
#define	DTYPE_SAFE		4

ENTER_FUNC;
	value = HTCGetProp(tag,"value",0);
	name = HTCGetProp(tag,"name",0);
	type = HTCGetProp(tag,"type",0);
	if		(	(  value  !=  NULL  )
			||	(  name   !=  NULL  ) ) {
		if		(  type  !=  NULL  ) {
			if		(  !stricmp(type,"html")  ) {
				LBS_EmitString(htc->code,"<div");
				Style(htc,tag);
				LBS_EmitString(htc->code,">\n");
				dtype = DTYPE_DIV;
			} else
			if		(  !stricmp(type,"raw")  ) {
				dtype = DTYPE_RAW;
			} else
			if		(  !stricmp(type,"pre")  ) {
				LBS_EmitString(htc->code,"<pre");
				Style(htc,tag);
				LBS_EmitString(htc->code,">");
				dtype = DTYPE_PRE;
			} else
			if		(  !stricmp(type,"safe")  ) {
				LBS_EmitString(htc->code,"<span");
				Style(htc,tag);
				LBS_EmitString(htc->code,">");
				dtype = DTYPE_SAFE;
			} else {
				LBS_EmitString(htc->code,"<span");
				Style(htc,tag);
				LBS_EmitString(htc->code,">");
				dtype = DTYPE_SPAN;
			}
		} else {
			LBS_EmitString(htc->code,"<span");
			Style(htc,tag);
			LBS_EmitString(htc->code,">");
			dtype = DTYPE_SPAN;
		}
		if		(  ( link = HTCGetProp(tag,"link",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code,"<a href=\"");

			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(link));
			EmitCode(htc,OPC_PHSTR);
			EmitCode(htc,OPC_EMITSTR);

			LBS_EmitString(htc->code,"\"");
			if		(  ( target = HTCGetProp(tag,"target",0) )  !=  NULL  ) {
				LBS_EmitString(htc->code," target=\"");
				LBS_EmitString(htc->code,target);
				LBS_EmitString(htc->code,"\"");
			}
			LBS_EmitString(htc->code,">");
		}
		if		(  value  !=  NULL  ) {
			EmitCode(htc,OPC_SCONST);
			LBS_EmitPointer(htc->code,StrDup(value));
		} else {
			if		(	(  name[0]  ==  '$'  )
					||	(  name[0]  ==  '#'  ) ) {
				EmitCode(htc,OPC_EVAL);
				LBS_EmitPointer(htc->code,StrDup(name));
			} else {
				EmitCode(htc,OPC_EVAL);
				LBS_EmitPointer(htc->code,StrDup(name));
				EmitCode(htc,OPC_PHSTR);
			}
		}
		switch	(dtype) {
		  case	DTYPE_RAW:
		  case	DTYPE_DIV:
			EmitCode(htc,OPC_EMITRAW);
			break;
		  case	DTYPE_SAFE:
			EmitCode(htc,OPC_EMITSAFE);
			break;
		  case	DTYPE_PRE:
		  default:
			EmitCode(htc,OPC_EMITSTR);
			break;
		}
		if		(  link  !=  NULL  ) {
			LBS_EmitString(htc->code,"</a>");
		}
		switch	(dtype) {
		  case	DTYPE_RAW:
			break;
		  case	DTYPE_DIV:
			LBS_EmitString(htc->code,"</div>");
			break;
		  case	DTYPE_SPAN:
		  case	DTYPE_SAFE:
			LBS_EmitString(htc->code,"</span>\n");
			break;
		  case	DTYPE_PRE:
			LBS_EmitString(htc->code,"</pre>");
			break;
		}
	}
LEAVE_FUNC;
}

static	void
_Combo(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	name[SIZE_LONGNAME+1];
	char	*size
		,	*vname;
	size_t	pos;
	int		arg;

ENTER_FUNC;
	LBS_EmitString(htc->code,"<select name=\"");
	EmitCode(htc,OPC_EVAL);
	vname = StrDup(HTCGetProp(tag,"name",0));
	LBS_EmitPointer(htc->code,vname);
	EmitCode(htc,OPC_EMITSTR);
	size = HTCGetProp(tag,"size",0);
	if		(  size  ==  NULL  ) {
		LBS_EmitString(htc->code,"\"");
	} else {
		LBS_EmitString(htc->code,"\" size=");
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"size",0)));
		EmitCode(htc,OPC_EMITSTR);
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
	LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"count",0)));
	EmitCode(htc,OPC_ICONST);							/*	1	step	*/
	LBS_EmitInt(htc->code,1);
	Push(LBS_GetPos(htc->code));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,0);

	LBS_EmitString(htc->code,"<option ");
	/*	selected	*/
	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,vname);
	EmitCode(htc,OPC_PHSTR);
	EmitCode(htc,OPC_EVAL);
	sprintf(name,"%s[#]",HTCGetProp(tag,"item",0));
	LBS_EmitPointer(htc->code,StrDup(name));
	EmitCode(htc,OPC_PHSTR);
	EmitCode(htc,OPC_SCMP);
	EmitCode(htc,OPC_JNZNP);
	arg = LBS_GetPos(htc->code);
	LBS_EmitInt(htc->code,0);
	EmitCode(htc,OPC_SCONST);
	LBS_EmitPointer(htc->code,"selected");
	EmitCode(htc,OPC_EMITSTR);
	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,arg);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
	Style(htc,tag);
	LBS_EmitString(htc->code,">");

	EmitCode(htc,OPC_EVAL);
	sprintf(name,"%s[#]",HTCGetProp(tag,"item",0));
	LBS_EmitPointer(htc->code,StrDup(name));
	EmitCode(htc,OPC_PHSTR);
	EmitCode(htc,OPC_EMITSTR);
	LBS_EmitString(htc->code,"</option>\n");

	EmitCode(htc,OPC_LEND);
	LBS_EmitInt(htc->code,Pop);
	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,TOP(0));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
	LBS_EmitString(htc->code,"</select>");
LEAVE_FUNC;
}

static	void
_Form(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name
		,	*defaultevent
		,	*target;

ENTER_FUNC;
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
    EmitCode(htc, OPC_EMITSTR);
    LBS_EmitString(htc->code, "\"");
	Style(htc,tag);
	if		(  ( name = HTCGetProp(tag,"name",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," name=\"");
		LBS_EmitString(htc->code,name);
		LBS_EmitString(htc->code,"\"");
	}
	if		(  ( target = HTCGetProp(tag,"target",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," target=\"");
		LBS_EmitString(htc->code,target);
		LBS_EmitString(htc->code,"\"");
	}
	LBS_EmitString(htc->code,">\n");

    /* document.forms[htc->FormNo].elements[0] for _event */
	LBS_EmitString(htc->code,
				   "\n<input type=\"hidden\" name=\"__event__\" value=\"\">");

    /* document.forms[htc->FormNo].elements[1] for event data */
	LBS_EmitString(htc->code,
				   "\n<input type=\"hidden\" name=\"__data__\" value=\"\">");

	LBS_EmitString(htc->code,
				   "\n<input type=\"hidden\" name=\"_name\" value=\"");
	EmitGetValue(htc,"_name");
	LBS_EmitString(htc->code,"\">");
	if		(  !fCookie  ) {
		LBS_EmitString(htc->code,
					   "\n<input type=\"hidden\" name=\"_sesid\" value=\"");
		EmitGetValue(htc,"_sesid");
		LBS_EmitString(htc->code,"\">");
	}

    if ((defaultevent = HTCGetProp(tag, "defaultevent", 0)) != NULL) {
        htc->DefaultEvent = StrDup(defaultevent);
	}
LEAVE_FUNC;
}

static	void
_Text(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name
		,	*cols
		,	*rows
		,	*type;
ENTER_FUNC;
	LBS_EmitString(htc->code,"<textarea");
#ifdef	USE_MCE
	if		(	(  ( type = HTCGetProp(tag,"type",0) )  ==  NULL  )
			||	(  !stricmp(type,"text")                      ) ) {
	} else {
		if		(  fJavaScript  ) {
			InvokeJs("html_edit");
#if	0
			InvokeJs("html_setting");
#endif
			LBS_EmitString(htc->code," mce_editable=\"true\"");
			if		(  !stricmp(type,"xml")  ) {
				SetFilter(HTCGetProp(tag,"name",0),(byte *(*)(byte *))StrDup,NULL);
			}
		}
	}
#endif

	LBS_EmitString(htc->code," name=\"");
	EmitCode(htc,OPC_EVAL);
	name = StrDup(HTCGetProp(tag,"name",0));
	LBS_EmitPointer(htc->code,name);
	EmitCode(htc,OPC_EMITSTR);
	LBS_EmitString(htc->code,"\"");
	if		(  ( cols = HTCGetProp(tag,"cols",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," cols=");
		EmitAttributeValue(htc,cols,TRUE,FALSE,FALSE);
	}
	if		(  ( rows = HTCGetProp(tag,"rows",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," rows=");
		EmitAttributeValue(htc,rows,TRUE,FALSE,FALSE);
	}
    JavaScriptEvent   (htc, tag, "onchange");
    JavaScriptKeyEvent(htc, tag, "onkeydown");
    JavaScriptKeyEvent(htc, tag, "onkeyup");
	Style(htc,tag);
	LBS_EmitString(htc->code,">\n");

	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,name);
	EmitCode(htc,OPC_PHSTR);
	if		(	(  ( type = HTCGetProp(tag,"type",0) )  ==  NULL  )
			||	(  !stricmp(type,"text")                      ) ) {
		EmitCode(htc,OPC_EMITSAFE);
	} else {
		EmitCode(htc,OPC_EMITRAW);
	}
	LBS_EmitString(htc->code,"</textarea>\n");
LEAVE_FUNC;
}

static	void
_Head(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*js;

ENTER_FUNC;
	if		(  ( js = HTCGetProp(tag,"javascript",0) )  !=  NULL  ) {
		if		(	(  stricmp(js,"no")     ==  0  )
				||	(  stricmp(js,"off")    ==  0  )
				||	(  stricmp(js,"false")  ==  0  ) ) {
			fNoJavaScript = TRUE;
		}
	}
	if		(  !fNoHeader  ) {
		LBS_EmitString(htc->code,"<head");
		EmitAttribute(htc,tag,"profile");
		EmitAttribute(htc,tag,"class");
		EmitAttribute(htc,tag,"id");
		EmitAttribute(htc,tag,"dir");
		EmitAttribute(htc,tag,"lang");
		LBS_EmitString(htc->code,">\n");
		LBS_EmitString(htc->code,
					   "<meta http-equiv=\"Pragma\" content=\"no-cache\">\n");
		fHead = TRUE;
	}
	EmitCode(htc,OPC_FLJS);
LEAVE_FUNC;
}

static	void
_Body(
	HTCInfo	*htc,
	Tag		*tag)
{
ENTER_FUNC;
	if		(  !fHead  ) {
		if		(  !fNoHeader  ) {
			LBS_EmitString(htc->code,
						   "<head>\n<meta http-equiv=\"Pragma\" content=\"no-cache\">\n");
		}
		EmitCode(htc,OPC_FLJS);
		fHead = TRUE;
	}
	if		(  !fNoHeader  ) {
		LBS_EmitString(htc->code,"</head>\n");
		LBS_EmitString(htc->code,"<body");
		EmitAttribute(htc,tag,"text");
		EmitAttribute(htc,tag,"link");
		EmitAttribute(htc,tag,"vlink");
		EmitAttribute(htc,tag,"alink");
		EmitAttribute(htc,tag,"bgcolor");
		EmitAttribute(htc,tag,"background");
		EmitAttribute(htc,tag,"bgproperties");
		EmitAttribute(htc,tag,"marginheight");
		EmitAttribute(htc,tag,"marginwidth");
		EmitAttribute(htc,tag,"topmargin");
		EmitAttribute(htc,tag,"leftmargin");
		EmitAttribute(htc,tag,"bottommargin");
		EmitAttribute(htc,tag,"rightmargin");
		EmitAttribute(htc,tag,"scroll");
		EmitAttribute(htc,tag,"dir");
		EmitAttribute(htc,tag,"lang");
		EmitAttribute(htc,tag,"title");
		JavaScriptEvent(htc,tag,"oncontextmenu");
		JavaScriptEvent(htc,tag,"onload");
		Style(htc,tag);
		LBS_EmitString(htc->code,">\n");
	}
LEAVE_FUNC;
}

static	void
_eBody(
	HTCInfo	*htc,
	Tag		*tag)
{
ENTER_FUNC;
	if		(  fDebug  ) {
		LBS_EmitString(htc->code,"<span>");
		LBS_EmitString(htc->code,__DATE__);
		LBS_EmitString(htc->code,__TIME__);
		LBS_EmitString(htc->code,"</span>");
	}
	if		(  !fDump  ) {
		if		( !fNoHeader  ) {
			LBS_EmitString(htc->code,"</body>");
		}
	}
LEAVE_FUNC;
}	

static	void
_eHtml(
	HTCInfo	*htc,
	Tag		*tag)
{
ENTER_FUNC;
	if		(  !fDump  ) {
		if		(  !fNoHeader  ) {
			LBS_EmitString(htc->code,"</html>");
		}
	}
	LBS_EmitEnd(htc->code);
LEAVE_FUNC;
}

static	void
_Count(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*from
	,		*to
	,		*step;
	size_t	pos;

ENTER_FUNC;
	EmitCode(htc,OPC_VAR);
	LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"var",0)));		/*	3	var		*/
	if		(  ( from = HTCGetProp(tag,"from",0) )  !=  NULL  ) {
		if		(  isdigit(*from)  ) {
			EmitCode(htc,OPC_ICONST);
			LBS_EmitInt(htc->code,atoi(from));
		} else {
			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(from));
			EmitCode(htc,OPC_PHINT);
		}
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,0);
	}
	EmitCode(htc,OPC_STORE);

	if		(  ( to = HTCGetProp(tag,"to",0) )  !=  NULL  ) {
		if		(  isdigit(*to)  ) {
			EmitCode(htc,OPC_ICONST);
			LBS_EmitInt(htc->code,atoi(to));
		} else {
			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(to));
			EmitCode(htc,OPC_PHINT);
		}
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,0);
	}													/*	2	limit	*/
	if		(  ( step = HTCGetProp(tag,"step",0) )  !=  NULL  ) {
		if		(  isdigit(*step)  ) {
			EmitCode(htc,OPC_ICONST);
			LBS_EmitInt(htc->code,atoi(step));
		} else {
			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(step));
			EmitCode(htc,OPC_PHINT);
		}
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,1);
	}													/*	1	step	*/
	pos = LBS_GetPos(htc->code);
	dbgprintf("pos = %d\n",pos);
	Push(pos);
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,0);
LEAVE_FUNC;
}

static	void
_eCount(
	HTCInfo	*htc,
	Tag		*tag)
{
	size_t	pos;
ENTER_FUNC;
	EmitCode(htc,OPC_LEND);
	LBS_EmitInt(htc->code,Pop);
	pos = LBS_GetPos(htc->code);
	dbgprintf("pos = %d\n",pos);
	dbgprintf("TOP = %d\n",TOP(0));
	LBS_SetPos(htc->code,TOP(0));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
LEAVE_FUNC;
}

static	void
_Button(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*face
		,	*event
		,	*size;
	char	buf[SIZE_BUFF];
	char	*state;
	int		arg
		,	pos;

ENTER_FUNC;
	if		(  ( state = HTCGetProp(tag,"state",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(state));
		EmitCode(htc,OPC_PHINT);
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,0);
	}
	EmitCode(htc,OPC_ICONST);
	LBS_EmitInt(htc->code,0);
	EmitCode(htc,OPC_SUB);
	EmitCode(htc,OPC_JNZNP);
	arg = LBS_GetPos(htc->code);
	LBS_EmitInt(htc->code,0);

	if		(  HTCGetProp(tag,"onclick",0)  !=  NULL  ) {
		fButton = TRUE;
		LBS_EmitString(htc->code,"<button");
        JavaScriptEvent(htc, tag, "onclick");
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
		if ((face = HTCGetProp(tag, "face", 0)) == NULL) {
			face = "click";
		}
		LBS_EmitString(htc->code,face);
	} else {
		fButton = FALSE;
		event = HTCGetProp(tag, "event", 0);
		if		(  event  ==  NULL  ) {
			HTC_Error("`event' or `on' attribute is required for <%s>\n", tag->name);
			return;
		}
		if ((face = HTCGetProp(tag, "face", 0)) == NULL) {
			face = event;
		}
#if	0
		if (htc->DefaultEvent == NULL) {
			htc->DefaultEvent = event;
		}
#endif
#ifdef	USE_IE5
		if (event != NULL) {
			g_hash_table_insert(htc->Trans,StrDup(face),StrDup(event));
			LBS_EmitString(htc->code,
						   "<input type=\"submit\" name=\"_event\" value=\"");
		} else {
			LBS_EmitString(htc->code,
						   "<input type=\"button\" value=\"");
		}
		LBS_EmitString(htc->code,face);
		LBS_EmitString(htc->code,"\"");
		if		(  ( size = HTCGetProp(tag,"size",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code," size=");
			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(size));
			EmitCode(htc,OPC_EMITSTR);
		}
#else
		if		(	(  fJavaScript     )
				&&	(  !fNoJavaScript  ) ) {
			LBS_EmitString(htc->code,"<input type=\"button\" value=\"");
			LBS_EmitString(htc->code,face);
			LBS_EmitString(htc->code,"\"");
			LBS_EmitChar(htc->code, ' ');
			InvokeJs("send_event");
			LBS_EmitString(htc->code, "onclick");
			snprintf(buf, SIZE_BUFF,
					 "=\""
					 "send_event(%d,'%s');\"",htc->FormNo,event);
			LBS_EmitString(htc->code, buf);
		} else {
			g_hash_table_insert(htc->Trans,StrDup(face),StrDup(event));
			LBS_EmitString(htc->code,
						   "<input type=\"submit\" name=\"_event\" value=\"");
			LBS_EmitString(htc->code,face);
			LBS_EmitString(htc->code,"\"");
			if		(  ( size = HTCGetProp(tag,"size",0) )  !=  NULL  ) {
				LBS_EmitString(htc->code," size=");
				EmitCode(htc,OPC_EVAL);
				LBS_EmitPointer(htc->code,StrDup(size));
				EmitCode(htc,OPC_EMITSTR);
			}
		}
#endif
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
	}

	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,arg);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
LEAVE_FUNC;
}

static	void
_eButton(
	HTCInfo	*htc,
	Tag		*tag)
{
#ifndef	USE_IE5
	if		(  fButton  ) {
		LBS_EmitString(htc->code,"</button>\n");
	}
#endif
}

static	void
_Span(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*state;
	int		arg
		,	pos;

ENTER_FUNC;
	if		(  ( state = HTCGetProp(tag,"state",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(state));
		EmitCode(htc,OPC_PHINT);
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,0);
	}
	EmitCode(htc,OPC_ICONST);
	LBS_EmitInt(htc->code,0);
	EmitCode(htc,OPC_SUB);
	EmitCode(htc,OPC_JNZNP);
	arg = LBS_GetPos(htc->code);
	LBS_EmitInt(htc->code,0);
	LBS_EmitString(htc->code,"<span");
	JavaScriptEvent(htc, tag, "onclick");
	JavaScriptEvent(htc, tag, "onchange");
	JavaScriptEvent(htc, tag, "onkeydown");
	JavaScriptEvent(htc, tag, "onkeypress");
	JavaScriptEvent(htc, tag, "onkeyup");
	JavaScriptEvent(htc, tag, "onmouseover");
	JavaScriptEvent(htc, tag, "onmousedown");
	JavaScriptEvent(htc, tag, "onmousemove");
	JavaScriptEvent(htc, tag, "onmouseout");
	JavaScriptEvent(htc, tag, "onmousesetup");
	Style(htc,tag);
	LBS_EmitString(htc->code,">");
	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,arg);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
LEAVE_FUNC;
}

static	void
_A(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*state
		,	*name
		,	*href
		,	*target;
	int		arg
		,	pos;

ENTER_FUNC;
	if		(  ( state = HTCGetProp(tag,"state",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(state));
		EmitCode(htc,OPC_PHINT);
	} else {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,0);
	}
	EmitCode(htc,OPC_ICONST);
	LBS_EmitInt(htc->code,0);
	EmitCode(htc,OPC_SUB);
	EmitCode(htc,OPC_JNZNP);
	arg = LBS_GetPos(htc->code);
	LBS_EmitInt(htc->code,0);
	LBS_EmitString(htc->code,"<a");
	JavaScriptEvent(htc, tag, "onclick");
	JavaScriptEvent(htc, tag, "onchange");
	JavaScriptEvent(htc, tag, "onkeydown");
	JavaScriptEvent(htc, tag, "onkeypress");
	JavaScriptEvent(htc, tag, "onkeyup");
	JavaScriptEvent(htc, tag, "onmouseover");
	JavaScriptEvent(htc, tag, "onmousedown");
	JavaScriptEvent(htc, tag, "onmousemove");
	JavaScriptEvent(htc, tag, "onmouseout");
	JavaScriptEvent(htc, tag, "onmousesetup");
	Style(htc,tag);
	if		(  ( name = HTCGetProp(tag,"name",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," name=");
		ExpandAttributeString(htc,name);
	}
	if		(  ( href = HTCGetProp(tag,"href",0) )  !=  NULL  ) {
		if		(  strlicmp(href,"javascript:")  ==  0  ) {
			JavaScriptEvent(htc, tag, "href");
		} else {
			LBS_EmitString(htc->code," href=");
			ExpandAttributeString(htc,href);
		}
	}
	if		(  ( target = HTCGetProp(tag,"target",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," target=");
		ExpandAttributeString(htc,target);
	}
	LBS_EmitString(htc->code,">");
	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,arg);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
LEAVE_FUNC;
}

static	void
_ToggleButton(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*label
		,	*name;

ENTER_FUNC;
	if		(  ( name = HTCGetProp(tag,"name",0) )  !=  NULL  ) {
		if		(  g_hash_table_lookup(htc->Toggle,name)  ==  NULL  ) {
			g_hash_table_insert(htc->Toggle,StrDup(name),(void *)1);
		}
		LBS_EmitString(htc->code,"<input type=\"checkbox\" name=\"");
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_EMITSTR);
		LBS_EmitString(htc->code,"\" ");

		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_HBES);
		LBS_EmitPointer(htc->code,"checked");

		LBS_EmitString(htc->code," value=\"TRUE\"");
		JavaScriptEvent(htc, tag, "onclick");
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
		if		(  ( label = HTCGetProp(tag,"label",0) )  !=  NULL  ) {
			if		(  *label  ==  '$'  ) {
				EmitCode(htc,OPC_EVAL);
				LBS_EmitPointer(htc->code,StrDup(label+1));
				EmitCode(htc,OPC_PHSTR);
				EmitCode(htc,OPC_EMITSTR);
			} else {
				LBS_EmitString(htc->code,label);
			}
		}
	}
LEAVE_FUNC;
}

static	void
_CheckButton(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*label
		,	*name;

ENTER_FUNC;
	if		(  ( name = HTCGetProp(tag,"name",0) )  !=  NULL  ) {
		if		(  g_hash_table_lookup(htc->Check,name)  ==  NULL  ) {
			g_hash_table_insert(htc->Check,StrDup(name),(void *)1);
		}
		LBS_EmitString(htc->code,"<input type=\"checkbox\" name=\"");
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_EMITSTR);
		LBS_EmitString(htc->code,"\" ");
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(name));
		EmitCode(htc,OPC_HBES);
		LBS_EmitPointer(htc->code,"checked");
		LBS_EmitString(htc->code," value=\"TRUE\"");
		JavaScriptEvent(htc, tag, "onclick");
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
		if		(  ( label = HTCGetProp(tag,"label",0) )  !=  NULL  ) {
			if		(  *label  ==  '$'  ) {
				EmitCode(htc,OPC_EVAL);
				LBS_EmitPointer(htc->code,StrDup(label+1));
				EmitCode(htc,OPC_PHSTR);
				EmitCode(htc,OPC_EMITSTR);
			} else {
				LBS_EmitString(htc->code,label);
			}
		}
	}
LEAVE_FUNC;
}

static	void
_RadioButton(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*group
		,	*name
		,	*label;
	GHashTable	*ritem;

ENTER_FUNC;
	group = HTCGetProp(tag,"group",0); 
	name = HTCGetProp(tag,"name",0); 
	if		(  ( ritem = g_hash_table_lookup(htc->Radio,group) )  ==  NULL  ) {
		ritem = NewNameHash();
		g_hash_table_insert(htc->Radio,StrDup(group),ritem);
	}
	if		(  g_hash_table_lookup(ritem,name)  ==  NULL  ) {
		g_hash_table_insert(ritem,StrDup(name),(void *)1);
	}

	LBS_EmitString(htc->code,"<input type=\"radio\" name=\"");
	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,StrDup(group));
	EmitCode(htc,OPC_EMITSTR);
	LBS_EmitString(htc->code,"\" ");

	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"name",0)));
	EmitCode(htc,OPC_HBES);
	LBS_EmitPointer(htc->code,"checked");

	LBS_EmitString(htc->code," value=\"");
	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,StrDup(name));
	EmitCode(htc,OPC_EMITSTR);
	LBS_EmitString(htc->code,"\"");
	JavaScriptEvent(htc, tag, "onclick");

	Style(htc,tag);
	LBS_EmitString(htc->code,">");
	if		(  ( label = HTCGetProp(tag,"label",0) )  !=  NULL  ) {
		if		(  *label  ==  '$'  ) {
			EmitCode(htc,OPC_EVAL);
			LBS_EmitPointer(htc->code,StrDup(label+1));
			EmitCode(htc,OPC_PHSTR);
			EmitCode(htc,OPC_EMITSTR);
		} else {
			LBS_EmitString(htc->code,label);
		}
	}

LEAVE_FUNC;
}

static void
_List(HTCInfo *htc, Tag *tag)
{
	char	buf[SIZE_LONGNAME+1];
	char	*name, *label, *count, *size, *multiple, *value;
	size_t	pos;

ENTER_FUNC;
    if ((name = HTCGetProp(tag,"name",0)) != NULL &&
        (label = HTCGetProp(tag,"label",0)) != NULL &&
        (count = HTCGetProp(tag,"count",0)) != NULL) {
        LBS_EmitString(htc->code,"<select name=\"");
        EmitCode(htc,OPC_EVAL);
        LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"name",0)));
        EmitCode(htc,OPC_EMITSTR);
        LBS_EmitString(htc->code,"\"");
        if ((size = HTCGetProp(tag,"size",0)) != NULL) {
            LBS_EmitString(htc->code," size=\"");
            EmitCode(htc,OPC_EVAL);
            LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"size",0)));
            EmitCode(htc,OPC_EMITSTR);
            LBS_EmitString(htc->code,"\"");
        }
        if ((multiple = HTCGetProp(tag,"multiple",0)) != NULL &&
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
        if ((value = HTCGetProp(tag,"value",0)) != NULL) {
			EmitCode(htc,OPC_EVAL);
			sprintf(buf,"%s[#]",value);
			LBS_EmitPointer(htc->code,StrDup(buf));
			EmitCode(htc,OPC_PHSTR);
			EmitCode(htc,OPC_EMITSTR);
		} else {
			EmitCode(htc,OPC_LDVAR);
			LBS_EmitPointer(htc->code,"");
			EmitCode(htc,OPC_REFINT);
		}
        LBS_EmitString(htc->code,"\"");

        EmitCode(htc,OPC_EVAL);
        sprintf(buf,"%s[#] ",name);
        LBS_EmitPointer(htc->code,StrDup(buf));
        EmitCode(htc,OPC_HBES);
        LBS_EmitPointer(htc->code,"selected");
        LBS_EmitString(htc->code,">");

        EmitCode(htc,OPC_EVAL);
        sprintf(buf,"%s[#]",label);
        LBS_EmitPointer(htc->code,StrDup(buf));
		EmitCode(htc,OPC_PHSTR);
		EmitCode(htc,OPC_EMITSTR);
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
LEAVE_FUNC;
}

static void
_Optionmenu(HTCInfo *htc, Tag *tag)
{
	char	buf[SIZE_LONGNAME+1];
	char	*item, *count, *sel;
	size_t	pos;

ENTER_FUNC;
	if		(	(  ( item  = HTCGetProp(tag,"item",0) )    !=  NULL  )
			&&	(  ( sel   = HTCGetProp(tag,"select",0) )  !=  NULL  )
			&&	(  ( count = HTCGetProp(tag,"count",0) )   !=  NULL  ) ) {
        LBS_EmitString(htc->code,"<select name=\"");
        EmitCode(htc,OPC_EVAL);
        LBS_EmitPointer(htc->code,StrDup(sel));
        EmitCode(htc,OPC_EMITSTR);
        LBS_EmitString(htc->code,"\" size=\"1\"");
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
		LBS_EmitPointer(htc->code,NULL);
		EmitCode(htc,OPC_REFINT);
        LBS_EmitString(htc->code,"\" ");

		EmitCode(htc,OPC_LDVAR);
		LBS_EmitPointer(htc->code,NULL);
        EmitCode(htc,OPC_EVAL);
        LBS_EmitPointer(htc->code,StrDup(sel));
		EmitCode(htc,OPC_PHSTR);
		EmitCode(htc,OPC_TOINT);
		EmitCode(htc,OPC_SUB);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);

		EmitCode(htc,OPC_SCONST);
        LBS_EmitPointer(htc->code,"selected");
		EmitCode(htc,OPC_EMITSTR);

		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);

        LBS_EmitString(htc->code,">");

        EmitCode(htc,OPC_EVAL);
        sprintf(buf,"%s[#]",item);
        LBS_EmitPointer(htc->code,StrDup(buf));
		EmitCode(htc,OPC_PHSTR);
		EmitCode(htc,OPC_EMITSTR);
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
LEAVE_FUNC;
}

static void
_Select(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*name
		,	*size
		,	*multiple
		,	*type;
	char	buf[SIZE_BUFF];

ENTER_FUNC;
	if		(  ( name = HTCGetProp(tag,"name",0) )  !=  NULL  ) {
        LBS_EmitString(htc->code,"<select name=\"");
        EmitCode(htc,OPC_EVAL);
        LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"name",0)));
        EmitCode(htc,OPC_EMITSTR);
        LBS_EmitString(htc->code,"\"");
        if ((size = HTCGetProp(tag,"size",0)) != NULL) {
            LBS_EmitString(htc->code," size=\"");
            EmitCode(htc,OPC_EVAL);
            LBS_EmitPointer(htc->code,StrDup(HTCGetProp(tag,"size",0)));
            EmitCode(htc,OPC_EMITSTR);
            LBS_EmitString(htc->code,"\"");
        }
        if ((multiple = HTCGetProp(tag,"multiple",0)) != NULL &&
            IsTrue(multiple)) {
            LBS_EmitString(htc->code," multiple");
        }
		if		(	(  ( type = HTCGetProp(tag,"type",0) )  !=  NULL  )
				&&	(  !stricmp(type,"menu")                      ) ) {
			if		(	(  fJavaScript     )
					&&	(  !fNoJavaScript  ) ) {
				InvokeJs("send_event");
				LBS_EmitString(htc->code, "onchange");
				snprintf(buf, SIZE_BUFF,
						 "=\""
						 "send_event(%d,this.options[this.selectedIndex].value);\"",
						 htc->FormNo);
				LBS_EmitString(htc->code, buf);
			}
		} else {
			JavaScriptEvent(htc, tag, "onchange");
		}
        Style(htc,tag);
        LBS_EmitString(htc->code,">\n");
    }
LEAVE_FUNC;
}

static	void
_Option(
	HTCInfo	*htc,
	Tag	*tag)
{
	char	*select
		,	*value;

ENTER_FUNC;
	LBS_EmitString(htc->code,"<option");
	if		(  ( value = HTCGetProp(tag,"value",0) )  !=  NULL  )	{
		LBS_EmitString(htc->code," value=");
		ExpandAttributeString(htc,value);
		LBS_EmitString(htc->code," ");
	}
	if		(  ( select = HTCGetProp(tag,"select",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,StrDup(select));
		EmitCode(htc,OPC_HBES);
		LBS_EmitPointer(htc->code,"selected");
	}
	Style(htc,tag);
	LBS_EmitString(htc->code,">");
LEAVE_FUNC;
}

static	void
_FileSelection(HTCInfo *htc, Tag *tag)
{
	char *name, *filename, *size, *maxlength;

ENTER_FUNC;
    if ((name = HTCGetProp(tag, "name", 0)) != NULL &&
        (filename = HTCGetProp(tag, "filename", 0)) != NULL) {
		LBS_EmitString(htc->code, "<input type=\"file\" name=\"");
		EmitCode(htc, OPC_EVAL);
		LBS_EmitPointer(htc->code, StrDup(name));
		EmitCode(htc, OPC_EMITSTR);

		LBS_EmitString(htc->code, "\" value=\"");
		EmitCode(htc, OPC_EVAL);
		LBS_EmitPointer(htc->code, StrDup(filename));
		EmitCode(htc,OPC_PHSTR);
		EmitCode(htc,OPC_EMITSTR);
        LBS_EmitString(htc->code, "\"");

		if ((size = HTCGetProp(tag, "size", 0)) != NULL) {
			LBS_EmitString(htc->code, " size=\"");
			EmitCode(htc, OPC_EVAL);
			LBS_EmitPointer(htc->code, StrDup(size));
			EmitCode(htc, OPC_EMITSTR);
			LBS_EmitString(htc->code, "\"");
		}

        if ((maxlength = HTCGetProp(tag, "maxlength", 0)) != NULL) {
			LBS_EmitString(htc->code, " maxlength=\"");
			EmitCode(htc, OPC_EVAL);
			LBS_EmitPointer(htc->code, StrDup(maxlength));
			EmitCode(htc, OPC_EMITSTR);
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
LEAVE_FUNC;
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
    char *disposition;
    char *target;
    char buf[SIZE_BUFF];

ENTER_FUNC;
	if ((event = HTCGetProp(tag, "event", 0)) != NULL) {
		LBS_EmitString(htc->code, "<a");
		Style(htc,tag);

		LBS_EmitString(htc->code, " href=\"");

		if (fJavaScriptLink && fJavaScript && htc->FormNo >= 0 &&
            (file = HTCGetProp(tag, "file", 0)) == NULL) {
            LBS_EmitString(htc->code, "javascript:");
            snprintf(buf, SIZE_BUFF,
					 "send_event(%d,'",htc->FormNo);
            LBS_EmitString(htc->code, buf);
            EmitCode(htc, OPC_EVAL);
            LBS_EmitPointer(htc->code, StrDup(event));
            EmitCode(htc, OPC_ESCJSS);
            EmitCode(htc, OPC_EMITSTR);
            LBS_EmitString(htc->code, "';");

            if ((name = HTCGetProp(tag, "name", 0)) != NULL &&
                (value = HTCGetProp(tag, "value", 0)) != NULL) {
                snprintf(buf, SIZE_BUFF,
                         "document.forms[%d].elements[1].name='", htc->FormNo);
                LBS_EmitString(htc->code, buf);
                EmitCode(htc, OPC_EVAL);
                LBS_EmitPointer(htc->code, StrDup(name));
                EmitCode(htc, OPC_ESCJSS);
                EmitCode(htc, OPC_EMITSTR);
                LBS_EmitString(htc->code, "';");
                snprintf(buf, SIZE_BUFF,
                         "document.forms[%d].elements[1].value='", htc->FormNo);

                LBS_EmitString(htc->code, buf);
                EmitAttributeValue(htc, value, FALSE, FALSE, TRUE); 
                LBS_EmitString(htc->code, "';");
            }

            snprintf(buf, SIZE_BUFF,
                     "document.forms[%d].submit();\"", htc->FormNo);
            LBS_EmitString(htc->code, buf);
        } else {
            LBS_EmitString(htc->code,ScriptName);
            if ((filename = HTCGetProp(tag, "filename", 0)) != NULL) {
                LBS_EmitChar(htc->code, '/');
                EmitCode(htc, OPC_EVAL);
                LBS_EmitPointer(htc->code, StrDup(filename));
                EmitCode(htc, OPC_PHSTR);
                EmitCode(htc, OPC_UTF8URI);
                EmitCode(htc, OPC_EMITSTR);
            }
            LBS_EmitString(htc->code, "?_name=");
            EmitGetValue(htc,"_name");
            LBS_EmitString(htc->code, "&amp;_event=");
            EmitCode(htc, OPC_EVAL);
            LBS_EmitPointer(htc->code, StrDup(event));
            EmitCode(htc, OPC_EMITSTR);
            if (!fCookie) {
                LBS_EmitString(htc->code, "&amp;_sesid=");
                EmitGetValue(htc,"_sesid");
            }
            if ((name = HTCGetProp(tag, "name", 0)) != NULL &&
                (value = HTCGetProp(tag, "value", 0)) != NULL) {
                LBS_EmitString(htc->code,"&amp;");
                EmitCode(htc, OPC_EVAL);
                LBS_EmitPointer(htc->code,StrDup(name));
                EmitCode(htc, OPC_EMITSTR);
                LBS_EmitString(htc->code,"=");
                EmitAttributeValue(htc, value, FALSE, TRUE, FALSE); 
            }
            if ((file = HTCGetProp(tag, "file", 0)) != NULL) {
                LBS_EmitString(htc->code, "&amp;_file=");
                EmitCode(htc, OPC_EVAL);
                LBS_EmitPointer(htc->code, StrDup(file));
                EmitCode(htc, OPC_EMITSTR);
            }
            if (filename != NULL) {
                LBS_EmitString(htc->code, "&amp;_filename=");
                EmitCode(htc, OPC_EVAL);
                LBS_EmitPointer(htc->code, StrDup(filename));
                EmitCode(htc, OPC_EMITSTR);
            }
            if ((contenttype = HTCGetProp(tag, "contenttype", 0)) != NULL) {
                LBS_EmitString(htc->code, "&amp;_contenttype=");
                EmitCode(htc, OPC_EVAL);
                LBS_EmitPointer(htc->code, StrDup(contenttype));
                EmitCode(htc, OPC_EMITSTR);
            }
            if ((disposition = HTCGetProp(tag, "disposition", 0)) != NULL) {
                LBS_EmitString(htc->code, "&amp;_disposition=");
                EmitCode(htc, OPC_EVAL);
                LBS_EmitPointer(htc->code, StrDup(disposition));
                EmitCode(htc, OPC_EMITSTR);
            }
        }
		LBS_EmitString(htc->code,"\"");

        if ((target = HTCGetProp(tag, "target", 0)) != NULL) {
            LBS_EmitString(htc->code, " target=\"");
            LBS_EmitString(htc->code, target);
            LBS_EmitString(htc->code, "\"");
        }
		LBS_EmitString(htc->code,">");
	}
LEAVE_FUNC;
}

static void
_eHyperLink(HTCInfo *htc, Tag *tag)
{
ENTER_FUNC;
	LBS_EmitString(htc->code, "</a>");
LEAVE_FUNC;
}

static	void
_Panel(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*visible;
    int		has_style;

ENTER_FUNC;
    if ((visible = HTCGetProp(tag, "visible", 0)) == NULL) {
        Push(0);
    }
    else {
        EmitCode(htc, OPC_EVAL);
        LBS_EmitPointer(htc->code, StrDup(visible));
        EmitCode(htc, OPC_PHBOOL);
        EmitCode(htc, OPC_JNZP);
        Push(LBS_GetPos(htc->code));
        LBS_EmitInt(htc->code, 0);
    }
    has_style =
        HTCGetProp(tag, "id", 0) !=  NULL ||
        HTCGetProp(tag, "class", 0) !=  NULL;
    if (has_style) {
        LBS_EmitString(htc->code,"<div");
    }
    Style(htc,tag);
    if (has_style) {
        LBS_EmitString(htc->code,">");
    }
    Push(has_style);
LEAVE_FUNC;
}

static void
_ePanel(HTCInfo *htc, Tag *tag)
{
    size_t jnzp_pos, pos;
ENTER_FUNC;
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
LEAVE_FUNC;
}

static	void
_Calendar(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*year
	,		*month
	,		*day;
	char	*lborder
	,		*lcellspacing
	,		*lcellpadding
	,		*lfontsize
	,		*sborder
	,		*scellspacing
	,		*scellpadding
	,		*sfontsize
	,		*lbgcolor
	,		*lfontcolor
	,		*sbgcolor
	,		*sfontcolor;
	time_t		nowtime;
	struct	tm	*Now;
	int		this_yy
	,		this_mm
	,		this_dd;
	size_t	pos;

ENTER_FUNC;
	time(&nowtime);
	Now = localtime(&nowtime);
	this_yy = Now->tm_year + 1900;
	this_mm = Now->tm_mon + 1;
	this_dd = Now->tm_mday;
	if		(  ( year = HTCGetProp(tag,"year",0) )  ==  NULL  ) {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_yy);
	} else {
		year = StrDup(year);
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,year);
		EmitCode(htc,OPC_PHINT);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_yy);
		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);
	}
	if		(  ( month = HTCGetProp(tag,"month",0) )  ==  NULL  ) {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_mm);
	} else {
		month = StrDup(month);
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,month);
		EmitCode(htc,OPC_PHINT);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_mm);
		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);
	}
	if		(  ( day = HTCGetProp(tag,"day",0) )  ==  NULL  ) {
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_dd);
	} else {
		day = StrDup(day);
		EmitCode(htc,OPC_EVAL);
		LBS_EmitPointer(htc->code,day);
		EmitCode(htc,OPC_PHINT);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);
		EmitCode(htc,OPC_ICONST);
		LBS_EmitInt(htc->code,this_dd);
		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);
	}
	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,year);
	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,month);
	EmitCode(htc,OPC_EVAL);
	LBS_EmitPointer(htc->code,day);
	EmitCode(htc,OPC_CALENDAR);
	if		(  ( lborder = HTCGetProp(tag,"lborder",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,1);
	} else {
		LBS_EmitInt(htc->code,atoi(lborder));
	}
	if		(  ( lcellspacing = HTCGetProp(tag,"lcellspacing",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,0);
	} else {
		LBS_EmitInt(htc->code,atoi(lcellspacing));
	}
	if		(  ( lcellpadding = HTCGetProp(tag,"lcellpadding",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,0);
	} else {
		LBS_EmitInt(htc->code,atoi(lcellpadding));
	}
	if		(  ( lfontsize = HTCGetProp(tag,"lfontsize",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,4);
	} else {
		LBS_EmitInt(htc->code,atoi(lfontsize));
	}
	if		(  ( sborder = HTCGetProp(tag,"sborder",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,0);
	} else {
		LBS_EmitInt(htc->code,atoi(sborder));
	}
	if		(  ( scellspacing = HTCGetProp(tag,"scellspacing",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,0);
	} else {
		LBS_EmitInt(htc->code,atoi(scellspacing));
	}
	if		(  ( scellpadding = HTCGetProp(tag,"scellpadding",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,0);
	} else {
		LBS_EmitInt(htc->code,atoi(scellpadding));
	}
	if		(  ( sfontsize = HTCGetProp(tag,"sfontsize",0) )  ==  NULL  ) {
		LBS_EmitInt(htc->code,3);
	} else {
		LBS_EmitInt(htc->code,atoi(sfontsize));
	}
	if		(  ( lbgcolor = HTCGetProp(tag,"lbgcolor",0) )  ==  NULL  ) {
		LBS_EmitPointer(htc->code,"#F0F0F0");
	} else {
		LBS_EmitPointer(htc->code,StrDup(lbgcolor));
	}
	if		(  ( lfontcolor = HTCGetProp(tag,"lfontcolor",0) )  ==  NULL  ) {
		LBS_EmitPointer(htc->code,"#000000");
	} else {
		LBS_EmitPointer(htc->code,StrDup(lfontcolor));
	}
	if		(  ( sbgcolor = HTCGetProp(tag,"sbgcolor",0) )  ==  NULL  ) {
		LBS_EmitPointer(htc->code,"#FFFFFF");
	} else {
		LBS_EmitPointer(htc->code,StrDup(sbgcolor));
	}
	if		(  ( sfontcolor = HTCGetProp(tag,"sfontcolor",0) )  ==  NULL  ) {
		LBS_EmitPointer(htc->code,"#FF0000");
	} else {
		LBS_EmitPointer(htc->code,StrDup(sfontcolor));
	}
		
LEAVE_FUNC;
}


static	void
_Htc(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*js;

ENTER_FUNC;
	Codeset = HTCGetProp(tag,"coding",0);
	fButton = FALSE;
	fHead = FALSE;
	fNoJavaScript = FALSE;
	if		(  ( js = HTCGetProp(tag,"javascript",0) )  !=  NULL  ) {
		if		(	(  stricmp(js,"no")     ==  0  )
				||	(  stricmp(js,"off")    ==  0  )
				||	(  stricmp(js,"false")  ==  0  ) ) {
			fNoJavaScript = TRUE;
		}
	}
LEAVE_FUNC;
}	

static	void
AddArg(
	Tag		*tag,
	char	*name,
	Bool	fPara)
{
	TagType	*type;

	dbgprintf("install %s:%s",tag->name,name);
	if		(  ( type = g_hash_table_lookup(tag->args,name) )  ==  NULL  ) {
		type = New(TagType);
		type->name = StrDup(name);
		g_hash_table_insert(tag->args,type->name,type);
	}
	type->fPara = fPara;
	type->nPara = 0;
	type->Para = NULL;
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
	if		(  *name  !=  '/'  ) {
		AddArg(tag,"class",TRUE);
		AddArg(tag,"dir",TRUE);
		AddArg(tag,"id",TRUE);
		AddArg(tag,"lang",TRUE);
		AddArg(tag,"style",TRUE);
		AddArg(tag,"title",TRUE);
	}
	return	(tag);
}

extern	void
TagsInit(char *script_name)
{
	Tag		*tag;

ENTER_FUNC;
    ScriptName = script_name; 

	pAStack = 0; 
	Tags = NewNameiHash();

	tag = NewTag("INCLUDE",_Include);
	AddArg(tag,"src",TRUE);
	AddArg(tag,"type",TRUE);

	tag = NewTag("ENTRY",_Entry);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"maxlength",TRUE);
	AddArg(tag,"visible",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"onkeydown",TRUE);
	AddArg(tag,"onkeyup",TRUE);
	tag = NewTag("/ENTRY",NULL);

	tag = NewTag("VAR",_Variable);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"type",TRUE);
	AddArg(tag,"domain",TRUE);
	AddArg(tag,"path",TRUE);
	AddArg(tag,"secure",TRUE);
	AddArg(tag,"expires",TRUE);
	AddArg(tag,"value",TRUE);
	tag = NewTag("/VAR",NULL);

	tag = NewTag("COMBO",_Combo);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"item",TRUE);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"onchange",TRUE);
	tag = NewTag("/COMBO",NULL);

	tag = NewTag("FIXED",_Fixed);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"type",TRUE);
	AddArg(tag,"link",TRUE);
	AddArg(tag,"target",TRUE);
	AddArg(tag,"value",TRUE);
	tag = NewTag("/FIXED",NULL);

	tag = NewTag("SPAN",_Span);
	AddArg(tag,"onclick",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"onkeydown",TRUE);
	AddArg(tag,"onkeypress",TRUE);
	AddArg(tag,"onkeyup",TRUE);
	AddArg(tag,"onmousedown",TRUE);
	AddArg(tag,"onmousemove",TRUE);
	AddArg(tag,"onmouseout",TRUE);
	AddArg(tag,"onmouseover",TRUE);
	AddArg(tag,"onmousesetup",TRUE);

	tag = NewTag("A",_A);
	AddArg(tag,"onclick",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"onkeydown",TRUE);
	AddArg(tag,"onkeypress",TRUE);
	AddArg(tag,"onkeyup",TRUE);
	AddArg(tag,"onmousedown",TRUE);
	AddArg(tag,"onmousemove",TRUE);
	AddArg(tag,"onmouseout",TRUE);
	AddArg(tag,"onmouseover",TRUE);
	AddArg(tag,"onmousesetup",TRUE);
	AddArg(tag,"href",TRUE);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"target",TRUE);

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
	AddArg(tag,"type",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"onkeydown",TRUE);
	AddArg(tag,"onkeyup",TRUE);
	tag = NewTag("/TEXT",NULL);

	tag = NewTag("BUTTON",_Button);
	AddArg(tag,"event",TRUE);
	AddArg(tag,"face",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"onclick",TRUE);
	AddArg(tag,"state",TRUE);
	tag = NewTag("/BUTTON",_eButton);

	tag = NewTag("TOGGLEBUTTON",_ToggleButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"onclick",TRUE);
	tag = NewTag("/TOGGLEBUTTON",NULL);

	tag = NewTag("CHECKBUTTON",_CheckButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"onclick",TRUE);
	tag = NewTag("/CHECKBUTTON",NULL);

	tag = NewTag("RADIOBUTTON",_RadioButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"group",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"onclick",TRUE);
	tag = NewTag("/RADIOBUTTON",NULL);

	tag = NewTag("LIST",_List);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"value",TRUE);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"multiple",TRUE);
	AddArg(tag,"onchange",TRUE);
	tag = NewTag("/LIST",NULL);

	tag = NewTag("SELECT",_Select);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"type",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"multiple",TRUE);
	AddArg(tag,"onchange",TRUE);
	//tag = NewTag("/SELECT",NULL);

	tag = NewTag("FILESELECTION",_FileSelection);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"filename",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"maxlength",TRUE);
	tag = NewTag("/FILESELECTION",NULL);

	tag = NewTag("HYPERLINK",_HyperLink);
	AddArg(tag,"event",TRUE);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"value",TRUE);
	AddArg(tag,"file",TRUE);
	AddArg(tag,"filename",TRUE);
	AddArg(tag,"contenttype",TRUE);
	AddArg(tag,"disposition",TRUE);
    AddArg(tag,"target",TRUE);
	tag = NewTag("/HYPERLINK",_eHyperLink);

	tag = NewTag("PANEL", _Panel);
	AddArg(tag, "visible", TRUE);
	tag = NewTag("/PANEL", _ePanel);

	tag = NewTag("CALENDAR",_Calendar);
	AddArg(tag,"year",TRUE);
	AddArg(tag,"month",TRUE);
	AddArg(tag,"day",TRUE);
	AddArg(tag,"lborder",TRUE);
    AddArg(tag,"lcellspacing",TRUE);
    AddArg(tag,"lcellpadding",TRUE);
    AddArg(tag,"lfontsize",TRUE);
    AddArg(tag,"sborder",TRUE);
	AddArg(tag,"scellspacing",TRUE);
	AddArg(tag,"scellpadding",TRUE);
    AddArg(tag,"sfontsize",TRUE);
    AddArg(tag,"lbgcolor",TRUE);
    AddArg(tag,"lfontcolor",TRUE);
    AddArg(tag,"sbgcolor",TRUE);
    AddArg(tag,"sfontcolor",TRUE);
	tag = NewTag("/CALENDAR",NULL);

	tag = NewTag("OPTION", _Option);
	AddArg(tag,"select",TRUE);
	AddArg(tag,"value",TRUE);

	tag = NewTag("OPTIONMENU",_Optionmenu);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"select",TRUE);
	AddArg(tag,"item",TRUE);
	tag = NewTag("/OPTIONMENU",NULL);

	tag = NewTag("HTC",_Htc);
	AddArg(tag,"coding",TRUE);
	AddArg(tag,"javascript",TRUE);
	tag = NewTag("/HTC",NULL);

	tag = NewTag("FORM",_Form);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"defaultevent",TRUE);
	AddArg(tag,"target",TRUE);

	tag = NewTag("HEAD",_Head);
	AddArg(tag,"profile",TRUE);
	AddArg(tag,"dir",TRUE);
	AddArg(tag,"lang",TRUE);
	AddArg(tag,"javascript",TRUE);
	tag = NewTag("/HEAD",NULL);

	tag = NewTag("BODY",_Body);
	AddArg(tag,"text",TRUE);
	AddArg(tag,"link",TRUE);
	AddArg(tag,"vlink",TRUE);
	AddArg(tag,"alink",TRUE);
	AddArg(tag,"bgcolor",TRUE);
	AddArg(tag,"background",TRUE);
	AddArg(tag,"bgproperties",TRUE);
	AddArg(tag,"marginheight",TRUE);
	AddArg(tag,"marginwidth",TRUE);
	AddArg(tag,"topmargin",TRUE);
	AddArg(tag,"leftmargin",TRUE);
	AddArg(tag,"bottommargin",TRUE);
	AddArg(tag,"rightmargin",TRUE);
	AddArg(tag,"scroll",TRUE);
	AddArg(tag,"dir",TRUE);
	AddArg(tag,"lang",TRUE);
	AddArg(tag,"oncontextmenu",TRUE);
	AddArg(tag,"onload",TRUE);

	tag = NewTag("/BODY",_eBody);
	tag = NewTag("/HTML",_eHtml);
LEAVE_FUNC;
}

static	Js		*
NewJs(
	char	*name,
	char	*body,
	Bool	fFile)
{
	Js		*js;

	js = New(Js);
	js->name = name;
	js->body = body;
	js->fUse = FALSE;
	js->fFile = fFile;
	g_hash_table_insert(Jslib,js->name,js);
	return	(js);
}

extern	void
InvokeJs(
	char	*name)
{
	Js		*js;

	if		(  !fJavaScript  )	return;
	if		(  ( js = g_hash_table_lookup(Jslib,name) )  !=  NULL  ) {
		js->fUse = TRUE;
	}
}

extern	void
JslibInit(void)
{
ENTER_FUNC;
	if		(  !fJavaScript  )
		return;
	Jslib = NewNameiHash();
#ifdef	USE_MCE
	NewJs("html_edit","./jscripts/tiny_mce/tiny_mce.js",TRUE);
	NewJs("html_setting",
		  "tinyMCE.init({\n"
		  "  theme    : \"advanced\",\n"
		  "  language : \"jp\",\n"
		  //"  invalid_elements : \"br\",\n"
		  "  mode     : \"specific_textareas\"\n"
		  "});\n"
		  ,FALSE);
#endif
	NewJs("send_event",
		  "var sent_event = 0;\n"
		  "function send_event(no,event){\n"
#ifdef	USE_MCE
		  "  if (typeof(tinyMCE) != \"undefined\") {\n"
		  "    tinyMCE.triggerSave();\n"
		  "  }\n"
#endif
		  "  if  ( sent_event == 0 ) {\n"
		  "    sent_event = 1;\n"
		  "    document.forms[no].elements[0].name='_event';\n"
		  "    document.forms[no].elements[0].value=event;\n"
		  "    document.forms[no].submit();\n"
		  "  }\n"
		  "}\n",FALSE);
		  
	InvokeJs("send_event");
LEAVE_FUNC;
}
