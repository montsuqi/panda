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
#include	"debug.h"

#define	SRC_CODESET		"euc-jp"

#define	SIZE_CHARS		16

static	NETFILE	*fpServ;
static	char	*ServerPort;
static	char	*Command;

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

static	char	*
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
	char	*istr)
{
	iconv_t	cd;
	size_t	sib
	,		sob;
	char	*ostr;
	static	char	cbuff[SIZE_BUFF];

	cd = iconv_open("utf8",Codeset);
	sib = strlen(istr);
	ostr = cbuff;
	sob = SIZE_BUFF;
	if		(  iconv(cd,&istr,&sib,&ostr,&sob)  !=  0  ) {
		dbgprintf("error = %d\n",errno);
	}
	*ostr = 0;
	iconv_close(cd);
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

static	void
GetArgs(void)
{
	char	name[SIZE_BUFF];
	byte	value[SIZE_BUFF];

	if		(  ( ScanArgValue = getenv("QUERY_STRING") )  !=  NULL  ) {
		while	(  ScanEnv(name,value)  ) {
			if		(  g_hash_table_lookup(Values,name)  ==  NULL  ) {
				g_hash_table_insert(Values,StrDup(name),StrDup(value));
			}
		}
	}
	while	(  ScanPost(name,value)  ) {
		if		(  g_hash_table_lookup(Values,name)  ==  NULL  ) {
			g_hash_table_insert(Values,StrDup(name),StrDup(value));
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
	int		rc
	,		ch;
	iconv_t	cd;

ENTER_FUNC;
	cd = iconv_open("utf8",codeset);

	RewindLBS(lbs);
	if		(  codeset  !=  NULL  ) {
		cd = iconv_open(codeset,"utf8");
		while	(  !LBS_Eof(lbs)  ) {
			count = 0;
			ic = ibuff;
			do {
				ch = LBS_FetchChar(lbs);
				*ic ++ = ch;
				count ++;
				istr = ibuff;
				sib = count;
				oc = obuff;
				sob = SIZE_CHARS;
				rc = iconv(cd,&istr,&sib,&oc,&sob);
			}	while	(	(  rc  !=  0  )
						&&	(  ch  !=  0  ) );
			for	( oc = obuff ; sob < SIZE_CHARS ; oc ++ , sob ++ ) {
				fprintf(output,"%c",*oc);
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
	printf("Content-type: text/html\n");
	LBS_EmitEnd(html);
	if		(  fCookie  ) {
		if		(  ( sesid = g_hash_table_lookup(Values,"_sesid") )  !=  NULL  ) {
			printf("Set-Cookie: _sesid=%s;\n",sesid);
		}
	}
	printf("\n");
	WriteLargeString(stdout,html,Codeset);
dbgmsg("<PutHTML");
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

static	void
SendValueDelim(
	char		*name,
	byte		*value)
{
	char	buff[SIZE_BUFF];

	HT_SendString(name);
	HT_SendString(": ");
	EncodeStringURL(buff,ConvUTF8(value));
	HT_SendString(buff);
	HT_SendString("\n");
	dbgprintf("send value = [%s: %s]\n",name,buff);

}

static	void
SendEvent(void)
{
	char	*button;
	char	*event;
	char	*sesid;
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
	
	if		(  ( button = g_hash_table_lookup(Values,"_event") )  ==  NULL  ) {
		event = "";
		htc = NULL;
	} else {
		if		(  ( name = g_hash_table_lookup(Values,"_name") )  !=  NULL  ) {
			htc = HTCParser(name);
			if		(  ( event = g_hash_table_lookup(htc->Trans,ConvUTF8(button)) )
					   ==  NULL  ) {
				event = button;
			}
		} else {
			event = "";
			htc = NULL;
		}
	}
	if		(  htc  !=  NULL  ) {
		g_hash_table_foreach(htc->Radio,(GHFunc)GetRadio,NULL);
	}

	HT_SendString(event);
	HT_SendString("\n");

	g_hash_table_foreach(Values,(GHFunc)SendValueDelim,NULL);
	HT_SendString("\n");

	sesid = g_hash_table_lookup(Values,"_sesid");
	Values = NewNameHash();
	g_hash_table_insert(Values,"_sesid",sesid);
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
	,		*name;
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
				goto	retry;
			}
		}
		if		(  !fError  ) {
			HT_RecvString(SIZE_BUFF,buff);
			if		(  strncmp(buff,"Window: ",8)  ==  0  ) {
				name = StrDup(buff+8);
				HT_RecvString(SIZE_BUFF,buff);	/*	\n	*/
				g_hash_table_insert(Values,"_name",name);
				htc = HTCParser(name);
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
	HTCParserInit();
	Values = NewNameHash();
	GetArgs();
DumpV();
	Session();
	Dump();
	return	(0);
}
