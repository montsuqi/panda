/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2006 Ogochan.
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
#include	<glib.h>
#include	<ctype.h>
#include	<iconv.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"HTCparser.h"
#include	"cgi.h"
#include	"tags.h"
#include	"multipart.h"
#include	"exec.h"
#include	"debug.h"

#define	SRC_CODESET		"utf-8"
#define	NEN		"年"
#define	TSUKI	"月"
#define	HI		"日"
#define	ATAMA	"<TH>日</TH><TH>月</TH><TH>火</TH><TH>水</TH><TH>木</TH><TH>金</TH><TH>土</TH></TR>\n"

#define	SIZE_RSTACK		100

static	Expr	Stack[SIZE_RSTACK];
static	size_t	pStack;
static	GHashTable	*VarArea;


#define	Pop			Stack[-- pStack]
#define	TOP(n)		Stack[pStack-(n)]

#ifdef	DEBUG
static	void
Push(
	Expr	v)
{
	dbgprintf("pStack = %d\n",pStack);
	dbgprintf("val = %d\n",v.body.ival);
    if (pStack == SIZE_RSTACK) {
        fprintf(stderr, "stack level too deep\n");
        exit(1);
    }
	Stack[pStack ++] = v;
}
#else
#define	Push(v)		do { \
    if (pStack == SIZE_RSTACK) { \
        fprintf(stderr, "stack level too deep\n"); \
        exit(1); \
    } \
    Stack[pStack ++] = (v); \
} while (0)
#endif

static	int
youbi(
	int		yy,
	int		mm,
	int		dd)
{
	int		wday;

	if		(  mm  <  3  ) {
		yy --;
		mm += 12;
	}
	wday = ( yy + yy / 4 - yy / 100 + yy / 400 + ( 13 * mm + 8 ) / 5 + dd ) % 7;
	return	(wday);			 
}

static	Bool
uru(
	int		yy)
{
	Bool	rc;

	if		(	(	(  ( yy % 4 )    ==  0  )
				&&	(  ( yy % 4 )    !=  0  ) )
			||	(  ( yy % 400 )  ==  0  ) ) {
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

static	void
ExecCalendar(
	LargeByteString	*html,
	int		yy,
	int		mm,
	int		dd,
	int		lborder,
	int		lcellspacing,
	int		lcellpadding,
	char	*lbgcolor,
	char	*lfontcolor,
	int		lfontsize,
	int		sborder,
	int		scellspacing,
	int		scellpadding,
	char	*sbgcolor,
	char	*sfontcolor,
	int		sfontsize,
	char	*year,
	char	*month,
	char	*day)
{
	int		sun
	,		one
	,		lday
	,		i;
	char	buff[5];
	static	int		tlday[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

ENTER_FUNC;
	lday = tlday[mm - 1];
	if		(	(  mm  ==  2  )
			&&	(  uru(yy)    ) ) {
		lday ++;
	}

	if		(  year  !=  NULL  ) {
		LBS_EmitString(html,"<INPUT TYPE=\"hidden\" name=\"");
		LBS_EmitString(html,year);
		LBS_EmitString(html,"\" value=\"");
		sprintf(buff,"%d",yy);
		LBS_EmitString(html,buff);
		LBS_EmitString(html,"\">\n");
	}

	if		(  month  !=  NULL  ) {
		LBS_EmitString(html,"<INPUT TYPE=\"hidden\" name=\"");
		LBS_EmitString(html,month);
		LBS_EmitString(html,"\" value=\"");
		sprintf(buff,"%d",mm);
		LBS_EmitString(html,buff);
		LBS_EmitString(html,"\">\n");
	}

	LBS_EmitString(html,"<TABLE BORDER=\"");
	sprintf(buff,"%d",lborder);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\" CELLSPACING=\"");
	sprintf(buff,"%d",lcellspacing);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\" CELLPADDING=\"");
	sprintf(buff,"%d",lcellpadding);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\" BGCOLOR=\"");
	sprintf(buff,"%s",lbgcolor);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\"><TR align=\"center\"><TH>\n");
	LBS_EmitString(html,"<FONT COLOR=\"");
	sprintf(buff,"%s",lfontcolor);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\" SIZE=\"");
	sprintf(buff,"%d",lfontsize);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\">");

	sprintf(buff,"%d",yy);
	LBS_EmitString(html,buff);
	LBS_EmitUTF8(html,NEN,SRC_CODESET);
	sprintf(buff,"%d",mm);
	LBS_EmitString(html,buff);
	LBS_EmitUTF8(html,TSUKI,SRC_CODESET);
	LBS_EmitString(html,"</FONT></TH></TR>\n");

	LBS_EmitString(html,"<TR align=\"center\"><TD>\n");
	LBS_EmitString(html,"<TABLE BORDER=\"");
	sprintf(buff,"%d",sborder);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\" CELLSPACING=\"");
	sprintf(buff,"%d",scellspacing);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\" CELLPADDING=\"");
	sprintf(buff,"%d",scellpadding);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\" BGCOLOR=\"");
	sprintf(buff,"%s",sbgcolor);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\">\n");
	LBS_EmitString(html,"<TR align=\"center\"><TH><FONT COLOR=\"");
	sprintf(buff,"%s",sfontcolor);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\">\n");
	LBS_EmitUTF8(html,HI,SRC_CODESET);
	LBS_EmitString(html,"</FONT></TH>");
	LBS_EmitUTF8(html,ATAMA,SRC_CODESET);
	one = youbi(yy,mm,1);
	for	( sun = 1 - one ; sun <= 31 ; sun += 7 ) {
		LBS_EmitString(html,"<TR align=\"center\">");
		for	( i = sun ; i < sun + 7 ; i ++ ) {
			if		(	(  i  >   0     )
					&&	(  i  <=  lday  ) ) {
				LBS_EmitString(html,"<TD>");
				sprintf(buff,"%d",i);
				if		(  day  !=  NULL  ) {
					LBS_EmitString(html,"<input type=\"radio\" name=\"");
					LBS_EmitString(html,day);
					LBS_EmitString(html,"\" value=\"");
					LBS_EmitString(html,buff);
					LBS_EmitString(html,"\"");
					if		(  i  ==  dd  ) {
						LBS_EmitString(html," checked>");
					} else {
						LBS_EmitString(html,">");
					}
					LBS_EmitString(html,buff);
				}
			} else {
				LBS_EmitString(html,"<TD>");
			}
			LBS_EmitString(html,"</TD>");
		}
		LBS_EmitString(html,"</TR>\n");
	}
	LBS_EmitString(html,"</TABLE></TD></TR></TABLE>");
LEAVE_FUNC;
}

static	char	*
ParseName(
	char	*str)
{
	static	char	buff[SIZE_BUFF]
			,		name[SIZE_BUFF];
	char	*p
	,		*q;
	Expr	*var;
	char	*outvalue(
		char	*pp,
		Expr	*val)
	{
		switch	(val->type) {
		  case	VAR_INT:
			pp += sprintf(pp,"%d",val->body.ival);
			break;
		  case	VAR_STR:
			pp += sprintf(pp,"%s",val->body.sval);
			break;
		  default:
			pp += sprintf(pp,"(%d)",val->type);
			break;
		}
		return	(pp);
	}
		

ENTER_FUNC;
dbgprintf("name = [%s]\n",str);
	if		(  str  ==  NULL  )	return	NULL;
	p = buff;
	while	(  *str  !=  0  ) {
		switch	(*str) {
		  case	'$':
			str ++;
			if		(  *str  ==  '('  ) {
				str ++;
				q = name;
				while	(  *str !=  ')'  ) {
					*q ++ = *str ++;
				}
				str ++;
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p = outvalue(p,var);
				} else {
					p += sprintf(p,"$%s",name);
				}
			} else {
				q = name;
				while	(  isalnum(*str)  ) {
					*q ++ = *str ++;
				}
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p = outvalue(p,var);
				} else {
					p += sprintf(p,"$%s",name);
				}
			}
			break;
		  case	'#':
			str ++;
			if		(  *str  ==  '('  ) {
				str ++;
				q = name;
				while	(  *str !=  ')'  ) {
					*q ++ = *str ++;
				}
				str ++;
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p = outvalue(p,var);
				} else {
					p += sprintf(p,"#%s",name);
				}
			} else {
				q = name;
				while	(  isalnum(*str)  ) {
					*q ++ = *str ++;
				}
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p = outvalue(p,var);
				} else {
					p += sprintf(p,"#%s",name);
				}
			}
			break;
		  case	'\\':
			str ++;
			/*	through	*/
		  default:
			*p ++ = *str;
			str ++;
		}
	}
	*p = 0;
LEAVE_FUNC;
	return	(buff);
}

static size_t
EncodeURI(char *q, byte *p)
{
	char	*qq;

	qq = q;
	while (*p != 0) {
        if (*p <= 0x20 || *p >= 0x7f) {
            *q++ = '%';
            q += sprintf(q, "%02X", ((int) *p) & 0xFF);
        }
        else {
            switch (*p) {
                /* delims */
              case '<': case '>': case '#': case '%': case '"':
                /* unwise */
              case '{': case '}': case '|': case '\\': case '^':
              case '[': case ']': case '`':
                /* reserved */
              case ';': case '/': case '?': case ':': case '@':
              case '&': case '=': case '+': case '$': case ',':
                *q++ = '%';
                q += sprintf(q, "%02X", ((int) *p) & 0xFF);
                break;
              default:
                *q ++ = *p;
                break;
            }
        }
		p++;
	}
	*q = 0;
	return q - qq;
}

static size_t
EncodeLengthURI(byte *p)
{
	size_t ret;

    ret = 0;
	while (*p != 0) {
        if (*p <= 0x20 || *p >= 0x7f) {
            ret += 3;
        }
        else {
            switch (*p) {
                /* delims */
              case '<': case '>': case '#': case '%': case '"':
                /* unwise */
              case '{': case '}': case '|': case '\\': case '^':
              case '[': case ']': case '`':
                /* reserved */
              case ';': case '/': case '?': case ':': case '@':
              case '&': case '=': case '+': case '$': case ',':
                ret += 3;
                break;
              default:
                ret++;
                break;
            }
        }
		p++;
	}
	return ret;
}

static char *
EscapeJavaScriptString(char *str)
{
    char *p, *rp, *result;
    int len;

    len = 0;
    for (p = str; *p != '\0'; p++) {
        switch (*p) {
          case '\'':
          case '\\':
            len += 2;
            break;
          default:
            len++;
            break;
        }
    }

    result = (char *) xmalloc(len + 1);
    rp = result;
    for (p = str; *p != '\0'; p++) {
        switch (*p) {
          case '\'':
          case '\\':
            *rp++ = '\\';
            *rp++ = *p;
            break;
          default:
            *rp++ = *p;
            break;
        }
    }
    *rp = '\0';
    return result;
}

static	size_t
EmitChar(
	LargeByteString	*lbs,
	unsigned char	*str)
{
	size_t	len;
	byte	code[2]
		,	*p = str;
	char	buff[SIZE_BUFF]
		,	format[SIZE_BUFF];

	if		(  ( p[0] & 0xE0 )  ==  0xE0  ) {	/*	3byte code	*/
		code[0]  = ( 0x0F & p[0] ) << 4;
		code[0] |= ( 0x3C & p[1] ) >> 2;
		code[1]  = ( 0x03 & p[1] ) << 6;
		code[1] |= ( 0x3F & p[2] );
		if		(	(  code[0]  >=  0xE0  )
				&&	(  code[0]  <   0xF8  ) ) {
			sprintf(format,
					"<img src=\"%s\" alt=\"%%02X%%02X\""
					" style=\"vertical-align:text-bottom\">",FontTemplate);
			sprintf(buff,format,(int)code[0],(int)code[1],(int)code[0],(int)code[1]);
			LBS_EmitString(lbs,buff);
		} else {
			LBS_EmitChar(lbs,p[0]);
			LBS_EmitChar(lbs,p[1]);
			LBS_EmitChar(lbs,p[2]);
		}
		len = 3;
	} else
	if		(  ( p[0] & 0xC0 )  ==  0xC0  ) {	/*	2byte code	*/
		LBS_EmitChar(lbs,p[0]);
		LBS_EmitChar(lbs,p[1]);
		len = 2;
	} else {
		LBS_EmitChar(lbs,p[0]);
		len = 1;
	}
	return	(len);
}

extern	void
EmitWithEscape(
	LargeByteString	*lbs,
	unsigned char	*str)
{
	while	(  *str  !=  0  ) {
        switch (*str) {
		  case CHAR_NIL:
			str ++;
			break;
		  case '<':
			LBS_EmitString(lbs, "&lt;");
			str ++;
			break;
		  case '>':
			LBS_EmitString(lbs, "&gt;");
			str ++;
			break;
		  case '&':
			LBS_EmitString(lbs, "&amp;");
			str ++;
			break;
		  case '"':
			LBS_EmitString(lbs, "&quot;");
			str ++;
			break;
		  case ' ':
			LBS_EmitString(lbs, "&nbsp;");
			str ++;
			break;
		  default:
			str += EmitChar(lbs, str);
			break;
        }
	}
}

extern	void
EmitStringRaw(
	LargeByteString	*lbs,
	unsigned char	*str)
{
#if	0
	LBS_EmitString(lbs,str);
#else
	while	(  *str  !=  0  ) {
		str += EmitChar(lbs, str);
	}
#endif
}

static	void
OutputJs(
	LargeByteString	*html)
{
	void	_OutJs(
		char	*name,
		Js		*js)
	{
		if		(	(  js->fUse   )
				&&	(  !js->fFile  ) ) {
			LBS_EmitString(html,js->body);
		}
	}
	void	_OutJsFile(
		char	*name,
		Js		*js)
	{
		char	buff[SIZE_BUFF];

		if		(	(  js->fUse   )
				&&	(  js->fFile  ) ) {
			sprintf(buff,
					"\n<script language=\"javascript\" "
					"type=\"text/javascript\" "
					"src=\"%s\"></script>\n",
					js->body);
			LBS_EmitString(html,buff);
		}
	}

	if		(  Jslib  !=  NULL  ) {
		g_hash_table_foreach(Jslib,(GHFunc)_OutJsFile,NULL);
		LBS_EmitString(html,
					   "\n<script language=\"javascript\" "
					   "type=\"text/javascript\">\n"
					   "<!--\n");
		g_hash_table_foreach(Jslib,(GHFunc)_OutJs,NULL);
		LBS_EmitString(html,
					   "\n-->\n"
					   "</script>\n");
	}
}

extern	void
ExecCode(
	LargeByteString	*html,
	HTCInfo	*htc)
{
	int		c;
	char	*name
	,		*value
	,		*str;
	Expr	*var
	,		vval;
	size_t	pos;
	char	buff[SIZE_BUFF];

ENTER_FUNC;
	RewindLBS(htc->code);
	dbgprintf("code size = %d\n",LBS_Size(htc->code));
	VarArea = NewNameHash();
	pStack = 0;
	while	(  ( c = LBS_FetchChar(htc->code) )  !=  0  ) {
		if		(  c  ==  0x01  ) {	/*	code escape	*/
			switch	(LBS_FetchChar(htc->code)) {
			  case	OPC_DROP:
				(void)Pop;
				break;
			  case	OPC_REF:
				dbgmsg("OPC_REF");
				name = LBS_FetchPointer(htc->code);
				value = GetHostValue(name,FALSE);
				if		(  ( value = GetHostValue(name,FALSE) )  !=  NULL  ) {
					EmitWithEscape(html,value);
				}
				break;
			  case	OPC_REFINT:
				dbgmsg("OPC_REFINT");
				vval = Pop;
				sprintf(buff,"%d",vval.body.ival);
				EmitWithEscape(html,buff);
				break;
			  case	OPC_VAR:
				dbgmsg("OPC_VAR");
				name = LBS_FetchPointer(htc->code);
				if		(  name  ==  NULL  )	name = "";
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  ==  NULL  ) {
					var = New(Expr);
					g_hash_table_insert(VarArea,StrDup(name),var);
				}
				vval.body.ptr = var;
				vval.type = VAR_PTR;
				Push(vval);
				break;
			  case	OPC_EVAL:
				dbgmsg("OPC_EVAL");
				str = LBS_FetchPointer(htc->code);
				name = ParseName(str);
				vval.body.sval = StrDup(name);
				vval.type = VAR_STR;
				Push(vval);
				break;
			  case	OPC_TOINT:
				dbgmsg("OPC_TOINT");
				if		(  TOP(1).body.sval  !=  NULL  ) {
					TOP(1).body.ival = atoi(TOP(1).body.sval);
				} else {
					TOP(1).body.ival = 0;
				}
				TOP(1).type = VAR_INT;
				break;
			  case	OPC_PHSTR:
				dbgmsg("OPC_PHSTR");
				vval = Pop;
				vval.body.sval = GetHostValue(vval.body.sval,FALSE);
				dbgprintf("%s",vval.body.sval);
				vval.type = VAR_STR;
				Push(vval);
				break;
			  case	OPC_PHINT:
				dbgmsg("OPC_PHINT");
				vval = Pop;
				vval.body.ival = atoi(GetHostValue(vval.body.sval,FALSE));
				vval.type = VAR_INT;
				Push(vval);
				break;
			  case	OPC_PHBOOL:
				dbgmsg("OPC_PHBNAME");
				value = GetHostValue(TOP(1).body.sval,FALSE);
				TOP(1).body.ival = (stricmp(value,"TRUE") == 0);
				TOP(1).type = VAR_INT;
				break;
			  case	OPC_HIVAR:
				dbgmsg("OPC_HIVAR");
				name = LBS_FetchPointer(htc->code);
                str = ParseName(name);
                vval.body.ival = atoi(GetHostValue(StrDup(str),FALSE));
				vval.type = VAR_INT;
				Push(vval);
				break;
			  case	OPC_HBES:
				dbgmsg("OPC_HBES");
				vval = Pop;
#if	1
				value = GetHostValue(vval.body.sval,TRUE);
#else
				value = GetHostValue(vval.body.sval,FALSE);
#endif
				str = LBS_FetchPointer(htc->code);
				if		(  stricmp(value,"TRUE")  ==  0 ) {
					EmitWithEscape(html,str);
				}
				break;
			  case	OPC_SPY:
				dbgmsg("OPC_SPY");
				switch	(TOP(1).type) {
				  case	VAR_INT:
					sprintf(buff,"%d",TOP(1).body.ival);
					break;
				  case	VAR_STR:
					sprintf(buff,"%s",TOP(1).body.sval);
				}
				EmitWithEscape(html,TOP(1).body.sval);
				break;
			  case	OPC_ICONST:
				dbgmsg("OPC_ICONST");
				vval.body.ival = LBS_FetchInt(htc->code);
				vval.type = VAR_INT;
				Push(vval);
				break;
			  case	OPC_SCONST:
				dbgmsg("OPC_SCONST");
				vval.body.sval = LBS_FetchPointer(htc->code);
				vval.type = VAR_STR;
				Push(vval);
				break;
			  case	OPC_STORE:
				dbgmsg("OPC_STORE");
				vval = Pop;
				*(TOP(1).body.ptr) = vval;
				break;
			  case	OPC_LDVAR:
				dbgmsg("OPC_LDVAR");
				name = LBS_FetchPointer(htc->code);
				if		(  name  ==  NULL  )	name = "";
				var = g_hash_table_lookup(VarArea,name);
                vval = *var;
                Push(vval);
				break;
			  case	OPC_LEND:
				dbgmsg("OPC_LEND");
				(TOP(3).body.ptr)->body.ival += TOP(1).body.ival;
				pos = LBS_FetchInt(htc->code);
				LBS_SetPos(htc->code,pos);
				break;
			  case	OPC_BREAK:
				dbgmsg("OPC_BREAK");
				pos = LBS_FetchInt(htc->code);
				if		(  (TOP(3).body.ptr)->body.ival  >=  TOP(2).body.ival  ) {
					LBS_SetPos(htc->code,pos);
					(void)Pop;
					(void)Pop;
					(void)Pop;
				}
				break;
			  case	OPC_JNZP:
				dbgmsg("OPC_JNZP");
				pos = LBS_FetchInt(htc->code);
				if		(  TOP(1).body.ival  ==  0  ) {
					LBS_SetPos(htc->code,pos);
				}
				break;
			  case	OPC_JNZNP:
				dbgmsg("OPC_JNZNP");
				pos = LBS_FetchInt(htc->code);
				if		(  TOP(1).body.ival  !=  0  ) {
					LBS_SetPos(htc->code,pos);
				}
				(void)Pop;
				break;
			  case	OPC_SUB:
				TOP(2).body.ival = TOP(2).body.ival - TOP(1).body.ival;
				(void)Pop;
				break;
			  case	OPC_SCMP:
				TOP(2).body.ival = strcmp(TOP(2).body.sval,TOP(1).body.sval);
				(void)Pop;
				break;
			  case OPC_LOCURI:
			    {
                    int len;
                    char *local, *encoded;

                    dbgmsg("OPC_LOCURI");
                    vval = Pop;
                    local = ConvLocal(vval.body.sval);
                    len = EncodeLengthURI(local);
                    encoded = (char *) xmalloc(len + 1);
                    EncodeURI(encoded, (byte *)local);
                    vval.body.sval = encoded;
					vval.type = VAR_STR;
                    Push(vval);
                }
                break;
			  case OPC_UTF8URI:
                {
                    int len;
                    char *encoded;

                    dbgmsg("OPC_UTF8URI");
                    vval = Pop;
                    len = EncodeLengthURI(vval.body.sval);
                    encoded = (char *) xmalloc(len + 1);
                    EncodeURI(encoded, vval.body.sval);
                    vval.body.sval = encoded;
					vval.type = VAR_STR;
                    Push(vval);
                }
                break;
              case OPC_EMITSTR:
				dbgmsg("OPC_EMITSTR");
				vval = Pop;
				EmitWithEscape(html,vval.body.sval);
				break;
              case OPC_EMITRAW:
				dbgmsg("OPC_EMITRAW");
				vval = Pop;
                if (strlen(vval.body.sval) > 0) {
					EmitStringRaw(html,vval.body.sval);
                }
				break;
			  case OPC_ESCJSS:
                {
                    dbgmsg("OPC_ESCJSS");
                    vval = Pop;
                    vval.body.sval = EscapeJavaScriptString(vval.body.sval);
					vval.type = VAR_STR;
                    Push(vval);
                }
                break;
			  case	OPC_CALENDAR:
			  {
				  char	*year
				  ,		*month
				  ,		*day;
				  int	yy
				  ,		mm
				  ,		dd;
				  int	lborder
				  ,		lcellspacing
				  ,		lcellpadding
				  ,		lfontsize
				  ,		sborder
				  ,		scellspacing
				  ,		scellpadding
				  ,		sfontsize;
				  char	*lbgcolor
				  ,		*lfontcolor
				  ,		*sbgcolor
				  ,		*sfontcolor;

				  day = Pop.body.sval;
				  month = Pop.body.sval;
				  year = Pop.body.sval;
				  dd = Pop.body.ival;
				  mm = Pop.body.ival;
				  yy = Pop.body.ival;

				  lborder = LBS_FetchInt(htc->code);
				  lcellspacing = LBS_FetchInt(htc->code);
				  lcellpadding = LBS_FetchInt(htc->code);
				  lfontsize = LBS_FetchInt(htc->code);
				  sborder = LBS_FetchInt(htc->code);
				  scellspacing = LBS_FetchInt(htc->code);
				  scellpadding = LBS_FetchInt(htc->code);
				  sfontsize = LBS_FetchInt(htc->code);

				  lbgcolor = LBS_FetchPointer(htc->code);
				  lfontcolor = LBS_FetchPointer(htc->code);
				  sbgcolor = LBS_FetchPointer(htc->code);
				  sfontcolor = LBS_FetchPointer(htc->code);

				  ExecCalendar(html,yy,mm,dd,
					   lborder,lcellspacing,lcellpadding,
					   lbgcolor,lfontcolor,lfontsize,
					   sborder,scellspacing,scellpadding,
					   sbgcolor,sfontcolor,sfontsize,
					   year,month,day);
			  }
				break;
			  case	OPC_FLJS:
				OutputJs(html);
				break;
			  default:
				fprintf(stderr,"invalid opcode\n");
				break;
			}
		} else {
#ifdef	DEBUG
			dbgprintf("%c",c);
#endif
			LBS_EmitChar(html,c);
		}
	}
	LBS_EmitEnd(html);
LEAVE_FUNC;
}

static	void
_RadioReset(
	char	*name)
{
	SaveValue(name,"FALSE",FALSE);
}

static	void
GetRadio(
	char	*name,
	GHashTable	*ritem)
{
	char	*rname;

	g_hash_table_foreach(ritem,(GHFunc)_RadioReset,NULL);
	if		(  ( rname = LoadValue(name) )  !=  NULL  ) {
		SaveValue(rname,"TRUE",FALSE);
		RemoveValue(name);
	}
}

static	void
GetToggle(
	char	*name)
{
	if		(  LoadValue(name)  ==  NULL  ) {
		SaveValue(name,"FALSE",FALSE);
	} else {
		SaveValue(name,"TRUE",FALSE);
	}
}

extern	char	*
ParseInput(
	HTCInfo	*htc)
{
	char	*button;
	char	*event;

	void	ToUTF8(
		char	*name,
		CGIValue	*val)
	{
		char	*u8;

		if		(  val->body  !=  NULL  ) {
			u8 = ConvUTF8((byte *)val->body,Codeset);
			xfree(val->body);
			val->body = StrDup(u8);
		}
	}
	void	FileNameToUTF8(
		char	*name,
		MultipartFile	*file)
	{
		char	*u8;

		if		(  file->filename  !=  NULL  ) {
			u8 = ConvUTF8((byte *)file->filename,Codeset);
			xfree(file->filename);
			file->filename = StrDup(u8);
		}
	}
	void	_Dump(
		char	*name,
		char	*trans)
	{
		dbgprintf("[%s]",name);
		dbgprintf("[%s]\n",trans);
	}
	
ENTER_FUNC;
 	g_hash_table_foreach(Values,(GHFunc)ToUTF8,NULL);
 	g_hash_table_foreach(Files,(GHFunc)FileNameToUTF8,NULL);
#ifdef	DEBUG
	if		(  htc  !=  NULL  ) {
		g_hash_table_foreach(htc->Trans,(GHFunc)_Dump,NULL);
	}
#endif
	if		(	(  ( button = LoadValue("_event") )  ==  NULL  )
			||	(  *button  ==  0  ) ) {
		if (htc->DefaultEvent == NULL) {
			event = "";
			htc = NULL;
		} else {
			event = htc->DefaultEvent;
		}
	} else {
#ifdef	USE_IE5
		dbgprintf("button = [%s]\n",button);
		event = g_hash_table_lookup(htc->Trans,button);
		if (event == NULL) {
			event = button;
		}
#else
		if		(	(  fJavaScript     )
				&&	(  !fNoJavaScript  ) ) {
			event = button;
		} else {
			dbgprintf("button = [%s]\n",button);
			event = g_hash_table_lookup(htc->Trans,button);
			if (event == NULL) {
				event = button;
			}
		}
#endif
		dbgprintf("event  = [%s]\n",event);
	}
	if		(  htc  !=  NULL  ) {
		g_hash_table_foreach(htc->Radio,(GHFunc)GetRadio,NULL);
	}
	if		(  htc  !=  NULL  ) {
		g_hash_table_foreach(htc->Toggle,(GHFunc)GetToggle,NULL);
	}
	if		(  htc  !=  NULL  ) {
		g_hash_table_foreach(htc->Check,(GHFunc)GetToggle,NULL);
	}
LEAVE_FUNC;
	return	(event);
}
