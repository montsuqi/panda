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

#define	DEBUG
#define	TRACE
/*
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
#include	"htc.h"
#include	"cgi.h"
#include	"multipart.h"
#include	"exec.h"
#include	"debug.h"

#define	SRC_CODESET		"euc-jp"
#define	NEN		"年"
#define	TSUKI	"月</TH></TR><TR align=\"center\">"
#define	ATAMA	"<TH>日</TH><TH>月</TH><TH>火</TH><TH>水</TH><TH>木</TH><TH>金</TH><TH>土</TH></TR>\n"

#define	SIZE_RSTACK		100

#define	VAR_NULL	0
#define	VAR_INT		1
#define	VAR_STR		2
#define	VAR_PTR		3

typedef	struct	_VarType	{
	int			type;
	union {
		int			ival;
		char		*str;
		struct	_VarType	*ptr;
	}	body;
}	VarType;

static	VarType	Stack[SIZE_RSTACK];
static	size_t	pStack;
static	GHashTable	*VarArea;


#define	Pop			Stack[-- pStack]
#define	TOP(n)		Stack[pStack-(n)]

#ifdef	DEBUG
static	void
Push(
	VarType	v)
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

	LBS_EmitString(html,"<TABLE BORDER><TR align=\"center\"><TH colspan=7>\n");
	sprintf(buff,"%d",yy);
	LBS_EmitString(html,buff);
	LBS_EmitUTF8(html,NEN,SRC_CODESET);
	sprintf(buff,"%d",mm);
	LBS_EmitString(html,buff);
	LBS_EmitUTF8(html,TSUKI,SRC_CODESET);
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
				}
				LBS_EmitString(html,buff);
			} else {
				LBS_EmitString(html,"<TD>");
			}
			LBS_EmitString(html,"</TD>");
		}
		LBS_EmitString(html,"</TR>\n");
	}
	LBS_EmitString(html,"</TABLE>");
LEAVE_FUNC;
}

static	char	*
ParseName(
	char	*str)
{
	static	char	buff[SIZE_ARG]
			,		name[SIZE_ARG];
	char	*p
	,		*q;
	VarType	*var;
	char	*outvalue(
		char	*pp,
		VarType	*val)
	{
		switch	(val->type) {
		  case	VAR_INT:
			pp += sprintf(pp,"%d",val->body.ival);
			break;
		  case	VAR_STR:
			pp += sprintf(pp,"%s",val->body.str);
			break;
		  default:
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
				}
			} else {
				q = name;
				while	(  isalnum(*str)  ) {
					*q ++ = *str ++;
				}
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p = outvalue(p,var);
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
				}
			} else {
				q = name;
				while	(  isalnum(*str)  ) {
					*q ++ = *str ++;
				}
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p = outvalue(p,var);
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

static void
EmitWithEscape(LargeByteString *lbs, char *str)
{
    char *p;

    for (p = str; *p != '\0'; p++) {
        switch (*p) {
        case CHAR_NIL:
            break;
        case '<':
            LBS_EmitString(lbs, "&lt;");
            break;
        case '>':
            LBS_EmitString(lbs, "&gt;");
            break;
        case '&':
            LBS_EmitString(lbs, "&amp;");
            break;
        case '"':
            LBS_EmitString(lbs, "&quot;");
            break;
        case ' ':
            LBS_EmitString(lbs, "&nbsp;");
            break;
        default:
            LBS_EmitChar(lbs, *p);
            break;
        }
	}
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

extern	void
ExecCode(
	LargeByteString	*html,
	HTCInfo	*htc)
{
	int		c;
	char	*name
	,		*value
	,		*str;
	VarType	*var
	,		vval;
	size_t	pos;
	char	buff[SIZE_BUFF];

ENTER_FUNC;
	RewindLBS(htc->code);
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
                EmitWithEscape(html,value);
				break;
			  case	OPC_VAR:
				dbgmsg("OPC_VAR");
				name = LBS_FetchPointer(htc->code);
				if		(  name  ==  NULL  )	name = "";
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  ==  NULL  ) {
					var = New(VarType);
					g_hash_table_insert(VarArea,StrDup(name),var);
				}
				vval.body.ptr = var;
				vval.type = VAR_PTR;
				Push(vval);
				break;
			  case	OPC_NAME:
				dbgmsg("OPC_NAME");
				str = LBS_FetchPointer(htc->code);
				name = ParseName(str);
				vval.body.str = StrDup(name);
				vval.type = VAR_STR;
				Push(vval);
				break;
			  case	OPC_TOINT:
				dbgmsg("OPC_TOINT");
				TOP(1).body.ival = atoi(TOP(1).body.str);
				TOP(1).type = VAR_INT;
				break;
			  case	OPC_HSNAME:
				dbgmsg("OPC_HSNAME");
				vval = Pop;
				vval.body.str = GetHostValue(vval.body.str,FALSE);
				vval.type = VAR_STR;
				Push(vval);
				break;
			  case	OPC_EHSNAME:
				dbgmsg("OPC_EHSNAME");
				vval = Pop;
				value = GetHostValue(vval.body.str,FALSE);
				EmitWithEscape(html,value);
				break;
			  case	OPC_HINAME:
				dbgmsg("OPC_HINAME");
				vval = Pop;
				vval.body.ival = atoi(GetHostValue(vval.body.str,FALSE));
				vval.type = VAR_INT;
				Push(vval);
				break;
			  case	OPC_HBNAME:
				dbgmsg("OPC_HBNAME");
				value = GetHostValue(TOP(1).body.str,FALSE);
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
				value = GetHostValue(vval.body.str,TRUE);
				str = LBS_FetchPointer(htc->code);
				if		(  stricmp(value,"TRUE")  ==  0 ) {
					EmitWithEscape(html,str);
				}
				break;
			  case	OPC_REFSTR:
				dbgmsg("OPC_REFSTR");
				vval = Pop;
				EmitWithEscape(html,vval.body.str);
				break;
			  case	OPC_SPY:
				dbgmsg("OPC_SPY");
				switch	(TOP(1).type) {
				  case	VAR_INT:
					sprintf(buff,"%d",TOP(1).body.ival);
					break;
				  case	VAR_STR:
					sprintf(buff,"%s",TOP(1).body.str);
				}
				EmitWithEscape(html,TOP(1).body.str);
				break;
			  case	OPC_REFINT:
				dbgmsg("OPC_REFINT");
				vval = Pop;
				sprintf(buff,"%d",vval.body.ival);
				EmitWithEscape(html,buff);
				break;
			  case	OPC_ICONST:
				dbgmsg("OPC_ICONST");
				vval.body.ival = LBS_FetchInt(htc->code);
				vval.type = VAR_INT;
				Push(vval);
				break;
			  case	OPC_SCONST:
				dbgmsg("OPC_SCONST");
				vval.body.str = LBS_FetchPointer(htc->code);
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
				TOP(2).body.ival = strcmp(TOP(2).body.str,TOP(1).body.str);
				(void)Pop;
				break;
			  case OPC_LOCURI:
			    {
                    int len;
                    char *local, *encoded;

                    dbgmsg("OPC_LOCURI");
                    vval = Pop;
                    local = ConvLocal(vval.body.str);
                    len = EncodeLengthURI(local);
                    encoded = (char *) xmalloc(len + 1);
                    EncodeURI(encoded, local);
                    vval.body.str = encoded;
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
                    len = EncodeLengthURI(vval.body.str);
                    encoded = (char *) xmalloc(len + 1);
                    EncodeURI(encoded, vval.body.str);
                    vval.body.str = encoded;
					vval.type = VAR_STR;
                    Push(vval);
                }
                break;
              case OPC_EMITSTR:
				dbgmsg("OPC_EMITSTR");
				vval = Pop;
                if (strlen(vval.body.str) == 0) {
                    LBS_EmitString(html, "&nbsp;");
                }
                else {
                    EmitWithEscape(html,vval.body.str);
                }
				break;
              case OPC_EMITHTML:
				dbgmsg("OPC_EMITHTML");
				vval = Pop;
                if (strlen(vval.body.str) > 0) {
					LBS_EmitString(html,vval.body.str);
                }
				break;
			  case OPC_ESCJSS:
                {
                    dbgmsg("OPC_ESCJSS");
                    vval = Pop;
                    vval.body.str = EscapeJavaScriptString(vval.body.str);
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

				  day = Pop.body.str;
				  month = Pop.body.str;
				  year = Pop.body.str;
				  dd = Pop.body.ival;
				  mm = Pop.body.ival;
				  yy = Pop.body.ival;
				  ExecCalendar(html,yy,mm,dd,year,month,day);
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

extern	char	*
ParseInput(
	HTCInfo	*htc)
{
	char	*button;
	char	*event;

	void	GetRadio(
		char	*name)
	{
		char	*rname;

		if		(  ( rname = LoadValue(name) )  !=  NULL  ) {
			SaveValue(rname,"TRUE",FALSE);
			RemoveValue(name);
		}
	}
	void	ToUTF8(
		char	*name,
		CGIValue	*val)
	{
		char	*u8;

		if		(  val->body  !=  NULL  ) {
			u8 = ConvUTF8(val->body,Codeset);
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
			u8 = ConvUTF8(file->filename,Codeset);
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
		event = button;
#endif
		dbgprintf("event  = [%s]\n",event);
	}
	g_hash_table_foreach(htc->Radio,(GHFunc)GetRadio,NULL);
LEAVE_FUNC;
	return	(event);
}
