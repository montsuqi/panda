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
#include	"exec.h"
#include	"tags.h"
#include	"mon.h"
#include	"option.h"
#include	"debug.h"

static	Bool	fError;

#define	GetSymbol	(HTC_Token = HTCLex(FALSE))
#define	GetName		(HTC_Token = HTCLex(TRUE))
#define	GetChar		(HTC_Token = HTCGetChar())

#undef	Error

static	void
Error(char *msg, ...)
{
    va_list args;

    fError = TRUE;
    fprintf(stderr, "%s:%d:", HTC_FileName, HTC_cLine);
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    fputc('\n', stderr);
    va_end(args);
}

static	void
CopyTag(
	HTCInfo	*htc)
{
dbgmsg(">CopyTag");
	while	(  GetChar  !=  '>'  ) {
		LBS_EmitChar(htc->code,HTC_Token);
	}
	LBS_EmitChar(htc->code,HTC_Token);
dbgmsg("<CopyTag");
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
							Error("invalid type argument");
						}
					} else {
						Error("= missing");
					}
				} else {
					type->Para = (void *)1;
				}
			} else {
				Error("invalid type: %s", tag->name);
			}
		} else {
			Error("invalid tag: %s", tag->name);
		}
	}
LEAVE_FUNC;
}

static	void
ParTag(
	HTCInfo	*htc)
{
	Tag		*tag;

dbgmsg(">ParTag");
	if		(  GetSymbol  ==  T_SYMBOL  ) {
dbgprintf("tag = [%s]\n",HTC_ComSymbol);
		if		(  ( tag = g_hash_table_lookup(Tags,HTC_ComSymbol) )  !=  NULL  ) {
			ParMacroTag(htc,tag);
			tag->emit(htc,tag);
		} else {
			LBS_EmitString(htc->code,"<");
			LBS_EmitString(htc->code,HTC_ComSymbol);
			CopyTag(htc);
		}
	} else {
		LBS_EmitString(htc->code,"<");
		LBS_EmitChar(htc->code,HTC_Token);
		CopyTag(htc);
	}
dbgmsg("<ParTag");
}

static	void
ParHTC(
	HTCInfo	*htc)
{
	int		c;

dbgmsg(">ParHTC");
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
dbgmsg("<ParHTC");
}

extern	HTCInfo	*
HTCParser(
	char	*base)
{
	char	name[SIZE_BUFF+1];
	FILE	*fp;
	HTCInfo	*ret;

dbgmsg(">HTCParser");
	sprintf(name,"%s/%s.htc",ScreenDir,base);
	if		(  ( fp = fopen(name,"r") )  !=  NULL  ) {
		fError = FALSE;
		HTC_FileName = name;
		HTC_cLine = 1;
		HTC_File = fp;
		ret = New(HTCInfo);
		ret->code = NewLBS();
		ret->Trans = NewNameHash();
		ret->Radio = NewNameHash();
		ret->FileSelection = NewNameHash();
        ret->EnctypePos = 0;
        ret->FormNo = -1;
		LBS_EmitStart(ret->code);
		ParHTC(ret);
		fclose(HTC_File);
		if		(  fError  ) {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
dbgmsg("<HTCParser");
	return	(ret);
}

extern	void
HTCParserInit(void)
{
dbgmsg(">HTCParserInit");
	HTCLexInit();
	TagsInit();
	Codeset = "euc-jp";
dbgmsg("<HTCParserInit");
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

	g_hash_table_foreach(htc->Trans,(GHFunc)Clear,NULL);
	FreeLBS(htc->code);
	xfree(htc);
}
