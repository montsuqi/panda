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
#include	"print.h"
#include	"printservice.h"
#include	"notify.h"
#include	"download.h"
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
Download(
	char	*path,
	char	**outfile,
	size_t	*size)
{
	FILE *fp;
	int fd;
	mode_t mode;
	gchar *scheme;
	gchar *url;
	gchar *fname;
	gchar *userpass;
	gchar *msg;
	CURL *curl;
	CURLcode ret;
	int doretry;
    long http_code;
	gboolean fSSL;

	doretry = 0;
	*outfile = NULL;
	*size = 0;
	curl = curl_easy_init();
	if (!curl) {
		Warning("couldn't init curl\n");
		return doretry;
	}

	mode = umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	fname = g_strdup_printf("%s/glclient_download_XXXXXX",TempDir);
	userpass = g_strdup_printf("%s:%s",User,Pass);

	fSSL = !strncmp("https",RESTURI(Session),5);

	url = g_strdup_printf("%s/%s",RESTURI(Session),path);

	if ((fd = mkstemp(fname)) == -1) {
		Warning("mkstemp failure");
		goto DO_PRINT_ERROR;
	}

	if ((fp = fdopen(fd, "w")) == NULL) {
		Warning("fdopne failure");
		goto DO_PRINT_ERROR;
	}

	wrote_size = 0;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)fp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);
	if (fSSL) {
		curl_easy_setopt(curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2);
	}

	ret = curl_easy_perform(curl);
	fclose(fp);
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (ret == CURLE_OK) {
		if (http_code == 200) {
			if (wrote_size == 0) {
				doretry = 1;
				remove(fname);
			} else {
				MessageLogPrintf("url[%s] fname[%s] size:[%ld]\n",
					url,fname,(long)wrote_size);
				*size = wrote_size;
				*outfile = fname;
			}
		} else if (http_code != 204) { /* 204 HTTP No Content */
			msg = g_strdup_printf(_("download failure\npath:%s\n"),path);
			Notify(_("glclient download notify"),msg,"gtk-dialog-error",0);
			g_free(msg);
		}
	}

DO_PRINT_ERROR:
	curl_easy_cleanup(curl);
	umask(mode);
	g_free(userpass);
	g_free(url);
	return doretry;
}

static int 
DoPrint(
	PrintRequest	*req)
{
	int	doretry;
	char *fname;
	size_t size;

	doretry = Download(req->path,&fname,&size);
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

	doretry = Download(req->path,&fname,&size);
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
