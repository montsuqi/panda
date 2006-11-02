/*
 * PANDA -- a simple transaction monitor
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
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	<errno.h>
#include	<iconv.h>
#include	<dirent.h>
#include	<time.h>

#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"multipart.h"
#include	"cgi.h"
#include    "HTClex.h"
#include    "tags.h"
#include    "exec.h"
#include	"debug.h"

static	GET_VALUE	_GetValue;

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
        val->inFilter = StrDup;
		g_hash_table_insert(Values, val->name, val);
	} else {
		if		(  val->body  !=  NULL  ) {
			xfree(val->body);
		}
	}
	if		(  value  !=  NULL  ) {
        if      (  val->inFilter  !=  NULL  ) {
            val->body = (*val->inFilter)(value);
        } else {
            val->body = StrDup(value);
        }
	} else {
		val->body = NULL;
	}
	val->fSave = fSave;
	ret = val->body;
LEAVE_FUNC;
	return	(ret);
}

extern  void
SetFilter(
    char    *name,
    char    *(*inFilter)(char *in),
    char    *(*outFilter)(char *out))
{
	CGIValue	*val;

ENTER_FUNC;
	if		(  ( val = g_hash_table_lookup(Values, name) )  ==  NULL  )	{
		val = New(CGIValue);
		val->name = StrDup(name);
        val->body = NULL;
    }
    val->inFilter = inFilter;
    val->outFilter = outFilter;
    g_hash_table_insert(Values, val->name, val);
LEAVE_FUNC;
}

extern  char    *
SaveArgValue(
    char    *name,
    char    *value,
    Bool fSave)
{
    char    *val
        ,   *ret;
    char    *str;

    if      (  *name  ==  0  ) {
        RemoveValue(name);
        ret = SaveValue(name, value,fSave);
    } else
    if ((val = LoadValue(name)) != NULL) {

        str = (char *) xmalloc(strlen(val) + strlen(value) + 2);
        sprintf(str, "%s,%s", val, value);
        ret = SaveValue(name, str, fSave);
        xfree(str);
    } else {
        ret = SaveValue(name, value, fSave);
    }
    return  (ret);
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
	static	char	cbuff[SIZE_LARGE_BUFF];

	cd = iconv_open(Codeset,"utf8");
	sib = strlen(istr);
	ostr = cbuff;
	sob = SIZE_LARGE_BUFF;
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
	static	char	cbuff[SIZE_LARGE_BUFF];

	cd = iconv_open("utf8",code);
	istr = (char *)str;
dbgprintf("size = %d\n",strlen(str));
 if		(  ( sib = strlen((char *)str)  )  >  0  ) {
		ostr = cbuff;
		sob = SIZE_LARGE_BUFF;
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

extern  void
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
	ScanArgValue = env;
}

static	Bool
ScanEnv(
	char	*name,
	char	*value)
{
	char	buff[SIZE_LARGE_BUFF];
	char	*p;
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
		if		(  ( p = strchr((char *)buff,'=') )  !=  NULL  ) {
			*p = 0;
			strcpy(name,(char *)buff);
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
			strcpy((char *)value,buff);
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
	char	*value)
{
	char	buff[SIZE_LARGE_BUFF];
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
			strcpy((char *)value,buff);
		}
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

extern  void
CGI_InitValues(void)
{
	Values = NewNameHash();
    Files = NewNameHash();
}

extern	void
GetArgs(void)
{
	char	name[SIZE_LONGNAME+1];
	char	value[SIZE_LARGE_BUFF]
		,	buff[SIZE_LARGE_BUFF];
    char	*boundary;
	char	*env;
	char	*val
		,	*str
		,	*p;

ENTER_FUNC;
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
				strcpy((char *)buff,env);
				if      (  ( p = strrchr((char *)buff,';') )  !=  NULL  )   *p = 0;
                StartScanEnv((char *)buff);
				while	(  ScanEnv(name,value)  ) {
					dbgprintf("var name = [%s]\n",name);
                    if      (  *name  ==  0  ) {
                        RemoveValue(name);
						SaveValue(name, (char *)value,FALSE);
                    } else
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
	char	name[SIZE_LARGE_BUFF];
	char	value[SIZE_LARGE_BUFF];
	int		fd;
	Bool	ret;
	struct	stat	sb;
	char	*p;

ENTER_FUNC;
	if		(   ( ( sesid = LoadValue("_sesid") )  !=  NULL  )
            &&  (  *sesid  !=  0  ) ) {
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
                        if      (  *name  !=  0  ) {
                            SetSave(name,TRUE);
                        }
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
		if		(  ( p = (byte *)value->body )  !=  NULL  ) {
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

extern  void
CheckSessionExpire(void)
{
	DIR		*dir;
	struct	dirent	*ent;
	struct	stat	sb;
	time_t		    nowtime
        ,           exp;
    char    fname[SIZE_LONGNAME+1];

    time(&nowtime);
    exp = nowtime - SesExpire;
	dir = opendir(SesDir);
	while	(  ( ent = readdir(dir) )  !=  NULL  ) {
        sprintf(fname,"%s/%s",SesDir,ent->d_name);
		if		(	(  stat(fname,&sb)  ==  0  )
                &&	(  S_ISREG(sb.st_mode)     ) ) {
            if      (  sb.st_mtime  <  exp  ) {
                unlink(fname);
            }
		}
	}
	closedir(dir);
}

extern	Bool
PutSessionValues(void)
{
	char	fname[SIZE_LONGNAME+1];
	Bool	ret;
	FILE	*fp;
	char	*sesid;

ENTER_FUNC;
	CheckSessionExpire();
	if		(   (  ( sesid = LoadValue("_sesid") )  !=  NULL  )
            &&  (  *sesid  !=  0  ) ) {
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
			&&	(  codeset  !=  NULL  )
            &&  (  stricmp(codeset,"utf8")  !=  0  ) ) {
		cd = iconv_open(codeset,"utf8");
		while	(  !LBS_Eof(lbs)  ) {
			count = 0;
			ic = ibuff;
			do {
				if		(  ( ch = LBS_FetchChar(lbs) )  ==  0  )	break;
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
		fprintf(output,"%s\n",(char *)LBS_Body(lbs));
	}
LEAVE_FUNC;
}

extern	void
PutHTML(
	LargeByteString	*header,
	LargeByteString	*html,
	int				code)
{
	char	*sesid;

ENTER_FUNC;
	if		(  code  ==  0  ) {
		code = 200;
	}
	printf("Status: %d\r\n",code);
	printf("Content-Type: text/html; charset=%s\r\n", Codeset);
	if		(  fCookie  ) {
		if		(  ( sesid = LoadValue("_sesid") )  !=  NULL  ) {
            printf("Set-Cookie: _sesid=%s;\r\n",sesid);
		} else {
			printf("Set-Cookie: _sesid=;\r\n");
		}
	}
#if	0
	printf("Cache-Control: no-cache\r\n");
#endif
	if		(  header  !=  NULL  ) {
		LBS_EmitEnd(header);
		WriteLargeString(stdout,header,Codeset);
	}
	if		(  html  !=  NULL  ) {
		printf("\r\n");
		LBS_EmitEnd(html);
		WriteLargeString(stdout,html,Codeset);
	}
LEAVE_FUNC;
}

extern	void
DumpValues(
	LargeByteString	*html,
	GHashTable	*args)
{
	char	buff[SIZE_LARGE_BUFF];
	void
	DumpValue(
			char		*name,
			CGIValue	*value,
			gpointer	user_data)
	{
		if		(  value->body  !=  NULL  ) {
			sprintf(buff,"<tr><td>%s</td><td>",name);
			LBS_EmitUTF8(html,buff,NULL);
			EmitWithEscape(html,value->body);
			LBS_EmitUTF8(html,"</td></tr>\n",NULL);
		}
	}

	LBS_EmitUTF8(html,
				 "<HR>\n"
				 "<H2>args</H2>"
				 "<table border>\n"
				 "<tr><td width=\"150\">name<td width=\"150\">value</tr>\n"
				 ,NULL);
	g_hash_table_foreach(args,(GHFunc)DumpValue,NULL);
	LBS_EmitUTF8(html,
				 "</table>\n"
				 ,NULL);
}

extern	void
DumpFiles(
	LargeByteString	*html,
	GHashTable	*args)
{
	char	buff[SIZE_LARGE_BUFF];
	void
	DumpFile(
			char		*name,
			MultipartFile	*value,
			gpointer	user_data)
	{
		sprintf(buff,"<TR><TD>%s<TD>%-10d\n",value->filename,LBS_Size(value->body));
		LBS_EmitString(html,buff);
	}

	LBS_EmitUTF8(html,
				 "<HR>\n"
				 "<H2>files</H2>"
				 "<TABLE BORDER>\n"
				 "<TR><TD width=\"150\">file name<TD width=\"150\">size\n"
				 ,NULL);
	g_hash_table_foreach(args,(GHFunc)DumpFile,NULL);
	LBS_EmitUTF8(html,
				 "</TABLE>\n"
				 ,NULL);
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
				 "<H2>environments</H2>\n",NULL);
	while	(  *env  !=  NULL  ) {
		LBS_EmitUTF8(html,"[",NULL);
		LBS_EmitUTF8(html,*env,NULL);
		env ++;
		LBS_EmitUTF8(html,
					 "]<BR>\n",NULL);
	}
}

extern	void
Dump(void)
{
	LargeByteString	*html;

	html = NewLBS();
	LBS_EmitStart(html);
    if (fDump) {
        LBS_EmitUTF8(html,
                     "<HR>\n",NULL);
        PutEnv(html);
        DumpValues(html,Values);
        DumpFiles(html,Files);
        LBS_EmitUTF8(html,
                     "</BODY>\n"
                     "</HTML>\n",NULL);
    }
	LBS_EmitEnd(html);
	WriteLargeString(stdout,html,Codeset);
}

extern	char	*
GetHostValue(
	char	*name,
	Bool	fClear)
{
	ValueStruct			*item;
	char	*value;

	dbgprintf("name = [%s]\n",name);
	if		(  ( value = LoadValue(name) )  ==  NULL  )	{
		if		(  _GetValue  !=  NULL  ) {
			if		(  ( item = (_GetValue)(name, fClear) )  ==  NULL  ) {
				value = StrDup(name);
			} else {
				value = ValueToString(item, NULL);
			}
			value = SaveValue(name,value,FALSE);
		}
	}
	dbgprintf("value = [%s]\n",value);
	return	(value);
}

extern	void
InitHTC(
	char	*script_name,
	GET_VALUE	func)
{
ENTER_FUNC;
    if (script_name == NULL) {
        script_name = "mon.cgi";
    }
	HTCLexInit();
	JslibInit();

	TagsInit(script_name);
	Codeset = "utf-8";
	_GetValue = func;
LEAVE_FUNC;
}

extern	void
InitCGI(void)
{
ENTER_FUNC;
    CGI_InitValues();
	GetArgs();
LEAVE_FUNC;
}

/*
 * Local variables:
 * tab-width: 4
 * End:
 */
