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

#define	MAIN
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

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"socket.h"
#include	"net.h"
#include	"comms.h"
#include	"HTCparser.h"
#include	"mon.h"
#include	"tags.h"
#include	"exec.h"
#include	"option.h"
#include	"message.h"
#include	"multipart.h"
#include	"debug.h"

#define	SRC_CODESET		"euc-jp"

#define	SIZE_CHARS		16

static	NETFILE	*fpServ;
static	char	*ServerPort;
static	char	*Command;
static	LargeByteString	*lbs;

//char		*RecordDir;	/*	dummy	*/
static	ARG_TABLE	option[] = {
	{	"server",	STRING,		TRUE,	(void*)&ServerPort,
		"サーバポート"	 								},
	{	"screen",	STRING,		TRUE,	(void*)&ScreenDir,
		"画面格納ディレクトリ"	 						},
	{	"command",	STRING,		TRUE,	(void*)&Command,
		"サーバコマンドライン"	 						},
	{	"get",		BOOLEAN,	TRUE,	(void*)&fGet,
		"actionをgetで処理する"	 						},
	{	"dump",		BOOLEAN,	TRUE,	(void*)&fDump,
		"変数のダンプを行う"	 						},
	{	"cookie",	BOOLEAN,	TRUE,	(void*)&fCookie,
		"セション変数をcookieで行う"					},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

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

	if		(  libmondai_i18n  ) {
		cd = iconv_open(Codeset,"utf8");
		sib = strlen(istr);
		ostr = cbuff;
		sob = SIZE_BUFF;
		iconv(cd,&istr,&sib,&ostr,&sob);
		*ostr = 0;
		iconv_close(cd);
	} else {
		strcpy(cbuff,istr);
	}
	return	(cbuff);
}

extern	char	*
ConvUTF8(
	char	*istr)
{
	iconv_t	cd;
	size_t	sib
	,		sob;
	char	*ostr;
	static	char	cbuff[SIZE_BUFF];

	if		(  libmondai_i18n  ) {
		cd = iconv_open("utf8",Codeset);
		sib = strlen(istr);
		ostr = cbuff;
		sob = SIZE_BUFF;
		if		(  iconv(cd,&istr,&sib,&ostr,&sob)  !=  0  ) {
			dbgprintf("error = %d\n",errno);
		}
		*ostr = 0;
		iconv_close(cd);
	} else {
		strcpy(cbuff,istr);
	}

	return	(cbuff);
}

static	char	*ScanArgValue;
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
			strcpy(value,p+1);
		} else {
			strcpy(value,buff);
			*name = 0;
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
			strcpy(value,p+1);
		} else {
			strcpy(value,buff);
			*name = 0;
		}
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

extern void
StoreValue(GHashTable *hash, char *name, char *value)
{
    char *old_value;
    if ((old_value = g_hash_table_lookup(hash, name)) == NULL) {
        g_hash_table_insert(hash, StrDup(name), StrDup(value));
    }
    else {
        char *new_value = (char *) xmalloc(strlen(old_value) + 1 +
                                           strlen(value) + 1);
        sprintf(new_value, "%s,%s", old_value, value);
        g_hash_table_insert(hash, StrDup(name), new_value);
    }
}

static	void
GetArgs(void)
{
	char	name[SIZE_BUFF];
	byte	value[SIZE_BUFF];
    char	*boundary;
    char    *old_value;

	if		(  ( ScanArgValue = getenv("QUERY_STRING") )  !=  NULL  ) {
		while	(  ScanEnv(name,value)  ) {
            StoreValue(Values, name, value);
		}
	}
    if ((boundary = GetMultipartBoundary(getenv("CONTENT_TYPE"))) != NULL) {
        if (ParseMultipart(stdin, boundary, Values, Files) < 0) {
            fprintf(stderr, "malformed multipart/form-data\n");
            exit(1);
        }
    }
    else {
        while	(  ScanPost(name,value)  ) {
            StoreValue(Values, name, value);
        }
    }
	if		(  fCookie  ) {
		if		(  ( ScanArgValue = getenv("HTTP_COOKIE") )  !=  NULL  ) {
			while	(  ScanEnv(name,value)  ) {
				if		(  g_hash_table_lookup(Values,name)  ==  NULL  ) {
					g_hash_table_insert(Values,StrDup(name),StrDup(value));
				}
			}
		}
	}
}

static	void
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
	if		(	(  libmondai_i18n  )
			&&	(  codeset  !=  NULL  ) ) {
		cd = iconv_open(codeset,"utf8");
		while	(  !LBS_Eof(lbs)  ) {
			count = 0;
			ic = ibuff;
			do {
				ch = LBS_FetchChar(lbs);
				if		(  ( ch & 0x80 )  ==  0  ) {
					*obuff = ch;
					sob = SIZE_CHARS - 1;
					break;
				} else {
					*ic ++ = ch;
					count ++;
					istr = ibuff;
					sib = count;
					oc = obuff;
					sob = SIZE_CHARS;
					if		(  iconv(cd,&istr,&sib,&oc,&sob)  ==  0  )	break;
				}
			}	while	(	(  ch     !=  0           )
						&&	(  count  <   SIZE_CHARS  ) );
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

static	void
PutHTML(
	LargeByteString	*html)
{
	char	*sesid;
	int		c;

dbgmsg(">PutHTML");
	printf("Content-Type: text/html; charset=%s\r\n", Codeset);
	LBS_EmitEnd(html);
	if		(  fCookie  ) {
		if		(  ( sesid = g_hash_table_lookup(Values,"_sesid") )  !=  NULL  ) {
			printf("Set-Cookie: _sesid=%s;\r\n",sesid);
		}
	}
	printf("Cache-Control: no-cache\r\n");
	printf("\r\n");
	WriteLargeString(stdout,html,Codeset);
dbgmsg("<PutHTML");
}

static void
PutFile(ValueStruct *file)
{
    char *filename_field, *ctype_field;
    char *filename, *ctype;
    char *p;

    if ((ctype_field = g_hash_table_lookup(Values, "_contenttype")) != NULL) {
        ctype = HT_GetValueString(ctype_field, FALSE);
        printf("Content-Type: %s\r\n", ctype);
    }
    else {
        printf("Content-Type: application/octet-stream\r\n");
    }
    if ((filename_field = g_hash_table_lookup(Values, "_filename")) != NULL) {
        filename = HT_GetValueString(filename_field, FALSE);
        printf("Content-Disposition: attachment; filename=%s\r\n", filename);
    }
	printf("Cache-Control: no-cache\r\n");
	printf("\r\n");
    switch (ValueType(file)) {
    case GL_TYPE_BYTE:
    case GL_TYPE_BINARY:
        fwrite(ValueByte(file), 1, ValueByteLength(file), stdout);
        break;
    default:
        fputs(ValueToString(file, Codeset), stdout);
        break;
    }
    fDump = FALSE;
}

static	void
DumpValues(
	LargeByteString	*html,
	GHashTable	*args)
{
	char	buff[SIZE_BUFF];
	void
	DumpValue(
			char		*name,
			char		*value,
			gpointer	user_data)
	{
		sprintf(buff,"<TR><TD>%s<TD>%s\n",name,value);
		LBS_EmitString(html,buff);
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

static	void
Dump(void)
{
	LargeByteString	*html;

	if		(  fDump  ) {
		html = NewLBS();
		LBS_EmitStart(html);
		LBS_EmitUTF8(html,
					 "<HR>\n",SRC_CODESET);
		//					 "<H2>コマンドライン</H2>\n"
		//					 "<PRE>\n"
		//					 //		PrintUsage(option,"");
		//		printf("</PRE>\n");
		PutEnv(html);
		DumpValues(html,Values);
		LBS_EmitUTF8(html,
					 "</BODY>\n"
					 "</HTML>\n",SRC_CODESET);
		LBS_EmitEnd(html);
		WriteLargeString(stdout,html,Codeset);
	}
}

static	void
SetDefault(void)
{
	ServerPort = "localhost:8010";
	ScreenDir = getcwd(NULL,0);
	Command = "demo";
	fDump = FALSE;
	fGet = FALSE;
	fCookie = FALSE;
}

extern	void
HT_SendString(
	char	*str)
{
	//	dbgprintf("send [%s]\n",str);
	SendStringDelim(fpServ,str);
}

extern	Bool
HT_RecvString(
	size_t	size,
	char	*str)
{
	Bool	rc;

	rc = RecvStringDelim(fpServ,size,str);
	dbgprintf("recv [%s]\n",str);
	return	(rc);
}

extern ValueStruct *
HT_GetValue(char *name, Bool fClear)
{
	char	buff[SIZE_BUFF+1];
	PacketDataType	type;
    ValueStruct *value;

    LBS_EmitStart(lbs);
    sprintf(buff,"%s%s\n",name,(fClear ? " clear" : "" ));
    HT_SendString(buff);
    RecvLBS(fpServ, lbs);
    if (LBS_Size(lbs) == 0)
        return NULL;
    type = *(PacketDataType *) LBS_Body(lbs);
    value = NewValue(type);
    NativeUnPackValue(NULL, LBS_Body(lbs), value);
	return value;
}

extern char	*
HT_GetValueString(char *name, Bool fClear)
{
	char	*value;
    ValueStruct *val;
    char	*s;

	if (*name == '\0') {
		return "";
	}
    else if ((value = g_hash_table_lookup(Values, name))  ==  NULL) {
        val = HT_GetValue(name, fClear);
        if (val == NULL)
            return "";
        value = StrDup(ValueToString(val, NULL));
        g_hash_table_insert(Values, StrDup(name), value);
        return value;
	}
}

static	void
SendValueDelim(
	char		*name,
	byte		*value)
{
    char	*s;
    size_t len;

	HT_SendString(name);
	HT_SendString("\n");
    s = ConvUTF8(value);
    len = strlen(s);
	SendLength(fpServ, len);
    Send(fpServ, s, len);
	dbgprintf("send value = [%s: %s]\n",name,value);
}

static	void
SendFile(char *name, MultipartFile *file, HTCInfo *htc)
{
    char *filename;
    char buff[SIZE_BUFF * 3 + 1];
    char *p, *pend;
    int len = 0;
    int x = 0;

    filename = (char *) g_hash_table_lookup(htc->FileSelection, name);
    if (filename != NULL) {
        HT_SendString(name);
        HT_SendString("\n");
        SendLength(fpServ, file->length);
        Send(fpServ, file->value, file->length);
        dbgprintf("send value = [%s]\n", name);

        SendValueDelim(filename, file->filename);
    }
}

static	void
SendEvent(void)
{
	char	*button;
	char	*event;
	char	*sesid;
	char	*file;
	char	*filename;
	char	*contenttype;
	HTCInfo	*htc;
	char	*name;

	void	GetRadio(
		char	*name)
		{
			char	*rname;

			if		(  ( rname = g_hash_table_lookup(Values,name) )  !=  NULL  ) {
				g_hash_table_remove(Values,name);
				g_hash_table_insert(Values,rname,"TRUE");
			}
		}
	
    if		(  ( name = g_hash_table_lookup(Values,"_name") )  !=  NULL  ) {
        htc = HTCParser(name);
        if (htc == NULL)
            exit(1);
        if ((button = g_hash_table_lookup(Values,"_event")) == NULL) {
            if (htc->DefaultEvent == NULL) {
                event = "";
                htc = NULL;
            }
            else {
                event = htc->DefaultEvent;
            }
        }
        else {
            event = g_hash_table_lookup(htc->Trans,ConvUTF8(button));
            if (event == NULL) {
                event = button;
            }
        }
    } else {
        event = "";
        htc = NULL;
    }
	if		(  htc  !=  NULL  ) {
		g_hash_table_foreach(htc->Radio,(GHFunc)GetRadio,NULL);
	}

	HT_SendString(event);
	HT_SendString("\n");

	g_hash_table_foreach(Values,(GHFunc)SendValueDelim,NULL);
	g_hash_table_foreach(Files,(GHFunc)SendFile,htc);
	HT_SendString("\n");

	sesid = g_hash_table_lookup(Values,"_sesid");
    file = g_hash_table_lookup(Values,"_file");
    filename = g_hash_table_lookup(Values,"_filename");
    contenttype = g_hash_table_lookup(Values,"_contenttype");
	Values = NewNameHash();
	g_hash_table_insert(Values,"_sesid",sesid);
    g_hash_table_insert(Values,"_file",file);
    g_hash_table_insert(Values,"_filename",filename);
    g_hash_table_insert(Values,"_contenttype",contenttype);
	Files = NewNameHash();
}

static	LargeByteString	*
Expired(void)
{
	LargeByteString	*html;
	HTCInfo	*htc;

	if		(  ( htc = HTCParser("expired") )  !=  NULL  ) {
		html = ExecCode(htc);
	} else {
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
	}
	return	(html);
}

static	void
Session(void)
{
	char	*user
	,		*sesid
	,		*button
	,		*name
	,		*file;
	char	buff[SIZE_BUFF];
	HTCInfo	*htc;
	LargeByteString	*html;
	Bool	fError;

ENTER_FUNC;
  retry:
	fError = FALSE;
	if		(  ( fpServ = OpenPort(ServerPort,PORT_HTSERV) )  !=  NULL  ) {
		if		(  ( sesid = g_hash_table_lookup(Values,"_sesid") )  ==  NULL  ) {
			if		(  ( user = getenv("REMOTE_USER") )  ==  NULL  ) {
				if		(  ( user = getenv("USER") )  ==  NULL  ) {
					user = "anonymous";
				}
			}
			sprintf(buff,"Start: %s\t%s\n",Command,user);
			HT_SendString(buff);
			HT_RecvString(SIZE_BUFF,buff);
			sesid = (char *)xmalloc(SIZE_SESID+1);
			strncpy(sesid,buff,SIZE_SESID);
			g_hash_table_insert(Values,"_sesid",sesid);
		} else {
			sprintf(buff,"Session: %s\n",sesid);
			HT_SendString(buff);
			if		(  HT_RecvString(SIZE_BUFF,buff)  ) {
				if		(  *buff  !=  0  ) {
					strncpy(sesid,buff,SIZE_SESID);
					SendEvent();
				} else {
					fError = TRUE;
				}
			} else {
				CloseNet(fpServ);
				Values = NewNameHash();
				Files = NewNameHash();
				goto	retry;
			}
		}
		if		(  !fError  ) {
			HT_RecvString(SIZE_BUFF,buff);
			if		(  strncmp(buff,"Window: ",8)  ==  0  ) {
				name = StrDup(buff+8);
				HT_RecvString(SIZE_BUFF,buff);	/*	\n	*/
				g_hash_table_insert(Values,"_name",name);
                if ((file = g_hash_table_lookup(Values, "_file")) != NULL) {
                    ValueStruct *value = HT_GetValue(file, TRUE);

                    if (value != NULL && !IS_VALUE_NIL(value)) {
                        PutFile(value);
                        HT_SendString("\n");
                        return;
                    }
                }
                htc = HTCParser(name);
                if (htc == NULL)
                    exit(1);
                html = ExecCode(htc);
				HT_SendString("\n");
			} else {
				html = Expired();
			}
		} else {
			html = Expired();
		}
		PutHTML(html);
	} else {
		fprintf(stderr,"htserver down??\n");
	}
LEAVE_FUNC;
}

static	void
DumpV(void)
{
	char	buff[SIZE_BUFF];
	void
	DumpVs(
			char		*name,
			char		*value,
			gpointer	user_data)
	{
		dbgprintf("[%s][%s]\n",name,value);
	}

ENTER_FUNC;
	g_hash_table_foreach(Values,(GHFunc)DumpVs,NULL);
LEAVE_FUNC;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	SetDefault();
	(void)GetOption(option,argc,argv);
	InitMessage("mon","@localhost");

	InitNET();
	HTCParserInit(getenv("SCRIPT_NAME"));
	Values = NewNameHash();
    Files = NewNameHash();
    lbs = NewLBS();
	GetArgs();
DumpV();
	Session();
	Dump();
	return	(0);
}
