/*
PANDA -- a simple transaction monitor
Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define	_HTC_PARSER
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	"types.h"
#include	"libmondai.h"
#include	"HTCparser.h"
#include	"HTClex.h"
#include	"cgi.h"
#include	"htc.h"
#include	"exec.h"
#include	"tags.h"
#include	"debug.h"

static	Bool	fError;

#define	GetSymbol	(HTC_Token = HTCLex(FALSE))
#define	GetName		(HTC_Token = HTCLex(TRUE))
#define	GetChar		(HTC_Token = _HTCGetChar())

void
HTC_Error(char *msg, ...)
{
	char	buff[SIZE_LONGNAME+1];
    va_list args;

    fError = TRUE;
    fprintf(stderr, "%s:%d: ", HTC_FileName, HTC_cLine);
    sprintf(buff, "%s:%d: ", HTC_FileName, HTC_cLine);
	dbgmsg(buff);
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    vsprintf(buff, msg, args);
	dbgmsg(buff);
    fputc('\n', stderr);
    va_end(args);
}

static	void
CopyTag(
	HTCInfo	*htc)
{
	char	*para;

ENTER_FUNC;
	LBS_EmitString(htc->code,"<");
	LBS_EmitString(htc->code,HTC_ComSymbol);
	GetName;
	while	(  HTC_Token  !=  '>'  ) {
		LBS_EmitChar(htc->code,' ');
		switch	(HTC_Token)	{
		  case	T_SYMBOL:
			LBS_EmitString(htc->code,HTC_ComSymbol);
			if		(  GetName  ==  '='  ) {
				LBS_EmitChar(htc->code,HTC_Token);
				switch	(GetName) {
				  case	T_SCONST:
					para = HTC_ComSymbol;
					ExpandAttributeString(htc,para);
					break;
				  case	T_SYMBOL:
					LBS_EmitString(htc->code,HTC_ComSymbol);
					break;
				  default:
					LBS_EmitChar(htc->code,HTC_Token);
					break;
				}
				GetName;
			} else {
				switch( HTC_Token ){
				  case T_SYMBOL:
					LBS_EmitChar( htc->code, ' ' );
					LBS_EmitString(htc->code,HTC_ComSymbol);
					break;
				  default:
					break;
				}
			}
			break;
		  case	T_SCONST:
			para = HTC_ComSymbol;
			ExpandAttributeString(htc,para);
			GetName;
			break;
		  default:
			GetName;
			break;
		}
	}
	LBS_EmitChar(htc->code,HTC_Token);
LEAVE_FUNC;
}

static	void
CopyCommentTag(
	HTCInfo	*htc)
{
	char	*p;
	char	cbuff[3+1];
	int		i;

ENTER_FUNC;
	LBS_EmitString(htc->code,"<!-- ");
	for	( i = 0 , p = cbuff ; i < 3 ; i ++ , p ++ ) {
		if		(  ( *p = _HTCGetChar() )  ==  EOF  )	return;
	}
	*p = 0;
	while	(  strcmp(cbuff,"-->")  !=  0  ) {
		LBS_EmitChar(htc->code,cbuff[0]);
		for	( i = 0 ; i < 2 ; i ++ ) {
			cbuff[i] = cbuff[i+1];
		}
		if		(  ( cbuff[2] = _HTCGetChar() )  ==  EOF  )	return;
		cbuff[3] = 0;
	}
	LBS_EmitString(htc->code," -->");
LEAVE_FUNC;
}

static	void
ClearTagValue(
	Tag		*tag)
{
	void	Clear(
		char	*name,
		TagType	*type)
		{
			int		i;

			if		(  type->fPara  ) {
				for	( i = 0 ; i < type->nPara ; i ++ ) {
					xfree(type->Para[i]);
				}
				xfree(type->Para);
			}
			type->nPara = 0;
			type->Para = NULL;
		}
	g_hash_table_foreach(tag->args,(GHFunc)Clear,NULL);
}

static	void
AddPara(
	TagType	*type,
	char	*para)
{
	char	**Para;

	Para = (char **)xmalloc(sizeof(char *) * ( type->nPara + 1 ));
	if		(  type->nPara  >  0  ) {
		memcpy(Para,type->Para,(sizeof(char *) * type->nPara));
		xfree(type->Para);
	}
	type->Para = Para;
	type->Para[type->nPara] = StrDup(para);
	type->nPara ++;
}

static	void
ParMacroTag(
	HTCInfo	*htc,
	Tag		*tag)
{
	TagType	*type;

ENTER_FUNC;
	ClearTagValue(tag);
	while	(  GetSymbol  !=  '>'  ) {
		if		(  HTC_Token  ==  T_SYMBOL  ) {
			if		(  ( type = g_hash_table_lookup(tag->args,HTC_ComSymbol) )  !=  NULL  ) {
				if		(  type->fPara  ) {
					if		(  GetSymbol  ==  '='  ) {
						if		(	(  GetSymbol  ==  T_SYMBOL  )
								||	(  HTC_Token  ==  T_SCONST  ) ) {
							AddPara(type,HTC_ComSymbol);
						} else {
							HTC_Error("invalid type argument");
						}
					} else {
						HTC_Error("= missing");
					}
				} else {
					type->Para = (void *)1;
				}
			} else {
				HTC_Error("unknown attribute for <%s>: %s",
                      tag->name, HTC_ComSymbol);
                if (  GetSymbol  ==  '='  )
                    GetSymbol;
			}
		} else {
			HTC_Error("invalid tag: <%s>", tag->name);
		}
	}
LEAVE_FUNC;
}

static	void
ParTag(
	HTCInfo	*htc)
{
	Tag		*tag;

ENTER_FUNC;
	switch	(GetSymbol) {
	  case	T_SYMBOL:
		dbgprintf("tag = [%s]\n",HTC_ComSymbol);
		tag = g_hash_table_lookup(Tags, HTC_ComSymbol);
        if (tag == NULL) {
            if (strnicmp(HTC_ComSymbol, "/HTC:", 5) == 0) {
                char buf[SIZE_SYMBOL + 1];
                buf[0] = '/';
                strcpy(buf + 1, HTC_ComSymbol + 5);
                tag = g_hash_table_lookup(Tags, buf);
            }
            else if (strnicmp(HTC_ComSymbol, "HTC:", 4) == 0) {
                tag = g_hash_table_lookup(Tags, HTC_ComSymbol + 4);
            }
        }
		if		(  tag  !=  NULL  ) {
			ParMacroTag(htc,tag);
			if		(  tag->emit  !=  NULL  ) {
				tag->emit(htc,tag);
			}
		} else {
			CopyTag(htc);
		}
		break;
	  case	T_COMMENT:
		CopyCommentTag(htc);
		break;
	  default:
		CopyTag(htc);
		break;
	}
LEAVE_FUNC;
}

static	void
ParHTC(
	HTCInfo	*htc)
{
	int		c;

ENTER_FUNC;
	while	(  GetChar  >= 0  ) {
		switch	(HTC_Token) {
		  case	'<':
			ParTag(htc);
			break;
		  default:
			if		(  HTC_Token  ==  0x01  ) {	/*	change for compile	*/
				c = 0x20;
			} else {
				c = HTC_Token;
			}
			LBS_Emit(htc->code,c);
			break;
		}
	}
	LBS_EmitEnd(htc->code);
LEAVE_FUNC;
}

static	HTCInfo	*
HTCParserCore(
	int		(*getfunc)(void),
	void	(*ungetfunc)(int c))
{
	HTCInfo	*ret;

	_HTCGetChar = getfunc;
	_HTCUnGetChar = ungetfunc;
	ret = New(HTCInfo);
	ret->code = NewLBS();
	ret->Trans = NewNameHash();
	ret->Radio = NewNameHash();
	ret->FileSelection = NewNameHash();
	ret->DefaultEvent = NULL;
	ret->EnctypePos = 0;
	ret->FormNo = -1;
	LBS_EmitStart(ret->code);
	ParHTC(ret);
	return	(ret);
}

extern	HTCInfo	*
HTCParseFile(
	char	*fname)
{
	FILE	*fp;
	HTCInfo	*ret;

ENTER_FUNC;
	if		(  ( fp = fopen(fname,"r") )  !=  NULL  ) {
		fError = FALSE;
		HTC_FileName = fname;
		HTC_cLine = 1;
		HTC_File = fp;
		HTC_Memory = NULL;
		ret = HTCParserCore(GetCharFile,UnGetCharFile);
		fclose(HTC_File);
		if		(  fError  ) {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}

	if		(  ret  ==  NULL  ) {
        fprintf(stderr, "HTC file not found: %s\n", fname);
        dbgprintf("HTC file not found: %s\n", fname);
	}
LEAVE_FUNC;
	return	(ret);
}

extern	HTCInfo	*
HTCParseScreen(
	char	*name)
{
	FILE	*fp;
	HTCInfo	*ret;
	char	buff[SIZE_LONGNAME+1];
	char	fname[SIZE_LONGNAME+1];
	char	*p
		,	*q;

ENTER_FUNC;
	strcpy(buff,ScreenDir);
	p = buff;
	ret = NULL;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(fname,"%s/%s.htc",p,name);
		if		(  ( fp = fopen(fname,"r") )  !=  NULL  ) {
			fError = FALSE;
			HTC_FileName = fname;
			HTC_cLine = 1;
			HTC_File = fp;
			HTC_Memory = NULL;
			ret = HTCParserCore(GetCharFile,UnGetCharFile);
			fclose(HTC_File);
			if		(  fError  ) {
				ret = NULL;
			} else
				break;
		}
		p = q + 1;
	}	while	(  q  !=  NULL  );

	if		(  ret  ==  NULL  ) {
        fprintf(stderr, "HTC file not found: %s\n", fname);
        dbgprintf("HTC file not found: %s\n", fname);
	}
LEAVE_FUNC;
	return	(ret);
}

static	int
GetCharMemory(void)
{
	int		c;

	if		(  ( c = *HTC_Memory )  ==  0  ) {
		c = -1;
	} else {
		HTC_Memory ++;
	}
	return	(c);
}

static	void
UnGetCharMemory(
	int		c)
{
	HTC_Memory --;
	*HTC_Memory = c;
}

extern	HTCInfo	*
HTCParseMemory(
	char	*buff)
{
	HTCInfo	*ret;

ENTER_FUNC;
	if		(  buff  !=  NULL  ) {
		fError = FALSE;
		HTC_FileName = "*memory*";
		HTC_cLine = 1;
		HTC_File = NULL;
		HTC_Memory = buff;
		ret = HTCParserCore(GetCharMemory,UnGetCharMemory);
		if		(  fError  ) {
			ret = NULL;
		}
	} else {
        dbgprintf("HTC memory is null\n",NULL);
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
DestroyHTC(
	HTCInfo	*htc)
{
	void	Clear(
		char	*face,
		char	*event)
		{
			xfree(face);
			xfree(event);
		}

ENTER_FUNC;
	if		(  htc->Trans  !=  NULL  ) {
		g_hash_table_foreach(htc->Trans,(GHFunc)Clear,NULL);
	}
	FreeLBS(htc->code);
	xfree(htc);
LEAVE_FUNC;
}
