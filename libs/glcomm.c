/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
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
#include	<errno.h>
#include	<string.h>
#include	<strings.h>
#include	<netinet/in.h>
#include    <sys/types.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<math.h>

#include	"libmondai.h"
#include	"net.h"
#include	"glcomm.h"
#include	"message.h"
#include	"debug.h"

#ifdef	__APPLE__
#include	<machine/endian.h>
#else
#include	<endian.h>
#endif

#define	RECV32(v)	ntohl(v)
#define	RECV16(v)	ntohs(v)
#define	SEND32(v)	htonl(v)
#define	SEND16(v)	htons(v)

static	LargeByteString	*Buff;
static	Bool fNetwork;

extern	void
GL_SendPacketClass(
	NETFILE	*fp,
	PacketClass	c)
{
	nputc(c,fp);
	Flush(fp);
}

extern	PacketClass
GL_RecvPacketClass(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

#define	SendChar(fp,c)	nputc((c),(fp))
#define	RecvChar(fp)	ngetc(fp)

extern	void
GL_SendInt(
	NETFILE	*fp,
	int		data)
{
	unsigned char	buff[sizeof(int)];

	if		(  fNetwork  ) {
		data = SEND32(data);
	}
	memcpy(buff,&data,sizeof(int));
	Send(fp,buff,sizeof(int));
}

extern	int
GL_RecvInt(
	NETFILE	*fp)
{
	unsigned char	buff[sizeof(int)];
	int		data;

	Recv(fp,buff,sizeof(int));
	memcpy(&data,buff,sizeof(int));
	if		(  fNetwork  ) {
		data = RECV32(data);
	}
	return	(data);
}

extern	void
GL_SendLength(
	NETFILE	*fp,
	size_t	data)
{
	GL_SendInt(fp,data);
}

extern	size_t
GL_RecvLength(
	NETFILE	*fp)
{
	int len = GL_RecvInt(fp);
	if (len < 0) {
		len = 0;
	}
	return (size_t)len;
}


extern	void
GL_RecvLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
	size_t	size;
ENTER_FUNC;
	size = GL_RecvLength(fp);
	LBS_ReserveSize(lbs,size,FALSE);
	if		(  size  >  0  ) {
		Recv(fp,LBS_Body(lbs),size);
	}
LEAVE_FUNC;
}

extern	void
GL_SendLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
ENTER_FUNC;
	GL_SendLength(fp,LBS_Size(lbs));
	if		(  LBS_Size(lbs)  >  0  ) {
		Send(fp,LBS_Body(lbs),LBS_Size(lbs));
	}
LEAVE_FUNC;
}

extern	void
GL_RecvString(
	NETFILE	*fp,
	size_t  size,
	char	*str)
{
	size_t	lsize;

ENTER_FUNC;
	lsize = GL_RecvLength(fp);
	if		(	size >= lsize 	){
		size = lsize;
		Recv(fp,str,size);
		str[size] = 0;
	} else {
		CloseNet(fp);
		Warning("Error: receive size to large [%zu]. defined size [%zu]", lsize, size);
	}
LEAVE_FUNC;
}

extern	void
GL_SendString(
	NETFILE	*fp,
	char	*str)
{
	size_t	size;

ENTER_FUNC;
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size);
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
LEAVE_FUNC;
}

#define MAX_FIXED_FLEN (64)

extern	Fixed	*
GL_RecvFixed(
	NETFILE	*fp)
{
	Fixed	*xval;

ENTER_FUNC;
	xval = New(Fixed);
	xval->flen = GL_RecvLength(fp);
	if (xval->flen > MAX_FIXED_FLEN) {
		Error("over MAX_FIXED_FLEN:%zu flen:%zu",MAX_FIXED_FLEN,xval->flen);
	}
	xval->slen = GL_RecvLength(fp);
	xval->sval = (char *)xmalloc(xval->flen+1);
	GL_RecvString(fp, xval->flen, xval->sval);
LEAVE_FUNC;
	return	(xval); 
}

extern	void
GL_SendFixed(
	NETFILE	*fp,
	Fixed	*xval)
{
	GL_SendLength(fp,xval->flen);
	GL_SendLength(fp,xval->slen);
	GL_SendString(fp,xval->sval);
}

extern	double
GL_RecvFloat(
	NETFILE	*fp)
{
	double	data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

extern	void
GL_SendFloat(
	NETFILE	*fp,
	double	data)
{
	Send(fp,&data,sizeof(data));
}

extern	Bool
GL_RecvBool(
	NETFILE	*fp)
{
	char	buf[1];

	Recv(fp,buf,1);
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

extern	void
GL_SendBool(
	NETFILE	*fp,
	Bool	data)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
}

extern	void
InitGL_Comm()
{
	Buff = NewLBS();
}

extern	void
SetfNetwork(
	Bool f)
{
	fNetwork = f;
}
