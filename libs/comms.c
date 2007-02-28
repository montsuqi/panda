/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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
#include	<strings.h>
#include	<glib.h>
#include	<math.h>

#include	"types.h"
#include	"libmondai.h"
#include	"comm.h"
#define	_COMMS
#include	"comms.h"
#include	"debug.h"

extern	void
SendStringDelim(
	NETFILE	*fp,
	char	*str)
{
	size_t	size;

	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
}

extern	void
SendLargeString(
	NETFILE			*fp,
	LargeByteString	*lbs)
{
	size_t	size;
	char	*str;

dbgmsg(">SendLargeString");
	str = LBS_Body(lbs);
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
dbgmsg("<SendLargeString");
}

extern	Bool
RecvStringDelim(
	NETFILE	*fp,
	size_t	size,
	char	*str)
{
	Bool	rc;
	int		c;
	char	*p;

	p = str;
	while	(	(  ( c = RecvChar(fp) )  >=  0     )
			&&	(  c                     !=  '\n'  ) )	{
		*p ++ = c;
		size --;
		if		(  size  ==  0  )	break;
	}
	*p = 0;
	if		(  c  >=  0  ) {
		StringChop(str);
		rc = TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

extern	Bool
RecvLargeString(
	NETFILE			*fp,
	LargeByteString	*lbs)
{
	Bool	rc;
	int		c;

ENTER_FUNC;
	LBS_Clear(lbs);
	while	(	(  ( c = RecvChar(fp) )  >=  0     )
			&&	(  c                     !=  '\n'  ) )	{
		LBS_EmitChar(lbs,c);
	}
	LBS_EmitChar(lbs,0);
	if		(  c  >=  0  ) {
		rc = TRUE;
	} else {
		rc = FALSE;
	}
dbgmsg("<RecvLargeString");
	return	(rc);
}

static	char	namebuff[SIZE_BUFF+1];

extern	void
SetValueName(
	char	*name)
{
	strcpy(namebuff,name);
}

extern	void
SendValueString(
	NETFILE		*fpComm,
	ValueStruct	*value,
	char		*name,
	Bool		fName,
	Bool		fType,
	char		*encode)
{
	char	buff[SIZE_BUFF+1];
	int		i;

ENTER_FUNC;
	if		(  name  ==  NULL  ) { 
		name = namebuff + strlen(namebuff);
	}
	switch	(ValueType(value)) {
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			sprintf(name,"[%d]",i);
			SendValueString(fpComm,
							ValueArrayItem(value,i),name+strlen(name),fName,fType,encode);
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			sprintf(name,".%s",ValueRecordName(value,i));
			SendValueString(fpComm,
							ValueRecordItem(value,i),name+strlen(name),fName,fType,encode);
		}
		break;
	  default:
		if		(  fName  ) {
			SendStringDelim(fpComm,namebuff);
			if		(  fType  ) {
				SendStringDelim(fpComm,";");
				switch	(ValueType(value)) {
				  case	GL_TYPE_INT:
					sprintf(buff,"integer");
					break;
				  case	GL_TYPE_FLOAT:
					sprintf(buff,"float");
					break;
				  case	GL_TYPE_BOOL:
					sprintf(buff,"Bool");
					break;
				  case	GL_TYPE_CHAR:
					sprintf(buff,"char(%d)",(int)ValueStringLength(value));
					break;
				  case	GL_TYPE_VARCHAR:
					sprintf(buff,"varchar(%d)",(int)ValueStringLength(value));
					break;
				  case	GL_TYPE_DBCODE:
					sprintf(buff,"code(%d)",(int)ValueStringLength(value));
					break;
				  case	GL_TYPE_NUMBER:
					sprintf(buff,"number(%d,%d)",
							(int)ValueFixedLength(value),(int)ValueFixedSlen(value));
					break;
				  case	GL_TYPE_TEXT:
					sprintf(buff,"text");
					break;
				  case	GL_TYPE_OBJECT:
					sprintf(buff,"object");
					break;
				  default:
					*buff = 0;
					break;
				}
				SendStringDelim(fpComm,buff);
			}
			SendStringDelim(fpComm,": ");
		}
		EncodeStringURL(buff,ValueToString(value,encode));
		SendStringDelim(fpComm,buff);
		SendStringDelim(fpComm,"\n");
		break;
	}
LEAVE_FUNC;
}

