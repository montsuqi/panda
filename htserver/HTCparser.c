/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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

#define	_HTC_PARSER
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<unistd.h>
#include	<iconv.h>
#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<fcntl.h>
#include	"types.h"
#include	"libmondai.h"
#include	"HTCparser.h"
#include	"HTClex.h"
#include	"cgi.h"
#include	"exec.h"
#include	"eruby.h"
#include	"tags.h"
#include	"debug.h"

static	Bool	fError;

#define	GetSymbol	(HTC_Token = HTCLex(FALSE))
#define	GetName		(HTC_Token = HTCLex(TRUE))

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
					ExpandAttributeString(htc,HTC_ComSymbol);
					break;
				  case	T_SYMBOL:
					LBS_EmitString(htc->code,HTC_ComSymbol);
					break;
				  default:
					LBS_EmitChar(htc->code,HTC_Token);
					break;
				}
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
			break;
		  default:
			break;
		}
		GetName;
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
		if		(  ( *p = GetChar() )  ==  EOF  )	return;
	}
	*p = 0;
	while	(  strcmp(cbuff,"-->")  !=  0  ) {
		LBS_EmitChar(htc->code,cbuff[0]);
		for	( i = 0 ; i < 2 ; i ++ ) {
			cbuff[i] = cbuff[i+1];
		}
		if		(  ( cbuff[2] = GetChar() )  ==  EOF  )	return;
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
            } else
			if (strnicmp(HTC_ComSymbol, "HTC:", 4) == 0) {
                tag = g_hash_table_lookup(Tags, HTC_ComSymbol + 4);
            }
        }
		if		(  tag  !=  NULL  ) {
			if		(  tag->emit  !=  NULL  ) {
				ParMacroTag(htc,tag);
				tag->emit(htc,tag);
			} else {
				while	(  GetSymbol  !=  '>'  );
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
	while	(  ( c = GetChar() )  >= 0  ) {
		switch	(c)	{
		  case	'<':
			ParTag(htc);
			break;
		  default:
			if		(  c  ==  0x01  ) {	/*	change for compile	*/
				c = 0x20;
			}
			LBS_Emit(htc->code,c);
			break;
		}
	}
	LBS_EmitEnd(htc->code);
LEAVE_FUNC;
}

extern	HTCInfo	*
NewHTCInfo(void)
{
	HTCInfo	*ret;

	ret = New(HTCInfo);
	ret->code = NULL;
	ret->Trans = NULL;
	ret->Radio = NULL;
	ret->FileSelection = NULL;
	ret->DefaultEvent = NULL;
	ret->EnctypePos = 0;
	ret->FormNo = -1;
	ret->fHTML = TRUE;

	return	(ret);
}

extern	HTCInfo	*
HTCParserCore(void)
{
	HTCInfo	*ret;

	ret = NewHTCInfo();
	ret->code = NewLBS();
	ret->Trans = NewNameHash();
	ret->Radio = NewNameHash();
	ret->FileSelection = NewNameHash();
	LBS_EmitStart(ret->code);
	fError = FALSE;
	ParHTC(ret);
	if		(  fError  ) {
		ret = NULL;
	}
	ret->fHTML = FALSE;
	return	(ret);
}

extern	HTCInfo	*
HTCParseFile(
	char	*fname)
{
	HTCInfo	*ret;
	char	*str
		,	*m;
	struct	stat	sb;
	int		fd;

ENTER_FUNC;
	if		(  ( fd = open(fname,O_RDONLY ) )  >=  0  ) {
		fstat(fd,&sb);
		m = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0);
		str = (char *)xmalloc(sb.st_size+1);
		memcpy(str,m,sb.st_size);
		munmap(m,sb.st_size);
		str[sb.st_size] = 0;
		close(fd);
		fError = FALSE;
		HTC_FileName = fname;
		HTC_cLine = 1;
		HTC_Memory = str;
		_HTC_Memory = str;
		ret = HTCParserCore();
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
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
		HTC_Memory = (byte *)buff;
		_HTC_Memory = NULL;
		ret = HTCParserCore();
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
			if		(  event  !=  NULL  ) {
				xfree(event);
			}
		}

ENTER_FUNC;
	if		(  htc->Trans  !=  NULL  ) {
		g_hash_table_foreach(htc->Trans,(GHFunc)Clear,NULL);
	}
	FreeLBS(htc->code);
	xfree(htc);
LEAVE_FUNC;
}

extern	HTCInfo	*
ParseScreen(
	char	*name)
{
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
		sprintf(fname,"%s/%s.rhml",p,name);
		if		(  ( ret = RHTMLParseFile(fname) )  !=  NULL  )	break;
		sprintf(fname,"%s/%s.rhtc",p,name);
		if		(  ( ret = RHTCParseFile(fname) )  !=  NULL  )	break;
		sprintf(fname,"%s/%s.htc",p,name);
		if		(  ( ret = HTCParseFile(fname) )  !=  NULL  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );

	if		(  ret  ==  NULL  ) {
        fprintf(stderr, "HTC file not found: %s\n", fname);
        dbgprintf("HTC file not found: %s\n", fname);
	}
LEAVE_FUNC;
	return	(ret);
}

