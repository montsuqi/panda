/*	PANDA -- a simple transaction monitor

Copyright (C) 2002-2004 Ogochan & JMA (Japan Medical Association).

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
#include	"cgi.h"
#include	"enum.h"
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

#define IsTrue(s)  (*(s) == 'T' || *(s) == 't')

static	char	*
GetArg(
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
            EmitCode(htc,OPC_NAME);
            LBS_EmitPointer(htc->code,StrDup(str+1));
            EmitCode(htc,OPC_HSNAME);
            if (fEncodeURL) {
                EmitCode(htc,OPC_LOCURI);
            }
            if (fEscapeJavaScriptString) {
                EmitCode(htc,OPC_ESCJSS);
            }
            EmitCode(htc,OPC_REFSTR);
            break;
        default:
            EmitCode(htc,OPC_NAME);
            LBS_EmitPointer(htc->code,StrDup(str));
            if (fEncodeURL) {
                EmitCode(htc,OPC_LOCURI);
				EmitCode(htc,OPC_REFSTR);
            } else
            if (fEscapeJavaScriptString) {
                EmitCode(htc,OPC_ESCJSS);
				EmitCode(htc,OPC_REFSTR);
            } else {
				EmitCode(htc,OPC_EMITHTML);
			}
            break;
	}
	if		(  fQuote  ) {
		LBS_EmitString(htc->code,"\"");
	}
}

extern	void
ExpandAttributeString(
	HTCInfo	*htc,
	char	*para)
{
	LBS_EmitChar(htc->code,'"');
	switch	(*para) {
	  case	'$':
		para ++;
		if		(  *para  ==  '$'  ) {
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(para));
            EmitCode(htc,OPC_REFSTR);
		} else {
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(para));
			EmitCode(htc,OPC_EHSNAME);
		}
		break;
	  case	'\\':
		LBS_EmitString(htc->code,para+1);
		break;
	  default:
		LBS_EmitString(htc->code,para);
		break;
	}
	LBS_EmitChar(htc->code,'"');
}

static void
JavaScriptEvent(HTCInfo *htc, Tag *tag, char *event)
{
    char 	*value
		,	*p;
    char buf[SIZE_BUFF];
	size_t	len;

    if ((value = GetArg(tag, event, 0)) == NULL)
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
					EmitCode(htc,OPC_NAME);
					LBS_EmitPointer(htc->code,StrDup(buf));
					EmitCode(htc,OPC_REFSTR);
				} else {
					EmitCode(htc,OPC_NAME);
					LBS_EmitPointer(htc->code,StrDup(buf));
					EmitCode(htc,OPC_EHSNAME);
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
	char	*id
		,	*klass
		,	*style;

	if		(  ( id = GetArg(tag,"id",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," id=");
		EmitAttributeValue(htc,id,TRUE,FALSE,FALSE);
	}
	if		(  ( klass = GetArg(tag,"class",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," class=");
		EmitAttributeValue(htc,klass,TRUE,FALSE,FALSE);
	}
	if		(  ( style = GetArg(tag,"style",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," style=");
		EmitAttributeValue(htc,style,TRUE,FALSE,FALSE);
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

ENTER_FUNC;
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

ENTER_FUNC;
	value = GetArg(tag,"value",0);
	name = GetArg(tag,"name",0);
	type = GetArg(tag,"type",0);
	if		(	(  value  !=  NULL  )
			||	(  name   !=  NULL  ) ) {
		if		(	(  type  !=  NULL  )
				&&	(  !stricmp(type,"html")  ) ) {
			LBS_EmitString(htc->code,"<div");
			Style(htc,tag);
			LBS_EmitString(htc->code,">\n");
		} else {
			LBS_EmitString(htc->code,"<span");
			Style(htc,tag);
			LBS_EmitString(htc->code,">");
		}
		if		(  ( link = GetArg(tag,"link",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code,"<a href=\"");

			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(link));
			EmitCode(htc,OPC_EHSNAME);

			LBS_EmitString(htc->code,"\"");
			if		(  ( target = GetArg(tag,"target",0) )  !=  NULL  ) {
				LBS_EmitString(htc->code," target=\"");
				LBS_EmitString(htc->code,target);
				LBS_EmitString(htc->code,"\"");
			}
			LBS_EmitString(htc->code,">");
		}
		if		(  value  !=  NULL  ) {
			LBS_EmitString(htc->code,value);
		} else {
			if		(	(  name[0]  ==  '$'  )
					||	(  name[0]  ==  '#'  ) ) {
				EmitCode(htc,OPC_NAME);
				LBS_EmitPointer(htc->code,StrDup(name));
				EmitCode(htc,OPC_REFSTR);
			} else {
				EmitCode(htc,OPC_NAME);
				LBS_EmitPointer(htc->code,StrDup(name));
				EmitCode(htc,OPC_HSNAME);
				if		(	(  type  !=  NULL  )
						&&	(  !stricmp(type,"html")  ) ) {
					EmitCode(htc,OPC_EMITHTML);
				} else {
					EmitCode(htc,OPC_EMITSTR);
				}
			}
		}
		if		(  link  !=  NULL  ) {
			LBS_EmitString(htc->code,"</a>");
		}
		if		(	(  type  !=  NULL  )
				&&	(  !stricmp(type,"html")  ) ) {
			LBS_EmitString(htc->code,"</div>");
		} else {
			LBS_EmitString(htc->code,"</span>\n");
		}
	}
LEAVE_FUNC;
}

static	void
_Combo(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	name[SIZE_ARG];
	char	*size
		,	*vname;
	size_t	pos;
	int		arg;

ENTER_FUNC;
	LBS_EmitString(htc->code,"<select name=\"");
	EmitCode(htc,OPC_NAME);
	vname = StrDup(GetArg(tag,"name",0));
	LBS_EmitPointer(htc->code,vname);
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
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"count",0)));
	EmitCode(htc,OPC_ICONST);							/*	1	step	*/
	LBS_EmitInt(htc->code,1);
	Push(LBS_GetPos(htc->code));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,0);

	LBS_EmitString(htc->code,"<option ");
	/*	selected	*/
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,vname);
	EmitCode(htc,OPC_HSNAME);
	EmitCode(htc,OPC_NAME);
	sprintf(name,"%s[#]",GetArg(tag,"item",0));
	LBS_EmitPointer(htc->code,StrDup(name));
	EmitCode(htc,OPC_HSNAME);
	EmitCode(htc,OPC_SCMP);
	EmitCode(htc,OPC_JNZNP);
	arg = LBS_GetPos(htc->code);
	LBS_EmitInt(htc->code,0);
	EmitCode(htc,OPC_SCONST);
	LBS_EmitPointer(htc->code,"selected");
	EmitCode(htc,OPC_REFSTR);
	pos = LBS_GetPos(htc->code);
	LBS_SetPos(htc->code,arg);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
	Style(htc,tag);
	LBS_EmitString(htc->code,">");

	EmitCode(htc,OPC_NAME);
	sprintf(name,"%s[#]",GetArg(tag,"item",0));
	LBS_EmitPointer(htc->code,StrDup(name));
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
    EmitCode(htc, OPC_REFSTR);
    LBS_EmitString(htc->code, "\"");
	Style(htc,tag);
	if		(  ( name = GetArg(tag,"name",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," name=\"");
		LBS_EmitString(htc->code,name);
		LBS_EmitString(htc->code,"\"");
	}
	if		(  ( target = GetArg(tag,"target",0) )  !=  NULL  ) {
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

    if ((defaultevent = GetArg(tag, "defaultevent", 0)) != NULL) {
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
	if		(	(  ( type = GetArg(tag,"type",0) )  ==  NULL  )
			||	(  !stricmp(type,"text")                      ) ) {
	} else {
		InvokeJs("html_edit");
		InvokeJs("html_setting");
		LBS_EmitString(htc->code," mce_editable=\"true\"");
		if		(  !stricmp(type,"xml")  ) {
			SetFilter(GetArg(tag,"name",0),StrDup,NULL);
		}
	}
#endif

	LBS_EmitString(htc->code," name=\"");
	EmitCode(htc,OPC_NAME);
	name = StrDup(GetArg(tag,"name",0));
	LBS_EmitPointer(htc->code,name);
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\"");
	if		(  ( cols = GetArg(tag,"cols",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," cols=");
		EmitAttributeValue(htc,cols,TRUE,FALSE,FALSE);
	}
	if		(  ( rows = GetArg(tag,"rows",0) )  !=  NULL  ) {
		LBS_EmitString(htc->code," rows=");
		EmitAttributeValue(htc,rows,TRUE,FALSE,FALSE);
	}
    JavaScriptEvent(htc, tag, "onchange");
    JavaScriptKeyEvent(htc, tag, "onkeydown");
    JavaScriptKeyEvent(htc, tag, "onkeyup");
	Style(htc,tag);
	LBS_EmitString(htc->code,">\n");

	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,name);
	if		(	(  ( type = GetArg(tag,"type",0) )  ==  NULL  )
			||	(  !stricmp(type,"text")                      ) ) {
		EmitCode(htc,OPC_EHSNAME);
	} else {
		EmitCode(htc,OPC_HSNAME);
		EmitCode(htc,OPC_EMITHTML);
	}
	LBS_EmitString(htc->code,"</textarea>\n");
LEAVE_FUNC;
}

static	void
_Head(
	HTCInfo	*htc,
	Tag		*tag)
{
ENTER_FUNC;
	LBS_EmitString(htc->code,
				   "<head>\n<meta http-equiv=\"Pragma\" content=\"no-cache\">");
	EmitCode(htc,OPC_FLJS);
LEAVE_FUNC;
}	

static	void
_eBody(
	HTCInfo	*htc,
	Tag		*tag)
{
ENTER_FUNC;
	if		(  !fDump  ) {
		LBS_EmitString(htc->code,"</body>");
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
		LBS_EmitString(htc->code,"</html>");
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

ENTER_FUNC;
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
	LBS_SetPos(htc->code,TOP(0));
	EmitCode(htc,OPC_BREAK);
	LBS_EmitInt(htc->code,pos);
	LBS_SetPos(htc->code,pos);
LEAVE_FUNC;
}

static	Bool	fButton;
static	void
_Button(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*face
		,	*event;
	char	buf[SIZE_BUFF];
	char	*state;
	int		arg
		,	pos;

ENTER_FUNC;
	if		(  ( state = GetArg(tag,"state",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(state));
		EmitCode(htc,OPC_HINAME);
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

	if		(  GetArg(tag,"onclick",0)  !=  NULL  ) {
		fButton = TRUE;
		LBS_EmitString(htc->code,"<button");
        JavaScriptEvent(htc, tag, "onclick");
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
		if ((face = GetArg(tag, "face", 0)) == NULL) {
			face = "click";
		}
		LBS_EmitString(htc->code,face);
	} else {
		fButton = FALSE;
		event = GetArg(tag, "event", 0);
		if		(  event  ==  NULL  ) {
			HTC_Error("`event' or `on' attribute is required for <%s>\n", tag->name);
			return;
		}
		if ((face = GetArg(tag, "face", 0)) == NULL) {
			face = event;
		}
		if (htc->DefaultEvent == NULL)
			htc->DefaultEvent = event;
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
		if		(  ( size = GetArg(tag,"size",0) )  !=  NULL  ) {
			LBS_EmitString(htc->code," size=");
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(size));
			EmitCode(htc,OPC_REFSTR);
		}
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
#else
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
		Style(htc,tag);
		LBS_EmitString(htc->code,">");
#endif
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
	if		(  fButton  ) {
		LBS_EmitString(htc->code,"</button>\n");
	}
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
	if		(  ( state = GetArg(tag,"state",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(state));
		EmitCode(htc,OPC_HINAME);
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
	char	*state;
	int		arg
		,	pos;

ENTER_FUNC;
	if		(  ( state = GetArg(tag,"state",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(state));
		EmitCode(htc,OPC_HINAME);
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
	JavaScriptEvent(htc, tag, "href");
	Style(htc,tag);
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
	char	*label;

ENTER_FUNC;
	LBS_EmitString(htc->code,"<input type=\"checkbox\" name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\" ");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HBES);
	LBS_EmitPointer(htc->code,"checked ");
	LBS_EmitString(htc->code," value=\"TRUE\"");
	JavaScriptEvent(htc, tag, "onclick");
	Style(htc,tag);
	LBS_EmitString(htc->code,">");
	if		(  ( label = GetArg(tag,"label",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_NAME);
		LBS_EmitPointer(htc->code,StrDup(label));
		EmitCode(htc,OPC_EHSNAME);
	}
LEAVE_FUNC;
}

static	void
_CheckButton(
	HTCInfo	*htc,
	Tag		*tag)
{
	char	*label;

ENTER_FUNC;
	LBS_EmitString(htc->code,"<input type=\"checkbox\" name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\" ");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HBES);
	LBS_EmitPointer(htc->code,"checked");
	LBS_EmitString(htc->code," value=\"TRUE\"");
	JavaScriptEvent(htc, tag, "onclick");
	Style(htc,tag);
	LBS_EmitString(htc->code,">");
	if		(  ( label = GetArg(tag,"label",0) )  !=  NULL  ) {
		if		(  *label  ==  '$'  ) {
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(label+1));
			EmitCode(htc,OPC_EHSNAME);
		} else {
			LBS_EmitString(htc->code,label);
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

ENTER_FUNC;
	group = GetArg(tag,"group",0); 
	name = GetArg(tag,"name",0); 
	LBS_EmitString(htc->code,"<input type=\"radio\" name=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(group));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\" ");

	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(GetArg(tag,"name",0)));
	EmitCode(htc,OPC_HBES);
	LBS_EmitPointer(htc->code,"checked");

	LBS_EmitString(htc->code," value=\"");
	EmitCode(htc,OPC_NAME);
	LBS_EmitPointer(htc->code,StrDup(name));
	EmitCode(htc,OPC_REFSTR);
	LBS_EmitString(htc->code,"\"");
	JavaScriptEvent(htc, tag, "onclick");

	Style(htc,tag);
	LBS_EmitString(htc->code,">");
	if		(  ( label = GetArg(tag,"label",0) )  !=  NULL  ) {
		if		(  *label  ==  '$'  ) {
			EmitCode(htc,OPC_NAME);
			LBS_EmitPointer(htc->code,StrDup(label+1));
			EmitCode(htc,OPC_EHSNAME);
		} else {
			LBS_EmitString(htc->code,label);
		}
	}

	g_hash_table_insert(htc->Radio,StrDup(group),(void*)1);
LEAVE_FUNC;
}

static void
_List(HTCInfo *htc, Tag *tag)
{
	char	buf[SIZE_ARG];
	char	*name, *label, *count, *size, *multiple, *value;
	size_t	pos;

ENTER_FUNC;
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
        if ((value = GetArg(tag,"value",0)) != NULL) {
			EmitCode(htc,OPC_NAME);
			sprintf(buf,"%s[#]",value);
			LBS_EmitPointer(htc->code,StrDup(buf));
			EmitCode(htc,OPC_EHSNAME);
		} else {
			EmitCode(htc,OPC_LDVAR);
			LBS_EmitPointer(htc->code,"");
			EmitCode(htc,OPC_REFINT);
		}
        LBS_EmitString(htc->code,"\"");

        EmitCode(htc,OPC_NAME);
        sprintf(buf,"%s[#] ",name);
        LBS_EmitPointer(htc->code,StrDup(buf));
        EmitCode(htc,OPC_HBES);
        LBS_EmitPointer(htc->code,"selected");
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
LEAVE_FUNC;
}

static void
_Optionmenu(HTCInfo *htc, Tag *tag)
{
	char	buf[SIZE_ARG];
	char	*item, *count, *sel;
	size_t	pos;

ENTER_FUNC;
	if		(	(  ( item  = GetArg(tag,"item",0) )    !=  NULL  )
			&&	(  ( sel   = GetArg(tag,"select",0) )  !=  NULL  )
			&&	(  ( count = GetArg(tag,"count",0) )   !=  NULL  ) ) {
        LBS_EmitString(htc->code,"<select name=\"");
        EmitCode(htc,OPC_NAME);
        LBS_EmitPointer(htc->code,StrDup(sel));
        EmitCode(htc,OPC_REFSTR);
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
        EmitCode(htc,OPC_NAME);
        LBS_EmitPointer(htc->code,StrDup(sel));
		EmitCode(htc,OPC_HSNAME);
		EmitCode(htc,OPC_TOINT);
		EmitCode(htc,OPC_SUB);
		EmitCode(htc,OPC_JNZNP);
		Push(LBS_GetPos(htc->code));
		LBS_EmitInt(htc->code,0);

		EmitCode(htc,OPC_SCONST);
        LBS_EmitPointer(htc->code,"selected");
		EmitCode(htc,OPC_REFSTR);

		pos = LBS_GetPos(htc->code);
		LBS_SetPos(htc->code,Pop);
		LBS_EmitInt(htc->code,pos);
		LBS_SetPos(htc->code,pos);

        LBS_EmitString(htc->code,">");

        EmitCode(htc,OPC_NAME);
        sprintf(buf,"%s[#]",item);
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
	if		(  ( name = GetArg(tag,"name",0) )  !=  NULL  ) {
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
		if		(	(  ( type = GetArg(tag,"type",0) )  !=  NULL  )
				&&	(  !stricmp(type,"menu")                      ) ) {
			InvokeJs("send_event");
			LBS_EmitString(htc->code, "onchange");
			snprintf(buf, SIZE_BUFF,
					 "=\""
					 "send_event(%d,this.options[this.selectedIndex].value);\"",htc->FormNo);
			LBS_EmitString(htc->code, buf);
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
	if		(  ( value = GetArg(tag,"value",0) )  !=  NULL  )	{
		LBS_EmitString(htc->code," value=");
		ExpandAttributeString(htc,value);
		LBS_EmitString(htc->code,"\" ");
	}
	if		(  ( select = GetArg(tag,"select",0) )  !=  NULL  ) {
		EmitCode(htc,OPC_NAME);
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
	if ((event = GetArg(tag, "event", 0)) != NULL) {
		LBS_EmitString(htc->code, "<a");
		Style(htc,tag);

		LBS_EmitString(htc->code, " href=\"");

		if (fJavaScriptLink && htc->FormNo >= 0 &&
            (file = GetArg(tag, "file", 0)) == NULL) {
            LBS_EmitString(htc->code, "javascript:");
            snprintf(buf, SIZE_BUFF,
                     "document.forms[%d].elements[0].name='_event';"
                     "document.forms[%d].elements[0].value='",
                     htc->FormNo, htc->FormNo);
            LBS_EmitString(htc->code, buf);
            EmitCode(htc, OPC_NAME);
            LBS_EmitPointer(htc->code, StrDup(event));
            EmitCode(htc, OPC_ESCJSS);
            EmitCode(htc, OPC_REFSTR);
            LBS_EmitString(htc->code, "';");

            if ((name = GetArg(tag, "name", 0)) != NULL &&
                (value = GetArg(tag, "value", 0)) != NULL) {
                snprintf(buf, SIZE_BUFF,
                         "document.forms[%d].elements[1].name='", htc->FormNo);
                LBS_EmitString(htc->code, buf);
                EmitCode(htc, OPC_NAME);
                LBS_EmitPointer(htc->code, StrDup(name));
                EmitCode(htc, OPC_ESCJSS);
                EmitCode(htc, OPC_REFSTR);
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
        }
        else {
            LBS_EmitString(htc->code,ScriptName);
            if ((filename = GetArg(tag, "filename", 0)) != NULL) {
                LBS_EmitChar(htc->code, '/');
                EmitCode(htc, OPC_NAME);
                LBS_EmitPointer(htc->code, StrDup(filename));
                EmitCode(htc, OPC_HSNAME);
                EmitCode(htc, OPC_UTF8URI);
                EmitCode(htc, OPC_REFSTR);
            }
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
                EmitCode(htc, OPC_NAME);
                LBS_EmitPointer(htc->code,StrDup(name));
                EmitCode(htc, OPC_REFSTR);
                LBS_EmitString(htc->code,"=");
                EmitAttributeValue(htc, value, FALSE, TRUE, FALSE); 
            }
            if ((file = GetArg(tag, "file", 0)) != NULL) {
                LBS_EmitString(htc->code, "&amp;_file=");
                EmitCode(htc, OPC_NAME);
                LBS_EmitPointer(htc->code, StrDup(file));
                EmitCode(htc, OPC_REFSTR);
            }
            if (filename != NULL) {
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
            if ((disposition = GetArg(tag, "disposition", 0)) != NULL) {
                LBS_EmitString(htc->code, "&amp;_disposition=");
                EmitCode(htc, OPC_NAME);
                LBS_EmitPointer(htc->code, StrDup(disposition));
                EmitCode(htc, OPC_REFSTR);
            }
        }
		LBS_EmitString(htc->code,"\"");

        if ((target = GetArg(tag, "target", 0)) != NULL) {
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
LEAVE_FUNC;
}


static	void
_Htc(
	HTCInfo	*htc,
	Tag		*tag)
{
ENTER_FUNC;
	Codeset = GetArg(tag,"coding",0); 
	HTCSetCodeset(Codeset);
LEAVE_FUNC;
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

ENTER_FUNC;
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
	AddArg(tag,"style",TRUE);
	tag = NewTag("/ENTRY",NULL);

	tag = NewTag("COMBO",_Combo);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"item",TRUE);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	tag = NewTag("/COMBO",NULL);

	tag = NewTag("FIXED",_Fixed);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"type",TRUE);
	AddArg(tag,"link",TRUE);
	AddArg(tag,"target",TRUE);
	AddArg(tag,"value",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
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
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	//	tag = NewTag("/SPAN",NULL);

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
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);

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
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	tag = NewTag("/TEXT",NULL);

	tag = NewTag("BUTTON",_Button);
	AddArg(tag,"event",TRUE);
	AddArg(tag,"face",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"onclick",TRUE);
	AddArg(tag,"state",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	tag = NewTag("/BUTTON",_eButton);

	tag = NewTag("TOGGLEBUTTON",_ToggleButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	AddArg(tag,"onclick",TRUE);
	tag = NewTag("/TOGGLEBUTTON",NULL);

	tag = NewTag("CHECKBUTTON",_CheckButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	AddArg(tag,"onclick",TRUE);
	tag = NewTag("/CHECKBUTTON",NULL);

	tag = NewTag("RADIOBUTTON",_RadioButton);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"group",TRUE);
	AddArg(tag,"label",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
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
	AddArg(tag,"id",TRUE);
	AddArg(tag,"style",TRUE);
	AddArg(tag,"class",TRUE);
	tag = NewTag("/LIST",NULL);

	tag = NewTag("SELECT",_Select);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"type",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"multiple",TRUE);
	AddArg(tag,"onchange",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	tag = NewTag("/SELECT",NULL);

	tag = NewTag("FILESELECTION",_FileSelection);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"filename",TRUE);
	AddArg(tag,"size",TRUE);
	AddArg(tag,"maxlength",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
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
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	tag = NewTag("/HYPERLINK",_eHyperLink);

	tag = NewTag("PANEL", _Panel);
	AddArg(tag, "visible", TRUE);
	AddArg(tag, "id", TRUE);
	AddArg(tag, "class", TRUE);
	AddArg(tag,"style",TRUE);
	tag = NewTag("/PANEL", _ePanel);

	tag = NewTag("CALENDAR",_Calendar);
	AddArg(tag,"year",TRUE);
	AddArg(tag,"month",TRUE);
	AddArg(tag,"day",TRUE);
	tag = NewTag("/CALENDAR",NULL);

	tag = NewTag("OPTION", _Option);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	AddArg(tag,"select",TRUE);
	AddArg(tag,"value",TRUE);
	tag = NewTag("/OPTION", NULL);

	tag = NewTag("OPTIONMENU",_Optionmenu);
	AddArg(tag,"count",TRUE);
	AddArg(tag,"select",TRUE);
	AddArg(tag,"item",TRUE);
	AddArg(tag,"id",TRUE);
	AddArg(tag,"class",TRUE);
	AddArg(tag,"style",TRUE);
	tag = NewTag("/OPTIONMENU",NULL);

	tag = NewTag("HTC",_Htc);
	AddArg(tag,"coding",TRUE);
	tag = NewTag("/HTC",NULL);

	tag = NewTag("FORM",_Form);
	AddArg(tag,"name",TRUE);
	AddArg(tag,"defaultevent",TRUE);
	AddArg(tag,"target",TRUE);

	tag = NewTag("HEAD",_Head);
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

	if		(  ( js = g_hash_table_lookup(Jslib,name) )  !=  NULL  ) {
		js->fUse = TRUE;
	}
}

extern	void
JslibInit(void)
{
ENTER_FUNC;
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
		  "function send_event(no,event){\n"
#ifdef	USE_MCE
		  "  if (typeof(tinyMCE) != \"undefined\") {\n"
		  "    tinyMCE.triggerSave();\n"
		  "  }\n"
#endif
		  "  document.forms[no].elements[0].name='_event';\n"
		  "  document.forms[no].elements[0].value=event;\n"
		  "  document.forms[no].submit();\n"
		  "}\n",FALSE);
		  
LEAVE_FUNC;
}
