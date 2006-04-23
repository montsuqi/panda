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
#include	"comm.h"
#include	"comms.h"
#include	"glterm.h"
#include	"HTCparser.h"
#include	"eruby.h"
#include	"cgi.h"
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
static	Bool		fComm;

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
	{	"debug",	BOOLEAN,	TRUE,	(void*)&fDebug,
		"デバッグモード"		 						},
	{	"cookie",	BOOLEAN,	TRUE,	(void*)&fCookie,
		"セション変数をcookieで行う"					},
	{	"jslink",	BOOLEAN,	TRUE,	(void*)&fJavaScriptLink,
		"<htc:hyperlink>によるリンクをJavaScriptで行う"	},
	{	"js",		BOOLEAN,	TRUE,	(void*)&fJavaScript,
		"JavaScriptを使ったHTML生成を行う"	},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	ServerPort = "localhost:8010";
	ScreenDir = getcwd(NULL,0);
	SesDir = NULL;
	Command = "demo";
	fDump = FALSE;
	fGet = FALSE;
	fComm = FALSE;
	fCookie = FALSE;
	fJavaScript = TRUE;
	fDebug = FALSE;
	CommandLine = NULL;
}

static	ValueStruct	*
GetValue(char *name, Bool fClear)
{
    ValueStruct *value;

ENTER_FUNC;
	if		(  fComm  ) {
		SendPacketClass(fpServ,GL_GetData);
		SendString(fpServ,name);
		SendBool(fpServ,fClear);
		RecvLBS(fpServ, lbs);
		if (LBS_Size(lbs) == 0) {
			value = NULL;
		} else {
			value = NativeRestoreValue(LBS_Body(lbs),TRUE);
		}
	} else {
		value = NULL;
	}
LEAVE_FUNC;
	return value;
}

static	void
SendValue(
	char		*name,
	CGIValue	*value)
{
    size_t len;

ENTER_FUNC;
	dbgprintf("send value = [%s:",name);
	if		(	(  *name  !=  0  )
			&&	(  value->body  !=  NULL  ) ) {
		SendPacketClass(fpServ,GL_ScreenData);
		SendString(fpServ,name);
		len = strlen(value->body);
		SendLength(fpServ, len);
		Send(fpServ, value->body, len);
		dbgprintf("%s]\n",value->body);
	} else {
		dbgmsg("]");
	}
LEAVE_FUNC;
}

static size_t
EncodeRFC2231(char *q, byte *p)
{
	char	*qq;

	qq = q;
	while (*p != 0) {
        if (*p <= 0x20 || *p >= 0x7f) {
            *q++ = '%';
            q += sprintf(q, "%02X", ((int) *p) & 0xFF);
        }
        else {
            switch (*p) {
            case '*': case '\'': case '%':
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
                *q++ = '%';
                q += sprintf(q, "%02X", ((int) *p) & 0xFF);
                break;
            default:
                *q ++ = *p;
                break;
            }
        }
		p++;
	}
	*q = 0;
	return q - qq;
}

static size_t
EncodeLengthRFC2231(byte *p)
{
	size_t ret;

    ret = 0;
	while (*p != 0) {
        if (*p <= 0x20 || *p >= 0x7f) {
            ret += 3;
        }
        else {
            switch (*p) {
              case '*': case '\'': case '%':
              case '(': case ')': case '<': case '>': case '@':
              case ',': case ';': case ':': case '\\': case '"':
              case '/': case '[': case ']': case '?': case '=':
                ret += 3;
                break;
              default:
                ret++;
                break;
            }
        }
		p++;
	}
	return ret;
}

static	char	*
ConvShiftJIS(
	char	*istr)
{
	iconv_t	cd;
	size_t	sib
	,		sob;
	char	*ostr;
	static	char	cbuff[SIZE_BUFF];

	cd = iconv_open("sjis","utf8");
	sib = strlen(istr);
	ostr = cbuff;
	sob = SIZE_BUFF;
	iconv(cd,&istr,&sib,&ostr,&sob);
	*ostr = 0;
	iconv_close(cd);
	return	(cbuff);
}

extern	void
PutFile(ValueStruct *file)
{
    char *ctype_field;
    char *filename_field;

    if ((ctype_field = LoadValue("_contenttype")) != NULL) {
        char *ctype = GetHostValue(ctype_field, FALSE);
        if (*ctype != '\0')
            printf("Content-Type: %s\r\n", ctype);
    }
    else {
        printf("Content-Type: application/octet-stream\r\n");
    }
    if ((filename_field = LoadValue("_filename")) != NULL) {
        char *filename = GetHostValue(filename_field, FALSE);
        char *disposition_field = LoadValue("_disposition");
        char *disposition = "attachment";

        if (disposition_field != NULL) {
            char *tmp = GetHostValue(disposition_field, FALSE);
            if (*tmp != '\0')
                disposition = tmp;
        }
        if (*filename != '\0') {
			if		(  ( AgentType & AGENT_TYPE_MSIE )  ==  AGENT_TYPE_MSIE  ) {
                printf("Content-Disposition: %s;"
                       " filename=%s\r\n",
                       disposition,
                       ConvShiftJIS(filename));
            }
            else {
                int len = EncodeLengthRFC2231((byte *)filename);
                char *encoded = (char *) xmalloc(len + 1);
                EncodeRFC2231(encoded, (byte *)filename);
                printf("Content-Disposition: %s;"
                       " filename*=utf-8''%s\r\n",
                       disposition,
                       encoded);
                xfree(encoded);
            }
        }
    }
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
SendFile(char *name, MultipartFile *file, HTCInfo *htc)
{
    char *filename;
	CGIValue	cgivalue;

    filename = (char *) g_hash_table_lookup(htc->FileSelection, name);
    if (filename != NULL) {
        SendString(fpServ,name);
        SendLength(fpServ, file->length);
        Send(fpServ, file->value, file->length);
        dbgprintf("send value = [%s]\n", name);

		cgivalue.body = file->filename;
        SendValue(filename, &cgivalue);
    }
}

static	void
SendEvent(void)
{
	char	*input;
	HTCInfo	*htc;
	char	*name
		,	*window
		,	*widget
		,	*event
		,	*p;
	char	buff[SIZE_LONGNAME+1];

ENTER_FUNC;
    if		(  ( window = LoadValue("_name") )  !=  NULL  ) {
		fComm = FALSE;
		if		(  ( htc = ParseScreen(window,fComm,FALSE) )  ==  NULL  ) {
			exit(1);
		}
	} else {
		exit(1);
	}
	fComm = TRUE;
	input = StrDup(ParseInput(htc));
	if		(  *input  ==  0  ) {
		event = "";
		widget = "";
	} else
	if		(  ( p = strchr(input,':') )  !=  NULL  ) {
		*p = 0;
		event = input;
		widget = p + 1;
	} else {
		event = input;
		widget = "";
	}
	if		(  *event  ==  '.'  ) {
		strcpy(buff,event+1);
		if		(  ( p = strrchr(buff,'.') )  !=  NULL  ) {
			*p = 0;
			name = buff;
			event = p + 1;
		} else {
			name = window;
		}
	} else
	if		(  *event  !=  0  ) {
		sprintf(buff,"%s.%s",window,event);
		if		(  ( p = strrchr(buff,'.') )  !=  NULL  ) {
			*p = 0;
			name = buff;
			event = p + 1;
		} else {
			name = window;
		}
	}

	SendPacketClass(fpServ,GL_Event);
	SendString(fpServ,name);
	SendString(fpServ,widget);
	SendString(fpServ,event);

	g_hash_table_foreach(Values,(GHFunc)SendValue,NULL);
	g_hash_table_foreach(Files,(GHFunc)SendFile,htc);
	SendPacketClass(fpServ,GL_END);

	SetSave("_sesid",TRUE);
    SetSave("_file",TRUE);
    SetSave("_filename",TRUE);
    SetSave("_contenttype",TRUE);
    SetSave("_disposition",TRUE);
	//CGI_InitValues();
	ClearValues();
	Files = NewNameHash();
LEAVE_FUNC;
}

static	void
ParseUserAgent(void)
{
	char	*agent
		,	*p;

#define	SKIP_SPACE	while	(	(  *agent  !=  0  )		\
							&&	(  isspace(*agent)  ) )	agent ++

	AgentType = AGENT_TYPE_NULL | AGENT_TYPE_JS;

	if		(  ( agent = getenv("HTTP_USER_AGENT") )  !=  NULL  ) {
		if		(  strlicmp(agent,"w3m")  ==  0  ) {
			AgentType &= ~AGENT_TYPE_JS;
			AgentType |= AGENT_TYPE_TEXT;
		} else
		if		(  strlcmp(agent,"Mozilla/")  !=  0  ) {
			agent += 8;
			if		(  strstr(agent,"compatible;")  !=  NULL  ) {
				agent += 11;
				SKIP_SPACE;
				if		(  strlcmp(agent,"MSIE")  !=  0  ) {
					AgentType |= AGENT_TYPE_MSIE;
					agent += 4;
					SKIP_SPACE;
					if		(	(  *agent  ==  '5'  )
							||	(  *agent  ==  '4' ) ) {
						AgentType |= AGENT_TYPE_MSIE_OLD;
					}
				}
			} else {
				if		(  strstr(agent,"Gecko")  !=  NULL  ) {
					if		(  strstr(agent,"Netscape6")  !=  NULL  ) {
						AgentType |= AGENT_TYPE_NS6;
					} else
					if		(  strstr(agent,"Firefox")  !=  NULL  ) {
						AgentType |= AGENT_TYPE_FF;
					} else {
						AgentType |= AGENT_TYPE_MOZILLA;
					}
				} else {
					AgentType |= AGENT_TYPE_NN;
				}
			}
			if		(  ( p = strstr(agent,"Opera") )  !=  NULL  ) {
				AgentType |= AGENT_TYPE_OPERA;
			}
		} else
		if		(  strlcmp(agent,"Opera/")  ==  0  ) {	/*	pure Opera	*/
			AgentType |= AGENT_TYPE_OPERA;
		}
	}

	if		(  ( AgentType & AGENT_TYPE_JS )  ==  0  ) {
		fJavaScript = FALSE;
		fJavaScriptLink = FALSE;
	}

#ifdef	DEBUG
	SaveValue("_js",(fJavaScript ? "true" : "false"),FALSE);
	SaveValue("_agent",agent,FALSE);
#endif
}

static	LargeByteString	*
Expired(
	int		code)
{
	LargeByteString	*html;
	HTCInfo	*htc;
	char	buff[SIZE_BUFF];

ENTER_FUNC;
	Codeset = SRC_CODESET;
	html = NewLBS();
	LBS_EmitStart(html);
    if      (  ( htc = ParseScreen("expired",FALSE,FALSE) )  ==  NULL  ) {
        LBS_EmitUTF8(html,
                     "<html><head>"
                     "<title>htserver error</title>"
                     "</head><body>\n"
                     "<H1>htserver error</H1>\n"
                     "<p>maybe session was expired. please retry.</p>\n"
					 ,NULL);
		sprintf(buff,"<p>end code = %d</p>",code);
		LBS_EmitUTF8(html,buff,NULL);
		LBS_EmitUTF8(html,"</body></html>\n",NULL);
    } else {
        ExecCode(html,htc);
    }
LEAVE_FUNC;
	return	(html);
}

static	void
Session(void)
{
	char	*user
	,		*sesid
	,		*name
	,		*file;
	char	buff[SIZE_BUFF];
	HTCInfo	*htc;
	LargeByteString	*html;
	Bool	fError;
	PacketClass	klass;

ENTER_FUNC;
  retry:
	fError = FALSE;
	if		(  ( fpServ = OpenPort(ServerPort,PORT_HTSERV) )  !=  NULL  ) {
		ParseUserAgent();
		if		(  ( sesid = LoadValue("_sesid") )  ==  NULL  ) {
			if		(  ( user = getenv("REMOTE_USER") )  ==  NULL  ) {
				if		(  ( user = getenv("USER") )  ==  NULL  ) {
					user = "anonymous";
				}
			}
			SendPacketClass(fpServ,GL_Connect);
			SendString(fpServ,Command);
			SendPacketClass(fpServ,GL_Name);
			SendString(fpServ,user);
			RecvPacketClass(fpServ);	/*	session	*/
			RecvString(fpServ,buff);
			sesid = StrDup(buff);
			SaveValue("_sesid",sesid,FALSE);
		} else {
			SendPacketClass(fpServ,GL_Session);
			SendString(fpServ,sesid);
			if		(  ( klass = RecvPacketClass(fpServ) )  ==  GL_Session  ) {
				RecvString(fpServ,buff);
				dbgprintf("ses = [%s]",buff);
				if		(  *buff  !=  0  ) {
					strncpy(sesid,buff,SIZE_SESID);
					SendEvent();
				} else {
					fError = TRUE;
				}
			} else {
				CloseNet(fpServ);
				CGI_InitValues();
				goto	retry;
			}
		}
		dbgmsg("*");
		if		(  !fError  ) {
			name = NULL;
			while	(  ( klass = RecvPacketClass(fpServ) )  ==  GL_WindowName  ) {
				RecvString(fpServ,buff);
				name = StrDup(buff);
				dbgprintf("name = [%s]",name);
				SaveValue("_name",name,FALSE);
			}
			if		(  name  ==  NULL  ) {
				html = Expired(1);
			} else {
				if ((file = LoadValue("_file")) != NULL) {
					ValueStruct *value = GetValue(file, TRUE);
					if (value != NULL && !IS_VALUE_NIL(value)) {
						PutFile(value);
						return;
					}
				}
				if		(  ( htc = ParseScreen(name,TRUE,FALSE) )  ==  NULL  ) {
					exit(1);
				}
				if		(  htc->fHTML  ) {
					html = htc->code;
				} else {
					html = NewLBS();
					LBS_EmitStart(html);
					ExecCode(html,htc);
				}
				SendPacketClass(fpServ,GL_END);
			}
		} else {
			html = Expired(2);
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
	//InitMessage("mon","@localhost");
	InitMessage("mon",NULL);

	InitNET();
    lbs = NewLBS();
	InitCGI();
	InitHTC(getenv("SCRIPT_NAME"),GetValue);
DumpV();
	Session();
	Dump();
	return	(0);
}
