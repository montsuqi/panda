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
#include	"glcomm.h"
#include	"debug.h"

static	LargeByteString	*Buff;

extern	void
GL_SendPacketClass(
	NETFILE	*fp,
	PacketClass	c,
	Bool	fNetwork)
{
#ifdef	DEBUG
	printf("SendPacketClass = %X\n",c);
#endif
	nputc(c,fp);
}

extern	PacketClass
GL_RecvPacketClass(
	NETFILE	*fp,
	Bool	fNetwork)
{
	PacketClass	c;

	c = ngetc(fp);
#ifdef	DEBUG
	printf("RecvPacketClass = %X\n",c);
#endif
	return	(c);
}

extern	void
GL_SendInt(
	NETFILE	*fp,
	int		data,
	Bool	fNetwork)
{
	if		(  fNetwork  ) {
		SendChar(fp,((data&0xFF000000) >> 24));
		SendChar(fp,((data&0x00FF0000) >> 16));
		SendChar(fp,((data&0x0000FF00) >>  8));
		SendChar(fp,((data&0x000000FF)      ));
	} else {
		Send(fp,&data,sizeof(data));
	}
}

static	void
GL_SendUInt(
	NETFILE	*fp,
	unsigned	int		data,
	Bool	fNetwork)
{
	if		(  fNetwork  ) {
		SendChar(fp,((data&0xFF000000) >> 24));
		SendChar(fp,((data&0x00FF0000) >> 16));
		SendChar(fp,((data&0x0000FF00) >>  8));
		SendChar(fp,((data&0x000000FF)      ));
	} else {
		Send(fp,&data,sizeof(data));
	}
}

extern	void
GL_SendLong(
	NETFILE	*fp,
	long	data,
	Bool	fNetwork)
{
	if		(  fNetwork  ) {
		SendChar(fp,((data&0xFF000000) >> 24));
		SendChar(fp,((data&0x00FF0000) >> 16));
		SendChar(fp,((data&0x0000FF00) >>  8));
		SendChar(fp,((data&0x000000FF)      ));
	} else {
		Send(fp,&data,sizeof(data));
	}
}

static	void
GL_SendLength(
	NETFILE	*fp,
	size_t	size,
	Bool	fNetwork)
{
	if		(  fNetwork  ) {
		SendChar(fp,((size&0xFF000000) >> 24));
		SendChar(fp,((size&0x00FF0000) >> 16));
		SendChar(fp,((size&0x0000FF00) >>  8));
		SendChar(fp,((size&0x000000FF)      ));
	} else {
		Send(fp,&size,sizeof(size_t));
	}
}

extern	int
GL_RecvInt(
	NETFILE	*fp,
	Bool	fNetwork)
{
	int		data;

	if		(  fNetwork  ) {
		data =	( RecvChar(fp) << 24 )
			|	( RecvChar(fp) << 16 )
			|	( RecvChar(fp) <<  8 )
			|	( RecvChar(fp)       );
	} else {
		Recv(fp,&data,sizeof(data));
	}
	return	(data);
}

static	size_t
GL_RecvLength(
	NETFILE	*fp,
	Bool	fNetwork)
{
	size_t	size;

	if		(  fNetwork  ) {
		size =	( RecvChar(fp) << 24 )
			|	( RecvChar(fp) << 16 )
			|	( RecvChar(fp) <<  8 )
			|	( RecvChar(fp)       );
	} else {
		Recv(fp,&size,sizeof(size_t));
	}
	return	(size);
}

extern	void
GL_RecvString(
	NETFILE	*fp,
	char	*str,
	Bool	fNetwork)
{
	size_t	size;

dbgmsg(">RecvString");
	size = GL_RecvLength(fp,fNetwork);
	Recv(fp,str,size);
	str[size] = 0;
dbgmsg("<RecvString");
}

static	void
GL_SendLBS(
	NETFILE	*fp,
	LargeByteString	*lbs,
	Bool	fNetwork)
{
	GL_SendLength(fp,LBS_Size(lbs),fNetwork);
	if		(  LBS_Size(lbs)  >  0  ) {
		Send(fp,LBS_Body(lbs),LBS_Size(lbs));
	}
}

extern	void
GL_SendString(
	NETFILE	*fp,
	char	*str,
	Bool	fNetwork)
{
	size_t	size;

dbgmsg(">GL_SendString");
#ifdef	DEBUG
	printf("[%s]\n",str);
#endif
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size,fNetwork);
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
dbgmsg("<GL_SendString");
}

static	void
GL_SendObject(
	NETFILE	*fp,
	MonObjectType	*obj,
	Bool	fNetwork)
{
	int		i;

	GL_SendInt(fp,obj->source,fNetwork);
	for	( i = 0 ; i < SIZE_OID/sizeof(unsigned int) ; i ++ ) {
		GL_SendUInt(fp,obj->id.el[i],fNetwork);
	}
}

extern	Fixed	*
GL_RecvFixed(
	NETFILE	*fp,
	Bool	fNetwork)
{
	Fixed	*xval;

dbgmsg(">RecvFixed");
	xval = New(Fixed);
	xval->flen = GL_RecvLength(fp,fNetwork);
	xval->slen = GL_RecvLength(fp,fNetwork);
	xval->sval = (char *)xmalloc(xval->flen+1);
	GL_RecvString(fp,xval->sval,fNetwork);
dbgmsg("<RecvFixed");
	return	(xval); 
}

static	void
GL_SendFixed(
	NETFILE	*fp,
	Fixed	*xval,
	Bool	fNetwork)
{
	GL_SendLength(fp,xval->flen,fNetwork);
	GL_SendLength(fp,xval->slen,fNetwork);
	GL_SendString(fp,xval->sval,fNetwork);
}

extern	double
GL_RecvFloat(
	NETFILE	*fp,
	Bool	fNetwork)
{
	double	data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

static	void
GL_SendFloat(
	NETFILE	*fp,
	double	data,
	Bool	fNetwork)
{
	Send(fp,&data,sizeof(data));
}

extern	Bool
GL_RecvBool(
	NETFILE	*fp,
	Bool	fNetwork)
{
	char	buf[1];

	Recv(fp,buf,1);
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

static	void
GL_SendBool(
	NETFILE	*fp,
	Bool	data,
	Bool	fNetwork)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
}

static	void
GL_SendDataType(
	NETFILE	*fp,
	PacketClass	c,
	Bool	fNetwork)
{
#ifdef	DEBUG
	printf("SendDataType = %X\n",c);
#endif
	nputc(c,fp);
}

extern	PacketDataType
GL_RecvDataType(
	NETFILE	*fp,
	Bool	fNetwork)
{
	PacketClass	c;

	c = ngetc(fp);
#ifdef	DEBUG
	printf("RecvDataType = %X\n",c);
#endif
	return	(c);
}

/*
 *	This function sends value with valiable name.
 */
extern	void
GL_SendValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*coding,
	Bool		fExpand,
	Bool		fNetwork)
{
	int		i;

	ValueIsNotUpdate(value);
	GL_SendDataType(fp,ValueType(value),fNetwork);
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		GL_SendInt(fp,ValueInteger(value),fNetwork);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,ValueBool(value),fNetwork);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
		LBS_ReserveSize(Buff,ValueByteLength(value),FALSE);
		memcpy(LBS_Body(Buff),ValueByte(value),ValueByteLength(value));
		GL_SendLBS(fp,Buff,fNetwork);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,ValueToString(value,coding),fNetwork);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,ValueFloat(value),fNetwork);
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFixed(fp,&ValueFixed(value),fNetwork);
		break;
	  case	GL_TYPE_OBJECT:
		if		(  fExpand  ) {
		} else {
			GL_SendObject(fp,ValueObject(value),fNetwork);
		}
		break;
	  case	GL_TYPE_ARRAY:
		GL_SendInt(fp,ValueArraySize(value),fNetwork);
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			GL_SendValue(fp,ValueArrayItem(value,i),coding,fExpand,fNetwork);
		}
		break;
	  case	GL_TYPE_RECORD:
		GL_SendInt(fp,ValueRecordSize(value),fNetwork);
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			GL_SendString(fp,ValueRecordName(value,i),fNetwork);
			GL_SendValue(fp,ValueRecordItem(value,i),coding,fExpand,fNetwork);
		}
		break;
	  default:
		break;
	}
}

extern	void
InitGL_Comm(void)
{
	Buff = NewLBS();
}
