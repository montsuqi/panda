/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
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
#include	<glib.h>
#include	<math.h>

#include	"types.h"
#include	"libmondai.h"
#define	_COMM
#include	"comm.h"
#include	"debug.h"

extern	void
SendPacketClass(
	NETFILE	*fp,
	PacketClass	c)
{
	nputc(c,fp);
}

extern	PacketClass
RecvPacketClass(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

extern	void
SendLength(
	NETFILE	*fp,
	size_t	size)
{
	Send(fp,&size,sizeof(size));
}

extern	size_t
RecvLength(
	NETFILE	*fp)
{
	size_t	size;

	Recv(fp,&size,sizeof(size));
	return	(size);
}

extern	void
SendString(
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
	SendLength(fp,size);
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
LEAVE_FUNC;
}

extern	void
RecvString(
	NETFILE	*fp,
	char	*str)
{
	size_t	size;

ENTER_FUNC;
	size = RecvLength(fp);
	Recv(fp,str,size);
	str[size] = 0;
LEAVE_FUNC;
}

extern	void
RecvnString(
	NETFILE	*fp,
	size_t  size,
	char	*str)
{
	size_t  lsize;
ENTER_FUNC;
    lsize = RecvLength(fp);
	if		( size >= lsize ) {
		size = lsize;
		Recv(fp,str,size);
		str[size] = 0;
	} else {
		CloseNet(fp);
		Error("error size mismatch !");
	}
LEAVE_FUNC;
}

extern	void
SendLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
	SendLength(fp,LBS_Size(lbs));
	if		(  LBS_Size(lbs)  >  0  ) {
		Send(fp,LBS_Body(lbs),LBS_Size(lbs));
	}
}

extern	void
RecvLBS(
	NETFILE	*fp,
	LargeByteString	*lbs)
{
	size_t	size;

	size = RecvLength(fp);
	LBS_ReserveSize(lbs,size,FALSE);
	if		(  size  >  0  ) {
		Recv(fp,LBS_Body(lbs),size);
	}
}	

extern	int
RecvInt(
	NETFILE	*fp)
{
	int		data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

extern	void
SendInt(
	NETFILE	*fp,
	int		data)
{
	Send(fp,&data,sizeof(data));
}

extern	unsigned int
RecvUInt(
	NETFILE	*fp)
{
	unsigned	int		data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

extern	void
SendUInt(
	NETFILE	*fp,
	unsigned int		data)
{
	Send(fp,&data,sizeof(data));
}

extern	uint64_t
RecvUInt64(
	NETFILE	*fp)
{
	uint64_t	data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

extern	void
SendUInt64(
	NETFILE	*fp,
	uint64_t	data)
{
	Send(fp,&data,sizeof(data));
}

extern	int
RecvChar(
	NETFILE	*fp)
{
	byte	data;
	int		ret;

	if		(  Recv(fp,&data,sizeof(data))  ==  sizeof(data)  ) {
		ret = data;
	} else {
		ret = -1;
	}
	return	(ret);
}

extern	void
SendChar(
	NETFILE	*fp,
	int		data)
{
	byte	buf;

	buf = (byte)data;
	Send(fp,&buf,sizeof(buf));
}

extern	Bool
RecvBool(
	NETFILE	*fp)
{
	char	buf[1];

	Recv(fp,buf,1);
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

extern	void
SendBool(
	NETFILE	*fp,
	Bool	data)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
}

extern	void
SendObject(
	NETFILE	*fp,
	MonObjectType	obj)
{
	SendUInt64(fp,obj);
}

extern	MonObjectType
RecvObject(
	NETFILE	*fp)
{
	return	((MonObjectType)RecvUInt64(fp));
}

extern	void
InitComm(void)
{
}

