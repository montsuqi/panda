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
#include	<curl/curl.h>

#include	"glclient.h"
#include	"interface.h"
#include	"printservice.h"
#include	"message.h"
#include	"debug.h"
#include	"gettext.h"

static size_t wrote_size = 0;

static size_t 
WriteData(
	void *buf,
	size_t size,
	size_t nmemb,
	void *userp)
{
	FILE *fp;

	fp = (FILE*)userp;
	wrote_size += size * nmemb;
	return fwrite(buf,size,nmemb,fp);
}

static int 
DoPrint(
	char *path,
	char *title,
	int showdialog)
{
	FILE *fp;
	int fd;
	mode_t mode;
	char url[1024+1];
	char fname[256];
	char userpass[256];
	char buf[1024];
	CURL *curl;
	CURLcode ret;
	int doretry;
    long http_code;

	doretry = 0;
	curl = curl_easy_init();
	if (!curl) {
		printf("couldn't init curl\n");
		return doretry;
	}

	sprintf(fname,"%s/glclient_print_XXXXXX",TempDir);
	sprintf(userpass,"%s:%s",User,Pass);
	snprintf(url,sizeof(url),"http://%s/%s",PortNumber,path);

	mode = umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if ((fd = mkstemp(fname)) != -1) {
		if ((fp = fdopen(fd, "w")) != NULL) {
			wrote_size = 0;
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)fp);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
			curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);
			ret = curl_easy_perform(curl);
			fclose(fp);
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (ret == CURLE_OK) {
				if (http_code == 200) {
					if (wrote_size == 0) {
						doretry = 1;
					} else {
						//show_dialog
						if (showdialog) {
							MessageLogPrintf("url[%s] fname[%s] size:[%ld]\n",
								url,fname,(long)wrote_size);
							UI_ShowPrintDialog(title,fname,wrote_size);
						} else {
							UI_PrintWithDefaultPrinter(fname);
						}
					}
				} else {
					sprintf(buf,_("print failure\ntitle:%s\n"),title);
					UI_Notify(_("glclient print service"),buf,"gtk-dialog-error",0);
				}
			}
			curl_easy_cleanup(curl);
		} else {
			Warning("fdopne failure");
		}
	} else {
		Warning("mkstemp failure");
	}
	umask(mode);
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

	if (PrintList == NULL) {
		return;
	}
	for (i=0; i < g_list_length(PrintList); i++) {
		req = (PrintRequest*)g_list_nth_data(PrintList,i);
		if (req == NULL) {
			Warning("print request is NULL.");
			continue;
		}
		MessageLogPrintf("donwload path[%s]\n",req->path);
		if (DoPrint(req->path,req->title,req->showdialog)) {
			// retry
			if (req->nretry == 0) {
				list = g_list_append(list,req);
			} else {
				if (req->nretry <= 1) {
					sprintf(buf,_("print failure\ntitle:%s\n"),req->title);
					UI_Notify(_("glclient print service"),buf,"gtk-dialog-error",0);
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
	g_list_free(PrintList);
	PrintList = list;
}
