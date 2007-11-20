/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"multipart.h"
#include	"file.h"
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
		"htserver port name"							},

	{	"screen",	STRING,		TRUE,	(void*)&ScreenDir,
		"template directory"	 						},
	{	"font",		STRING,		TRUE,	(void*)&FontTemplate,
		"alternate font image converting template"		},

	{	"command",	STRING,		TRUE,	(void*)&Command,
		"command name"	 								},

	{	"get",		BOOLEAN,	TRUE,	(void*)&fGet,
		"action by GET"			 						},
	{	"dump",		BOOLEAN,	TRUE,	(void*)&fDump,
		"dump variables"		 						},
	{	"debug",	BOOLEAN,	TRUE,	(void*)&fDebug,
		"debug mode"			 						},
	{	"cookie",	BOOLEAN,	TRUE,	(void*)&fCookie,
		"session keeps by cookie"						},
	{	"jslink",	BOOLEAN,	TRUE,	(void*)&fJavaScriptLink,
		"<htc:hyperlink> links by JavaScript"			},
	{	"js",		BOOLEAN,	TRUE,	(void*)&fJavaScript,
		"use JavaScript"								},

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
	FontTemplate = "font/%02X%02X.png";
	fNoHeader = FALSE;
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
			if		(  ValueType(value)  ==  GL_TYPE_OBJECT  ) {
				RecvLBS(fpServ,lbs);
				value = NewValue(GL_TYPE_BINARY);
				ValueByteLength(value) = LBS_Size(lbs);
				ValueByteSize(value) = LBS_Size(lbs);
				ValueByte(value) = LBS_ToByte(lbs);
				ValueIsNonNil(value);
			}
		}
	} else {
		value = NULL;
	}
LEAVE_FUNC;
	return value;
}

extern	void
SendValue(
	char		*name,
	char		*value,
	size_t		size)
{
ENTER_FUNC;
	if		(	(  name   !=  NULL  )
			&&	(  value  !=  NULL  ) ) {
		dbgprintf("send value = [%s:%s]",name,value);
		SendPacketClass(fpServ,GL_ScreenData);
		SendString(fpServ,name);
		SendLength(fpServ,size);
		Send(fpServ, value, size);
		dbgprintf(":%s]\n",value);
	} else {
		dbgprintf("send value = [%s]",name);
	}
LEAVE_FUNC;
}

static	void
_SendValue(
	char		*name,
	CGIValue	*value)
{
ENTER_FUNC;
	if		(  value->body  !=  NULL  ) {
		SendValue(value->name,value->body,strlen(value->body));
	}
LEAVE_FUNC;
}

static	void
SendValues(
	HTCInfo	*htc)
{
	g_hash_table_foreach(Values,(GHFunc)_SendValue,NULL);
	ON_IO_ERROR(fpServ,badio);
	g_hash_table_foreach(Files,(GHFunc)SendFile,htc);
	ON_IO_ERROR(fpServ,badio);
	SendPacketClass(fpServ,GL_END);
  badio:;
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
    if		(  ( window = LoadValue("_name") )  ==  NULL  ) {
		exit(1);
	}
	SendPacketClass(fpServ,GL_GetData);
	if		(  ( htc = ParseScreen(window,fComm,FALSE) )  ==  NULL  ) {
		exit(1);
	}
	SendPacketClass(fpServ,GL_END);
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
	} else {
		name = "";
	}

	SendPacketClass(fpServ,GL_Event);		ON_IO_ERROR(fpServ,badio);
	SendString(fpServ,name);				ON_IO_ERROR(fpServ,badio);
	SendString(fpServ,widget);				ON_IO_ERROR(fpServ,badio);
	SendString(fpServ,event);				ON_IO_ERROR(fpServ,badio);

	SendValues(htc);

	SetSave("_sesid",TRUE);
    SetSave("_file",TRUE);
    SetSave("_filename",TRUE);
    SetSave("_contenttype",TRUE);
    SetSave("_disposition",TRUE);
  badio:
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
			if		(  ( p = strstr(agent,"compatible;") )  !=  NULL  ) {
				agent = p + 11;
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
InfomationPage(
	char	*name,
	char	*msg)
{
	LargeByteString	*html;
	HTCInfo	*htc;

ENTER_FUNC;
	Codeset = SRC_CODESET;
	html = NewLBS();
	LBS_EmitStart(html);
    if      (  ( htc = ParseScreen(name,FALSE,FALSE) )  ==  NULL  ) {
        LBS_EmitUTF8(html,
                     "<html><head>"
                     "<title>Infomation</title>"
                     "</head><body>\n",NULL);
		if		(  msg  !=  NULL  ) {
			LBS_EmitUTF8(html,msg,NULL);
		}
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
		,	*sesid
		,	*name
		,	*file;
	char	buff[SIZE_LARGE_BUFF];
	HTCInfo	*htc;
	LargeByteString	*html
		,			*header;
	Bool	fError
		,	fInit;
	PacketClass	klass;

ENTER_FUNC;
	fError = FALSE;
  retry:
	if		(  ( fpServ = OpenPort(ServerPort,PORT_HTSERV) )  !=  NULL  ) {
		ParseUserAgent();
		fComm = TRUE;
		if		(  ( sesid = LoadValue("_sesid") )  ==  NULL  ) {
			if		(  ( user = getenv("REMOTE_USER") )  ==  NULL  ) {
				if		(  ( user = getenv("USER") )  ==  NULL  ) {
					user = "anonymous";
				}
			}
			SendPacketClass(fpServ,GL_Connect);			ON_IO_ERROR(fpServ,busy);
			SendString(fpServ,Command);					ON_IO_ERROR(fpServ,busy);
			SendString(fpServ,user);					ON_IO_ERROR(fpServ,busy);
			SendValues(NULL);
			RecvPacketClass(fpServ);	/*	session	*/	ON_IO_ERROR(fpServ,busy);
			RecvString(fpServ,buff);					ON_IO_ERROR(fpServ,busy);
			sesid = StrDup(buff);
			SaveValue("_sesid",sesid,FALSE);
			fInit = TRUE;
		} else {
			fInit = FALSE;
			SendPacketClass(fpServ,GL_Session);			ON_IO_ERROR(fpServ,busy);
			SendString(fpServ,sesid);					ON_IO_ERROR(fpServ,busy);
			if		(  ( klass = RecvPacketClass(fpServ) )  ==  GL_Session  ) {
				ON_IO_ERROR(fpServ,busy);
				RecvString(fpServ,buff);					ON_IO_ERROR(fpServ,busy);
				dbgprintf("ses = [%s]",buff);
				if		(  *buff  !=  0  ) {
					strncpy(sesid,buff,SIZE_SESID);
					SendEvent();								ON_IO_ERROR(fpServ,busy);
				} else {
					fError = TRUE;
				}
			} else {
				CloseNet(fpServ);
				CGI_InitValues();
				goto	retry;
			}
		}
		if		(  !fError  ) {
			name = NULL;
			while	(  ( klass = RecvPacketClass(fpServ) )  ==  GL_WindowName  ) {
				ON_IO_ERROR(fpServ,busy);
				RecvString(fpServ,buff);					ON_IO_ERROR(fpServ,busy);
				name = StrDup(buff);
				dbgprintf("name = [%s]",name);
				SaveValue("_name",name,FALSE);
			}
			dbgmsg("*");
			if		(  klass  ==  GL_RedirectName  ) {
				CGI_InitValues();
				RecvString(fpServ,buff);					ON_IO_ERROR(fpServ,busy);
				if		(  *buff  ==  0  ) {
					html = InfomationPage("exited",
										  "<H1>session exited</H1>\n"
										  "<p>left MONTSUQI session.</p>");
					PutHTML(NULL,NULL,html,500);
				} else {
					header = NewLBS();
					LBS_EmitStart(header);
					name = StrDup(buff);
					dbgprintf("name = [%s]",name);
					sprintf(buff,"Location: %s\r\n",name);
					LBS_EmitString(header,buff);
					PutHTML(header,NULL,NULL,200);
				}
			} else {
				if		(  name  ==  NULL  ) {
					html = InfomationPage("expired",
										  "<H1>htserver error</H1>\n"
										  "<p>null screen name.</p>");
					PutHTML(NULL,NULL,html,500);
				} else {
					if		(  ( file = LoadValue("_file") )  !=  NULL  )	{
						ValueStruct *value;
						value = GetValue(file, TRUE);
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
					dbgmsg("*");
					PutHTML(NULL,htc->Cookie,html,200);
				}
			}
		} else {
			html = InfomationPage("expired",
								  "<H1>htserver error</H1>\n"
								  "<p>session expired.</p>\n");
			PutHTML(NULL,NULL,html,500);
		}
		SendPacketClass(fpServ,GL_END);
		CloseNet(fpServ);
	} else {
	  busy:
		html = InfomationPage("busy",
							  "<H1>session open error</H1>\n"
							  "<p>can't start application session.(busy)</p>\n");
		PutHTML(NULL,NULL,html,500);
	}
LEAVE_FUNC;
}

static	void
DumpVs(
	char		*name,
	char		*value,
	gpointer	user_data)
{
	dbgprintf("[%s][%s]\n",name,value);
}

static	void
DumpV(void)
{

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
	(void)GetOption(option,argc,argv,NULL);
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
