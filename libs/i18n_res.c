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
#define	MAIN
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"types.h"
#include	"i18n_const.h"
#include	"i18n_struct.h"
#include	"misc.h"
#include	"i18n_code.h"
#include	"i18n_codeset.h"
#include	"i18n_res.h"
#include	"debug.h"

#define	ResGetChar(p)		getc(p)
#define	ResUnGetChar(c,p)	ungetc(c,p)

#define	T_SYMBOL		256
#define	T_CODESET		257
#define	T_END			258
#define	T_G0			259
#define	T_G1			260
#define	T_G2			261
#define	T_G3			262
#define	T_GL			263
#define	T_GR			264
#define	T_DBCS			265
#define	T_SBCS			266
#define	T_EOF			267
#define	T_TYPE			268
#define	T_SET			269

#define	TOKENNUM		13
static	struct	{
	char	*str;
	int		token;
}	tokentable[] = {
	{	"CodeSet",	T_CODESET	},
	{	"DBCS",		T_DBCS	},
	{	"end",		T_END	},
	{	"G0",		T_G0	},
	{	"G1",		T_G1	},
	{	"G2",		T_G2	},
	{	"G3",		T_G3	},
	{	"GL",		T_GL	},
	{	"GR",		T_GR	},
	{	"SBCS",		T_SBCS	},
	{	"set",		T_SET	},
	{	"type",		T_TYPE	},
	{	"z",		0		}
};

static	int
ResCheckReserved(
	char	*str)
{	int		ret;
	int		eq
	,		cent
	,		cent1
	,		upper
	,		lower;

dbgmsg(">ResCheckReserved");
	ret = T_SYMBOL;
	lower = 0;
	upper = TOKENNUM - 1;
	cent = 0;
	do	{
		cent1 = cent;
		cent = ( upper + lower ) / 2;
		eq = stricmp(str,tokentable[cent].str);
		if		(  eq  <  0  )	{
			upper = cent;
		} else
		if		(  eq  >  0  )	{
			lower = cent;
		} else {
			ret = tokentable[cent].token;
			lower = upper;
		}
	}	while	(  cent1  !=  cent  );
dbgmsg("<ResCheckReserved");
	return	(ret);
}

static	int
ResGetWord(
	FILE	*fp)
{	char	buf[SIZE_BUFF]
	,		*p;
	int		c
	,		ret;

dbgmsg(">ResGetWord");
	while	(  isspace( c = ResGetChar(fp) )  );
	switch	(c)	{
	  case	'|':
	  case	':':
	  case	',':
	  case	';':
	  case	'#':
		ret = c;
		break;
	  case	EOF:
		ret = T_EOF;
		break;
	  default:
		p = buf;
		while	(	(  !isspace(c)  )
				 &&	(  c  !=  '|'   )
				 &&	(  c  !=  ':'   )
				 &&	(  c  !=  ','   )
				 &&	(  c  !=  ';'   ) )	{
			*p ++ = c;
			c = ResGetChar(fp);
		}
		ResUnGetChar(c,fp);
		*p = 0;
		ret = ResCheckReserved(buf);
		break;
	}
#ifdef	TRACE
	if		(  ret  <  256  )	{
		printf("word = [%c]\n",ret);
	} else {
		printf("word = [%s] %d\n",buf,ret);
	}
#endif
dbgmsg("<ResGetWord");
	return	(ret);
}

#if	0
static	void
DumpStr(
	char	*s)
{
	while	(  *s !=  0  )	{
		printf("[%X]",(int)*s ++ );
	}
	printf("\n");
}
#endif
static	char	*
ResGetString(
	FILE	*fp,
	char	*buf)
{	char	*p;
	int		c
	,		cc;
	int		i;


dbgmsg(">ResGetString");
	while	(  isspace( c = ResGetChar(fp) )  );
	p = buf;
	if		(  c  ==  '"'  )	{
		while	(  ( c = ResGetChar(fp) )  !=  '"'  )	{
			switch	(c)	{
			  case	'^':
				c -= '@';
				break;
			  case	'\\':
				c = ResGetChar(fp);
				break;
			  case	'#':
				c = 0;
				for	( i = 0 ; i < 2 ; i ++ )	{
					cc = ResGetChar(fp);
					c <<= 4;
					if		(  isdigit(cc)  )	{
						c += cc - '0';
					} else
					if		(	(  cc  >=  'a'  )
							 &&	(  cc  <=  'f'  ) )	{
						c += cc - 'a' + 10;
					} else	
					if		(	(  cc  >=  'A'  )
							 &&	(  cc  <=  'F'  ) )	{
						c += cc - 'A' + 10;
					}
				}
				break;
			  default:
				break;
			}
			*p ++ = c;
		}
	} else {
		while	(  !isspace(c)  )	{
			switch	(c)	{
			  case	'^':
				c -= '@';
				break;
			  case	'\\':
				c = ResGetChar(fp);
				break;
			  case	'$':
				c = 0;
				for	( i = 0 ; i < 2 ; i ++ )	{
					cc = ResGetChar(fp);
					c <<= 4;
					if		(  isdigit(cc)  )	{
						c += cc - '0';
					} else
					if		(	(  cc  >=  'a'  )
							 &&	(  cc  <=  'f'  ) )	{
						c += cc - 'a' + 10;
					} else	
					if		(	(  cc  >=  'A'  )
							 &&	(  cc  <=  'F'  ) )	{
						c += cc - 'A' + 10;
					}
				}
				break;
			  default:
				break;
			}
			*p ++ = c;
			c = ResGetChar(fp);
		}
	}
	*p = 0;
#if	0
	printf("string = ");DumpStr(buf);
#endif
dbgmsg("<ResGetString");
	return	(buf);
}

static	int
ResGetNumber(
	FILE	*fp)
{	int		val;
	int		c;
	int		i;


dbgmsg(">ResGetNumber");
	val = 0;
	while	(  isspace( c = ResGetChar(fp) )  );
	if		(  c  ==  '$'  )	{
		for	( i = 0 ; i < 4 ; i ++ )	{
			c = ResGetChar(fp);
			val <<= 4;
			if		(  isdigit(c)  )	{
				val += c - '0';
			} else
			if		(	(  c  >=  'a'  )
					 &&	(  c  <=  'f'  ) )	{
				val += c - 'a' + 10;
			} else		
			if		(	(  c  >=  'A'  )
					 &&	(  c  <=  'F'  ) )	{
				val += c - 'A' + 10;
			} else {
				ResUnGetChar(c,fp);
				break;
			}
		}
	} else
	while	(  isdigit(c)  )	{
		val = val * 10 + ( c - '0' );
		c = ResGetChar(fp);
	}
#if	0
	printf("integer = [%d]\n",val);
#endif
dbgmsg("<ResGetNumber");
	return	(val);
}

static	char	*
ResStringDup(
	char	*s)
{	char	*ret;

	ret = (char *)xmalloc(strlen(s)+1);
	strcpy(ret,s);
	return	(ret);
}

#define	SYNTAX_ERROR	printf("syntax error %d\n",__LINE__)

static	void
ResParseOne(
	FILE	*fp)
{	CODE_SET	*set;
	char		buf[SIZE_BUFF]
	,			*esc[4];
	Bool		fGR;
	int			token;
	char		*name;
	int			class;
	int			num;
	int			c;
	int			i;

dbgmsg(">ResParseOne");
	name = ResStringDup(ResGetString(fp,buf));
	num = -1;
	class = SET_SBCS;
	for	( i = 0 ; i < 4 ; i ++ )	{
		esc[i] = (char *)xmalloc(SIZE_BUFF);
		*esc[i] = 0;
	}
	fGR = FALSE;
	while	(  ( token = ResGetWord(fp) )  !=  T_END  )	{
		switch	(token)	{
		  case	'#':
			while	(  ( c = ResGetChar(fp) )  !=  '\n'  );
			ResUnGetChar(c,fp);
			break;
		  case	T_TYPE:
			if		(  ResGetWord(fp)  ==  ':'  )	{
				token = ResGetWord(fp);
				switch	(token)	{
				  case	T_SBCS:
					class = SET_SBCS;
					break;
				  case	T_DBCS:
					class = SET_DBCS;
					break;
				  default:
					SYNTAX_ERROR;
					break;
				}
			}
			break;
		  case	T_SET:
			if		(  ResGetWord(fp)  ==  ':'  )	{
				num = ResGetNumber(fp);
			}
			break;
		  case	T_G0:
			if		(  ResGetWord(fp)  ==  ':'  )	{
				strcpy(esc[0],ResGetString(fp,buf));
			} else {
				SYNTAX_ERROR;
			}
			break;
		  case	T_G1:
			if		(  ResGetWord(fp)  ==  ':'  )	{
				strcpy(esc[1],ResGetString(fp,buf));
			} else {
				SYNTAX_ERROR;
			}
			break;
		  case	T_G2:
			if		(  ResGetWord(fp)  ==  ':'  )	{
				strcpy(esc[2],ResGetString(fp,buf));
			} else {
				SYNTAX_ERROR;
			}
			break;
		  case	T_G3:
			if		(  ResGetWord(fp)  ==  ':'  )	{
				strcpy(esc[3],ResGetString(fp,buf));	
			} else {
				SYNTAX_ERROR;
			}
			break;
		  case	T_GL:
			fGR = FALSE;
			break;
		  case	T_GR:
			fGR = TRUE;
			break;
		  default:
			SYNTAX_ERROR;
			break;
		}
	}
	if		(  num  >=  0  )	{
		set = AddCodeSet(name,num,class);
		set->fGR = fGR;
		for	( i = 0 ; i < 4 ; i ++ )	{
			if		(  strlen(esc[i])  >  0  )	{
				AddEscape(esc[i],set,i);
			}
		}
	}
#ifdef	DEBUG
	printf("name = [%s]\n",name);
	printf("num  = 0x%X\n",(int)num);
	printf("set  = 0x%X\n",(int)set);
	for	( i = 0 ; i < 4 ; i ++ ) {
		printf("ESC[%d] = ",i);
		_DumpArray(esc[i],strlen(esc[i]),PARA_HEX);
	}
#endif
	for	( i = 0 ; i < 4 ; i ++ )	{
		xfree(esc[i]);
	}
dbgmsg("<ResParseOne");
}

extern	void
ResParse(
	char	*res)
{	FILE	*fp;
	int		token;

dbgmsg(">ResParse");
	if		(  ( fp = fopen(res,"r") )  !=  NULL  )	{
		while	(  ( token = ResGetWord(fp) )  !=  T_EOF  )	{
			if		(  token  ==  T_CODESET  )	{
				ResParseOne(fp);
			} else {
				SYNTAX_ERROR;
			}
		}
		fclose(fp);
	}
dbgmsg("<ResParse");
}


#ifdef	MAIN
void
main(void)
{
	ResParse("codeset");
}
#endif
