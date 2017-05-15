/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2009 Ogochan & JMA (Japan Medical Association).
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
#include	<unistd.h>
#include	<json.h>

#include	"const.h"

#include	"http.h"
#include	"libmondai.h"
#include	"comm.h"
#include	"message.h"
#include	"debug.h"

#define 	MONBLOB	"monblobapi"

static HTTP_RESPONSE *
BlobToResponse(
	HTTP_REQUEST *req,
	HTTP_RESPONSE *res,
	char *tempsocket)
{
	Port	*port;
	int		fd, _fd;
	NETFILE	*fp;
	char *recv;
	char *body;
	json_object *json_res;
	size_t	size;

	port = ParPortName(tempsocket);
	_fd =InitServerPort(port,1);
	if		(  ( fd = accept(_fd,0,0) )  <  0  )	{
		Error("INET Domain Accept");
	}
	fp = SocketToNet(fd);

	recv = RecvStringNew(fp);	ON_IO_ERROR(fp,badio);
	json_res = json_tokener_parse(recv);
	xfree(recv);

	size = RecvLength(fp);	ON_IO_ERROR(fp,badio);
	if (size > 0) {
		body = (char *)xmalloc(size);
		Recv(fp, body, size); ON_IO_ERROR(fp,badio);
		res->body_size = size;
		res->body = body;
		res->status = HTTP_OK;
	}

	json_object *json_content_type;
	char *content_type;
	json_content_type = json_object_object_get(json_res,"content-type");
	if (CheckJSONObject(json_content_type,json_type_string)) {
		content_type = (char*)json_object_get_string(json_content_type);
		res->content_type = content_type;
	}

	json_object *json_filename;
	char *filename;
	json_filename = json_object_object_get(json_res,"filename");
	if (CheckJSONObject(json_filename,json_type_string)) {
		filename = (char*)json_object_get_string(json_filename);
		res->filename = filename;
	}

	json_object *json_status;
	json_status = json_object_object_get(json_res,"status");
	if (CheckJSONObject(json_status,json_type_int)) {
		res->status = json_object_get_int(json_status);
	}

badio:
	close(_fd);
	CloseNet(fp);
	CleanUNIX_Socket(port);
	return res;
}

extern void
GetBlobAPI(
	HTTP_REQUEST *req,
	HTTP_RESPONSE *res)
{
	pid_t	pid;
	char tempdir[PATH_MAX];
	char tempsocket[PATH_MAX];
	char *cmd;

	snprintf(tempdir, PATH_MAX, "/tmp/blobapi_XXXXXX");
	if (!mkdtemp(tempdir)){
		Error("mkdtemp: %s", strerror(errno));
	}
	snprintf(tempsocket, PATH_MAX, "%s/%s", tempdir, "blobapi");
	cmd = BIN_DIR "/" MONBLOB;
	if ( ( pid = fork() ) <0 ) {
		Error("fork: %s", strerror(errno));
	}
	if (pid == 0){
		/* child */
		if (execl(cmd,MONBLOB,"-export", req->session_id,"-socket", tempsocket, NULL) < 0) {
			Error("execl: %s:%s", strerror(errno), cmd);
		}
	}
	/* parent */
	res = BlobToResponse(req, res, tempsocket);

	rmdir(tempdir);
	return;
}
