/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2011 JMA (Japan Medical Association).
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
#include	<unistd.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>

#include	"glclient.h"
#include	"notify.h"

#define DEFAULT_TIMEOUT 3000

static gboolean fInit = FALSE;

void
Notify(gchar *summary,
	gchar *body,
	gchar *icon,
	gint timeout)
{
	static NotifyNotification *n = NULL;
	gint to = DEFAULT_TIMEOUT;

	if (!fInit) {
		notify_init("glclient2");
		fInit = TRUE;
	}

	n = notify_notification_new (summary, body, icon);

	if (timeout > 0) {
		to = timeout * 1000;
	}
	notify_notification_set_timeout (n, to); 

	if (!notify_notification_show (n, NULL)) {
		fprintf(stderr, "failed to send notification\n");
	}
	g_object_unref(G_OBJECT(n));
}

