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
#define	_COMMS
#include	"comms.h"
#include	"debug.h"

extern	void
SendStringDelim(
	FILE	*fp,
	char	*str)
{
	size_t	size;

#ifdef	DEBUG
	printf(">>[%s]\n",str);
#endif
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	if		(  size  >  0  ) {
		fwrite(str,1,size,fp);
	}
	fflush(fp);
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
	FILE		*fpComm,
	ValueStruct	*value,
	char		*name,
	Bool		fName)
{
	char	buff[SIZE_BUFF+1];
	int		i;

dbgmsg(">SendValueString");
	if		(  name  ==  NULL  ) { 
		name = namebuff + strlen(namebuff);
	}
	switch	(value->type) {
	  case	GL_TYPE_ARRAY:
		for	( i = 0 ; i < value->body.ArrayData.count ; i ++ ) {
			sprintf(name,"[%d]",i);
			SendValueString(fpComm,
							value->body.ArrayData.item[i],name+strlen(name),fName);
		}
		break;
	  case	GL_TYPE_RECORD:
		for	( i = 0 ; i < value->body.RecordData.count ; i ++ ) {
			sprintf(name,".%s",value->body.RecordData.names[i]);
			SendValueString(fpComm,
							value->body.RecordData.item[i],name+strlen(name),fName);
		}
		break;
	  default:
		if		(  fName  ) {
			SendStringDelim(fpComm,namebuff);
			SendStringDelim(fpComm,": ");
		}
		EncodeString(buff,ToString(value));
		SendStringDelim(fpComm,buff);
		SendStringDelim(fpComm,"\n");
		break;
	}
dbgmsg("<SendValueString");
}

