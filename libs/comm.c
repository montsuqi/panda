/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002 Ogochan & JMA (Japan Medical Association).

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
#include	<string.h>
#include	<strings.h>
#include	<glib.h>
#include	<math.h>

#include	"types.h"
#include	"misc.h"
#include	"value.h"
#include	"LBSfunc.h"
#include	"glterm.h"
#define	_COMM
#include	"comm.h"
#include	"debug.h"

extern	void
SendPacketClass(
	FILE	*fp,
	PacketClass	c)
{
#ifdef	DEBUG
	printf("SendPacketClass = %X\n",c);
#endif
	fputc(c,fp);
	fflush(fp);
}

extern	PacketClass
RecvPacketClass(
	FILE	*fp)
{
	PacketClass	c;

	c = fgetc(fp);
#ifdef	DEBUG
	printf("RecvPacketClass = %X\n",c);
#endif
	return	(c);
}

extern	void
SendDataType(
	FILE	*fp,
	PacketClass	c)
{
#ifdef	DEBUG
	printf("SendDataType = %X\n",c);
#endif
	fputc(c,fp);
}

extern	PacketDataType
RecvDataType(
	FILE	*fp)
{
	PacketClass	c;

	c = fgetc(fp);
#ifdef	DEBUG
	printf("RecvDataType = %X\n",c);
#endif
	return	(c);
}

extern	void
SendPacketDataType(
	FILE	*fp,
	PacketDataType	t)
{
	fputc(t,fp);
}

extern	PacketDataType
RecvPacketDataType(
	FILE	*fp)
{
	return	(fgetc(fp));
}

extern	void
SendLength(
	FILE	*fp,
	size_t	size)
{
	fwrite(&size,sizeof(size),1,fp);
}

extern	size_t
RecvLength(
	FILE	*fp)
{
	size_t	size;

dbgmsg(">RecvLength");
	fread(&size,sizeof(size),1,fp);
#ifdef	DEBUG
	printf("[%d]\n",size);
#endif
dbgmsg("<RecvLength");
	return	(size);
}

extern	void
SendString(
	FILE	*fp,
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
		fwrite(str,1,size,fp);
	}
dbgmsg("<SendString");
}

extern	void
SendFixedString(
	FILE	*fp,
	char	*str,
	size_t	size)
{
dbgmsg(">SendFixedString");
#ifdef	DEBUG
	printf("[%s]\n",str);
#endif
	SendLength(fp,size);
	if		(  size  >  0  ) {
		fwrite(str,1,size,fp);
	}
dbgmsg("<SendFixedString");
}

extern	void
SendLBS(
	FILE	*fp,
	LargeByteString	*lbs)
{
	SendLength(fp,lbs->ptr);
	if		(  lbs->ptr  >  0  ) {
		fwrite(lbs->body,1,lbs->ptr,fp);
	}
}

extern	void
RecvLBS(
	FILE	*fp,
	LargeByteString	*lbs)
{
	size_t	size;

	size = RecvLength(fp);
	LBS_RequireSize(lbs,size,FALSE);
	if		(  size  >  0  ) {
		fread(lbs->body,1,size,fp);
	}
	lbs->ptr = size;
}	

extern	void
RecvStringBody(
	FILE	*fp,
	char	*str,
	size_t	size)
{
dbgmsg(">RecvStringBody");
	fread(str,1,size,fp);
	str[size] = 0;
#ifdef	DEBUG
	printf("[%s]\n",str);
#endif
dbgmsg("<RecvStringBody");
}

extern	void
RecvString(
	FILE	*fp,
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
	FILE	*fp)
{
	long	data;

	fread(&data,sizeof(data),1,fp);
	return	(data);
}

extern	void
SendLong(
	FILE	*fp,
	long	data)
{
	fwrite(&data,sizeof(data),1,fp);
}

extern	int
RecvInt(
	FILE	*fp)
{
	int		data;

	fread(&data,sizeof(data),1,fp);
	return	(data);
}

extern	void
SendInt(
	FILE	*fp,
	int		data)
{
	fwrite(&data,sizeof(data),1,fp);
}

extern	char
RecvChar(
	FILE	*fp)
{
	char	data;

	fread(&data,sizeof(data),1,fp);
	return	(data);
}

extern	void
SendChar(
	FILE	*fp,
	char	data)
{
	fwrite(&data,sizeof(data),1,fp);
}

extern	double
RecvFloat(
	FILE	*fp)
{
	double	data;

	fread(&data,sizeof(data),1,fp);
	return	(data);
}

extern	void
SendFloat(
	FILE	*fp,
	double	data)
{
	fwrite(&data,sizeof(data),1,fp);
}

extern	Bool
RecvBool(
	FILE	*fp)
{
	char	buf[1];

	fread(buf,1,1,fp);
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

extern	void
SendBool(
	FILE	*fp,
	Bool	data)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	fwrite(buf,1,1,fp);
}

extern	void
SendFixed(
	FILE	*fp,
	Fixed	*xval)
{
	SendLength(fp,xval->flen);
	SendLength(fp,xval->slen);
	SendString(fp,xval->sval);
}

extern	Fixed	*
RecvFixed(
	FILE	*fp)
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
	FILE	*fp,
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
	FILE			*fp,
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
	FILE			*fp,
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
	FILE	*fp,
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
	FILE			*fp,
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
	FILE	*fp,
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
	FILE			*fp,
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
	FILE	*fp,
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
	FILE			*fp,
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
	FILE	*fp,
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
	FILE		*fp,
	ValueStruct	*value)
{
	int		i;

	if		(  value  ==  NULL  )	return;
	SendDataType(fp,value->type);
	switch	(value->type) {
	  case	GL_TYPE_INT:
		SendInt(fp,value->body.IntegerData);
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,value->body.FloatData);
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,value->body.BoolData);
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		SendFixedString(fp,value->body.CharData.sval,value->body.CharData.len);
		break;
	  case	GL_TYPE_NUMBER:
		SendFixed(fp,&value->body.FixedData);
		break;
	  case	GL_TYPE_TEXT:
		SendString(fp,value->body.CharData.sval);
		break;
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
			SendValueBody(fp,value->body.ArrayData.item[i]);
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
			SendValueBody(fp,value->body.RecordData.item[i]);
		}
		break;
	  default:
		break;
	}
}

extern	void
RecvValueBody(
	FILE		*fp,
	ValueStruct	*value)
{
	int		i;
	PacketDataType	type;
	size_t	size;

	if		(  value  ==  NULL  )	return;
	type = RecvDataType(fp);
	if		(  type  !=  value->type  ) {
		fprintf(stderr,"fatal type miss match\n");
		exit(1);
	}
	switch	(type) {
	  case	GL_TYPE_INT:
		value->body.IntegerData = RecvInt(fp);
		break;
	  case	GL_TYPE_FLOAT:
		value->body.FloatData = RecvFloat(fp);
		break;
	  case	GL_TYPE_BOOL:
		value->body.BoolData = RecvBool(fp);
		break;
	  case	GL_TYPE_TEXT:
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		size = RecvLength(fp);
		if		(  type  ==  GL_TYPE_TEXT  ) {
			if		(  size  >  value->body.CharData.len  ) {
				xfree(value->body.CharData.sval);
				value->body.CharData.sval = (char *)xmalloc(size+1);
				value->body.CharData.len = size;
			}
			memclear(value->body.CharData.sval,value->body.CharData.len+1);
		}
		RecvStringBody(fp,value->body.CharData.sval,size);
		break;
	  case	GL_TYPE_NUMBER:
		size = RecvLength(fp);
		if		(  size  >  value->body.FixedData.flen  ) {
			xfree(value->body.FixedData.sval);
			value->body.FixedData.sval = (char *)xmalloc(size+1);
			value->body.FixedData.flen = size;
		}
		memclear(value->body.FixedData.sval,value->body.FixedData.flen+1);
		value->body.FixedData.slen = RecvLength(fp);
		RecvString(fp,value->body.FixedData.sval);
		break;
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
			RecvValueBody(fp,value->body.ArrayData.item[i]);
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
			RecvValueBody(fp,value->body.RecordData.item[i]);
		}
		break;
	  default:
		break;
	}
}

/*
 *	This function sends value with valiable name.
 */
extern	void
SendValue(
	FILE		*fp,
	ValueStruct	*value)
{
	int		i;

	value->fUpdate = TRUE;
	SendDataType(fp,value->type);
	switch	(value->type) {
	  case	GL_TYPE_INT:
		SendInt(fp,value->body.IntegerData);
		break;
	  case	GL_TYPE_BOOL:
		SendBool(fp,value->body.BoolData);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		SendString(fp,value->body.CharData.sval);
		break;
	  case	GL_TYPE_FLOAT:
		SendFloat(fp,value->body.FloatData);
		break;
	  case	GL_TYPE_NUMBER:
		SendFixed(fp,&value->body.FixedData);
		break;
	  case	GL_TYPE_ARRAY:
		SendInt(fp,value->body.ArrayData.count);
		for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
			SendValue(fp,value->body.ArrayData.item[i]);
		}
		break;
	  case	GL_TYPE_RECORD:
		SendInt(fp,value->body.RecordData.count);
		for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
			SendString(fp,value->body.RecordData.names[i]);
			SendValue(fp,value->body.RecordData.item[i]);
		}
		break;
	  default:
		break;
	}
}

