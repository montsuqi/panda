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

#define		WIDGETCACHE

#include	"glclient.h"
#include	"widgetcache.h"

void
LoadWidgetCache(void)
{
	FILE *fp;
	char fname[256];
	char buff[SIZE_BUFF+1];
	char *head;
	char *key;
	char *value;

	WidgetCache = NewNameHash();
	snprintf(fname, sizeof(fname), "%s/%s", 
		ConfDir, "widgetcache.txt");
	if ((fp = fopen(fname, "r")) != NULL) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			head = strstr(buff, ":");
			if (head != NULL) {
				key = StrnDup(buff, head - buff);
				head += 1;
				value = StrnDup(head, strlen(head) - 1); /* chop */
				g_hash_table_insert(WidgetCache, key, value);
			}
		}
		fclose(fp);
	}
}

static void
_SaveWidgetCache(char *key, char *value, FILE *fp)
{
	fprintf(fp, "%s:%s\n", key, value);
}

void
SaveWidgetCache(void)
{
	FILE *fp;
	char fname[256];
	mode_t permission = 0600;

	if (WidgetCache == NULL) return;

	snprintf(fname, sizeof(fname), "%s/%s", 
		ConfDir, "widgetcache.txt");
	if ((fp = fopen(fname, "w")) != NULL) {
		g_hash_table_foreach(WidgetCache, (GHFunc)_SaveWidgetCache, fp);
		fclose(fp);
		chmod(fname, permission);
	}
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
