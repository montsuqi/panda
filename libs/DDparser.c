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

