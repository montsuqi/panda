/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan.

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
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<glib.h>
#include	<time.h>
#include	<sys/time.h>
#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"cgi.h"
#include	"HTClex.h"
#include	"htc.h"
#include	"debug.h"

static	GET_VALUE	_GetValue;

extern	char	*
GetHostValue(
	char	*name,
	Bool	fClear)
{
	char	*value;
    ValueStruct *val;

ENTER_FUNC;
dbgprintf("name = [%s]\n",name);
	if		(  ( value = LoadValue(name) )  ==  NULL  )	{
#ifdef	_INC_VALUE_H
		if		(  _GetValue  !=  NULL  ) {
			if		(  ( val = (_GetValue)(name, fClear) )  ==  NULL  ) {
				value = "";
			} else {
				value = ValueToString(val, NULL);
			}
			value = SaveValue(name,value,FALSE);
		} else {
			value = "";
		}
#else
		value = "";
#endif
	}
dbgprintf("value = [%s]\n",value);
LEAVE_FUNC;
	return	(value);
}

extern	void
InitHTC(
	char	*script_name,
	GET_VALUE	func)
{
ENTER_FUNC;
    if (script_name == NULL) {
        script_name = "mon.cgi";
    }
	HTCLexInit();
	TagsInit(script_name);
	Codeset = "utf-8";
	_GetValue = func;
LEAVE_FUNC;
}


