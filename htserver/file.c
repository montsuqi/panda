/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2002-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
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

#define	MAIN
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	<errno.h>
#include	<iconv.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"socket.h"
#include	"net.h"
#include	"comm.h"
#include	"comms.h"
#include	"cgi.h"
#include	"mon.h"
#include	"message.h"
#include	"multipart.h"
#include	"file.h"
#include	"debug.h"

static size_t
EncodeRFC2231(char *q, byte *p)
{
	char	*qq;

	qq = q;
	while (*p != 0) {
        if (*p <= 0x20 || *p >= 0x7f) {
            *q++ = '%';
            q += sprintf(q, "%02X", ((int) *p) & 0xFF);
        }
        else {
            switch (*p) {
            case '*': case '\'': case '%':
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
                *q++ = '%';
                q += sprintf(q, "%02X", ((int) *p) & 0xFF);
                break;
            default:
                *q ++ = *p;
                break;
            }
        }
		p++;
	}
	*q = 0;
	return q - qq;
}

static size_t
EncodeLengthRFC2231(byte *p)
{
	size_t ret;

    ret = 0;
	while (*p != 0) {
        if (*p <= 0x20 || *p >= 0x7f) {
            ret += 3;
        }
        else {
            switch (*p) {
              case '*': case '\'': case '%':
              case '(': case ')': case '<': case '>': case '@':
              case ',': case ';': case ':': case '\\': case '"':
              case '/': case '[': case ']': case '?': case '=':
                ret += 3;
                break;
              default:
                ret++;
                break;
            }
        }
		p++;
	}
	return ret;
}

static	char	*
ConvShiftJIS(
	char	*istr)
{
	iconv_t	cd;
	size_t	sib
	,		sob;
	char	*ostr;
	static	char	cbuff[SIZE_LARGE_BUFF];

	cd = iconv_open("sjis","utf8");
	sib = strlen(istr);
	ostr = cbuff;
	sob = SIZE_LARGE_BUFF;
	iconv(cd,&istr,&sib,&ostr,&sob);
	*ostr = 0;
	iconv_close(cd);
	return	(cbuff);
}

extern	void
PutFile(ValueStruct *file)
{
	char *ctype_field;
	char *filename_field;

	if		((ctype_field = LoadValue("_contenttype")) != NULL) {
		char *ctype = GetHostValue(ctype_field, FALSE);
		if (*ctype != '\0')
			printf("Content-Type: %s\r\n", ctype);
    }
    else {
		printf("Content-Type: application/octet-stream\r\n");
    }
	if ((filename_field = LoadValue("_filename")) != NULL) {
		char *filename = GetHostValue(filename_field, FALSE);
		char *disposition_field = LoadValue("_disposition");
		char *disposition = "attachment";

		if (disposition_field != NULL) {
			char *tmp = GetHostValue(disposition_field, FALSE);
			if (*tmp != '\0')
				disposition = tmp;
		}
		if (*filename != '\0') {
			if		(  ( AgentType & AGENT_TYPE_MSIE )  ==  AGENT_TYPE_MSIE  ) {
				printf("Content-Disposition: %s;"
					   " filename=%s\r\n",
					   disposition,
					   ConvShiftJIS(filename));
			} else {
				int len = EncodeLengthRFC2231(filename);
				char *encoded = (char *) xmalloc(len + 1);
				EncodeRFC2231(encoded, filename);
				printf("Content-Disposition: %s;"
					   " filename*=utf-8''%s\r\n",
					   disposition,
					   encoded);
				xfree(encoded);
			}
		}
	}
	printf("\r\n");
	switch (ValueType(file)) {
	  case GL_TYPE_BYTE:
	  case GL_TYPE_BINARY:
		fwrite(ValueByte(file), 1, ValueByteLength(file), stdout);
		break;
	  case	GL_TYPE_OBJECT:
		break;
	  default:
		fputs(ValueToString(file, Codeset), stdout);
		break;
	}
	fDump = FALSE;
}

extern	void
SendFile(
	char	*name,
	MultipartFile	*file,
	HTCInfo	*htc)
{
	char	*filename
		,	*sendname
		,	*p;
	char	buff[SIZE_LONGNAME+1];

ENTER_FUNC;
	if		(	(  htc  !=  NULL  )
			&&	(  ( filename = (char *) g_hash_table_lookup(htc->FileSelection, name) )
				   !=  NULL  ) ) {
		dbgprintf("name           = [%s]",name);
		dbgprintf("file->filename = [%s]",file->filename);
		dbgprintf("filename       = [%s]",filename);

		strcpy(buff,file->filename);
		if		(	(  ( p = strrchr(buff,'\\') )  !=  NULL  )
				||	(  ( p = strrchr(buff,'/') )   !=  NULL  ) ) {
			sendname = p + 1;
		} else {
			sendname = buff;
		}
		SendValue(filename,sendname,strlen(sendname));
		SendValue(name,LBS_Body(file->body),LBS_Size(file->body));
    }
LEAVE_FUNC;
}

