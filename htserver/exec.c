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
#include	<glib.h>
#include	<ctype.h>
#include	<iconv.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"mon.h"
#include	"exec.h"
#include	"debug.h"

#define	SRC_CODESET		"euc-jp"
#define	NEN		"年"
#define	TSUKI	"月</TH></TR><TR align=\"center\">"
#define	ATAMA	"<TH>日</TH><TH>月</TH><TH>火</TH><TH>水</TH><TH>木</TH><TH>金</TH><TH>土</TH></TR>\n"

#define	SIZE_RSTACK		100

typedef	union	_VarType	{
	size_t		size
	,			pos;
	int			ival;
	Bool		bval;
	char		*str;
	union	_VarType	*ptr;
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
	printf("pStack = %d\n",pStack);
	printf("val = %d\n",v.ival);
	Stack[pStack ++] = v;
}
#else
#define	Push(v)		Stack[pStack ++] = (v)
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

	if		(	(  ( yy % 4 )    ==  0  )
			&&	(  ( yy % 4 )    !=  0  )
			||	(  ( yy % 400 )  ==  0  ) ) {
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

#define	SIZE_CHARS		16
extern	char	*
LBS_EmitUTF8(
	LargeByteString	*lbs,
	char			*str,
	char			*codeset)
{
	char	*oc
	,		*istr;
	char	obuff[SIZE_CHARS];
	size_t	count
	,		sib
	,		sob
	,		csize;
	int		rc;
	iconv_t	cd;
	int		i;

ENTER_FUNC;
	if		(	(  libmondai_i18n  )
			&&	(  codeset  !=  NULL  ) ) {
		cd = iconv_open("utf8",codeset);
		while	(  *str  !=  0  )	{
			count = 1;
			do {
				istr = str;
				sib = count;
				oc = obuff;
				sob = SIZE_CHARS;
				if		(  ( rc = iconv(cd,&istr,&sib,&oc,&sob) )  <  0  ) {
					count ++;
				}
			}	while	(	(  rc            !=  0  )
						&&	(  str[count-1]  !=  0  ) );
			csize = SIZE_CHARS - sob;
			for	( oc = obuff , i = 0 ; i < csize ; i ++, oc ++ ) {
				LBS_Emit(lbs,*oc);
			}
			str += count;
		}
		iconv_close(cd);
	} else {
		while	(  *str  !=  0  ) {
			LBS_Emit(lbs,*str);
			str ++;
		}
	}
LEAVE_FUNC;
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

dbgmsg(">ExecCalendar");
	lday = tlday[mm - 1];
	if		(	(  mm  ==  2  )
			&&	(  uru(yy)    ) ) {
		lday ++;
	}

	LBS_EmitString(html,"<INPUT TYPE=\"hidden\" name=\"");
	LBS_EmitString(html,year);
	LBS_EmitString(html,"\" value=\"");
	sprintf(buff,"%d",yy);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\">\n");

	LBS_EmitString(html,"<INPUT TYPE=\"hidden\" name=\"");
	LBS_EmitString(html,month);
	LBS_EmitString(html,"\" value=\"");
	sprintf(buff,"%d",mm);
	LBS_EmitString(html,buff);
	LBS_EmitString(html,"\">\n");

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
dbgmsg("<ExecCalendar");
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

	if		(  str  ==  NULL  )	return	NULL;
	p = buff;
	while	(  *str  !=  0  ) {
		if		(  *str  ==  '$'  ) {
			str ++;
			if		(  *str ==  '$'  ) {
				*p ++ = '$';
			} else 
			if		(  *str  ==  '('  ) {
				str ++;
				q = name;
				while	(  *str !=  ')'  ) {
					*q ++ = *str ++;
				}
				str ++;
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p += sprintf(p,"%s",var->str);
				}
			} else {
				q = name;
				while	(  isalnum(*str)  ) {
					*q ++ = *str ++;
				}
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p += sprintf(p,"%s",var->str);
				}
			}
		} else
		if		(  *str  ==  '#'  ) {
			str ++;
			if		(  *str ==  '#'  ) {
				*p ++ = '#';
			} else 
			if		(  *str  ==  '('  ) {
				str ++;
				q = name;
				while	(  *str !=  ')'  ) {
					*q ++ = *str ++;
				}
				str ++;
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p += sprintf(p,"%d",var->ival);
				}
			} else {
				q = name;
				while	(  isalnum(*str)  ) {
					*q ++ = *str ++;
				}
				*q = 0;
				if		(  ( var = g_hash_table_lookup(VarArea,name) )  !=  NULL  ) {
					p += sprintf(p,"%d",var->ival);
				}
			}
		} else {
			*p ++ = *str;
			str ++;
		}
	}
	*p = 0;
	return	(buff);
}

static	char	*
HTGetValue(
	char		*name,
	Bool		fClear)
{
	char	buff[SIZE_BUFF+1];
	char	*value;

	if		(  *name  ==  0  ) {
		value = "";
	} else
	if		(  ( value = g_hash_table_lookup(Values,name) )  ==  NULL  ) {
		sprintf(buff,"%s%s\n",name,(fClear ? " clear" : "" ));
		HT_SendString(buff);
		HT_RecvString(SIZE_BUFF,buff);
		value = StrDup(buff);
		g_hash_table_insert(Values,StrDup(name),value);
	}
	return	(value);
}

static void
EmitWithEscape(LargeByteString *lbs, char *str)
{
    char *p;
    for (p = str; *p != '\0'; p++) {
        switch (*p) {
        case '&':
            LBS_EmitString(lbs, "&amp;");
            break;
        case '"':
            LBS_EmitString(lbs, "&quot;");
            break;
        case '<':
            LBS_EmitString(lbs, "&lt;");
            break;
        case '>':
            LBS_EmitString(lbs, "&gt;");
            break;
        default:
            LBS_EmitChar(lbs, *p);
            break;
        }
	}
}

extern	LargeByteString	*
ExecCode(
	HTCInfo	*htc)
{
	LargeByteString	*html;
	int		c;
	char	*name
	,		*value
	,		*str;
	VarType	*var
	,		vval;
	size_t	pos;
	char	buff[SIZE_BUFF];

dbgmsg(">ExecCode");
	html = NewLBS();
	LBS_EmitStart(html);
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
				value = HTGetValue(name,FALSE);
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
				vval.ptr = var;
				Push(vval);
				break;
			  case	OPC_NAME:
				dbgmsg("OPC_NAME");
				str = LBS_FetchPointer(htc->code);
				name = ParseName(str);
				vval.str = StrDup(name);
				Push(vval);
				break;
			  case	OPC_HSNAME:
				dbgmsg("OPC_HSNAME");
				vval = Pop;
				vval.str = HTGetValue(vval.str,FALSE);
				Push(vval);
				break;
			  case	OPC_EHSNAME:
				dbgmsg("OPC_EHSNAME");
				vval = Pop;
				value = HTGetValue(vval.str,FALSE);
				EmitWithEscape(html,value);
				break;
			  case	OPC_HINAME:
				dbgmsg("OPC_HINAME");
				vval = Pop;
				vval.ival = atoi(HTGetValue(vval.str,FALSE));
				Push(vval);
				break;
			  case	OPC_HIVAR:
				dbgmsg("OPC_HIVAR");
				name = LBS_FetchPointer(htc->code);
				vval.ival = atoi(HTGetValue(name,FALSE));
				Push(vval);
				break;
			  case	OPC_HBES:
				dbgmsg("OPC_HBES");
				vval = Pop;
				value = HTGetValue(vval.str,TRUE);
				str = LBS_FetchPointer(htc->code);
				if		(  stricmp(value,"TRUE")  ==  0 ) {
					EmitWithEscape(html,str);
				}
				break;
			  case	OPC_REFSTR:
				dbgmsg("OPC_REFSTR");
				vval = Pop;
				EmitWithEscape(html,vval.str);
				break;
			  case	OPC_REFINT:
				dbgmsg("OPC_REFINT");
				vval = Pop;
				sprintf(buff,"%d",(vval.ptr)->ival);
				EmitWithEscape(html,buff);
				break;
			  case	OPC_ICONST:
				dbgmsg("OPC_ICONST");
				vval.ival = LBS_FetchInt(htc->code);
				Push(vval);
				break;
			  case	OPC_SCONST:
				dbgmsg("OPC_SCONST");
				vval.str = LBS_FetchPointer(htc->code);
				Push(vval);
				break;
			  case	OPC_STORE:
				dbgmsg("OPC_STORE");
				vval = Pop;
				*(TOP(1).ptr) = vval;
				break;
			  case	OPC_LEND:
				dbgmsg("OPC_LEND");
				(TOP(3).ptr)->ival += TOP(1).ival;
				pos = LBS_FetchInt(htc->code);
				LBS_SetPos(htc->code,pos);
				break;
			  case	OPC_BREAK:
				dbgmsg("OPC_BREAK");
				pos = LBS_FetchInt(htc->code);
				if		(  (TOP(3).ptr)->ival  >=  TOP(2).ival  ) {
					LBS_SetPos(htc->code,pos);
					(void)Pop;
					(void)Pop;
					(void)Pop;
				}
				break;
			  case	OPC_JNZP:
				dbgmsg("OPC_JNZP");
				pos = LBS_FetchInt(htc->code);
				if		(  TOP(1).ival  ==  0  ) {
					LBS_SetPos(htc->code,pos);
				}
				break;
			  case	OPC_JNZNP:
				dbgmsg("OPC_JNZNP");
				pos = LBS_FetchInt(htc->code);
				if		(  TOP(1).ival  !=  0  ) {
					LBS_SetPos(htc->code,pos);
				}
				break;
			  case OPC_URLENC:
			  {
                int len;
                char *s;

				dbgmsg("OPC_URLENC");
				vval = Pop;
                len = EncodeStringLengthURL(vval.str);
				s = (char *) xmalloc(len + 1);
                EncodeStringURL(s, vval.str);
                vval.str = s;
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

				  day = Pop.str;
				  month = Pop.str;
				  year = Pop.str;
				  dd = Pop.ival;
				  mm = Pop.ival;
				  yy = Pop.ival;
				  ExecCalendar(html,yy,mm,dd,year,month,day);
			  }
				break;
			  default:
				fprintf(stderr,"invalid opcode\n");
				break;
			}
		} else {
#ifdef	DEBUG
			printf("%c",c);
#endif
			LBS_EmitChar(html,c);
		}
	}
	LBS_EmitEnd(html);
dbgmsg("<ExecCode");
	return	(html);
}
