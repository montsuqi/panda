/*
PANDA -- a simple transaction monitor
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

#define	MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <sys/socket.h>
#include	<unistd.h>
#ifdef	USE_SSL
#include	<openssl/crypto.h>
#include	<openssl/x509.h>
#include	<openssl/pem.h>
#include	<openssl/ssl.h>
#include	<openssl/err.h>
#endif

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"comms.h"
#include	"socket.h"
#include	"fdd.h"

#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*Command;
static	char	*Host;
static	char	*PortNumber;

static	void
InitSystem(void)
{
dbgmsg(">InitSystem");
	InitNET();
dbgmsg("<InitSystem");
}

extern	int
ExecuteClient(
	char	*localname,
	char	*remotename)
{
	struct	stat	stbuf;
	char	buff[SIZE_BUFF];
	int		fd;
	NETFILE	*net;
	FILE	*fp;
	int		rc
		,	ac;
	size_t	size
	,		left;

#ifdef	USE_SSL
	SSL_CTX	*ctx;
#endif
	if		(  stat(localname,&stbuf)  ==  0  ) {
		if		(  ( fd = ConnectIP_Socket(PortNumber,SOCK_STREAM,Host) )  <  0  ) {
			rc = ERROR_FDD_NOT_READY;
		} else {
#ifdef	USE_SSL
			if		(  fSsl  ) {
				if		(  ( ctx = MakeSSL_CTX(KeyFile,CertFile,CA_File,CA_Path,Ciphers) )
						   ==  NULL  ) {
					exit(1);
				}
				net = MakeSSL_Net(ctx,fd);
				if (StartSSLClientSession(net, remotename) != TRUE){
                    CloseNet(net);
                    SSL_CTX_free(ctx);
                    return ERROR_FILE_NOT_FOUND;
                }
			} else {
				net = SocketToNet(fd);
			}
#else
			net = SocketToNet(fd);
#endif
			fp = fopen(localname,"r");

			sprintf(buff,"Command: %s\n",Command);
			SendStringDelim(net,buff);
			sprintf(buff,"Filename: %s\n",(( remotename != NULL ) ? remotename : localname));
			SendStringDelim(net,buff);
			sprintf(buff,"Size: %d\n",(int)stbuf.st_size);
			SendStringDelim(net,buff);

			SendStringDelim(net,"\n");
			left = stbuf.st_size;
			rc = ERROR_INCOMPLETE;
			do {
				if		(  left  >  SIZE_BUFF  ) {
					size = SIZE_BUFF;
				} else {
					size = left;
				}
				size = fread(buff,1,size,fp);
				if		(  size  >  0  ) {
					Send(net,buff,size);			ON_IO_ERROR(net,badio);
					if		(  ( ac = RecvChar(net) )  !=  0  ) {
						rc = ac;
						goto	badio;
					}
					left -= size;
				}
			}	while	(  left  >  0  );
			rc = RecvChar(net);
		  badio:
			fclose(fp);
			CloseNet(net);
		}
	} else {
		rc = ERROR_FILE_NOT_FOUND;
	}
	return	(rc);
}

static	ARG_TABLE	option[] = {
	{	"command",	STRING,		TRUE,	(void*)&Command,
		"コマンド名"	 								},
	{	"host",		STRING,		TRUE,	(void*)&Host,
		"ホスト名"	 									},
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
#ifdef	USE_SSL
	{	"key",		STRING,		TRUE,	(void*)&KeyFile,
		"鍵ファイル名(pem)"		 						},
	{	"cert",		STRING,		TRUE,	(void*)&CertFile,
		"証明書ファイル名(pem)"	 						},
	{	"ssl",		BOOLEAN,	TRUE,	(void*)&fSsl,
		"SSLを使う"				 						},
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
	Command = "default";
	Host = "localhost";
	PortNumber = PORT_FDD;
#ifdef  USE_SSL
    fSsl = FALSE;
    KeyFile = NULL;
    CertFile = NULL;
    CA_Path = NULL;
    CA_File = NULL;
    Ciphers = "ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH";
#endif
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*localname
		,		*remotename;
	int			rc;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("fdw",NULL);

	if		(	(  fl  !=  NULL  )
			&&	(  fl->name  !=  NULL  ) ) {
		localname = fl->name;
		fl = fl->next;
		if		(  fl  !=  NULL  ) {
			remotename = fl->name;
		} else {
			remotename = NULL;
		}
	} else {
		localname = NULL;
		remotename = NULL;
	}
	InitSystem();
	rc = ExecuteClient(localname,remotename);
	return	(rc);
}
