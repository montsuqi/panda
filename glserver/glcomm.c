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
#include	"glcomm.h"
#include	"blobcache.h"
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

#if 0
static	void
GL_SendUInt(
	NETFILE	*fp,
	unsigned	int	data)
{
	unsigned char	buff[sizeof(unsigned int)];

	if		(  fNetwork  ) {
		*(unsigned int *)buff = SEND32(data);
	} else {
		*(unsigned int *)buff = data;
	}
	Send(fp,buff,sizeof(unsigned int));
}

static	unsigned	int
GL_RecvUInt(
	NETFILE	*fp,
	Bool	fNetwork)
{
	unsigned char	buff[sizeof(unsigned int)];
	unsigned	int	data;

	Recv(fp,buff,sizeof(unsigned int));
	if		(  fNetwork  ) {
		data = RECV32(*(unsigned int *)buff);
	} else {
		data = *(unsigned int *)buff;
	}
	return	(data);
}

extern	void
GL_SendLong(
	NETFILE	*fp,
	long	data)
{
	unsigned char	buff[sizeof(long)];

	if		(  fNetwork  ) {
		*(long *)buff = SEND32(data);
	} else {
		*(long *)buff = data;
	}
	Send(fp,buff,sizeof(long));
}
#endif
static	void
GL_SendLength(
	NETFILE	*fp,
	size_t	data)
{
	GL_SendInt(fp,data);
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

static	size_t
GL_RecvLength(
	NETFILE	*fp)
{
	return (size_t)GL_RecvInt(fp);
}

static	void
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

static	void
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
		Warning("Error: receive size to large [%d]. defined size [%d]", (int)lsize, (int)size);
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

extern	Fixed	*
GL_RecvFixed(
	NETFILE	*fp)
{
	Fixed	*xval;

ENTER_FUNC;
	xval = New(Fixed);
	xval->flen = GL_RecvLength(fp);
	xval->slen = GL_RecvLength(fp);
	xval->sval = (char *)xmalloc(xval->flen+1);
	GL_RecvString(fp, xval->flen, xval->sval);
LEAVE_FUNC;
	return	(xval); 
}

static	void
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

static	void
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

static	void
GL_SendBool(
	NETFILE	*fp,
	Bool	data)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
}

static	void
GL_SendDataType(
	NETFILE	*fp,
	PacketClass	c)
{
#ifdef	DEBUG
	printf("SendDataType = %X\n",c);
#endif
	nputc(c,fp);
}


extern	PacketDataType
GL_RecvDataType(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

static void
ReadFile(
	char *fname)
{
	FILE	*fpf;
	struct	stat	sb;

	if		(  ( fpf = Fopen(fname,"r") )  !=  NULL  ) {
		fstat(fileno(fpf),&sb);
		if (sb.st_size > 0) {
			LBS_ReserveSize(Buff,sb.st_size,FALSE);
			fread(LBS_Body(Buff),sb.st_size,1,fpf);
		} else {
			LBS_ReserveSize(Buff,0,FALSE);
		}
		fclose(fpf);
	} else {
		MessageLogPrintf("could not open for read: %s\n", fname);
		LBS_ReserveSize(Buff,0,FALSE);
	}
}

static	void
SendExpandObject(
	NETFILE	*fp,
	ValueStruct	*value)
{
	char		*fname;
ENTER_FUNC;
	if (IS_OBJECT_NULL(ValueObjectId(value))) {
		LBS_ReserveSize(Buff,0,FALSE);
		GL_SendLBS(fp,Buff);
		return;
	}
	fname = BlobCacheFileName(value);
	ReadFile(fname);
	GL_SendLBS(fp,Buff);
LEAVE_FUNC;
}

/*
 *	This function sends value with valiable name.
 */
extern	void
GL_SendValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*coding)
{
	int		i;

ENTER_FUNC;
	ValueIsNotUpdate(value);
	GL_SendDataType(fp,ValueType(value));
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		GL_SendInt(fp,ValueInteger(value));
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,ValueBool(value));
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
		LBS_ReserveSize(Buff,ValueByteLength(value),FALSE);
		memcpy(LBS_Body(Buff),ValueByte(value),ValueByteLength(value));
		GL_SendLBS(fp,Buff);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,ValueToString(value,coding));
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,ValueFloat(value));
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFixed(fp,&ValueFixed(value));
		break;
	  case	GL_TYPE_OBJECT:
		SendExpandObject(fp, value);
		break;
	  case	GL_TYPE_ARRAY:
		GL_SendInt(fp,ValueArraySize(value));
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			GL_SendValue(fp,ValueArrayItem(value,i),coding);
		}
		break;
	  case	GL_TYPE_RECORD:
		GL_SendInt(fp,ValueRecordSize(value));
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			GL_SendString(fp,ValueRecordName(value,i));
			GL_SendValue(fp,ValueRecordItem(value,i),coding);
		}
		break;
	  default:
		break;
	}
LEAVE_FUNC;
}

extern	void
GL_RecvValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*coding)
{
	PacketDataType	type;
	Fixed		*xval;
	int			ival;
	Bool		bval;
	double		fval;
	char		str[SIZE_BUFF];
	FILE		*fpf;

ENTER_FUNC;
	ValueIsUpdate(value);
	type = GL_RecvDataType(fp);
	ON_IO_ERROR(fp,badio);
	switch	(type)	{
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		dbgmsg("strings");
		GL_RecvString(fp,sizeof(str),str);	ON_IO_ERROR(fp,badio);
		SetValueString(value,str,coding);			ON_IO_ERROR(fp,badio);
		break;
	  case	GL_TYPE_NUMBER:
		dbgmsg("NUMBER");
		xval = GL_RecvFixed(fp);
		ON_IO_ERROR(fp,badio);
		SetValueFixed(value,xval);
		xfree(xval->sval);
		xfree(xval);
		break;
	  case	GL_TYPE_OBJECT:
		dbgmsg("OBJECT");
		GL_RecvLBS(fp,Buff);
		if		(  ( fpf = Fopen(BlobCacheFileName(value),"w") )  !=  NULL  ) {
			fwrite(LBS_Body(Buff),LBS_Size(Buff),1,fpf);
			fclose(fpf);	
		}
		break;
	  case	GL_TYPE_INT:
		dbgmsg("INT");
		ival = GL_RecvInt(fp);
		ON_IO_ERROR(fp,badio);
		SetValueInteger(value,ival);
		break;
	  case	GL_TYPE_FLOAT:
		dbgmsg("FLOAT");
		fval = GL_RecvFloat(fp);
		ON_IO_ERROR(fp,badio);
		SetValueFloat(value,fval);
		break;
	  case	GL_TYPE_BOOL:
		dbgmsg("BOOL");
		bval = GL_RecvBool(fp);
		ON_IO_ERROR(fp,badio);
		SetValueBool(value,bval);
		break;
	  default:
		printf("type = [%d]\n",type);
		break;
	}
  badio:
LEAVE_FUNC;
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
