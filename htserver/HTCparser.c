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
#include	"htc.h"
#include	"exec.h"
#include	"tags.h"
#include	"debug.h"

#define		ERUBY_PATH		"/usr/bin/eruby"

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

static	HTCInfo	*
HTCParserCore(void)
{
	HTCInfo	*ret;

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
	if		(  fError  ) {
		ret = NULL;
	}
	return	(ret);
}

extern	HTCInfo	*
HTCParseHTCFile(
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

static	char	*
CheckCoding(
	char	**sstr)
{
	char	*str = *sstr;
	int		blace;
	Bool	fMeta
		,	fHTC
		,	fXML
		,	quote;
	char	*p;
	static	char	coding[SIZE_NAME+1];

ENTER_FUNC;
	blace = 0;
	quote = FALSE;
	fMeta = FALSE;
	fHTC = FALSE;
	fXML = FALSE;
	strcpy(coding,"iso-2022-jp");
	while	(  *str  !=  0  ) {
		switch	(*str) {
		  case	'<':
			if		(  !strlicmp(str,"<HTC")  ) {
				fHTC = TRUE;
				str += strlen("<HTC");
			} else
			if		(  !strlicmp(str,"<?xml")  ) {
				fXML = TRUE;
				str += strlen("<?xml");
			}
			blace ++;
			break;
		  case	'>':
			blace --;
			if		(  blace  ==  0  ) {
				fMeta = FALSE;
			}
			break;
		  case	'm':
		  case	'M':
			if		(	(  blace  >   0  )
					&&	(  !strlicmp(str,"meta")  ) ) {
				fMeta = TRUE;
				str += strlen("meta");
			}
			break;
		  default:
			break;
		}
		if		(  fMeta  ) {
			if		(  !strlicmp(str,"charset")  ) {
				str += strlen("charset");
				p = coding;
				while	(  isspace(*str)  )	str ++;
				if		(  *str  ==  '='  )	str ++;
				while	(  isspace(*str)  )	str ++;
				while	(	(  !isspace(*str)  )
						&&	(  *str  !=  '"'   ) ) {
					*p ++ = *str ++;
				}
				*p = 0;
				str --;
				goto	quit;
			}
		} else
		if		(  fHTC  ) {
			while	(  isspace(*str)  )	str ++;
			while	(  *str  !=  '>'  ) {
				if		(  !strlicmp(str,"coding")  ) {
					str += strlen("coding");
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '='  )	str ++;
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '"'  )	str ++;
					p = coding;
					while	(	(  !isspace(*str)  )
								&&	(  *str  !=  '"'   ) ) {
						*p ++ = *str ++;
					}
					*p = 0;
				}
				str ++;
			}
			str ++;
			goto	quit;
		} else
		if		(  fXML  ) {
			while	(  *str  !=  '>'  ) {
				if		(  !strlicmp(str,"encoding")  ) {
					str += strlen("encoding");
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '='  )	str ++;
					while	(  isspace(*str)  )	str ++;
					if		(  *str  ==  '"'  )	str ++;
					p = coding;
					while	(	(  !isspace(*str)  )
							&&	(  *str  !=  '"'   ) ) {
						*p ++ = *str ++;
					}
					*p = 0;
				}
				str ++;
			}
			str ++;
			goto	quit;
		}
		str ++;
	}
  quit:
	*sstr = str;
LEAVE_FUNC;
	return	(coding);
}

static	char	*
ConvertEncoding(
	char	*tcoding,
	char	*coding,
	char	*istr)
{
	iconv_t	cd;
	size_t	sib
		,	sob;
	char	*ostr;
	char	*cbuff;

	cd = iconv_open(tcoding,coding);
	sib = strlen(istr);
	sob = sib * 2;
	cbuff = (char *)xmalloc(sob);
	ostr = cbuff;
	iconv(cd,&istr,&sib,&ostr,&sob);
	*ostr = 0;
	iconv_close(cd);
	return	(cbuff);
}

static	void
_OutValue(
	LargeByteString	*lbs,
	ValueStruct	*value,
	char		*name,
	char		*longname)
{
	int		i;

	if		(  value  ==  NULL  )	{
	} else {
		switch	(ValueType(value)) {
		  case	GL_TYPE_INT:
		  case	GL_TYPE_FLOAT:
		  case	GL_TYPE_BOOL:
		  case	GL_TYPE_NUMBER:
			LBS_EmitString(lbs,"hostvalue['");
			LBS_EmitString(lbs,longname);
			LBS_EmitString(lbs,"'] = ");
			if		(  IS_VALUE_NIL(value)  ) {
				LBS_EmitString(lbs,"nil");
			} else {
				LBS_EmitString(lbs,ValueToString(value,NULL));
			}
			LBS_EmitString(lbs,"\n");
			break;
		  case	GL_TYPE_CHAR:
		  case	GL_TYPE_VARCHAR:
		  case	GL_TYPE_DBCODE:
		  case	GL_TYPE_TEXT:
		  case	GL_TYPE_SYMBOL:
		  case	GL_TYPE_BINARY:
			LBS_EmitString(lbs,"hostvalue['");
			LBS_EmitString(lbs,longname);
			LBS_EmitString(lbs,"'] = ");
			if		(  IS_VALUE_NIL(value)  ) {
				LBS_EmitString(lbs,"nil");
			} else {
				LBS_EmitString(lbs,"'");
				LBS_EmitString(lbs,ValueToString(value,NULL));
				LBS_EmitString(lbs,"'");
			}
			LBS_EmitString(lbs,"\n");
			break;
		  case	GL_TYPE_ARRAY:
			for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
				sprintf(name,"[%d]",i);
				_OutValue(lbs,ValueArrayItem(value,i),(name+strlen(name)),longname);
			}
			break;
		  case	GL_TYPE_VALUES:
			for	( i = 0 ; i < ValueValuesSize(value) ; i ++ ) {
			}
			break;
		  case	GL_TYPE_RECORD:
			for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
				sprintf(name,".%s",ValueRecordName(value,i));
				_OutValue(lbs,ValueRecordItem(value,i),(name+strlen(name)),longname);
			}
			break;
		  case	GL_TYPE_OBJECT:
		  case	GL_TYPE_ALIAS:
			break;
		  default:
			break;
		}
	}
}
	

static	void
_OutValues(
	char	*name,
	ValueStruct	*value,
	LargeByteString	*lbs)
{
	char	longname[SIZE_LONGNAME+1];

	strcpy(longname,name);
	_OutValue(lbs,value,(longname + strlen(longname)),longname);
}

static	void
RecordData(
	LargeByteString	*lbs)
{
	LBS_EmitString(lbs,"<%\n");
	LBS_EmitString(lbs,"hostvalue = Hash.new\n");
	g_hash_table_foreach(Records,(GHFunc)_OutValues,lbs);
	LBS_EmitString(lbs,"%>\n");
}

extern	HTCInfo	*
HTCParseRHTCFile(
	char	*fname)
{
	HTCInfo	*ret;
	char	*str
		,	*m;
	struct	stat	sb;
	int		fd
		,	pid
		,	i;
	int		pSource[2]
		,	pResult[2];
	LargeByteString	*lbs;
	char	buff[SIZE_BUFF];
	size_t	size;

	Bool	fChange;
	char	*coding
		,	*istr
		,	*ostr;

ENTER_FUNC;
	if		(  stat(fname,&sb)  ==  0  ) {
		if		(  pipe(pSource)  <  0  ) {
			perror("pipe");
			ret = NULL;
		}
		if		(  pipe(pResult)  <  0  ) {
			perror("pipe");
			ret = NULL;
		}
		if		(  ( pid = fork() )  ==  0  ) {
			dup2(pSource[0],STDIN_FILENO);
			dup2(pResult[1],STDOUT_FILENO);

			close(pSource[0]);
			close(pSource[1]);
			close(pResult[0]);
			close(pResult[1]);
#if	1
			execl(ERUBY_PATH,"eruby","-Mf","-Ku",NULL);
#else
			execl("/usr/bin/tee","eruby","aa",NULL);
#endif
		}
		close(pSource[0]);
		close(pResult[1]);
		if		(  ( fd = open(fname,O_RDONLY ) )  >=  0  ) {
			m = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0);
			str = (char *)xmalloc(sb.st_size+1);
			memcpy(str,m,sb.st_size);
			munmap(m,sb.st_size);
			str[sb.st_size] = 0;
			close(fd);
			istr = str;
			coding = CheckCoding(&istr);
			if		(	(  strcmp(coding,"utf-8")  !=  0  )
					&&	(  strcmp(coding,"utf8")   !=  0  ) ) {
				ostr = ConvertEncoding("utf-8",coding,istr);
				xfree(str);
				str = ostr;
				fChange = TRUE;
			} else {
				fChange = FALSE;
			}
			lbs = NewLBS();
			RecordData(lbs);
			write(pSource[1],LBS_Body(lbs),LBS_Size(lbs));
#if	0
			printf("code:-------\n");
			printf("%s",(char *)LBS_Body(lbs));
			printf("%s",str);
			printf("------------\n");
#endif
			FreeLBS(lbs);
			write(pSource[1],str,strlen(str)+1);
			close(pSource[1]);
			(void)wait(&pid);
			xfree(str);
			fd = pResult[0];
			lbs = NewLBS();
			if		(  fChange  ) {
				sprintf(buff,"<htc coding=\"%s\">",coding);
				LBS_EmitString(lbs,buff);
			}
			while	(  ( size = read(fd,buff,SIZE_BUFF) )  >  0  ) {
				for	( i = 0 ; i < size ; i ++ ) {
					LBS_Emit(lbs,buff[i]);
				}
			}
			close(fd);
			str = LBS_ToString(lbs);
			FreeLBS(lbs);
			if		(  fChange  ) {
				ostr = ConvertEncoding(coding,"utf-8",str);
				xfree(str);
				str = ostr;
			}				
			fError = FALSE;
			HTC_FileName = fname;
			HTC_cLine = 1;
			HTC_Memory = str;
			_HTC_Memory = str;
			ret = HTCParserCore();
		} else {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	HTCInfo	*
HTCParseScreen(
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
		sprintf(fname,"%s/%s.rhtc",p,name);
		if		(  ( ret = HTCParseRHTCFile(fname) )  !=  NULL  )	break;
		sprintf(fname,"%s/%s.htc",p,name);
		if		(  ( ret = HTCParseHTCFile(fname) )  !=  NULL  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );

	if		(  ret  ==  NULL  ) {
        fprintf(stderr, "HTC file not found: %s\n", fname);
        dbgprintf("HTC file not found: %s\n", fname);
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
