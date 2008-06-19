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
#ifdef USE_GNOME
#    include <gnome.h>
#else
#    include <gtk/gtk.h>
#    include "gettext.h"
#endif	//USE_GNOME
#include	<glade/glade.h>
#ifdef	USE_PANDA
#include	<gtkpanda/gtkpanda.h>
#endif	//USE_PANDA
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
#include	"bd_config.h"
#include	"bd_component.h"
#include	"debug.h"

static	char	*PortNumber;
static	char	*Cache;
static	char	*Style;
static	char	*Gtkrc;
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
	TITLE(glSession) = NULL;
#ifdef	USE_SSL
	CTX(glSession) = NULL;
#ifdef  USE_PKCS11
	ENGINE(glSession) = NULL;
#endif	//USE_PKCS11
#endif	//USE_SSL
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
	PortNumber = g_strconcat(DEFAULT_HOST, ":", DEFAULT_PORT);
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
CacheFileName(
	char	*name)
{
	static	char	buf[SIZE_BUFF];

	sprintf(buf,"%s/%s/%s",Cache,PortNumber,name);
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

extern	void
SetSessionTitle(
		char *title)
{
	if ( TITLE(glSession) ) {
		xfree(TITLE(glSession));
	}
	TITLE(glSession) = StrDup(title);
}

extern  void
MakeCacheDir(
	char    *dname)
{
	struct stat st;

	if (stat(dname, &st) == 0){
		if (S_ISDIR(st.st_mode)) {
			return ;
		} else {
			unlink (dname);
		}
	}
	mkdir_p (Cache, 0755);
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
	printf(_("              2004-2007 Masami Ogoshi\n"));
}

static	void
list_config()
{
	BDConfig *config;
	BDConfigSection *section;
	GList *p;
	char *hostname;
	char *desc;
	
	config = bd_config_load_file();
	
	for (p = bd_config_get_sections (config); p != NULL; p = g_list_next (p)) {
		hostname = (char *) p->data;
		if (!strcmp (hostname, "glclient"))
			continue;
		section = bd_config_get_section (config, hostname);
		if (!strcmp("global", hostname)) {
        	desc = _("Custom");
		} else {
        	desc = bd_config_section_get_string (section, "description");
        }
		printf("------------------\n");
		printf("%s\n", desc);
		printf("\thost:\t\t%s\n", 
        	bd_config_section_get_string (section, "host"));
		printf("\tport:\t\t%s\n", 
        	bd_config_section_get_string (section, "port"));
		printf("\tapplication:\t%s\n", 
        	bd_config_section_get_string (section, "application"));
		printf("\tuser:\t\t%s\n", 
        	bd_config_section_get_string (section, "user"));
	}
	bd_config_free(config);
}

static	Bool
load_config (
	char *desc)
{
	BDConfig *config;
	BDConfigSection *section;
	GList *p;
	char *hostname;
	Bool ret;
	char *propdesc;
	char *host;
	char *port;
	
	ret = FALSE;
	config = bd_config_load_file();
	
	if ( !strcmp(_("Custom"), desc) || !strcmp("global", desc)) {
		section = bd_config_get_section (config, "global");
		ret = TRUE;
	} else {
		for ( 	p = bd_config_get_sections (config); 
				p != NULL; 
				p = g_list_next (p)) 
		{
			hostname = (char *) p->data;
			if (!strcmp (hostname, "glclient") || !strcmp("global", desc))
				continue;
			section = bd_config_get_section (config, hostname);
			propdesc = bd_config_section_get_string (section, "description");
			if (!strcmp(desc, propdesc)) {
				ret = TRUE;
				break;
			}
		}
	}
	if (ret) {
		host = bd_config_section_get_string (section, "host");
		port = bd_config_section_get_string (section, "port");
		PortNumber = g_strconcat(host, ":", port, NULL);
		CurrentApplication = bd_config_section_get_string (section, "application");
		Protocol1 = bd_config_section_get_bool (section, "protocol_v1");
		Protocol2 = bd_config_section_get_bool (section, "protocol_v2");
		Cache = bd_config_section_get_string (section, "cache");
		Style = bd_config_section_get_string (section, "style");
		Gtkrc = bd_config_section_get_string (section, "gtkrc");
		fMlog = bd_config_section_get_bool (section, "mlog");
		fKeyBuff = bd_config_section_get_bool (section, "keybuff");
		User = bd_config_section_get_string (section, "user");
		SavePass = bd_config_section_get_bool (section, "savepassword");
		if (SavePass) {
			Pass = bd_config_section_get_string (section, "password");
		} 
		fTimer = bd_config_section_get_bool (section, "timer");
		TimerPeriod = bd_config_section_get_string (section, "timerperiod");
#ifdef  USE_SSL
		fSsl = bd_config_section_get_bool (section, "ssl");
		CA_Path = bd_config_section_get_string (section, "CApath");
		if (!strcmp("", CA_Path)) CA_Path = NULL;
		CA_File = bd_config_section_get_string (section, "CAfile");
		if (!strcmp("", CA_File)) CA_File = NULL;
		KeyFile = bd_config_section_get_string (section, "key");
		if (!strcmp("", KeyFile)) KeyFile = NULL;
		CertFile = bd_config_section_get_string (section, "cert");
		if (!strcmp("", CertFile)) CertFile = NULL;
		Ciphers = bd_config_section_get_string (section, "ciphers");
#ifdef  USE_PKCS11
		fPKCS11 = bd_config_section_get_bool (section, "pkcs11");
		PKCS11_Lib = bd_config_section_get_string (section, "pkcs11_lib");
		if (!strcmp("", PKCS11_Lib)) PKCS11_Lib = NULL;
		Slot = bd_config_section_get_string (section, "slot");
#endif
#endif
	} else {
		Warning(_("cannot load config:%s"), desc);
	}
	return ret;
}

static gboolean
show_boot_dialog ()
{
    if (!boot_dialog_run ())
        return FALSE;
	if (!load_config("global"))
		return FALSE;
	if (!SavePass) {
		Pass = boot_dialog_get_password();
	}
    return TRUE;
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
			GLError(GetSSLErrorMessage());
			return FALSE;
        }
        if ((FPCOMM(glSession) = MakeSSL_Net(CTX(glSession),fd)) != NULL){
            if (StartSSLClientSession(FPCOMM(glSession), IP_HOST(glSession->port)) != TRUE){
				GLError(GetSSLErrorMessage());
				return FALSE;
            }
        }
		GLError(GetSSLWarningMessage());
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
		GLError(_("can not connect server(server port not found)"));
        return FALSE;
	}
	InitProtocol();
	if(MakeFPCOMM(fd) != TRUE) return FALSE;

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
#endif	//USE_PKCS11
#endif	//USE_SSL
    DestroyPort (glSession->port);
}

static void 
GLMessage(int level, char *file, int line, char *msg)
{
	switch(level){
	  case MESSAGE_WARN:
	  case MESSAGE_ERROR:
		GLError(msg);
		break;
	  default:
		__Message(level, file, line, msg);
		break;
	}
}

extern	int
main(
	int		argc,
	char	**argv)
{
	char	buff[SIZE_BUFF];
	char	password_[SIZE_BUFF];
	gboolean	do_run = TRUE;

	FILE_LIST	*fl;

	bannar();
	SetDefault();
	if	(  ( fl = GetOption(option,argc,argv,NULL) )  !=  NULL  ) {
		CurrentApplication = fl->name;
	}

	InitMessage("glclient",NULL);
	SetMessageFunction(GLMessage);
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
#endif	//USE_PANDA
	glade_gnome_init();
#else
	gtk_init(&argc, &argv);
#ifdef	USE_PANDA
	gtkpanda_init(&argc,&argv);
#endif	//USE_PANDA
	glade_init();
#endif	//USE_GNOME

    if (fConfigList) {
		list_config();
		exit(0);
    }

	if (strlen(Config) > 0) {
		fDialog = FALSE;
    	if (!load_config (Config))
			exit(0);
        if (!SavePass) {
			if(askpass(password_, SIZE_BUFF, _("Password")) != -1) {
				Pass = password_;
			} else {
				exit(0);
			}
		}
	}

	InitNET();
	SetAskPassFunction(askpass);

    if (fDialog) {
		do_run = show_boot_dialog() ;
    }

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

	while (do_run) {
		do_run = start_client();
		stop_client();
	}

    gtk_rc_reparse_all ();
	StyleParserTerm ();

	return 0;
}
