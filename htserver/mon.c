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
*/
#define	DEBUG
#define	TRACE

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
	{	"jslink",	BOOLEAN,	TRUE,	(void*)&fJavaScriptLink,
		"<htc:hyperlink>によるリンクをJavaScriptで行う"	},
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
	fCookie = FALSE;
}

static	void
HT_SendString(
	char	*str)
{
	//	dbgprintf("send [%s]\n",str);
	SendStringDelim(fpServ,str);
}

static	Bool
HT_RecvString(
	size_t	size,
	char	*str)
{
	Bool	rc;

	rc = RecvStringDelim(fpServ,size,str);
	dbgprintf("recv [%s]\n",str);
	return	(rc);
}

static	ValueStruct	*
HT_GetValue(char *name, Bool fClear)
{
	char	buff[SIZE_BUFF+1];
	PacketDataType	type;
    ValueStruct *value;

ENTER_FUNC;
    LBS_EmitStart(lbs);
    sprintf(buff,"%s%s\n",name,(fClear ? " clear" : "" ));
    HT_SendString(buff);
    RecvLBS(fpServ, lbs);
    LBS_EmitEnd(lbs);
    if (LBS_Size(lbs) == 0) {
        value = NULL;
	} else {
		type = *(PacketDataType *) LBS_Body(lbs);
		value = NewValue(type);
		NativeUnPackValue(NULL, LBS_Body(lbs), value);
	}
LEAVE_FUNC;
	return value;
}

static	void
SendValueDelim(
	char		*name,
	CGIValue	*value)
{
    char	*s;
    size_t len;

ENTER_FUNC;
	dbgprintf("send value = [%s:\n",name);
	if		(	(  *name  !=  0  )
			&&	(  value->body  !=  NULL  ) ) {
		HT_SendString(name);
		HT_SendString("\n");
		s = ConvUTF8(value->body,Codeset);
		len = strlen(s);
		SendLength(fpServ, len);
		Send(fpServ, s, len);
		dbgprintf(" %s]\n",value->body);
	} else {
		dbgmsg(" ]");
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
    char *user_agent;
    char *p;

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
            char *user_agent = getenv("HTTP_USER_AGENT");
            if (user_agent && strstr(user_agent, "MSIE") != NULL) {
                printf("Content-Disposition: %s;"
                       " filename=%s\r\n",
                       disposition,
                       ConvShiftJIS(filename));
            }
            else {
                int len = EncodeLengthRFC2231(filename);
                char *encoded = (char *) xmalloc(len + 1);
                EncodeRFC2231(encoded, filename);
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
    char buff[SIZE_BUFF * 3 + 1];
    char *p, *pend;
    int len = 0;
    int x = 0;
	CGIValue	cgivalue;

    filename = (char *) g_hash_table_lookup(htc->FileSelection, name);
    if (filename != NULL) {
        HT_SendString(name);
        HT_SendString("\n");
        SendLength(fpServ, file->length);
        Send(fpServ, file->value, file->length);
        dbgprintf("send value = [%s]\n", name);

		cgivalue.body = file->filename;
        SendValueDelim(filename, &cgivalue);
    }
}

static	void
SendEvent(void)
{
	char	*event;
	HTCInfo	*htc;
	char	*name;
	char	htcname[SIZE_LONGNAME+1];

ENTER_FUNC;
    if		(  ( name = LoadValue("_name") )  !=  NULL  ) {
		sprintf(htcname,"%s/%s.htc",ScreenDir,name);
		if		(  ( htc = HTCParser(htcname) )  ==  NULL  ) {
			exit(1);
		}
	} else {
		exit(1);
	}
	event = ParseInput(htc);

	HT_SendString(event);
	HT_SendString("\n");

	g_hash_table_foreach(Values,(GHFunc)SendValueDelim,NULL);
	g_hash_table_foreach(Files,(GHFunc)SendFile,htc);
	HT_SendString("\n");

	SetSave("_sesid",TRUE);
    SetSave("_file",TRUE);
    SetSave("_filename",TRUE);
    SetSave("_contenttype",TRUE);
    SetSave("_disposition",TRUE);
	ClearValues();
	Files = NewNameHash();
LEAVE_FUNC;
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
		if		(  ( sesid = LoadValue("_sesid") )  ==  NULL  ) {
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
            sesid[SIZE_SESID] = '\0';
			SaveValue("_sesid",sesid,FALSE);
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
				SaveValue("_name",name,FALSE);
                if ((file = LoadValue("_file")) != NULL) {
                    ValueStruct *value = HT_GetValue(file, TRUE);

                    if (value != NULL && !IS_VALUE_NIL(value)) {
                        PutFile(value);
                        HT_SendString("\n");
                        return;
                    }
                }
				sprintf(buff,"%s/%s.htc",ScreenDir,name);
                htc = HTCParser(buff);
                if (htc == NULL)
                    exit(1);
				html = NewLBS();
				LBS_EmitStart(html);
                ExecCode(html,htc);
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
    lbs = NewLBS();
	InitCGI();
	InitHTC(getenv("SCRIPT_NAME"),HT_GetValue);
DumpV();
	Session();
	Dump();
	return	(0);
}
