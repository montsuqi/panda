/*	PANDA -- a simple transaction monitor

Copyright (C) 2003 Ogochan & JMA (Japan Medical Association).

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

#include	"types.h"
#include	"libmondai.h"
#include	"port.h"
#include	"debug.h"

extern	void
DestroyPort(
	Port	*port)
{
	if		(  port->port  !=  NULL  ) {
		xfree(port->port);
	}
	if		(  port->host  !=  NULL  ) {
		xfree(port->host);
	}
	xfree(port);
}

extern	Port	*
ParPort(
	char	*str,
	int		def)
{
	Port	*ret;
	char	*p;
	char	dup[SIZE_LONGNAME+1];

	strncpy(dup,str,SIZE_LONGNAME);
	ret = New(Port);
	if		(  dup[0]  ==  '['  ) {
		if		(  ( p = strchr(dup,']') )  !=  NULL  ) {
			*p = 0;
			ret->host = StrDup(&dup[1]);
			p ++;
			if		(  *p  ==  ':'  ) {
				ret->port = StrDup(p+1);
			} else {
				if		(  def  <  0  ) {
					ret->port = NULL;
				} else {
					ret->port = IntStrDup(def);
				}
			}
		}
	} else {
		if		(  ( p = strchr(dup,':') )  !=  NULL  ) {
			*p = 0;
			ret->host = StrDup(dup);
			ret->port = StrDup(p+1);
		} else {
			ret->host = StrDup(dup);
			if		(  def  <  0  ) {
				ret->port = NULL;
			} else {
				ret->port = IntStrDup(def);
			}
		}
	}
	if		(  *ret->host  ==  0  ) {
		ret->host = "localhost";
	}
	return	(ret);
}

extern	void
ParseURL(
	URL		*url,
	char	*instr)
{
	char	*p
	,		*str;
	char	buff[256];
	Port	*port;

	strcpy(buff,instr);
	str = buff;
	if		(  ( p = strchr(str,':') )  ==  NULL  ) {
		url->protocol = StrDup("http");
	} else {
		*p = 0;
		url->protocol = StrDup(str);
		str = p + 1;
	}
	if		(  *str  ==  '/'  ) {
		str ++;
	}
	if		(  *str  ==  '/'  ) {
		str ++;
	}
	if		(  ( p = strchr(str,'/') )  !=  NULL  ) {
		*p = 0;
	}
	port = ParPort(str,-1);
	url->host = StrDup(port->host);
	url->port = StrDup(port->port);
	DestroyPort(port);
	if		(  p  !=  NULL  ) {
		url->file = StrDup(p+1);
	} else {
		url->file = "";
	}
}

