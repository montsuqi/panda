/*	PANDA -- a simple transaction monitor

Copyright (C) 2002 Ogochan & JMA (Japan Medical Association).

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
#include	"const.h"
#include	"types.h"
#include	"value.h"
#include	"mon.h"
#include	"exec.h"
#include	"misc.h"
#include	"debug.h"

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


#define	Pop			(Stack[-- pStack])
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

static	char	*
ParseName(
	char	*str)
{
	static	char	buff[SIZE_ARG]
			,		name[SIZE_ARG];
	char	*p
	,		*q;
	VarType	*var;

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
	char	buff[SIZE_BUFF];
	char	*value;

	if		(  *name  ==  0  ) {
		value = "";
	} else
	if		(  ( value = g_hash_table_lookup(Values,name) )  ==  NULL  ) {
		sprintf(buff,"%s%s\n",name,(fClear ? " clear" : "" ));
		HT_SendString(buff);
		HT_RecvString(SIZE_BUFF,buff);
		g_hash_table_insert(Values,StrDup(name),StrDup(buff));
	}
	if		(  ( value = g_hash_table_lookup(Values,name) )  ==  NULL  ) {
		value = "";
	}
	return	(value);
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
			  case	OPC_REF:
				dbgmsg("OPC_REF");
				name = LBS_FetchPointer(htc->code);
				value = HTGetValue(name,FALSE);
				LBS_EmitString(html,value);
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
				value = HTGetValue(vval.str,FALSE);
				LBS_EmitString(html,value);
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
					LBS_EmitString(html,str);
				}
				break;
			  case	OPC_REFSTR:
				dbgmsg("OPC_REFSTR");
				vval = Pop;
				LBS_EmitString(html,vval.str);
				break;
			  case	OPC_REFINT:
				dbgmsg("OPC_REFINT");
				vval = Pop;
				sprintf(buff,"%d",(vval.ptr)->ival);
				LBS_EmitString(html,buff);
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
