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
#include	"glcomms.h"
#include	"blobcache.h"
#include	"message.h"
#include	"debug.h"

#ifdef	__APPLE__
#include	<machine/endian.h>
#else
#include	<endian.h>
#endif

static	LargeByteString	*Buff = NULL;

static void
CheckBuff() {
	if (Buff == NULL) {
		Buff = NewLBS();
	}
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
#if 0
		MessageLogPrintf("could not open for read: %s\n", fname);
#endif
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
	CheckBuff();
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
	CheckBuff();
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
