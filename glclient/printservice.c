/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2010 NaCl & JMA (Japan Medical Association).
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
#include	"print.h"
#include	"printservice.h"
#include	"notify.h"
#include	"download.h"
#include	"protocol.h"
#include	"message.h"
#include	"debug.h"
#include	"gettext.h"

static int 
DoPrint(
	PrintRequest	*req)
{
	int	doretry;
	char *fname;
	size_t size;

	doretry = REST_APIDownload(req->path,&fname,&size);
	if (doretry == 0) {
		if (fname != NULL && size > 0) {
			if (req->showdialog) {
				ShowPrintDialog(req->title,fname,size);
			} else {
				PrintWithDefaultPrinter(fname);
			}
			g_free(fname);
		}
	}
	return doretry;
}

static void
FreePrintRequest(PrintRequest *req)
{
	xfree(req->path);
	xfree(req->title);
	xfree(req);
}

void
CheckPrintList()
{
	int i;
	GList *list = NULL;
	PrintRequest *req;
	gchar buf[1024];

	if (PRINTLIST(Session) == NULL) {
		return;
	}
	for (i=0; i < g_list_length(PRINTLIST(Session)); i++) {
		req = (PrintRequest*)g_list_nth_data(PRINTLIST(Session),i);
		if (req == NULL) {
			Warning("print request is NULL.");
			continue;
		}
		MessageLogPrintf("donwload path[%s]\n",req->path);
		if (DoPrint(req)) {
			// retry
			if (req->nretry == 0) {
				list = g_list_append(list,req);
			} else {
				if (req->nretry <= 1) {
					sprintf(buf,_("print failure\ntitle:%s\n"),req->title);
					Notify(_("glclient print notify"),buf,"gtk-dialog-error",0);
					FreePrintRequest(req);
				} else {
					req->nretry -= 1;
					list = g_list_append(list,req);
				}
			}
		} else {
			// delete request
			FreePrintRequest(req);
		}
	}
	g_list_free(PRINTLIST(Session));
	PRINTLIST(Session) = list;
}

static void
FreeDLRequest(DLRequest *req)
{
	xfree(req->path);
	xfree(req->filename);
	xfree(req->description);
	xfree(req);
}

static int 
DoDownload(
	DLRequest	*req)
{
	int				doretry;
	char			*fname;
	size_t			size;
	LargeByteString	*lbs;
	FILE 			*fp;

	doretry = REST_APIDownload(req->path,&fname,&size);
	if (doretry == 0) {
		if (fname != NULL && size > 0) {
			if ((fp = fopen(fname,"r")) != NULL) {
				lbs = NewLBS();
				LBS_ReserveSize(lbs,size,FALSE);
				fread(LBS_Body(lbs),size,1,fp);
				fclose(fp);
				ShowDownloadDialog(NULL,req->filename,req->description,lbs);
				FreeLBS(lbs);
			} else {
				Error("does not reach here;temporary file can not read");
			}
			xfree(fname);
		}
	}
	return doretry;
}

void
CheckDLList()
{
	int i;
	GList *list = NULL;
	DLRequest *req;
	gchar buf[1024];

	if (DLLIST(Session) == NULL) {
		return;
	}
	for (i=0; i < g_list_length(DLLIST(Session)); i++) {
		req = (DLRequest*)g_list_nth_data(DLLIST(Session),i);
		if (req == NULL) {
			Warning("download request is NULL.");
			continue;
		}
		MessageLogPrintf("donwload path[%s]\n",req->path);
		if (DoDownload(req)) {
			// retry
			if (req->nretry == 0) {
				list = g_list_append(list,req);
			} else {
				if (req->nretry <= 1) {
					sprintf(buf,
						_("download failure\nfilename:%s\ndescription:%s"),
						req->filename,req->description);
					Notify(_("glclient download notify"),
						buf,"gtk-dialog-error",0);
					FreeDLRequest(req);
				} else {
					req->nretry -= 1;
					list = g_list_append(list,req);
				}
			}
		} else {
			// delete request
			FreeDLRequest(req);
		}
	}
	g_list_free(DLLIST(Session));
	DLLIST(Session) = list;
}
