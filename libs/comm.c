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
#include	"misc.h"
#include	"value.h"
#include	"libmondai.h"
#define	_COMM
#include	"comm.h"
#include	"debug.h"

static	int
nputc(
	int		c,
	NETFILE	*fp)
{
	char	ch;

	ch = c;
	return	(Send(fp,&ch,1));
}

static	int
ngetc(
	NETFILE	*fp)
{
	char	ch;
	int		ret;

	if		(  Recv(fp,&ch,1)  >=  0  ) {
		ret = ch;
	} else {
		ret = -1;
	}
	return	(ret);
}

extern	void
SendPacketClass(
	NETFILE	*fp,
	PacketClass	c)
{
#ifdef	DEBUG
	printf("SendPacketClass = %X\n",c);
#endif
	nputc(c,fp);
}

extern	PacketClass
RecvPacketClass(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
#ifdef	DEBUG
	printf("RecvPacketClass = %X\n",c);
#endif
	return	(c);
}

extern	void
SendDataType(
	NETFILE	*fp,
	PacketClass	c)
{
#ifdef	DEBUG
	printf("SendDataType = %X\n",c);
#endif
	nputc(c,fp);
}

extern	PacketDataType
RecvDataType(
	NETFILE	*fp)
{
	PacketClass	c;

	c = ngetc(fp);
#ifdef	DEBUG
	printf("RecvDataType = %X\n",c);
#endif
	return	(c);
}

extern	void
SendPacketDataType(
	NETFILE	*fp,
	PacketDataType	t)
{
	nputc(t,fp);
}

extern	PacketDataType
RecvPacketDataType(
	NETFILE	*fp)
{
	return	(ngetc(fp));
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

dbgmsg(">RecvLength");
	Recv(fp,&size,sizeof(size));
#ifdef	DEBUG
	printf("[%d]\n",size);
#endif
dbgmsg("<RecvLength");
	return	(size);
}

extern	void
SendString(
	NETFILE	*fp,
	char	*str)
{
	size_t	size;

dbgmsg(">SendString");
#ifdef	DEBUG
	printf("[%s]\n",str);
#endif
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	SendLength(fp,size);
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
dbgmsg("<SendString");
}

extern	void
SendFixedString(
	NETFILE	*fp,
	char	*str,
	size_t	size)
{
dbgmsg(">SendFixedString");
#ifdef	DEBUG
	printf("[%s]\n",str);
#endif
	SendLength(fp,size);
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
dbgmsg("<SendFixedString");
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
		LBS_SetPos(lbs,size-1);
	}
}	

extern	void
RecvStringBody(
	NETFILE	*fp,
	char	*str,
	size_t	size)
{
dbgmsg(">RecvStringBody");
	Recv(fp,str,size);
	str[size] = 0;
#ifdef	DEBUG
	printf("[%s]\n",str);
#endif
dbgmsg("<RecvStringBody");
}

extern	void
RecvString(
	NETFILE	*fp,
	char	*str)
{
	size_t	size;

dbgmsg(">RecvString");
	size = RecvLength(fp);
	RecvStringBody(fp,str,size);
dbgmsg("<RecvString");
}

extern	long
RecvLong(
	NETFILE	*fp)
{
	long	data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

extern	void
SendLong(
	NETFILE	*fp,
	long	data)
{
	Send(fp,&data,sizeof(data));
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

extern	double
RecvFloat(
	NETFILE	*fp)
{
	double	data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

extern	void
SendFloat(
	NETFILE	*fp,
	double	data)
{
	Send(fp,&data,sizeof(data));
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
SendFixed(
	NETFILE	*fp,
	Fixed	*xval)
{
	SendLength(fp,xval->flen);
	SendLength(fp,xval->slen);
	SendString(fp,xval->sval);
}

extern	Fixed	*
RecvFixed(
	NETFILE	*fp)
{
	Fixed	*xval;

dbgmsg(">RecvFixed");
	xval = New(Fixed);
	xval->flen = RecvLength(fp);
	xval->slen = RecvLength(fp);
	xval->sval = (char *)xmalloc(xval->flen+1);
	RecvString(fp,xval->sval);
dbgmsg("<RecvFixed");
	return	(xval); 
}

extern	Bool
RecvFixedData(
	NETFILE	*fp,
	Fixed	**xval)
{
	Bool	ret;
	
	DataType = RecvDataType(fp);

	switch	(DataType) {
	  case	GL_TYPE_NUMBER:
		*xval = RecvFixed(fp);
		ret = TRUE;
		break;
	  default:
		printf("invalid data conversion\n");
		exit(1);
		ret = FALSE;
		break;
	}
	return	(ret);
}

extern	void
SendFixedData(
	NETFILE			*fp,
	PacketDataType	type,
	Fixed			*xval)
{
	SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		SendString(fp,(char *)xval->sval);
		break;
	  case	GL_TYPE_NUMBER:
		SendFixed(fp,xval);
		break;
	  default:
		printf("invalid data conversion\n");
		exit(1);
		break;
	}
}

extern	void
SendStringData(
	NETFILE			*fp,
	PacketDataType	type,
	char			*str)
{
	SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		SendString(fp,(char *)str);
		break;
	  case	GL_TYPE_INT:
		SendInt(fp,atoi(str));
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,atof(str));
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,(( *str == 'T' ) ? TRUE: FALSE ));
		break;
	  default:
		break;
	}
}

extern	Bool
RecvStringData(
	NETFILE	*fp,
	char	*str)
{
	Bool			ret;
	
	DataType = RecvDataType(fp);

	switch	(DataType) {
	  case	GL_TYPE_INT:
		sprintf(str,"%d",RecvInt(fp));
		ret = TRUE;
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		RecvString(fp,str);
		ret = TRUE;
		break;
	  default:
		ret = FALSE;
		break;
	}
	return	(ret);
}


extern	void
SendIntegerData(
	NETFILE			*fp,
	PacketDataType	type,
	int				val)
{
	char	buff[SIZE_BUFF];
	
	SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(buff,"%d",val);
		SendString(fp,(char *)buff);
		break;
	  case	GL_TYPE_NUMBER:
		SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_INT:
		SendInt(fp,val);
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,(( val == 0 ) ? FALSE : TRUE ));
		break;
	  default:
		break;
	}
}

extern	Bool
RecvIntegerData(
	NETFILE	*fp,
	int		*val)
{
	char	buff[SIZE_BUFF];
	Bool	ret;
	
	DataType = RecvDataType(fp);

	switch	(DataType) {
	  case	GL_TYPE_INT:
		*val = RecvInt(fp);
		ret = TRUE;
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		RecvString(fp,buff);
		*val = atoi(buff);
		ret = TRUE;
		break;
	  default:
		ret = 0;
		ret = FALSE;
		break;
	}
	return	(ret);
}

extern	void
SendBoolData(
	NETFILE			*fp,
	PacketDataType	type,
	Bool			val)
{
	SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		SendString(fp,( val ? "T" : "F" ));
		break;
	  case	GL_TYPE_INT:
		SendInt(fp,(int)val);
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,(double)val);
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,val);
		break;
	  default:
		break;
	}
}

extern	Bool
RecvBoolData(
	NETFILE	*fp,
	Bool	*val)
{
	char	buff[SIZE_BUFF];
	Bool	ret;
	
	DataType = RecvDataType(fp);

	ret = TRUE;
	switch	(DataType) {
	  case	GL_TYPE_INT:
		*val = ( RecvInt(fp) == 0 ) ? FALSE : TRUE;
		break;
	  case	GL_TYPE_BOOL:
		*val = RecvBool(fp);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		RecvString(fp,buff);
		*val = (  buff[0]  ==  'T'  ) ? TRUE : FALSE;
		break;
	  default:
		ret = FALSE;
		break;
	}
	return	(ret);
}

extern	void
SendFloatData(
	NETFILE			*fp,
	PacketDataType	type,
	double			val)
{
	char	buff[SIZE_BUFF];
	
	SendDataType(fp,type);
	switch	(type) {
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		sprintf(buff,"%g",val);
		SendString(fp,(char *)buff);
		break;
#if	0
	  case	GL_TYPE_NUMBER:
		xval = DoubleToFixed(val);
		SendFixed(fp,xval);
		FreeFixed(xval);
		break;
#endif
	  case	GL_TYPE_INT:
		SendInt(fp,(int)val);
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,val);
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,(( val == 0 ) ? FALSE : TRUE ));
		break;
	  default:
		printf("invalid data conversion\n");
		exit(1);
		break;
	}
}

extern	Bool
RecvFloatData(
	NETFILE	*fp,
	double	*val)
{
	char	buff[SIZE_BUFF];
	Bool	ret;
	Fixed	*xval;

dbgmsg(">RecvFloatData");	
	DataType = RecvDataType(fp);

	switch	(DataType)	{
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		RecvString(fp,buff);
		*val = atof(buff);
		ret = TRUE;
		break;
	  case	GL_TYPE_NUMBER:
		xval = RecvFixed(fp);
		*val = atof(xval->sval) / pow(10.0, xval->slen);
		xfree(xval);
		ret = TRUE;
		break;
	  case	GL_TYPE_INT:
		*val = (double)RecvInt(fp);
		ret = TRUE;
		break;
	  case	GL_TYPE_FLOAT:
		*val = RecvFloat(fp);
		ret = TRUE;
		break;
	  default:
		ret = 0;
		ret = FALSE;
		break;
	}
dbgmsg("<RecvFloatData");	
	return	(ret);
}

extern	void
SendValueBody(
	NETFILE		*fp,
	ValueStruct	*value)
{
	int		i;

	if		(  value  ==  NULL  )	return;
	SendDataType(fp,ValueType(value));
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		SendInt(fp,ValueInteger(value));
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,ValueFloat(value));
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,ValueBool(value));
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		SendFixedString(fp,ValueToString(value,NULL),ValueStringLength(value));
		break;
	  case	GL_TYPE_NUMBER:
		SendFixed(fp,&ValueFixed(value));
		break;
	  case	GL_TYPE_TEXT:
		SendString(fp,ValueToString(value,NULL));
		break;
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			SendValueBody(fp,ValueArrayItem(value,i));
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			SendValueBody(fp,ValueRecordItem(value,i));
		}
		break;
	  default:
		break;
	}
}

extern	void
RecvValueBody(
	NETFILE		*fp,
	ValueStruct	*value)
{
	int		i;
	PacketDataType	type;
	size_t	size;
	char	*recvBuffer;
	size_t	asize;

	if		(  value  ==  NULL  )	return;
	asize = 1;
	recvBuffer = (char *)xmalloc(asize);
	type = RecvDataType(fp);
	if		(  type  !=  ValueType(value)  ) {
		fprintf(stderr,"fatal type miss match\n");
		exit(1);
	}
	switch	(type) {
	  case	GL_TYPE_INT:
		ValueInteger(value) = RecvInt(fp);
		break;
	  case	GL_TYPE_FLOAT:
		ValueFloat(value) = RecvFloat(fp);
		break;
	  case	GL_TYPE_BOOL:
		ValueBool(value) = RecvBool(fp);
		break;
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		size = RecvLength(fp);
		if		(  ( size + 1)  >  asize  ) {
			xfree(recvBuffer);
			asize = size + 1;
			recvBuffer = (char *)xmalloc(asize);
		}
		memclear(recvBuffer,asize);
		RecvStringBody(fp,recvBuffer,size);
		SetValueString(value,recvBuffer,NULL);
		break;
	  case	GL_TYPE_NUMBER:
		size = RecvLength(fp);
		if		(  size  >  ValueFixedLength(value)  ) {
			xfree(ValueFixedBody(value));
			ValueFixedBody(value) = (char *)xmalloc(size+1);
			ValueFixedLength(value) = size;
		}
		memclear(ValueFixedBody(value),ValueFixedLength(value)+1);
		ValueFixedSlen(value) = RecvLength(fp);
		RecvString(fp,ValueFixedBody(value));
		break;
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			RecvValueBody(fp,ValueArrayItem(value,i));
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			RecvValueBody(fp,ValueRecordItem(value,i));
		}
		break;
	  default:
		break;
	}
	xfree(recvBuffer);
}

/*
 *	This function sends value with valiable name.
 */
extern	void
SendValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*locale)
{
	int		i;

	ValueIsUpdate(value);
	SendDataType(fp,ValueType(value));
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		SendInt(fp,ValueInteger(value));
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,ValueBool(value));
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		SendString(fp,ValueToString(value,locale));
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,ValueFloat(value));
		break;
	  case	GL_TYPE_NUMBER:
		SendFixed(fp,&ValueFixed(value));
		break;
	  case	GL_TYPE_ARRAY:
		SendInt(fp,ValueArraySize(value));
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			SendValue(fp,ValueArrayItem(value,i),locale);
		}
		break;
	  case	GL_TYPE_RECORD:
		SendInt(fp,ValueRecordSize(value));
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			SendString(fp,ValueRecordName(value,i));
			SendValue(fp,ValueRecordItem(value,i),locale);
		}
		break;
	  default:
		break;
	}
}

