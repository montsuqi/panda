/*
 * PANDA -- a simple transaction monitor
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
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<glib.h>
#include	<time.h>
#include	<sys/time.h>
#include	"const.h"
#include	"types.h"
#include	"cgi.h"
#include	"htc.h"
#include	"HTClex.h"
#include	"exec.h"
#include	"debug.h"

static	GET_VALUE	_GetValue;
static	Bool		fClear;

static	char	*
ValueSymbol(
	char	*name)
{
	char	*value;
	ValueStruct			*item;

	dbgprintf("name = [%s]\n",name);
	if		(  ( value = LoadValue(name) )  ==  NULL  )	{
		if		(  _GetValue  !=  NULL  ) {
			item = (_GetValue)(name, fClear);
			if		(  item  !=  NULL  ) {
				value = ValueToString(item, NULL);
			} else {
				value ="";
			}
			SaveValue(name,value,FALSE);
		} else {
			fprintf(stderr,"mon bug\n");
			exit(1);
		}
	}
	dbgprintf("value = [%s]\n",value);
	return	(value);
}

extern	char	*
GetHostValue(
	char	*name,
	Bool	_fClear)
{
	char	*value;

ENTER_FUNC;
dbgprintf("name = [%s]\n",name);
	fClear = _fClear;
	value = ValueSymbol(name);
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
	JslibInit();

	TagsInit(script_name);
	Codeset = "utf-8";
	_GetValue = func;
LEAVE_FUNC;
}

