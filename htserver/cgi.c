/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan

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
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	<errno.h>
#include	<iconv.h>

#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"multipart.h"
#include	"cgi.h"
#include	"cgi.h"
#include	"debug.h"

#define	SRC_CODESET		"euc-jp"

#define	SIZE_CHARS		16

extern	char	*
SaveValue(
	char		*name,
	char		*value,
	Bool		fSave)
{
	CGIValue	*val;
	char		*ret;

ENTER_FUNC;
	if		(  ( val = g_hash_table_lookup(Values, name) )  ==  NULL  )	{
		val = New(CGIValue);
		val->name = StrDup(name);
		g_hash_table_insert(Values, val->name, val);
	} else {
		if		(  val->body  !=  NULL  ) {
			xfree(val->body);
		}
	}
	if		(  value  !=  NULL  ) {
		val->body = StrDup(value);
	} else {
		val->body = NULL;
	}
	val->fSave = fSave;
	ret = val->body;
LEAVE_FUNC;
	return	(ret);
}

char *
SaveArgValue(char *name, char *value, Bool fSave)
{
    char *val, *str;

    if ((val = LoadValue(name)) != NULL) {
        char *str, *ret;

        str = (char *) xmalloc(strlen(val) + strlen(value) + 2);
        sprintf(str, "%s,%s", val, value);
        ret = SaveValue(name, str, fSave);
        xfree(str);
        return ret;
    } else {
        return SaveValue(name, value, fSave);
    }
}

extern	void
SetSave(
	char		*name,
	Bool		fSave)
{
	CGIValue	*val;

    if		(  ( val = g_hash_table_lookup(Values, name) )  ==  NULL  )	{
		val = New(CGIValue);
		val->name = StrDup(name);
		val->body = NULL;
        g_hash_table_insert(Values, val->name, val);
    }
	val->fSave = fSave;
}

extern	char	*
LoadValue(
	char		*name)
{
	CGIValue	*val;
	char		*value;

    if		(  ( val = g_hash_table_lookup(Values, name) )  ==  NULL  )	{
		value = NULL;
    } else {
		value = val->body;
    }
	return	(value);
}

extern	void
RemoveValue(
	char		*name)
{
	CGIValue	*val;
	char		*value;

    if		(  ( val = g_hash_table_lookup(Values, name) )  !=  NULL  )	{
		g_hash_table_remove(Values,name);
		xfree(val->name);
		if		(  val->body  !=  NULL  ) {
			xfree(val->body);
		}
		xfree(val);
    }
}

extern	void
ClearValues(void)
{
	void
		_ClearValue(
			char	*name,
			CGIValue	*val)
	{
		if		(  !val->fSave  ) {
			if		(  val->body  !=  NULL  ) {
				xfree(val->body);
				val->body = NULL;
			}
		}
	}

ENTER_FUNC;
	g_hash_table_foreach(Values,(GHFunc)_ClearValue,NULL);
LEAVE_FUNC;
}

static	int
HexCharToInt(
	int		c)
{
	int		ret;

	if		(	(  c  >=  '0'  )
				&&	(  c  <=  '9'  ) ) {
		ret = c - '0';
	} else
	if		(	(  c  >=  'A'  )
			&&	(  c  <=  'F'  ) ) {
		ret = c - 'A' + 10;
	} else
	if		(	(  c  >=  'a'  )
			&&	(  c  <=  'F'  ) ) {
		ret = c - 'a' + 10;
	} else {
		ret = 0;
	}
	return	(ret);
}

extern	char	*
ConvLocal(
	char	*istr)
{
	iconv_t	cd;
	size_t	sib
	,		sob;
	char	*ostr;
	static	char	cbuff[SIZE_BUFF];

	cd = iconv_open(Codeset,"utf8");
	sib = strlen(istr);
	ostr = cbuff;
	sob = SIZE_BUFF;
	iconv(cd,&istr,&sib,&ostr,&sob);
	*ostr = 0;
	iconv_close(cd);
	return	(cbuff);
}

extern	char	*
ConvUTF8(
	unsigned char	*str,
	char	*code)
{
	char	*istr;
	iconv_t	cd;
	size_t	sib
	,		sob;
	char	*ostr;
	static	char	cbuff[SIZE_BUFF];

	cd = iconv_open("utf8",code);
	istr = str;
dbgprintf("size = %d\n",strlen(str));
	if		(  ( sib = strlen(str)  )  >  0  ) {
		ostr = cbuff;
		sob = SIZE_BUFF;
		if		(  iconv(cd,&istr,&sib,&ostr,&sob)  !=  0  ) {
			dbgprintf("error = %d\n",errno);
		}
		*ostr = 0;
		iconv_close(cd);
	} else {
		*cbuff = 0;
	}

	return	(cbuff);
}

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
			if		(  isspace(*str)  ) {
				if		(  *str  !=  '\r'  ) {
					LBS_Emit(lbs,*str);
				}
				str ++;
			} else {
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

static	char	*ScanArgValue;
static	void
StartScanEnv(
	char	*env)
{
	int		c;

	ScanArgValue = env;

#if	0
	if		(  *env  !=  0  ) {
		while	(  *env  !=  0  ) {
			switch	(*env) {
			  case	'%':
				env ++;
				c = ( HexCharToInt(*env) << 4) ;
				env ++;
				c |= HexCharToInt(*env);
				break;
			  case	'+':
				c = ' ';
				break;
			  default:
				c = *env;
				break;
			}
			env ++;
		}
	}
#endif
}

static	Bool
ScanEnv(
	char	*name,
	byte	*value)
{
	byte	buff[SIZE_BUFF];
	byte	*p;
	int		c;
	Bool	rc;

	p = buff;
	if		(  *ScanArgValue  !=  0  ) {
		while	(	(  ( c = *ScanArgValue )  !=  0    )
				&&	(  c                     !=  '&'  ) ) {
			switch	(c) {
			  case	'%':
				ScanArgValue ++;
				*p = ( HexCharToInt(*ScanArgValue) << 4) ;
				ScanArgValue ++;
				*p |= HexCharToInt(*ScanArgValue);
				ScanArgValue ++;
				break;
			  case	'+':
				ScanArgValue ++;
				*p = ' ';
				break;
			  default:
				ScanArgValue ++;
				*p = c;
				break;
			}
			p ++;
		}
		if		(  c  ==  '&'  ) {
			ScanArgValue ++;
		}
	}
	*p = 0;
	if		(  p  !=  buff  ) {
		if		(  ( p = strchr(buff,'=') )  !=  NULL  ) {
			*p = 0;
			strcpy(name,buff);
			p ++;
			while	(  *p  !=  0  ) {
				if		(  *p  !=  '\r'  ) {
					*value ++ = *p;
				}
				p ++;
			}
			*value = 0;
		} else {
			*name = 0;
			strcpy(value,buff);
		}
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

static	Bool
ScanPost(
	char	*name,
	byte	*value)
{
	char	buff[SIZE_BUFF];
	char	*p;
	int		c;
	Bool	rc;

	p = buff;
	while	(	(  ( c = getchar() )  >=  0    )
			&&	(  c                  !=  '&'  ) ) {
		switch	(c) {
		  case	'%':
			*p = ( HexCharToInt(getchar()) << 4 ) | HexCharToInt(getchar());
			break;
		  case	'+':
			*p = ' ';
			break;
		  default:
			*p = c;
			break;
		}
		p ++;
	}
	*p = 0;
	if		(  p  !=  buff  ) {
		if		(  ( p = strchr(buff,'=') )  !=  NULL  ) {
			*p = 0;
			strcpy(name,buff);
			p ++;
			while	(  *p  !=  0  ) {
				if		(  *p  !=  '\r'  ) {
					*value ++ = *p;
				}
				p ++;
			}
			*value = 0;
		} else {
			*name = 0;
			strcpy(value,buff);
		}
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

extern	void
GetArgs(void)
{
	char	name[SIZE_LONGNAME+1];
	byte	value[SIZE_BUFF]
		,	buff[SIZE_BUFF];
    char	*boundary;
	char	*env;
	char	*val
		,	*str
		,	*p;

ENTER_FUNC;
	Values = NewNameHash();
    Files = NewNameHash();
	if		(  ( env  =  CommandLine )  ==  NULL  ) {
		env = getenv("QUERY_STRING");
	}
	if		(  env  !=  NULL  ) {
		StartScanEnv(env);
		while	(  ScanEnv(name,value)  ) {
            SaveArgValue(name, value, FALSE);
		}
	}
	if		(  CommandLine  ==  NULL  ) {
		if ((boundary = GetMultipartBoundary(getenv("CONTENT_TYPE"))) != NULL) {
			if (ParseMultipart(stdin, boundary, Values, Files) < 0) {
				fprintf(stderr, "malformed multipart/form-data\n");
				exit(1);
			}
		} else {
			while	(  ScanPost(name,value)  ) {
                SaveArgValue(name, value, FALSE);
			}
		}
		if		(  fCookie  ) {
			if		(  ( env = getenv("HTTP_COOKIE") )  !=  NULL  ) {
				strcpy(buff,env);
				if      (  ( p = strrchr(buff,';') )  !=  NULL  )   *p = 0;
                StartScanEnv(buff);
				while	(  ScanEnv(name,value)  ) {
					dbgprintf("var name = [%s]\n",name);
					if		(  ( val = LoadValue(name) )  !=  NULL  ) {
						str = (char *)xmalloc(strlen(val) + strlen(value) + 2);
						sprintf(str,"%s,%s",val,value);
						SaveValue(name,str,FALSE);
						xfree(str);
					} else {
						SaveValue(name, value,FALSE);
					}
				}
			}
		}
	}
LEAVE_FUNC;
}

extern	Bool
GetSessionValues(void)
{
	char	*sesid;
	char	fname[SIZE_LONGNAME+1];
	char	name[SIZE_BUFF];
	byte	value[SIZE_BUFF];
	int		fd;
	Bool	ret;
	struct	stat	sb;
	char	*p;

ENTER_FUNC;
	if		( ( sesid = LoadValue("_sesid") )  !=  NULL  ) {
		sprintf(fname,"%s/%s.ses",SesDir,sesid);
		if		(  ( fd = open(fname,O_RDONLY ) )  <  0  ) {
			ret = FALSE;
		} else {
			fstat(fd,&sb);
			if		(  ( p = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0) )  !=  NULL  ) {
				StartScanEnv(p);
				while	(  ScanEnv(name,value)  ) {
					if		(  LoadValue(name)  ==  NULL  ) {
						SaveValue(name,value,TRUE);
					} else {
						SetSave(name,TRUE);
					}
				}
				munmap(p,sb.st_size);
				ret = TRUE;
			} else {
				ret = FALSE;
			}
			close(fd);
		}
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
PutValue(
	char	*name,
	CGIValue	*value,
	FILE	*fp)
{
	byte	*p;

	if		(	(  value->fSave  )
			&&	(  *name  !=  0  ) ) {
		fprintf(fp,"%s=",name);
		if		(  ( p = value->body )  !=  NULL  ) {
			while	(  *p  !=  0  ) {
				if		(  *p  ==  0x20  ) {
					fputc('+',fp);
				} else
				if		(  isalnum(*p)  ) {
					fputc(*p,fp);
				} else {
					fputc('%',fp);
					fprintf(fp,"%02X",((int)*p)&0xFF);
				}
				p ++;
			}
		}
		fputc('&',fp);
	}
}

extern	Bool
PutSessionValues(void)
{
	char	fname[SIZE_LONGNAME+1];
	char	name[SIZE_BUFF];
	byte	value[SIZE_BUFF];
	Bool	ret;
	FILE	*fp;
	char	*p;
	char	*sesid;

ENTER_FUNC;
	if		(  ( sesid = LoadValue("_sesid") )  !=  NULL  ) {
		sprintf(fname,"%s/%s.ses",SesDir,sesid);
		if		(  ( fp = fopen(fname,"w") )  ==  NULL  ) {
			ret = FALSE;
		} else {
			g_hash_table_foreach(Values,(GHFunc)PutValue,fp);
			fputc(0,fp);
			fclose(fp);
			ret = TRUE;
		}
	} else {
		ret = FALSE;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
DeleteSessionValues(void)
{
	char	fname[SIZE_LONGNAME+1];
	char	*sesid;

	if		(  ( sesid = LoadValue("_sesid") )  !=  NULL  ) {
		sprintf(fname,"%s/%s.ses",SesDir,sesid);
		unlink(fname);
	}
}

extern	void
WriteLargeString(
	FILE			*output,
	LargeByteString	*lbs,
	char			*codeset)
{
	char	*oc
	,		*ic
	,		*istr;
	char	obuff[SIZE_CHARS]
	,		ibuff[SIZE_CHARS];
	size_t	count
	,		sib
	,		sob;
	int		ch;
	iconv_t	cd;

ENTER_FUNC;
	RewindLBS(lbs);
	if		(  output  ==  NULL  ) {
	} else
	if		(	(  libmondai_i18n  )
			&&	(  codeset  !=  NULL  ) ) {
		cd = iconv_open(codeset,"utf8");
		while	(  !LBS_Eof(lbs)  ) {
			count = 0;
			ic = ibuff;
			do {
				if		(  ( ch = LBS_FetchChar(lbs) )  <  0  )	break;
				*ic ++ = ch;
				count ++;
				istr = ibuff;
				sib = count;
				oc = obuff;
				sob = SIZE_CHARS;
				if		(  iconv(cd,&istr,&sib,&oc,&sob)  ==  0  )	break;
			}	while	(  count  <   SIZE_CHARS  );
			for	( oc = obuff ; sob < SIZE_CHARS ; oc ++ , sob ++ ) {
				if		(  *oc  !=  0  ) {
					fputc((int)*oc,output);
				}
			}
		}
		iconv_close(cd);
	} else {
		fprintf(output,"%s\n",LBS_Body(lbs));
	}
LEAVE_FUNC;
}

extern	void
PutHTML(
	LargeByteString	*html)
{
	char	*sesid
		,	*server;
	char	domain[SIZE_LONGNAME+1];
	int		c;

ENTER_FUNC;
	printf("Content-Type: text/html; charset=%s\r\n", Codeset);
	LBS_EmitEnd(html);
	if		(  ( server = getenv("SERVER_NAME") )  !=  NULL  ) {
		sprintf(domain,"domain=%s;",server);
	} else {
		strcpy(domain,"");
	}
	if		(  fCookie  ) {
		if		(  ( sesid = LoadValue("_sesid") )  !=  NULL  ) {
			printf("Set-Cookie: _sesid=%s;%s\r\n",sesid,domain);
		} else {
			printf("Set-Cookie: _sesid=;%s\r\n",domain);
		}
	}
	printf("Cache-Control: no-cache\r\n");
	printf("\r\n");
	WriteLargeString(stdout,html,Codeset);
	//	WriteLargeString(_fpLog,html,Codeset);
LEAVE_FUNC;
}

extern	void
DumpValues(
	LargeByteString	*html,
	GHashTable	*args)
{
	char	buff[SIZE_BUFF];
	void
	DumpValue(
			char		*name,
			CGIValue	*value,
			gpointer	user_data)
	{
		if		(  value->body  !=  NULL  ) {
			sprintf(buff,"<TR><TD>%s<TD>%s\n",name,value->body);
			LBS_EmitString(html,buff);
		}
	}

	LBS_EmitUTF8(html,
				 "<HR>\n"
				 "<H2>引数</H2>"
				 "<TABLE BORDER>\n"
				 "<TR><TD width=\"150\">名前<TD width=\"150\">値\n"
				 ,SRC_CODESET);
	g_hash_table_foreach(args,(GHFunc)DumpValue,NULL);
	LBS_EmitUTF8(html,
				 "</TABLE>\n"
				 ,SRC_CODESET);
}

extern	void
DumpFiles(
	LargeByteString	*html,
	GHashTable	*args)
{
	char	buff[SIZE_BUFF];
	void
	DumpFile(
			char		*name,
			MultipartFile	*value,
			gpointer	user_data)
	{
		sprintf(buff,"<TR><TD>%s<TD>%-10d\n",value->filename,value->length);
		LBS_EmitString(html,buff);
	}

	LBS_EmitUTF8(html,
				 "<HR>\n"
				 "<H2>ファイル</H2>"
				 "<TABLE BORDER>\n"
				 "<TR><TD width=\"150\">ファイル名<TD width=\"150\">大きさ\n"
				 ,SRC_CODESET);
	g_hash_table_foreach(args,(GHFunc)DumpFile,NULL);
	LBS_EmitUTF8(html,
				 "</TABLE>\n"
				 ,SRC_CODESET);
}

static	void
PutEnv(
	LargeByteString	*html)
{
	extern	char	**environ;
	char	**env;

	env = environ;
	LBS_EmitUTF8(html,
				 "<HR>\n"
				 "<H2>環境変数</H2>\n",SRC_CODESET);
	while	(  *env  !=  NULL  ) {
		LBS_EmitUTF8(html,"[",SRC_CODESET);
		LBS_EmitUTF8(html,*env,SRC_CODESET);
		env ++;
		LBS_EmitUTF8(html,
					 "]<BR>\n",SRC_CODESET);
	}
}

extern	void
Dump(void)
{
	LargeByteString	*html;

	html = NewLBS();
	LBS_EmitStart(html);
	LBS_EmitUTF8(html,
				 "<HR>\n",SRC_CODESET);
	PutEnv(html);
	DumpValues(html,Values);
	DumpFiles(html,Files);
	LBS_EmitUTF8(html,
				 "</BODY>\n"
				 "</HTML>\n",SRC_CODESET);
	LBS_EmitEnd(html);
	WriteLargeString(stdout,html,Codeset);
}

extern	LargeByteString	*
Expired(void)
{
	LargeByteString	*html;

	Codeset = SRC_CODESET;
	html = NewLBS();
	LBS_EmitStart(html);
	LBS_EmitUTF8(html,
				 "<html><head>"
				 "<title>htserver error</title>"
				 "</head><body>\n"
				 "<H1>htserver error</H1>\n"
				 "<p>maybe session was expired. please retry.</p>\n"
				 "<p>おそらくセション変数の保持時間切れでしょう。"
				 "もう一度最初からやり直して下さい。</p>\n"
				 "</body></html>\n",SRC_CODESET);
	return	(html);
}

extern	char	*
CheckCoding(
	char	*str)
{
	int		blace
		,	quote;
	Bool	fMeta
		,	fHTC
		,	fXML;
	char	*p;
	static	char	coding[SIZE_NAME+1];

	blace = 1;
	quote = 0;
	fMeta = FALSE;
	fHTC = FALSE;
	fXML = FALSE;
	strcpy(coding,SRC_CODESET);
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
			break;
		  case	'm':
		  case	'M':
			if		(	(  blace  >   0  )
					&&	(  quote  ==  0  )
					&&	(  !strlicmp(str,"meta")  ) ) {
				fMeta = TRUE;
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
				break;
			}
		} else
		if		(  fHTC  ) {
			while	(  isspace(*str)  )	str ++;
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
			break;
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
			break;
		}
		str ++;
	}
	return	(coding);
}

extern	char	*
ImportFile(
	LargeByteString	*html,
	char	*name,
	Bool	fExpand)
{
	LargeByteString	*text;
	int		fd;
	int		ch;
	struct	stat	sb;
	char	*p
		,	*str
		,	*coding
		,	*val;
	char	vname[SIZE_LONGNAME];

ENTER_FUNC;
	if		(  ( fd = open(name,O_RDONLY ) )  >=  0  ) {
		fstat(fd,&sb);
		if		(  S_ISREG(sb.st_mode)  ) {
			if		(  ( p = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0) )  !=  NULL  ) {
				str = (char *)xmalloc(sb.st_size+1);
				memcpy(str,p,sb.st_size);
				str[sb.st_size] = 0;
				coding = CheckCoding(str);
				text = NewLBS();
				LBS_EmitStart(text);
				LBS_EmitUTF8(text,str,coding);
				LBS_EmitEnd(text);
				munmap(p,sb.st_size);
				xfree(str);
			}
			close(fd);
			RewindLBS(text);
			while	(  !LBS_Eof(text)  ) {
				switch	(ch = LBS_FetchChar(text)) {
				  case	'$':
					if		(  fExpand  ) {
						p = vname;
						while	(	(  isalnum(ch = LBS_FetchChar(text))  )
									||	(  ch  ==  '_'  ) ) {
							*p ++ = ch;
						}
						*p = 0;
						if		(  ( val = LoadValue(vname) )  !=  NULL  ) {
							LBS_EmitString(html,val);
						}
					}
					LBS_Emit(html,ch);
					break;
				  case	0:
					break;
				  default:
					LBS_Emit(html,ch);
					break;
				}
			}
			FreeLBS(text);
		}
	} else {
		dbgprintf("file not found = [%s]\n",name);
	}
LEAVE_FUNC;
	return	(coding);
}

extern	void
InitCGI(void)
{
	GetArgs();
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * tab-width: 4
 * End:
 */
