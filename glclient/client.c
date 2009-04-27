/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#ifdef	USE_SSL
#include	<openssl/crypto.h>
#include	<openssl/x509.h>
#include	<openssl/pem.h>
#include	<openssl/ssl.h>
#include	<openssl/err.h>
#ifdef  USE_PKCS11
#include	<openssl/engine.h>
#endif	//USE_PKCS11
#endif	//USE_SSL
#define		MAIN
#include	"const.h"
#include	"types.h"
#include	"option.h"
#include	"socket.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"comm.h"
#include	"protocol.h"
#include	"message.h"
#include	"debug.h"
#include	"interface.h"
#include	"gettext.h"
#include	"widgetcache.h"

static	Bool 	fDialog;
static	char	*Config;
static	Bool 	fConfigList;

static void GLMessage(int level, char *file, int line, char *msg);

static	void
InitData(void)
{
	InitPool();
}

static	void
InitApplications(void)
{
	glSession = New(Session);
	FPCOMM(glSession) = NULL;
#ifdef	USE_SSL
	CTX(glSession) = NULL;
#ifdef  USE_PKCS11
	ENGINE(glSession) = NULL;
#endif	//USE_PKCS11
#endif	//USE_SSL
	TITLE(glSession) = NULL;
}

extern	void
InitSystem(void)
{
	InitData();
	InitApplications();
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,	TRUE,		(void*)&PortNumber,
		N_("Port")										},
	{	"cache",	STRING,		TRUE,	(void*)&Cache,
		N_("Cache Directory")						},
	{	"style",	STRING,		TRUE,	(void*)&Style,
		N_("Style Filename")							},
	{	"gtkrc",	STRING,		TRUE,	(void*)&Gtkrc,
		N_("GtkStyle Filename")							},
	{	"user",		STRING,		TRUE,	(void*)&User,
		N_("User")										},
	{	"pass",		STRING,		TRUE,	(void*)&Pass,
		N_("Password")									},
	{	"v1",		BOOLEAN,	TRUE,	(void*)&Protocol1,
		N_("Use Protocol Version 1")		},
	{	"v2",		BOOLEAN,	TRUE,	(void*)&Protocol2,
		N_("Use Protocol Version 2")		},
	{	"mlog",		BOOLEAN,	TRUE,	(void*)&fMlog,
		N_("Enable Logging")							},
	{	"keybuff",	BOOLEAN,	TRUE,	(void*)&fKeyBuff,
		N_("Enable Keybuffer")						},
	{	"dialog",	BOOLEAN,	TRUE,	(void*)&fDialog,
		N_("Use Startup Dialog")						},
	{	"config",		STRING,	TRUE,	(void*)&Config,
		N_("Specify Config")							},
	{	"configlist",BOOLEAN,	TRUE,	(void*)&fConfigList,
		N_("List Config")						},
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
		N_("SSL Cipher Sweet")							},
#ifdef  USE_PKCS11
	{	"pkcs11",		BOOLEAN,	TRUE,	(void*)&fPKCS11,
		N_("Use Security Device")	 						},
	{	"pkcs11_lib",STRING,	TRUE,	(void*)&PKCS11_Lib,
		N_("PKCS#11 Library")				         		},
	{	"slot",	    STRING,		TRUE,	(void*)&Slot,
		N_("Security Device Slot ID")			},
#endif  /* USE_PKCS11 */
#endif  /* USE_SSL */
	{	NULL,		0,			FALSE,	NULL,	NULL	},
};

static	void
SetDefault(void)
{
	char *cachename = g_strconcat(g_get_home_dir(), "/.glclient/cache", NULL);
	PortNumber = g_strconcat(DEFAULT_HOST, ":", DEFAULT_PORT, NULL);
	CurrentApplication = DEFAULT_APPLICATION;
	Cache =  cachename;
	Style = DEFAULT_STYLE;
	Gtkrc = DEFAULT_GTKRC;
	User = getenv("USER");
	Pass = DEFAULT_PASSWORD;
	SavePass = DEFAULT_SAVEPASSWORD;
	Protocol1 = DEFAULT_PROTOCOL_V1;
	Protocol2 = DEFAULT_PROTOCOL_V2;
	fMlog = DEFAULT_MLOG;
	fKeyBuff = DEFAULT_KEYBUFF;
	fTimer = TRUE;
	TimerPeriod = "1000";
	Config = "";
	fConfigList = FALSE;
	fDialog = FALSE;
#ifdef	USE_SSL
	fSsl = DEFAULT_SSL;
	KeyFile = DEFAULT_KEY;
	CertFile = DEFAULT_CERT;
	CA_Path = DEFAULT_CAPATH;
	CA_File = DEFAULT_CAFILE;
	Ciphers = DEFAULT_CIPHERS;
#ifdef  USE_PKCS11
    fPKCS11 = DEFAULT_PKCS11;
    PKCS11_Lib = DEFAULT_PKCS11;
    Slot = DEFAULT_SLOT;
#endif	//USE_PKCS11
#endif	//USE_SSL
}

extern	char	*
CacheDirName(void)
{
	static	char	buf[SIZE_BUFF];

	if (UI_Version() == UI_VERSION_1) {
		sprintf(buf,"%s/%s",Cache,PortNumber);
	} else {
		sprintf(buf,"%s/glclient2/%s",Cache,PortNumber);
	}
	return	(buf);
}

extern	char	*
CacheFileName(
	char	*name)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"%s/%s", CacheDirName() ,name);
	return	(buf);
}

extern  void
mkdir_p(
	char    *dname,
	int    mode)
{
	gchar *fn, *p;

	fn = g_strdup(dname);
	if (g_path_is_absolute (fn))
		p = (gchar *) g_path_skip_root (fn);	
	else
		p = fn;	

	do {
		while (*p && !(G_DIR_SEPARATOR == (*p)))
			p++;
		if (!*p)
			p = NULL;
		else
			*p = '\0';

		if (fn)
			mkdir (fn, mode);

		if (p)
		{
			*p++ = G_DIR_SEPARATOR;
			while (*p && (G_DIR_SEPARATOR == (*p)))
				p++;
		}
    }
	while (p);

	g_free (fn);
}

extern  void
MakeCacheDir(void)
{
	struct stat st;
	char *dir;

	dir = CacheDirName();

	if (stat(dir, &st) == 0){
		if (S_ISDIR(st.st_mode)) {
			return ;
		} else {
			unlink (dir);
		}
	}
	mkdir_p (dir, 0755);
}

extern	void SetSessionTitle(
	char *title)
{
	if ( TITLE(glSession) ) {
		xfree(TITLE(glSession));
	}
	TITLE(glSession) = StrDup(title);
}

static	void
bannar(void)
{
	printf(_("glclient ver %s\n"),VERSION);
	printf(_("Copyright (c) 1998-1999 Masami Ogoshi <ogochan@nurs.or.jp>\n"));
	printf(_("              2000-2003 Masami Ogoshi & JMA.\n"));
	printf(_("              2004-2007 Masami Ogoshi\n"));
}

#ifdef	USE_SSL
static void
_MakeSSL_CTX()
{
#ifdef  USE_PKCS11
	if (fPKCS11 == TRUE){
		CTX(glSession) = MakeSSL_CTX_PKCS11(&ENGINE(glSession), PKCS11_Lib,Slot,CA_File,CA_Path,Ciphers);
	}
	else{
		CTX(glSession) = MakeSSL_CTX(KeyFile,CertFile,CA_File,CA_Path,Ciphers);
	}
#else
    CTX(glSession) = MakeSSL_CTX(KeyFile,CertFile,CA_File,CA_Path,Ciphers);
#endif 	//USE_PKCS11
}
#endif	//USE_SSL	

static Bool
MakeFPCOMM (int fd)
{
#ifdef	USE_SSL
    if (!fSsl)
        FPCOMM(glSession) = SocketToNet(fd);
    else {
		_MakeSSL_CTX();

        if (CTX(glSession) == NULL){
			UI_ErrorDialog(GetSSLErrorMessage());
        }
        if ((FPCOMM(glSession) = MakeSSL_Net(CTX(glSession),fd)) != NULL){
            if (StartSSLClientSession(FPCOMM(glSession), IP_HOST(glSession->port)) != TRUE){
				UI_ErrorDialog(GetSSLErrorMessage());
            }
        }
		UI_ErrorDialog(GetSSLWarningMessage());
    }
#else
	FPCOMM(glSession) = SocketToNet(fd);
#endif	//USE_SSL
	return TRUE;
}

static gboolean
start_client ()
{
	int		fd;

    glSession->port = ParPort(PortNumber,PORT_GLTERM);
	if (  ( fd = ConnectSocket(glSession->port,SOCK_STREAM) )  <  0  ) {
		UI_ErrorDialog(_("can not connect server(server port not found)"));
        return FALSE;
	}
	InitProtocol();
	if(MakeFPCOMM(fd) != TRUE) return FALSE;
	LoadWidgetCache();

	if (SendConnect(FPCOMM(glSession),CurrentApplication)) {
		CheckScreens(FPCOMM(glSession),TRUE);
		(void)GetScreenData(FPCOMM(glSession));
		UI_Main();
	}
	
	return FALSE;
}

static void
stop_client ()
{
	GL_SendPacketClass(FPCOMM(glSession),GL_END);
	if	(  fMlog  ) {
		MessageLog("connection end\n");
	}
    CloseNet(FPCOMM(glSession));
#ifdef	USE_SSL
    if (CTX(glSession) != NULL)
        SSL_CTX_free (CTX(glSession));
#ifdef  USE_PKCS11
    if (ENGINE(glSession) != NULL){
        ENGINE_free(ENGINE(glSession));
        ENGINE_cleanup();
    }
#endif	//USE_PKCS11
#endif	//USE_SSL
    DestroyPort (glSession->port);
	SaveWidgetCache();
}

static void 
GLMessage(int level, char *file, int line, char *msg)
{
	switch(level){
	  case MESSAGE_WARN:
	  case MESSAGE_ERROR:
		UI_ErrorDialog(msg);
		break;
	  default:
		__Message(level, file, line, msg);
		break;
	}
}

static	void
askpass(char *pass)
{
	if (!SavePass) {
		if(UI_AskPass(pass, SIZE_BUFF, _("input Password")) != -1) {
			Pass = pass;
		} else {
			exit(0);
		}
	}
}

extern	int
main(
	int		argc,
	char	**argv)
{
	char		_password[SIZE_BUFF];
	gboolean	do_run = TRUE;
	char		*delay_str;
	int			delay;

	FILE_LIST	*fl;

	bannar();
	SetDefault();
	if	(  ( fl = GetOption(option,argc,argv,NULL) )  !=  NULL  ) {
		CurrentApplication = fl->name;
	}

	InitMessage("glclient",NULL);
	InitSystem();
	UI_Init(argc, argv);

	SetMessageFunction(GLMessage);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

    if (fConfigList) {
		UI_list_config();
		exit(0);
    }

	if (strlen(Config) > 0) {
		fDialog = FALSE;
    	UI_load_config(Config);
		askpass(_password);
	}

	InitNET();
#ifdef	USE_SSL
	SetAskPassFunction(UI_AskPass);
#endif

	if (fDialog) {
		do_run = UI_BootDialogRun();
		if (!do_run) {
			exit(0);
		}
	}

	UI_InitStyle();

	delay_str = getenv ("GL_SEND_EVENT_DELAY");
	if (delay_str) {
		delay = atoi(delay_str);
		if (delay > 0) {
			fTimer = TRUE;
			TimerPeriod = delay_str;
		} else {
			fTimer = FALSE;
		}
	}

	while (do_run) {
		do_run = start_client();
		stop_client();
	}

	UI_Final();

	return 0;
}
