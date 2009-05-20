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
#include	<string.h>
#include    <sys/types.h>
#include	<sys/stat.h>
#include    <unistd.h>
#include	<sys/time.h>
#include 	<gtk/gtk.h>
#include 	"gettext.h"
#include	<gtkpanda/gtkpanda.h>

#include	"types.h"
#include	"glclient.h"
#include	"glterm.h"
#include	"action.h"
#include	"marshaller.h"
#include	"interface.h"
#include	"widgetOPS.h"
#include	"bd_config.h"
#include	"bootdialog.h"
#include	"dialogs.h"
#include	"styleParser.h"
#define		TOPLEVEL
#include	"toplevel.h"
#include	"message.h"
#include	"debug.h"

extern	int
UI_Version()
{
	return UI_VERSION_2;
}

extern	void
UI_list_config()
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

extern void
UI_load_config (
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
		exit(0);
	}
}

extern  void
UI_ShowWindow(char *sname)
{
	return ShowWindow(sname);
}

extern	void
UI_CreateWindow(char *name)
{
	CreateWindow(name);
}

extern	void
UI_CloseWindow(char *name)
{
	CloseWindow(name);
}

extern  void        
UI_UpdateScreen(char *windowName)
{
	UpdateWindow(windowName);
}

extern	void
UI_ResetScrolledWindow(char *windowName)
{
	GtkWidget *widget;

	widget = GetWidgetByLongName(windowName);
	g_return_if_fail(widget != NULL);
	ResetScrolledWindow(widget, NULL);
}

extern  void
UI_GrabFocus(char *windowName, 
	char *widgetName)
{
	GtkWidget 	*widget;

	widget = GetWidgetByWindowNameAndName(windowName,widgetName);
	if (widget != NULL) {
		GrabFocus(widget);
	}
}

extern  void        
UI_ResetTimer(char *windowName)
{
	GtkWidget 	*widget;
	
	widget = GetWidgetByLongName(windowName);
	if (widget != NULL) {
		ResetTimer(GTK_WINDOW(widget));
	}
}

extern  gboolean    
UI_BootDialogRun(void)
{
	return boot_dialog_run();
}

extern  gboolean    
UI_IsWidgetName(char *name)
{
	return (GetWidgetByWindowNameAndLongName(ThisWindowName, name) != NULL);
}

extern  gboolean    
UI_IsWidgetName2(char *name)
{
	return (GetWidgetByWindowNameAndName(ThisWindowName, name) != NULL);
}

extern void
UI_ErrorDialog(const char *msg)
{
	error_dialog(msg);
}

extern  void        
UI_Init(int argc, 
	char **argv)
{
	GtkSettings *set;

	gtk_init(&argc, &argv);
	/* set gtk-entry-select-on-focus */
	set = gtk_settings_get_default();
    gtk_settings_set_long_property(set, "gtk-entry-select-on-focus", 0, 
		"glclient2");
	gtk_panda_init(&argc,&argv);
	gtk_set_locale();
	glade_init();

	WindowTable = NewNameHash();
	TopWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(TopWindow), 
		"delete_event", (GtkSignalFunc)gtk_true, NULL);
	TopNoteBook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(TopNoteBook), FALSE);
	gtk_container_add(GTK_CONTAINER(TopWindow), TopNoteBook);
}

extern	void
UI_InitStyle(void)
{
	char	buff[SIZE_BUFF];

	StyleParserInit();
	sprintf(buff,"%s/gltermrc",getenv("HOME"));
	StyleParser(buff);
	StyleParser("gltermrc");
	if      (  *Style  !=  0  ) {
		StyleParser(Style);
	}
	if (*Gtkrc != '\0') {
		gtk_rc_parse(Gtkrc);
	}
}

extern	int
UI_AskPass(char	*buf,
	size_t		buflen,
	const char	*prompt)
{
	return askpass(buf,buflen,prompt);
}

extern	void
UI_Main(void)
{
	gtk_main();
}

extern	void
UI_GetWidgetData(WidgetData	*data)
{
	return GetWidgetData(data);
}

extern WidgetType
UI_GetWidgetType(char *windowName, 
	char *widgetName)
{
	return GetWidgetType(windowName, widgetName);
}

