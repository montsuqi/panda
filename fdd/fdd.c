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
#include    <sys/wait.h>
#include    <errno.h>
#include    <sys/socket.h>
#include	<sys/stat.h>
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

static	char	*PortNumber;
static	int		Back;
static	char	*WorkDir;
static	char	*ExecDir;

static	int 
fdd_system(
	char	*name, 
	char	*command, 
	char	*tempname, 
	char	*filename)
{
	int		pid;
	int		status;

	if		(  command  !=  NULL  ) {
		if		(  ( pid = fork() )  <  0  )	{
			status = -1;
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
			execl(name, command, tempname, filename, NULL);
			_exit(127); /* execl error */
		} else
		if		(  pid   >  0  )	{	/*	parent	*/
			while(waitpid(pid, &status, 0) <0){
				if (errno != EINTR){
					status =  -1;
					break;
				}
			}
		}
	} else {
		status = -1;
	}
	return (status);
}

static	void
Process(
	NETFILE	*fpComm)
{
	Bool	fOK;
	char	buff[SIZE_BUFF+1]
		,	name[SIZE_LONGNAME+1];
	char	*filename
		,	*tempname
		,	*command;
	size_t	size
		,	left;
	int		fd;
	int		ac;
	FILE	*fp;
	char	*p
		,	*q;
	struct	stat	stbuf;

	filename = NULL;
	command = NULL;
	size = 0;
	do {
		if		(  ( fOK = RecvStringDelim(fpComm,SIZE_BUFF,buff) )  ) {
			if		(  strlicmp(buff,"Command: ")  ==  0  ) {
				command = StrDup(strchr(buff,' ')+1);
			} else
			if		(  strlicmp(buff,"Filename: ")  ==  0  ) {
				filename = StrDup(strchr(buff,' ')+1);
			} else
			if		(  strlicmp(buff,"Size: ")  ==  0  ) {
				left = atoi(strchr(buff,' ')+1);
			}
		} else
			break;
	}	while	(  *buff  !=  0  );
	printf("size  = [%d]\n",left);
	sprintf(buff,"%s/XXXXXX",WorkDir);
	fd = mkstemp(buff);
	fp = fdopen(fd,"w");
	tempname = StrDup(buff);
	do {
		if		(  left  >  SIZE_BUFF  ) {
			size = SIZE_BUFF;
		} else {
			size = left;
		}
		size = Recv(fpComm,buff,size);		ON_IO_ERROR(fpComm,badio);
		if		(  size  >  0  ) {
			fwrite(buff,size,1,fp);
			ac = 0;
		} else {
			ac = ERROR_WRITE;
		}
		SendChar(fpComm,ac);
		left -= size;
	}	while	(  left  >  0  );
	fclose(fp);
#if	0
	printf("command = [%s]\n",command);
	printf("file  = [%s]\n",filename);
	printf("temp  = [%s]\n",tempname);
#endif
	if		(  ( q = strrchr(command,'/') )  !=  NULL  ) {
		command = q + 1;
	}
	strcpy(buff,ExecDir);
	p = buff;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
		}
		sprintf(name,"%s/%s",p,command);
		if		(  stat(name,&stbuf)  ==  0  )	break;
		p = q + 1;
	}	while	(  q  !=  NULL  );

	ac = fdd_system(name, command, tempname, filename);
	unlink(tempname);
	if ( ac == -1){
			SendChar(fpComm,127);
	} else {
			SendChar(fpComm,WEXITSTATUS(ac));
	}
  badio:;
}

extern	void
ExecuteServer(void)
{
	int		pid;
	int		fd
	,		_fd;
	NETFILE	*fpComm;
#ifdef	USE_SSL
	SSL_CTX	*ctx;
#endif

ENTER_FUNC;
	signal(SIGCHLD,SIG_IGN);

	_fd = InitServerPort(PortNumber,Back);
#ifdef	USE_SSL
	ctx = NULL;
	if		(  fSsl  ) {
		if		(  ( ctx = MakeCTX(KeyFile,CertFile,CA_File,CA_Path,fVerify) )
				   ==  NULL  ) {
			fprintf(stderr,"CTX make error\n");
			exit(1);
		}
	}
#endif
	while	(TRUE)	{
		if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
			printf("_fd = %d\n",_fd);
			Error("INET Domain Accept");
		}
		if		(  ( pid = fork() )  >  0  )	{	/*	parent	*/
			close(fd);
		} else
		if		(  pid  ==  0  )	{	/*	child	*/
#ifdef	USE_SSL
			if		(  fSsl  ) {
				fpComm = MakeSSL_Net(ctx,fd);
				SSL_accept(NETFILE_SSL(fpComm));
			} else {
				fpComm = SocketToNet(fd);
			}
#else
			fpComm = SocketToNet(fd);
#endif
			close(_fd);
			Process(fpComm);
			CloseNet(fpComm);
			exit(0);
		}
	}
	close(_fd);
LEAVE_FUNC;
}

extern	void
InitSystem(void)
{
dbgmsg(">InitSystem");
	InitNET();
dbgmsg("<InitSystem");
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"ポート番号"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"接続待ちキューの数" 							},
	{	"workdir",	STRING,		TRUE,	(void*)&WorkDir,
		"一時ファイルを作るディレクトリ"				},
	{	"execdir",	STRING,		TRUE,	(void*)&ExecDir,
		"実行コマンドのあるディレクトリ"				},
#ifdef	USE_SSL
	{	"ssl",		BOOLEAN,	TRUE,	(void*)&fSsl,
		"SSLを使う"				 						},
	{	"key",		STRING,		TRUE,	(void*)&KeyFile,
		"鍵ファイル名(pem)"		 						},
	{	"cert",		STRING,		TRUE,	(void*)&CertFile,
		"証明書ファイル名(pem)"	 						},
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
	PortNumber = IntStrDup(PORT_FDD);
	Back = 5;
	WorkDir = "/tmp";
	ExecDir = ".";
#ifdef	USE_SSL
	fSsl = FALSE;
	KeyFile = NULL;
	CertFile = NULL;
	fVerify = FALSE;
	CA_Path = NULL;
	CA_File = NULL;
#endif	
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*name;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("fdd",NULL);

	InitSystem();
	ExecuteServer();
	return	(0);
}
