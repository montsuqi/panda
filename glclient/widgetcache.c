/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 NaCl & JMA (Japan Medical Association).
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
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<sys/time.h>
#include	<errno.h>
#include	<glib.h>
#include	<gconf/gconf-client.h>

#define		WIDGETCACHE

#include	"glclient.h"
#include	"widgetcache.h"

void
ConvertWidgetCache(void)
{
	FILE *fp;
	char fname[256];
	char buff[SIZE_BUFF+1];
	char *head;
	char *key;
	char *path;
	char *value;

	if (gconf_client_get_bool(GConfCTX,GL_GCONF_WCACHE_CONVERTED,NULL)) {
		return;
	}

	snprintf(fname, sizeof(fname), "%s/%s", 
		ConfDir, "widgetcache.txt");
	if ((fp = fopen(fname, "r")) != NULL) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			head = strstr(buff, ":");
			if (head != NULL) {
				key = g_strndup(buff, head - buff);
				head += 1;
				value = g_strndup(head, strlen(head) - 1); /* chop */
				path = g_strconcat(GL_GCONF_WCACHE,"/",key,NULL);
				gconf_client_set_string(GConfCTX,path,value,NULL);
				g_free(key);
				g_free(value);
				g_free(path);
			}
		}
		fclose(fp);
	}
	gconf_client_set_bool(GConfCTX,GL_GCONF_WCACHE_CONVERTED,TRUE,NULL);
	gconf_client_suggest_sync(GConfCTX,NULL);
}

void
LoadWidgetCache(void)
{
	GSList *list,*l;
	GConfEntry *entry;
	gchar *value;
	gchar *key;

	WidgetCache = NewNameHash();
	list = gconf_client_all_entries(GConfCTX,GL_GCONF_WCACHE,NULL);
	for(l=list;l!=NULL;l=l->next) {
		entry = (GConfEntry*)l->data;
		key = (char*)(gconf_entry_get_key(entry) + 
			strlen(GL_GCONF_WCACHE) + strlen("/"));
		value = gconf_client_get_string(GConfCTX,
			gconf_entry_get_key(entry),NULL);
		g_hash_table_insert(WidgetCache,
			g_strdup(key),
			g_strdup(value));
		gconf_entry_free(entry);
	}
	g_slist_free(list);
}

static void
_SaveWidgetCache(char *key, char *value, gpointer data)
{
	gchar *path;

	path = g_strconcat(GL_GCONF_WCACHE,"/",key,NULL);
	gconf_client_set_string(GConfCTX,path,value,NULL);
	g_free(path);
}

void
SaveWidgetCache(void)
{
	if (WidgetCache == NULL) return;
	g_hash_table_foreach(WidgetCache, (GHFunc)_SaveWidgetCache, NULL);
}

void
SetWidgetCache(char *key, char *value)
{
	gpointer oldvalue;
	gpointer oldkey;

	if (g_hash_table_lookup_extended(WidgetCache, key, &oldkey, &oldvalue)) {
		g_hash_table_remove(WidgetCache, key);
		xfree(oldkey);
		xfree(oldvalue);
	}
	g_hash_table_insert(WidgetCache, StrDup(key), StrDup(value));
}

char *
GetWidgetCache(char *key)
{
	return (char*)g_hash_table_lookup(WidgetCache, key);
}
