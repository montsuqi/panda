/*	PANDA -- a simple transaction monitor

Copyright (C) 1996-1999 Ogochan.
              2000-2002 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

/*
#define	TRACE
#define	DEBUG
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	"types.h"
#include	"i18n_struct.h"
#include	"i18n_const.h"
#include	"i18n_res.h"

#include	"misc.h"
#include	"i18n_code.h"
#include	"i18n_codeset.h"
#include	"debug.h"

STREAM	*STDOUT;
STREAM	*STDIN;

#define	IsDBCS_SET(s)			((0x8000&(s))==0x8000)

static	int		setDefault;

static	int
CodeFileGetByte(
	STREAM	*s)
{
	return	(fgetc(s->source.fp));
}

static	int
CodeFilePutByte(
	STREAM	*s,
	int		c)
{	int		r;

	r = fputc(c,s->source.fp);
#ifdef	DEBUG
	fflush(s->source.fp);
#endif
	return	(r);
}

static	int
CodeStringGetByte(
	STREAM	*s)
{	int		c;

	c = *s->source.str ++;
	if		(  c  ==  0  )	{
		c = -1;
	}
	return	(c);
}

static	int
CodeStringPutByte(
	STREAM	*s,
	int		c)
{
	*s->source.str ++ = c;
	return	(c);
}

static	int
CodeGetByte(
	STREAM	*s)
{	int		c;

	if		(  s->bBack  ==  -1  )	{
		if		(  ( c = s->GetByte(s) )  <  0  )	{
			c = GET_EMPTY;
		}
	} else {
		c = s->bBack;
		s->bBack = -1;
	}

	return	(c);
}

static	void
CodeUnGetByte(
	STREAM	*s,
	int		c)
{
	s->bBack = c;
}

static	int
CodePutByte(
	STREAM	*s,
	int		c)
{
	return	(s->PutByte(s,c));
}

static	void
CodeFileClose(
	STREAM *s)
{	
	if		(	(  s->source.fp  !=  NULL    )
			 &&	(  s->source.fp  !=  stdout  )
			 &&	(  s->source.fp  !=  stdin   ) ) {
		fclose(s->source.fp);
	}
	xfree(s);
}

static	void
CodeStringClose(STREAM *s)
{	
	xfree(s);
}

extern	void
CodeSetEncode(
	STREAM	*s,
	int		set)
{
	s->code.set = set;
	s->code.cBack = Char_EOF;
	s->bBack = -1;
	s->fEOF = FALSE;
	switch	(set)	{
	  case	ENCODE_SJIS:
		/*	not meaning	*/
		s->code.G0 = GetCodeSet("ASCII");
		s->code.G1 = SET_NULL;
		s->code.G2 = SET_NULL;
		s->code.G3 = SET_NULL;
		s->code.GL = &s->code.G0;
		s->code.GR = &s->code.G1;
		s->code.GLB = NULL;
		break;
	  case	ENCODE_ISO2022_:
		s->code.G0 = GetCodeSet("ASCII");
		s->code.G1 = SET_NULL;
		s->code.G2 = SET_NULL;
		s->code.G3 = SET_NULL;
		s->code.GL = &s->code.G0;
		s->code.GR = NULL;
		s->code.GLB = NULL;
		break;
	  case	ENCODE_NULL:
	  case	ENCODE_ISO2022:
	  case	ENCODE_EUC:
	  default:	/*	misc EUC	*/
		s->code.G0 = GetCodeSet("ASCII");
		s->code.G1 = GetCodeSet("JISX0208-1983");
		s->code.G2 = GetCodeSet("JISX0201-1976-kana");
		s->code.G3 = GetCodeSet("ASCII");
		s->code.GL = &s->code.G0;
		s->code.GR = &s->code.G1;
		s->code.GLB = NULL;
		break;
	}
}

static	Bool
IsKanjiSjis(
	int		c)
{	int		ret;

	ret = (	(	( c >= 0x80 )
			 && ( c <= 0x9F ) )
		   ||	( c>=0xE0 ) ) ? TRUE : FALSE;
	return	(ret);
}

static	void
SjisToJis(
	byte	*jis,
	byte	*org)
{
	if		(  org[0]  <  0xE0  )	{
		jis[0] = ( org[0] - 0x70 ) << 1;
	} else {
		jis[0] = ( org[0] - 0xB0 ) << 1;
	}
	if		(  org[1]  <  0x9F  )	{
		jis[0] --;
		if		(  org[1]  >  0x7F  )	{
			jis[1] = org[1] - 0x20;
		} else {
			jis[1] = org[1] - 0x1F;
		}
	} else {
		jis[1] = org[1] - 0x7E;
	}
}

static	void
ExecESC(
	STREAM	*s)
{	int		c;
	int		temp
	,		I;
	char	buf[SIZE_ESC_BUFF]
	,		*p;
	size_t	len;
	ESCAPE_TABLE	*e;

dbgmsg(">ExecESC");
	I = -1;
	temp = SET_NULL;
	switch	(c = CodeGetByte(s))	{
	  case	0x6E:	/*	LS2	*/
		s->code.GL = &s->code.G2;
		s->code.GLB = NULL;
		break;
	  case	0x6F:	/*	LS3	*/
		s->code.GL = &s->code.G3;
		s->code.GLB = NULL;
		break;
	  case	0x7E:	/*	LS1R	*/
		s->code.GR = &s->code.G1;
		break;
	  case	0x7D:	/*	LS2R	*/
		s->code.GR = &s->code.G2;
		break;
	  case	0x7C:	/*	LS3R	*/
		s->code.GR = &s->code.G3;
		break;
	  default:
		p = buf;
		*p ++ = '\x01B';
		*p ++ = c;
		len = 2;
		e = NULL;
		while	(	(  len                        <   SIZE_ESC_BUFF  )
				 &&	(  ( e = CheckESC(buf,len) )  ==  NULL           ) )	{
			*p ++= CodeGetByte(s);
			len ++;
		}
		if		(  e  !=  NULL  )	{
#ifdef	DEBUG
			printf("ESC(%d) = ",e->set->no);
			_DumpArray(buf,strlen(buf),PARA_HEX);
#endif
			temp = e->set->no;
			I = e->dig;
		} else {
			/*	error	*/
			temp = SET_NULL;
			I = 0;
		}
		break;
	}
	switch	(I)	{
	  case	0:
		s->code.G0 = ( temp != SET_NULL ) ? temp : s->code.G0;
		break;
	  case	1:
		s->code.G1 = ( temp != SET_NULL ) ? temp : s->code.G1;
		break;
	  case	2:
		s->code.G2 = ( temp != SET_NULL ) ? temp : s->code.G2;
		break;
	  case	3:
		s->code.G3 = ( temp != SET_NULL ) ? temp : s->code.G3;
		break;
	  default:		/*	shift	*/
		break;
	}
dbgmsg("<ExecESC");
}

static	Char
ExecC0_Code(
	STREAM	*s,
	int		c)
{	Char	ret;
	int		c2;

dbgmsg(">ExecC0_Code");
	ret = Char_NONE;
	switch	(c)	{
	  case	CHAR_NULL:
		break;
	  case	CHAR_BEL:
		ret = Char_BEL;
		break;
	  case	CHAR_ESC:
		ExecESC(s);
		break;
	  case	CHAR_APD:	/*	U.*X EOL is APD		*/
		ret = Char_LF;
		break;
	  case	CHAR_APR:	/*	MS EOL   is APR APD	*/
	  					/*	Mac EOL  is APR		*/
		if		(  ( c2 = CodeGetByte(s) )  !=  CHAR_APD  )	{
			CodeUnGetByte(s,c2);
		}
		ret = Char_LF;
		break;
	  case	CHAR_FF:
		ret = Char_FF;
		break;
	  case	CHAR_TAB:
		ret = Char_TAB;
		break;
	  case	CHAR_LS0:
		s->code.GL = &s->code.G0;
		s->code.GLB = NULL;
		break;
	  case	CHAR_LS1:
		s->code.GL = &s->code.G1;
		s->code.GLB = NULL;
		break;
	  case	CHAR_SS2:
		s->code.GLB = s->code.GL;
		s->code.GL = &s->code.G2;
		break;
	  case	CHAR_SS3:
		s->code.GLB = s->code.GL;
		s->code.GL = &s->code.G3;
		break;
	  default:
#ifdef	DEBUG
		printf("(?%X)\n",(int)c);
#endif
		break;
	}
dbgmsg("<ExecC0_Code");
	return	(ret);
}

#if	0
static	void
State(
	STREAM	*s)
{
	if		(  s->code.GL  ==  &s->code.G0  )	{
		printf("[G0->GL]");
	} else
	if		(  s->code.GL  ==  &s->code.G1  )	{
		printf("[G1->GL]");
	} else
	if		(  s->code.GL  ==  &s->code.G2  )	{
		printf("[G2->GL]");
	} else
	if		(  s->code.GL  ==  &s->code.G3  )	{
		printf("[G3->GL]");
	}
	if		(  s->code.GR  ==  &s->code.G0  )	{
		printf("[G0->GR]");
	} else
	if		(  s->code.GR  ==  &s->code.G1  )	{
		printf("[G1->GR]");
	} else
	if		(  s->code.GR  ==  &s->code.G2  )	{
		printf("[G2->GR]");
	} else
	if		(  s->code.GR  ==  &s->code.G3  )	{
		printf("[G3->GR]");
	}
}
#endif

static	Char
ExecC1_Code(
	STREAM	*s,
	int		c)
{	Char	ret;
dbgmsg(">ExecC1_Code");
	ret = ExecC0_Code(s,c&0x7F);
dbgmsg("<ExecC1_Code");
	return	(ret);
}

static	Char
ExecG_Code(
	STREAM	*s,
	Bool	fGL,
	int		c)
{	byte	cbuf[2];
	int		setG;
	Char	ret;
	word	code;

dbgmsg(">ExecG_Code");
	if		(  fGL  )	{
		setG = *s->code.GL;
	} else {
		setG = *s->code.GR;
	}
	if		(  IsDBCS_SET(setG)  )	{
		cbuf[0] = (byte)c;
		if		(  cbuf[0]  ==  0x20  )	{
			cbuf[0] = 0x21;
			cbuf[1] = 0x21;
		} else {
			cbuf[1] = ( (byte)CodeGetByte(s) & (byte)0x7F );
		}
		code = (word)(( cbuf[0] << 8 ) | cbuf[1]);
		ret = MAKE_Char(setG,code);
	}
	else {
		ret = MAKE_Char(SET_ASCII,c);
	}
	if		(	(  fGL  )
			&&	(  s->code.GLB  !=  NULL  ) )	{
		s->code.GL = s->code.GLB;
	}
dbgmsg("<ExecG_Code");
	return	(ret);
}

extern	Char
CodeGetChar(
	STREAM	*s)
{	Char	rc = Char_EOF;
	int		c;
	byte	sjis[2]
	,		jis[2];

	dbgmsg(">CodeGetChar");
	if		(  s->code.cBack  !=  Char_EOF  )	{
		rc = s->code.cBack;
		s->code.cBack = Char_EOF;
	} else
	if		(  s->fEOF  )	{
		rc = Char_EOF;
	} else {
		switch	(s->code.set)	{
		  case	ENCODE_NULL:
			c = CodeGetByte(s);
			if (c == '\033') {
				CodeSetEncode(s, ENCODE_ISO2022);
			}
			else if (c == GET_EMPTY) {
				goto empty;
			}
			else if (c <= 0x80) {
				/* cannot detect by this char */
			}
			else if (c < 0xa1) {
				CodeSetEncode(s, ENCODE_SJIS);
				CodeUnGetByte(s,c);
				goto sjis_enter;
			}
			else if (c > 0xea) {
				CodeSetEncode(s, ENCODE_EUC);
			}
			else {
				/* SJIS or EUC character -- ambiguous */
			    /* check 2nd byte */
			    int c2 = CodeGetByte(s);

			    if (c2 < 0xa1) {
					CodeSetEncode(s, ENCODE_SJIS);
					sjis[0] = (byte)c;
					sjis[1] = (byte)c2;
					goto sjis_fetch;
			    }
				rc = Char_NONE;
				CodeUnGetByte(s,c2);
				goto euc_fetch;
			}
			CodeUnGetByte(s,c);
			/* fall thru */
		  case	ENCODE_ISO2022_:
		  case	ENCODE_ISO2022:
		  case	ENCODE_EUC:
			rc = Char_NONE;
			while ((c = CodeGetByte(s)) != GET_EMPTY) {
			  euc_fetch:
			    if (c <=  0x1F) {
					rc = ExecC0_Code(s,c);
			    }
			    else if (c <= 0x7F)	{
					rc = ExecG_Code(s,TRUE,c & 0x7F);
			    }
			    else if (c <=  0x9F) {
					rc = ExecC1_Code(s,c);
			    }
			    else {
					rc = ExecG_Code(s,FALSE,c & 0x7F);
			    }
			    if (rc != Char_NONE) break;
			}
			if (c  ==  GET_EMPTY) {
		      empty:
				s->fEOF = TRUE;
				rc = Char_EOF;
			}
			break;
		  case	ENCODE_SJIS:
		  sjis_enter:
			while ((c = CodeGetByte(s)) != GET_EMPTY) {
				if (c <= 0x1F) {
					rc = ExecC0_Code(s,c);
				}
				else if (IsKanjiSjis(c)) {
					sjis[0] = (byte)c;
					sjis[1] = (byte)CodeGetByte(s);
					SjisToJis(jis,sjis);
			      sjis_fetch:
					rc = MAKE_Char(SET_JIS83,( ( jis[0] << 8 ) | jis[1] ));
				}
				else {
					rc = MAKE_Char(SET_ASCII,c);
				}
				if (rc != Char_NONE) break;
			} 
			if (c  ==  GET_EMPTY) {
				goto empty;
			}
			break;
		  default:
			rc = Char_EOF;
			break;
		}
	}
dbgmsg("<CodeGetChar");
	return	(rc);
}

extern	size_t
CharToSJIS(
	Char	c,
	char	*sjis)
{	
	word	code;

	code = CODE_Char(c);
	if		(  CODE_UPPER(code)  <=  0x5E  )	{
		sjis[0] = ( CODE_UPPER(code) >> 1 ) + (char)0x70;
	} else {
		sjis[0] = ( CODE_UPPER(code) >> 1 ) + (char)0xB0;
	}
	if		(  ( CODE_UPPER(code) % 2 )  !=  0  )	{
		sjis[0] ++;
		if		(  CODE_LOWER(code)  <=  0x5F  )	{
			sjis[1] = CODE_LOWER(code) + (char)0x1F;
		} else {
			sjis[1] = CODE_LOWER(code) + (char)0x20;
		}
	} else {
		sjis[1] = CODE_LOWER(code) + (char)0x7E;
	}
	return	(2);
}

extern	size_t
CharToEUC(
	Char	c,
	char	*e)
{
    word	code;

	code = CODE_Char(c);
	*e++ = CODE_UPPER(code) | 0x80;
	*e   = CODE_LOWER(code) | 0x80;
	return	(2);
}

static	void
CharTo2(
	Char	c,
	char	*e)
{
    word	code;

	code = CODE_Char(c);
	*e++ = CODE_UPPER(code);
	*e   = CODE_LOWER(code);
}

static	void
CodePutBytes(
	STREAM	*s,
	char	*buf,
	size_t	len)
{
dbgmsg(">CodePutBytes");
	for	( ; len > 0 ; len -- ,buf ++)	{
		CodePutByte(s,*buf);
	}
dbgmsg("<CodePutBytes");
}

/*#define NULL_CHAR_CHECK(c) ((c) != Char_NULL*/
#define NULL_CHAR_CHECK(c) 1

extern	void
CodePutChar(
	STREAM	*s,
	Char	c)
{	size_t	len;
	char	buf[2];
	CODE_SET	*cs;

dbgmsg(">CodePutChar");
	switch	(s->code.set)	{
	  case	ENCODE_EUC:
		if		(  IsDBCS(c)  )	{
			len = CharToEUC(c,buf);
			CodePutBytes(s,buf,len);
		} else {
		  if	(  NULL_CHAR_CHECK(c) ) {
				CodePutByte(s,CODE_Char(c));
			}
		}
		break;
	  case	ENCODE_SJIS:
		if		(  IsDBCS(c)  )	{
			len = CharToSJIS(c,buf);
			CodePutBytes(s,buf,len);
		} else {
			if		(  NULL_CHAR_CHECK(c)  ) {
				CodePutByte(s,CODE_Char(c));
			}
		}
		break;
	  case	ENCODE_ISO2022:
	  case	ENCODE_ISO2022_:
		if		(  IsDBCS_SET(s->code.G0)  )	{
			if		(  IsDBCS(c)  )	{
				CharTo2(c,buf);
				CodePutBytes(s,buf,2);
			} else {
				s->code.G0 = SET_Char(c);
				cs = GetEscape(s->code.G0);
				CodePutBytes(s,cs->G[0]->esc,strlen(cs->G[0]->esc));
				if		(  NULL_CHAR_CHECK(c) ) {
					CodePutByte(s,CODE_Char(c));
				}
			}
		} else {
			if		(  IsDBCS(c)  )	{
				s->code.G0 = SET_Char(c);
				cs = GetEscape(s->code.G0);
				CodePutBytes(s,cs->G[0]->esc,strlen(cs->G[0]->esc));
				CharTo2(c,buf);
				CodePutBytes(s,buf,2);
			} else {
				if		(  NULL_CHAR_CHECK(c) ) {
					CodePutByte(s,CODE_Char(c));
				}
			}
		}
		break;
	  default:
		break;
	}
dbgmsg("<CodePutChar");
}

extern	void
CodePrintCString(
	STREAM	*s,
	Char	*str)
{
	while	(  *str  !=  Char_NULL  ) {
		CodePutChar(s,*str ++);
	}
	CodePutChar(s,0);
}

extern	void
CodePrintString(
	STREAM	*s,
	char	*str)
{
	while	(  *str  !=  0  ) {
		CodePutChar(s,ToAscii(*str));
		str ++;
	}
	CodePutChar(s,0);
}

extern	int
CodeResolvSet(
	char	*name)
{	int		set;

	if		(  !strlicmp(name,"iso2022-")  ) {
		set = ENCODE_ISO2022_;
	} else
	if		(  !strlicmp(name,"iso-2022-")  ) {
		set = ENCODE_ISO2022_;
	} else
	if		(  !strlicmp(name,"iso2022")  ) {
		set = ENCODE_ISO2022;
	} else
	if		(  !strlicmp(name,"iso-2022")  ) {
		set = ENCODE_ISO2022;
	} else
	if		(  !strlicmp(name,"euc")  ) {
		set = ENCODE_EUC;
	} else
	if		(  !strlicmp(name,"sjis")  ) {
		set = ENCODE_SJIS;
	} else {
		set = ENCODE_NULL;
	}
	return	(set);
}

extern	STREAM	*
CodeMakeFileStream(
	FILE	*fp)
{	STREAM	*s;

dbgmsg(">CodeMakeFileStream");
	s = (STREAM *)xmalloc(sizeof(STREAM));
	s->source.fp = fp;
	s->GetByte = CodeFileGetByte;
	s->PutByte = CodeFilePutByte;
	s->Close = CodeFileClose;
	CodeSetEncode(s,setDefault);
dbgmsg("<CodeMakeFileStream");
	return	(s);
}

extern	STREAM	*
CodeOpenFile(
	char	*fname,
	char	*mode)
{	FILE	*fp;
	STREAM	*s;

dbgmsg(">CodeOpenFile");
	if		(  ( fp = fopen(fname,mode) )  !=  NULL  )	{
		s = CodeMakeFileStream(fp);
	} else {
		s = NULL;
	}
dbgmsg("<CodeOpenFile");
	return	(s);
}

extern	STREAM	*
CodeOpenString(
	char	*str)
{	STREAM	*s;

dbgmsg(">CodeOpenString");
	s = (STREAM *)xmalloc(sizeof(STREAM));
	s->source.str = str;
	s->GetByte = CodeStringGetByte;
	s->PutByte = CodeStringPutByte;
	s->Close = CodeStringClose;
	CodeSetEncode(s,setDefault);
dbgmsg("<CodeOpenString");
	return	(s);
}

extern	void
CodeClose(STREAM *s)
{	
dbgmsg(">CodeClose");
	if		(  s  !=  NULL  )	{
		s->Close(s);
	}
dbgmsg("<CodeClose");
}

extern	void
CodeUnGetChar(
	STREAM	*s,
	Char	c)
{
	if (s->code.cBack == Char_EOF) {
		s->code.cBack = c;
	}
}

extern	void
CodeInit(
	int		set)
{	char	*env;
dbgmsg(">CodeInit");
	InitCodeSet();
	if		(  ( setDefault = set )  ==  ENCODE_NULL  )	{
		env = getenv("LANG");
		if		(  !strcmp(env,"ja_JP.ujis")  )	{
			setDefault = ENCODE_EUC;
		} else
		if		(  !strcmp(env,"ja_JP.sjis")  )	{
			setDefault = ENCODE_SJIS;
		}
	}
	STDOUT = CodeMakeFileStream(stdout);
	STDIN  = CodeMakeFileStream(stdin);
dbgmsg("<CodeInit");
}


#if	0
extern	void	EucToJis(char *,char *);
extern	Bool	IsKanjiEuc(int);
extern	void	EUC_ToChar(char *,Char *);
extern	void	ToCharString(Char *,char *);

extern	void
ToCharString(
	Char	*cstr,
	char	*str)
{
	for (;;) {
		switch (*str) {
		  case 0:
			*cstr = Char_NULL;
			return;
		  default:
			*cstr = MAKE_Char(SET_ASCII,*str);
		}
		cstr++;
		str++;
	}
}

extern	Bool
IsKanjiEuc(
	int		c)
{	int		ret;

	ret = ( ( c & 0x80) == 0x80 ) ? TRUE : FALSE;
	return	(ret);
}

extern	void
EucToJis(
	char	*jis,
	char	*org)
{
	jis[0] = 0x7F & org[0];
	jis[1] = 0x7F & org[1];
}

extern	void
EUC_ToChar(
	char	*e,
	Char	*s)
{
    word set = GetCodeSet("JISX0208-1983");

    for (; *e; e++) {
		if (*e < ' ') *s++ = MAKE_Char(SET_ASCII,*e);
		else if (*e & 0x80) {
			byte	cbuf[2];
			word	code;
			cbuf[0] = (byte)*e++ & 0x7f;
			cbuf[1] = (byte)*e & 0x7f;
			code = (word)(( cbuf[0] << 8 ) | cbuf[1]);
			*s++ = MAKE_Char(set,code);
		} else {
			*s++ = MAKE_Char(SET_ASCII,*e);
		}
    }
    *s++ = Char_NULL;
}

#endif
