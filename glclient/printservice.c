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

static size_t wrote_size;
static LargeByteString *buf = NULL;

static size_t 
write_data(
	void *buf,
	size_t size,
	size_t nmemb,
	void *userp)
{
	size_t buf_size;
	LargeByteString *lbs;
	unsigned char *p;
	int i;

	lbs = (LargeByteString*)userp;
	buf_size = size * nmemb;
	for(i=0,p=(unsigned char*)buf;i<buf_size;i++,p++) {
		LBS_EmitChar(lbs,*p);
	}
	wrote_size += buf_size;
	return buf_size;
}

static int
Download(
	char *path)
{
	gchar *scheme,*url,*userpass,*msg;
	CURL *curl;
	CURLcode ret;
	int doretry;
    long http_code;

	doretry = 0;
	wrote_size = 0;

	if (buf == NULL) {
		buf = NewLBS();
	}
	LBS_EmitStart(buf);
	LBS_SetPos(buf,0);

	curl = curl_easy_init();
	if (!curl) {
		Warning("couldn't init curl\n");
		return doretry;
	}

	userpass = g_strdup_printf("%s:%s",User,Pass);
	scheme = fSsl ? "https" : "http";
	url = g_strdup_printf("%s://%s:%s/%s",scheme,Host,PortNum,path);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)buf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_data);
	if (fSsl) {
		curl_easy_setopt(curl,CURLOPT_USE_SSL,CURLUSESSL_ALL);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1);
		curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2);
		curl_easy_setopt(curl,CURLOPT_SSL_CIPHER_LIST,Ciphers);
		curl_easy_setopt(curl,CURLOPT_SSLCERT, CertFile); 
        curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE, "P12");	
        curl_easy_setopt(curl,CURLOPT_SSLKEYPASSWD, Passphrase); 
        curl_easy_setopt(curl,CURLOPT_CAINFO, CA_File); 
	} else {
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);
	}

	ret = curl_easy_perform(curl);
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (ret == CURLE_OK) {
		if (http_code == 200) {
			if (wrote_size == 0) {
				doretry = 1;
			}
		} else if (http_code != 204) { /* 204 HTTP No Content */
			msg = g_strdup_printf(_("download failure\npath:%s\n"),path);
			Notify(_("glclient download notify"),msg,"gtk-dialog-error",0);
			g_free(msg);
		}
	}

	curl_easy_cleanup(curl);
	g_free(userpass);
	g_free(url);
	return doretry;
}

static int 
DoPrint(
	PrintRequest	*req)
{
	int	doretry;

	doretry = Download(req->path);
	if (doretry == 0) {
		if (req->showdialog) {
			ShowPrintDialog(req->title,buf);
		} else {
			Print(req->title,NULL,buf);
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

	doretry = Download(req->path);
	if (doretry == 0) {
		ShowDownloadDialog(NULL,req->filename,req->description,buf);
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
