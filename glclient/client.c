/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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
#endif
#endif
#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#    include "gettext.h"
#endif
#include	<glade/glade.h>
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif
#define		MAIN
#include	"const.h"
#include	"types.h"
#include	"option.h"
#include	"socket.h"
#include	"glterm.h"
#include	"glclient.h"
#include	"styleParser.h"
#include	"dialogs.h"
#include	"comm.h"
#include	"protocol.h"
#include	"message.h"
#include	"bootdialog.h"
#include	"debug.h"

static	char	*PortNumber;
static	char	*Cache;
static	char	*Style;
static	char	*Gtkrc;
static	Bool 	fDialog;

static	void
InitData(void)
{
	InitPool();
}

static	void
InitApplications(void)
{
	glSession = New(Session);
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
		N_("Passwrod")									},
	{	"v1",		BOOLEAN,	TRUE,	(void*)&Protocol1,
		N_("Use Protocol Version 1")		},
	{	"v2",		BOOLEAN,	TRUE,	(void*)&Protocol2,
		N_("Use Protocol Version 2")		},
	{	"mlog",		BOOLEAN,	TRUE,	(void*)&fMlog,
		N_("Enable Loggin")							},
	{	"keybuff",	BOOLEAN,	TRUE,	(void*)&fKeyBuff,
		N_("Enable Keybuffer")						},
	{	"dialog",	BOOLEAN,	TRUE,	(void*)&fDialog,
		N_("Use Startup Dialog")						},
#ifdef	USE_SSL
	{	"key",		STRING,		TRUE,	(void*)&KeyFile,
		N_("SSL Key File(pem/p12)")		 						},
	{	"cert",		STRING,		TRUE,	(void*)&CertFile,
		N_("Certificate(pem/p12)")	 						},
	{	"ssl",		BOOLEAN,	TRUE,	(void*)&fSsl,
		N_("Use SSL")				 						},
	{	"CApath",	STRING,		TRUE,	(void*)&CA_Path,
		N_("CA Certificate Path")								},
	{	"CAfile",	STRING,		TRUE,	(void*)&CA_File,
		N_("CA Certificate File")								},
	{	"ciphers",	STRING,		TRUE,	(void*)&Ciphers,
		N_("SSL Cipher Sweet")						},
#ifdef  USE_PKCS11
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
	PortNumber = "localhost:" PORT_GLTERM;
	CurrentApplication = "demo";
	Cache = "cache";
	Style = "";
	Gtkrc = "";
	User = getenv("USER");
	Pass = "";
	Protocol1 = TRUE;
	Protocol2 = FALSE;
	fMlog = FALSE;
	fKeyBuff = FALSE;
	fDialog = FALSE;
#ifdef	USE_SSL
	fSsl = FALSE;
	KeyFile = NULL;
	CertFile = NULL;
	CA_Path = NULL;
	CA_File = NULL;
	Ciphers = "ALL:!ADH:!LOW:!MD5:!SSLv2:@STRENGTH";
#ifdef  USE_PKCS11
    fPKCS11 = FALSE;
    PKCS11_Lib = NULL;
    Slot = NULL;
#endif
#endif	
}

extern	char	*
CacheFileName(
	char	*name)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"%s/%s/%s",Cache,PortNumber,name);
	return	(buf);
}

extern  void
mkCacheDir(
	char    *dname)
{
	struct stat st;

	stat(dname, &st);
	if (S_ISDIR(st.st_mode)) {
		return ;
	} else {
		unlink (dname);
	}
	mkdir (Cache, 0755);
	if  (mkdir (dname, 0755) < 0) {
		GLError(_("could not write cache dir"));
		exit(1);
	}
}

static	void
bannar(void)
{
	printf(_("glclient ver %s\n"),VERSION);
	printf(_("Copyright (c) 1998-1999 Masami Ogoshi <ogochan@nurs.or.jp>\n"));
	printf(_("              2000-2003 Masami Ogoshi & JMA.\n"));
}

static gboolean
show_boot_dialog ()
{
    static char *PortNumber_ = NULL;

    BootProperty prop;

    if (!boot_dialog_run ())
        return FALSE;

    boot_property_config_to_property (&prop);

    g_free (PortNumber_);
    PortNumber = PortNumber_ = g_strconcat (prop.host, ":", prop.port, NULL);
    CurrentApplication = prop.application;
    Protocol1 = prop.protocol_v1;
    Protocol2 = prop.protocol_v2;
    Cache = prop.cache;
    Style = prop.style;
    Gtkrc = prop.gtkrc;
    fMlog = prop.mlog;
    fKeyBuff = prop.keybuff;
    User = prop.user;
    Pass = prop.password;
#ifdef	USE_SSL
	fSsl = prop.ssl;
	if ( strlen(prop.key) != 0 ){
		KeyFile = prop.key;
	}
	if ( strlen(prop.cert) != 0 ){
		CertFile = prop.cert;
	}
	if ( strlen(prop.CApath) != 0 ){
		CA_Path = prop.CApath;
	}
	if ( strlen(prop.CAfile) != 0 ){	
		CA_File = prop.CAfile;
	}
	if ( strlen(prop.ciphers) != 0 ){	
		Ciphers = prop.ciphers;
	}
#ifdef  USE_PKCS11
    fPKCS11 = prop.pkcs11;
    if (fPKCS11){
	    if ( strlen(prop.pkcs11_lib) != 0 ){	
	    	PKCS11_Lib = prop.pkcs11_lib;
	    }
	    if ( strlen(prop.slot) != 0 ){
	    	Slot = prop.slot;
	    }
    }
#endif
#endif
    return TRUE;
}

static gboolean
start_client ()
{
	int		fd;

	FPCOMM(glSession) = NULL;
#ifdef	USE_SSL
	CTX(glSession) = NULL;
#ifdef  USE_PKCS11
    ENGINE(glSession) = NULL;
#endif
#endif

    glSession->port = ParPort(PortNumber,PORT_GLTERM);
	if		(  ( fd = ConnectSocket(glSession->port,SOCK_STREAM) )  <  0  ) {
		GLError(_("can not connect server(server port not found)"));
        return;
	}
#ifdef	USE_SSL
    if (!fSsl)
        FPCOMM(glSession) = SocketToNet(fd);
    else {
#ifdef  USE_PKCS11
        if (PKCS11_Lib != NULL){
            CTX(glSession) = MakeSSL_CTX_PKCS11(&ENGINE(glSession), PKCS11_Lib,Slot,CA_File,CA_Path,Ciphers);
        }
        else{
            CTX(glSession) = MakeSSL_CTX(KeyFile,CertFile,CA_File,CA_Path,Ciphers);
        }
#else
        CTX(glSession) = MakeSSL_CTX(KeyFile,CertFile,CA_File,CA_Path,Ciphers);
#endif
        if (CTX(glSession) == NULL){
            GLError(_("MakeSSL_CTX failure"));
			return;
        }
        if ((FPCOMM(glSession) = MakeSSL_Net(CTX(glSession),fd)) != NULL){
            if (StartSSLClientSession(FPCOMM(glSession), IP_HOST(glSession->port)) != TRUE){
                GLError(_("could not start SSL session"));
				return;
            }
        }
    }
#else
	FPCOMM(glSession) = SocketToNet(fd);
#endif
	InitProtocol();

	if (SendConnect(FPCOMM(glSession),CurrentApplication)) {
		CheckScreens(FPCOMM(glSession),TRUE);
		(void)GetScreenData(FPCOMM(glSession));
		gtk_main();  
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
#endif
#endif
    DestroyPort (glSession->port);
}

extern	int
main(
	int		argc,
	char	**argv)
{
	char	buff[SIZE_BUFF];
	gboolean	do_run = TRUE;

	FILE_LIST	*fl;

	bannar();
	SetDefault();
	if		(  ( fl = GetOption(option,argc,argv) )  !=  NULL  ) {
		CurrentApplication = fl->name;
	}
	InitMessage("glclient",NULL);

	gtk_set_locale();
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	InitSystem();

	argc = 1;
	argv[1] = NULL;

#ifdef USE_GNOME
	gnome_init("glclient", VERSION, argc, argv);
#ifdef	USE_PANDA
	gtkpanda_init(&argc,&argv);
#endif
	glade_gnome_init();
#else
	gtk_init(&argc, &argv);
#ifdef	USE_PANDA
	gtkpanda_init(&argc,&argv);
#endif
	glade_init();
#endif

	InitNET();

	StyleParserInit();
	sprintf(buff,"%s/gltermrc",getenv("HOME"));
	StyleParser(buff);
	StyleParser("gltermrc");
	if		(  *Style  !=  0  ) {
		StyleParser(Style);
	}

    if (*Gtkrc != '\0') {
        gtk_rc_parse(Gtkrc);
    }


    if (fDialog) {
		do_run = show_boot_dialog() ;
    }

	while (do_run) {
		do_run = start_client();
		stop_client();
	}

    gtk_rc_reparse_all ();
	StyleParserTerm ();

	return 0;
}
