/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2004 Ogochan & JMA (Japan Medical Association).

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
	char	*fn;

#ifdef	USE_SSL
	SSL_CTX	*ctx;
#endif
	if		(  stat(localname,&stbuf)  ==  0  ) {
		if		(  ( fd = ConnectIP_Socket(PortNumber,SOCK_STREAM,Host) )  <  0  ) {
			rc = ERROR_FDD_NOT_READY;
		} else {
#ifdef	USE_SSL
			if		(  fSsl  ) {
				if		(  ( ctx = MakeCTX(KeyFile,CertFile,CA_File,CA_Path,fVerify) )
						   ==  NULL  ) {
					exit(1);
				}
				net = MakeSSL_Net(ctx,fd);
				SSL_connect(NETFILE_SSL(net));
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
			sprintf(buff,"Size: %d\n",stbuf.st_size);
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
	{	"verifypeer",BOOLEAN,	TRUE,	(void*)&fVerify,
		"クライアント証明書の検証を行う"				},
	{	"CApath",	STRING,		TRUE,	(void*)&CA_Path,
		"CA証明書へのパス"								},
	{	"CAfile",	STRING,		TRUE,	(void*)&CA_File,
		"CA証明書ファイル"								},
#endif

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	Command = "default";
	Host = "localhost";
	PortNumber = PORT_FDD;
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
