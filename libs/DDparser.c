/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
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
#include	<ctype.h>
#include	<glib.h>
#include	<sys/stat.h>	/*	for stbuf	*/
#include	"types.h"
#include	<libmondai.h>
#include	<RecParser.h>
#include	"struct.h"
#include	"monstring.h"
#include	"DDparser.h"
#include	"debug.h"


extern	RecordStruct	*
DD_Parse(
	CURFILE	*in)
{
	RecordStruct	*ret;
	ValueStruct		*value;

ENTER_FUNC;
	if		(  ( value = RecParseMain(in) )  !=  NULL  ) {
		ret = New(RecordStruct);
		ret->value = value;
		ret->name = StrDup(in->ValueName);
		ret->type = RECORD_NULL;
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	RecordStruct	*
ParseRecordFile(
	char	*name)
{
	RecordStruct	*ret;
	ValueStruct		*value;
	char			*ValueName;

ENTER_FUNC;
	dbgprintf("name = [%s]\n",name);
	if		(  ( value = RecParseValue(name,&ValueName) )  !=  NULL  ) {
		ret = New(RecordStruct);
		ret->value = value;
		ret->name = StrDup(ValueName);
		ret->type = RECORD_NULL;
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	RecordStruct	*
ParseRecordMem(
	char	*mem)
{
	RecordStruct	*ret;
	ValueStruct		*value;
	char			*ValueName;

ENTER_FUNC;
	if		(  ( value = RecParseValueMem(mem,&ValueName) )  !=  NULL  ) {
		ret = New(RecordStruct);
		ret->value = value;
		ret->name = StrDup(ValueName);
		ret->type = RECORD_NULL;
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	RecordStruct	*
ReadRecordDefine(
	char	*name)
{
	RecordStruct	*rec;
	char		buf[SIZE_LONGNAME+1]
	,			dir[SIZE_LONGNAME+1];
	char		*p
	,			*q;
	Bool		fExit;

ENTER_FUNC;
	strcpy(dir,RecordDir);
	p = dir;
	rec = NULL;
	do {
		if		(  ( q = strchr(p,':') )  !=  NULL  ) {
			*q = 0;
			fExit = FALSE;
		} else {
			fExit = TRUE;
		}
		sprintf(buf,"%s/%s",p,name);
		if		(  ( rec = ParseRecordFile(buf) )  !=  NULL  ) {
			break;
		}
		p = q + 1;
	}	while	(  !fExit  );
#if	0
	if		(  rec  ==  NULL  ) {
		rec = New(RecordStruct);
		rec->name = StrDup(name);
		rec->value = NULL;
		rec->type = RECORD_NULL;
	}
#endif
LEAVE_FUNC;
	return	(rec);
}

