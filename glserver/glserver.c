/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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
#include	"glcomm.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char		*AuthURL;
static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		N_("waiting port name")							},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		N_("connection waiting queue number")			},
	{	"screen",	STRING,		TRUE,	(void*)&ScreenDir,
		N_("screen directory")	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		N_("record directory")		 					},
	{	"cache",	STRING,		TRUE,	(void*)&CacheDir,
		N_("BLOB cache directory")						},
	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		N_("authentication server")	 					},
#ifdef	USE_SSL
	{	"key",		STRING,		TRUE,	(void*)&KeyFile,
		N_("SSL Key File(pem/p12)")		 				},
	{	"cert",		STRING,		TRUE,	(void*)&CertFile,
		N_("Certificate(pem/p12)")	 					},
	{	"ssl",		BOOLEAN,	TRUE,	(void*)&fSsl,
		N_("Use SSL")				 					},
	{	"CApath",	STRING,		TRUE,	(void*)&CA_Path,
		N_("CA Certificate Path")						},
	{	"CAfile",	STRING,		TRUE,	(void*)&CA_File,
		N_("CA Certificate File")						},
	{	"ciphers",	STRING,		TRUE,	(void*)&Ciphers,
		N_("SSL Cipher suite")							},
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
	AuthURL = "glauth://localhost:" PORT_GLAUTH;
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
	(void)GetOption(option,argc,argv,NULL);
	InitMessage("glserver",NULL);

	ParseURL(&Auth,AuthURL,"file");
	InitSystem(argc,argv);
#ifdef	USE_SSL
	if ( fSsl ){
		Message("glserver start (ssl)");
	} else {
		Message("glserver start");
	}
#else
	Message("glserver start");
#endif
	ExecuteServer();
	Message("glserver end");
	return	(0);
}
