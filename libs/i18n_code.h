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

#ifndef	_INC_CODE_H
#define	_INC_CODE_H
#include	"i18n_struct.h"
#include	"i18n_codeset.h"

#define	ToChar(c)			MAKE_Char(SET_NULL,(c))
#define	ToAscii(c)			MAKE_Char(SET_ASCII,(c))

#define	MAKE_Char(s,c)		((Char)(((s)<<16)|(c)))
#define	SET_Char(c)			((word)(((c)&0xFFFF0000)>>16))
#define	CODE_Char(c)		((word)(((c)&0xFFFF)))

#define	CODE_UPPER(c)		((byte)((c)>>8))
#define	CODE_LOWER(c)		((byte)((c)&0xFF))

#define	IsDBCS(c)			(((0x8000&(SET_Char(c)))==0x8000)&&(SET_Char(c)!=SET_NULL))
#define	IsCTRL(c)			((SET_Char(c)==SET_CTRL)||(CODE_Char(c)<0x0020))

#define	SET_DBCS		0x8000
#define	SET_SBCS		0x0000

#define	SET_NULL		0xFFFF
#define	SET_CTRL		0x0000

#define	ENCODE_NULL			0x0000
#define	ENCODE_ISO2022		0x0100
#define	ENCODE_ISO2022_		0x0101
#define	ENCODE_SJIS			0x0200
#define	ENCODE_EUC			0x0400

#define	GET_EMPTY		-1

#define	CHAR_NULL		0x00
#define	CHAR_BEL		0x07
#define	CHAR_BS			0x08
#define	CHAR_TAB		0x09
#define	CHAR_APD		0x0A
#define	CHAR_CS			0x0C
#define	CHAR_APR		0x0D
#define	CHAR_LS1		0x0E
#define	CHAR_LS0		0x0F
#define	CHAR_ESC		0x1B
#define	CHAR_SS2		0x19
#define	CHAR_SS3		0x1D

#define	CHAR_LF			0x0A
#define	CHAR_FF			0x0C
#define	CHAR_CR			0x0D
#define	CHAR_SPACE		0x20

#define	Char_NULL			MAKE_Char(SET_CTRL,0x0000)
#define	Char_EOF			MAKE_Char(SET_CTRL,0xFFFF)
#define	Char_NONE			MAKE_Char(SET_CTRL,0xFFFE)
#define	Char_SPACE			MAKE_Char(SET_CTRL,CHAR_SPACE)
#define	Char_BEL			MAKE_Char(SET_CTRL,CHAR_BEL)
#define	Char_BS				MAKE_Char(SET_CTRL,CHAR_BS)
#define	Char_CR				MAKE_Char(SET_CTRL,CHAR_CR)
#define	Char_LF				MAKE_Char(SET_CTRL,CHAR_LF)
#define	Char_FF				MAKE_Char(SET_CTRL,CHAR_FF)
#define	Char_TAB			MAKE_Char(SET_CTRL,CHAR_TAB)

extern	void	CodeInit(int);

extern	int		CodeResolvSet(char *);

extern	STREAM	*CodeMakeFileStream(FILE *);
extern	STREAM	*CodeOpenFile(char *,char *);
extern	STREAM	*CodeOpenString(char *);
extern	void	CodeClose(STREAM *);
extern	void	CodeSetEncode(STREAM *,int);

extern	Char	CodeGetChar(STREAM *);
extern	void	CodeUnGetChar(STREAM *,Char);
extern	void	CodePutChar(STREAM *,Char);
extern	void	CodePrintCString(STREAM *,Char *);
extern	void	CodePrintString(STREAM *,char *);

extern	size_t	CharToSJIS(Char,char *);
extern	size_t	CharToEUC(Char,char *);

/*	ctype.c	*/
extern	int		ToDigit(Char);
extern	Bool	IsSpace(Char);
extern	Bool	IsDigit(Char);
extern	Bool	IsAlnum(Char);

/*	strings.c	*/
extern	size_t	CStrLen(Char *);
extern	Char	*CStrCpy(Char *,Char *);
extern	Char	*CStrDup(Char *);
extern	long	CStrCmp(Char *,Char *);
extern	long	CStrnCmp(Char *,Char *,size_t);
extern	void	StrToCStr(Char *,char *,int);

extern	word	SET_ASCII
		,		SET_JIS83;
extern	STREAM	*STDOUT
		,		*STDIN;

#endif
