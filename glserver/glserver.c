/*
PANDA -- a simple transaction monitor
Copyright (C) 1998-1999 Ogochan.
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*
#define	DEBUG
#define	TRACE
*/

#define	MAIN
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>
#include	<signal.h>

#include	"types.h"
#include	"const.h"
#include	"glserver.h"
#include	"dirs.h"
#include	"RecParser.h"
#include	"front.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char		*AuthURL;
static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},
	{	"screen",	STRING,		TRUE,	(void*)&ScreenDir,
		"画面格納ディレクトリ"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"データ定義格納ディレクトリ"	 				},
	{	"cache",	STRING,		TRUE,	(void*)&CacheDir,
		"BLOBキャッシュディレクトリ名"					},
	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		"認証サーバ"			 						},
#ifdef	USE_SSL
	{	"ssl",		BOOLEAN,	TRUE,	(void*)&fSsl,
		"SSLを使う"				 						},
	{	"key",		STRING,		TRUE,	(void*)&KeyFile,
		"鍵ファイル名(pem)"		 						},
	{	"cert",		STRING,		TRUE,	(void*)&CertFile,
		"証明書ファイル名(pem)"	 						},
	{	"CApath",	STRING,		TRUE,	(void*)&CA_Path,
		"CA証明書へのパス"								},
	{	"CAfile",	STRING,		TRUE,	(void*)&CA_File,
		"CA証明書ファイル"								},
	{	"ciphers",	STRING,		TRUE,	(void*)&Ciphers,
		"SSLで使用する暗号スイート"						},
#endif

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = PORT_GLTERM;
	Back = 5;
	ScreenDir = ".";
	RecordDir = ".";
	AuthURL = "glauth://localhost:8001";	/*	PORT_GLAUTH	*/
	CacheDir = "cache";
#ifdef	USE_SSL
	fSsl = FALSE;
	KeyFile = NULL;
	CertFile = NULL;
	CA_Path = NULL;
	CA_File = NULL;
	Ciphers = "ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH";
#endif	
}

static	void
StopProcess(
	int		ec)
{
dbgmsg(">StopProcess");
dbgmsg("<StopProcess");
	exit(ec);
}
extern	int
main(
	int		argc,
	char	**argv)
{
	(void)signal(SIGPIPE,(void *)StopProcess);
	SetDefault();
	(void)GetOption(option,argc,argv);
	InitMessage("glserver",NULL);

	ParseURL(&Auth,AuthURL,"file");
	InitSystem(argc,argv);
	Message("glserver start");
	ExecuteServer();
	Message("glserver end");
	return	(0);
}
