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

#define		INTERFACE_MAIN

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
#include	"print.h"
#include	"styleParser.h"
#include	"widgetcache.h"
#include	"notify.h"
#include	"message.h"
#include	"debug.h"

extern	void
UI_list_config()
{
	GSList *p;
	gchar *serverkey;
	gchar *key;
	
	for (
		p = gconf_client_all_dirs(GConfCTX,GL_GCONF_SERVERS,NULL); 
		p != NULL; 
		p = p->next
	) {
		serverkey = (gchar*)p->data;
		key = serverkey + strlen(GL_GCONF_SERVERS) + 1;
		printf("------------------\n");
		printf("[%s]\n", key);
		printf("\tdescription:\t%s\n", 
			gl_config_get_string(serverkey,"description"));
		printf("\thost:\t\t%s\n", gl_config_get_string(serverkey,"host"));
		printf("\tport:\t\t%s\n", gl_config_get_string(serverkey,"port"));
		printf("\tapplication:\t%s\n", 
			gl_config_get_string(serverkey,"application"));
		printf("\tuser:\t\t%s\n",gl_config_get_string(serverkey,"user"));
		printf("\tgconfkey:\t%s\n",serverkey);
	}
}

extern void
UI_load_config (
	char *confnum)
{
	GSList *list,*p;
	gchar *serverkey;
	gchar *value;

	serverkey = g_strconcat(GL_GCONF_SERVERS,"/",confnum,NULL);
	/* description */
	if (!gconf_client_dir_exists(GConfCTX,serverkey,NULL)) {
		for(
			p = list = gl_config_get_server_list();
			p != NULL;
			p = p->next) 
		{
			value = gl_config_get_string((gchar*)p->data,"description");
			if (!g_strcmp0(confnum,value)) {
				serverkey = g_strdup((gchar*)p->data);
			}
			g_free(value);
			g_free(p->data);
		}
		g_slist_free(list);
	}
	if (gconf_client_dir_exists(GConfCTX,serverkey,NULL)) {
		gl_config_set_server(serverkey);
		Host = gl_config_get_string (serverkey,"host");
		PortNum = gl_config_get_string (serverkey,"port");
		CurrentApplication = gl_config_get_string (serverkey,"application");
		Protocol1 = gl_config_get_bool (serverkey,"protocol_v1");
		Protocol2 = gl_config_get_bool (serverkey,"protocol_v2");
		Style = gl_config_get_string (serverkey,"style");
		Gtkrc = gl_config_get_string (serverkey,"gtkrc");
		fMlog = gl_config_get_bool (serverkey,"mlog");
		fKeyBuff = gl_config_get_bool (serverkey,"keybuff");
		User = gl_config_get_string (serverkey,"user");
		SavePass = gl_config_get_bool (serverkey,"savepassword");
		if (SavePass) {
			Pass = gl_config_get_string (serverkey,"password");
		} 
		fTimer = gl_config_get_bool (serverkey,"timer");
		TimerPeriod = gl_config_get_string (serverkey,"timerperiod");
#ifdef  USE_SSL
		fSsl = gl_config_get_bool (serverkey,"ssl");
		CA_File = gl_config_get_string (serverkey,"CAfile");
		if (!strcmp("", CA_File)) CA_File = NULL;
		CertFile = gl_config_get_string (serverkey,"cert");
		if (!strcmp("", CertFile)) CertFile = NULL;
		Ciphers = gl_config_get_string (serverkey,"ciphers");
#ifdef  USE_PKCS11
		fPKCS11 = gl_config_get_bool (serverkey,"pkcs11");
		PKCS11_Lib = gl_config_get_string (serverkey,"pkcs11_lib");
		if (!strcmp("", PKCS11_Lib)) PKCS11_Lib = NULL;
		Slot = gl_config_get_string (serverkey,"slot");
#endif
#endif
	} else {
		g_error(_("cannot load config:%s"), confnum);
	}
}

extern  void
UI_ShowWindow(char *sname)
{
	return ShowWindow(sname);
}

extern	void
UI_CreateWindow(char *name,int size,char *buff)
{
	CreateWindow(name,size,buff);
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
UI_BootDialogRun(void)
{
	boot_dialog_run();
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
UI_MessageDialog(const char *msg)
{
	show_info_dialog(msg);
}

extern void
UI_ErrorDialog(const char *msg)
{
	show_error_dialog(msg);
}

extern  void        
UI_Init(int argc, 
	char **argv)
{
	gtk_init(&argc, &argv);
#if 1
	/* set gtk-entry-select-on-focus */
	GtkSettings *set = gtk_settings_get_default();
    gtk_settings_set_long_property(set, "gtk-entry-select-on-focus", 0, 
		"glclient2");
#endif
	gtk_panda_init(&argc,&argv);
#ifndef LIBGTK_3_0_0
	gtk_set_locale();
#endif
	glade_init();


	WindowTable = NewNameHash();
}

extern  void        
UI_InitTopWindow(void)
{
	InitTopWindow();
}

extern	void
UI_InitStyle(void)
{
	gchar *gltermrc;

	StyleParserInit();
	gltermrc = g_strconcat(g_get_home_dir(),"/gltermrc",NULL);
	StyleParser(gltermrc);
	StyleParser("gltermrc");
	g_free(gltermrc);
	if (  Style  != NULL  ) {
		StyleParser(Style);
	}
	if (Gtkrc != NULL) {
		gtk_rc_parse(Gtkrc);
	}
}

extern	int
UI_AskPass(char	*buf,
	size_t		buflen,
	const char	*prompt)
{
#ifdef USE_SSL
	int ret;

	ret = askpass(buf,buflen,prompt);
	Passphrase = StrDup(buf);
	return ret;
#else
	return 0;
#endif
}

extern	void
UI_Main(void)
{
	if (fIMKanaOff) {
		gtk_panda_entry_force_feature_off();
	}
	gtk_main();
}

extern	void
UI_GetWidgetData(WidgetData	*data)
{
	return GetWidgetData(data);
}

extern	WidgetType
UI_GetWidgetType(char *windowName, 
	char *widgetName)
{
	return GetWidgetType(windowName, widgetName);
}

extern	void
UI_SetPingTimerFunc(_PingTimerFunc func, gpointer data)
{
	g_timeout_add(PingTimerPeriod, func, data);
}

extern	void
UI_ShowPrintDialog(char *title,
	char *fname,
	size_t size)
{
	show_print_dialog(title,fname,size);
}

extern	void
UI_PrintWithDefaultPrinter(char *fname)
{
	print_with_default_printer(fname);
}

extern	void
UI_Notify(char *summary,
	char *body,
	char *icon,
	int timeout) 
{
	Notify(summary,body,icon,timeout);
}
