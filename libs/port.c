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
	switch	(port->type) {
	  case	PORT_IP:
		if		(  port->adrs.a_ip.port  !=  NULL  ) {
			xfree(port->adrs.a_ip.port);
		}
		if		(  port->adrs.a_ip.host  !=  NULL  ) {
			xfree(port->adrs.a_ip.host);
		}
		break;
	}
	xfree(port);
}

extern	Port	*
NewIP_Port(
	char	*host,
	char	*portnum)
{
	Port	*port;

	port = New(Port);
	port->type = PORT_IP;
	if		(	(  host   ==  NULL  )
			||	(  *host  ==  0     ) ) {
		port->adrs.a_ip.host = StrDup("localhost");
	} else {
		port->adrs.a_ip.host = StrDup(host);
	}
	port->adrs.a_ip.port = StrDup(portnum);
	return	(port);
}

extern	Port	*
NewUNIX_Port(
	char	*name,
	int		mode)
{
	Port	*port;

	port = New(Port);
	port->type = PORT_UNIX;
	port->adrs.a_unix.name = StrDup(name);
	port->adrs.a_unix.mode = mode;
	return	(port);
}

static	int
otoi(
	char	*str)
{
	int		ret;

	if		(  *str  ==  '0'  ) {
		str ++;
		ret = 0;
		while	(  *str  !=  0  ) {
			ret = ret * 8 + *str - '0';
			str ++;
		}
	} else {
		ret = atoi(str);
	}
	return	(ret);
}

extern	Port	*
ParPort(
	char	*str,
	char	*def)
{
	Port	*ret;
	char	*p;
	int		mode;
	char	dup[SIZE_LONGNAME+1];

	if		(  str  ==  NULL  ) {
		ret = NULL;
	} else {
		strncpy(dup,str,SIZE_LONGNAME);
		if		(  dup[0]  ==  '#'  ) {
			if		(  ( p = strchr(&dup[1],':') )  !=  NULL  ) {
				*p = 0;
				mode = otoi(p+1);
			} else {
				mode = 0666;
			}
			ret = NewUNIX_Port(&dup[1],mode);
		} else
			if		(	(  dup[0]  ==  '/'  )
					||	(  dup[0]  ==  '.'  ) ) {
			if		(  ( p = strchr(dup,':') )  !=  NULL  ) {
				*p = 0;
				mode = otoi(p+1);
			} else {
				mode = 0666;
			}
			ret = NewUNIX_Port(dup,mode);
		} else
		if		(  dup[0]  ==  '['  ) {
			if		(  ( p = strchr(dup,']') )  !=  NULL  ) {
				*p = 0;
				p ++;
				if		(  *p  ==  ':'  ) {
					ret = NewIP_Port(&dup[1],p+1);
				} else {
					ret = NewIP_Port(&dup[1],def);
				}
			} else {
				ret = NewIP_Port(&dup[1],def);
			}
		} else {
			if		(  ( p = strchr(dup,':') )  !=  NULL  ) {
				*p = 0;
				ret = NewIP_Port(dup,p+1);
			} else {
				ret = NewIP_Port(dup,def);
			}
		}
	}
	return	(ret);
}

extern	Port	*
ParPortName(
	char	*str)
{
	Port	*ret;
	int		mode;
	char	*p;

	if		(  str  ==  NULL  ) {
		ret = NULL;
	} else {
		if		(  *str  ==  '#'  ) {
			if		(  ( p = strchr(str+1,':') )  !=  NULL  ) {
				*p = 0;
				mode = otoi(p+1);
			} else {
				mode = 0666;
			}
			ret = NewUNIX_Port(str+1,mode);
		} else
			if		(	(  *str  ==  '/'  )
					||	(  *str  ==  '.'  ) ) {
			if		(  ( p = strchr(str,':') )  !=  NULL  ) {
				*p = 0;
				mode = otoi(p+1);
			} else {
				mode = 0666;
			}
			ret = NewUNIX_Port(str,mode);
		} else
		if		(  *str  ==  ':'  ) {
			ret = NewIP_Port(NULL,str+1);
		} else {
			ret = NewIP_Port(NULL,str);
		}
	}
	return	(ret);
}

extern	char	*
StringPort(
	Port	*port)
{
	static	char	buff[SIZE_LONGNAME];

	switch	(port->type) {
	  case	PORT_IP:
		if		(  IP_HOST(port)  !=  NULL  ) {
			sprintf(buff,"%s:%s",IP_HOST(port),IP_PORT(port));
		} else {
			sprintf(buff,":%s",IP_PORT(port));
		}
		break;
	  case	PORT_UNIX:
		if		(	(  *UNIX_NAME(port)  ==  '/'  )
				||	(  *UNIX_NAME(port)  ==  '.'  ) ) {
			sprintf(buff,"%s:0%o",UNIX_NAME(port),UNIX_MODE(port));
		} else {
			sprintf(buff,"#%s:0%o",UNIX_NAME(port),UNIX_MODE(port));
		}
		break;
	  default:
		*buff = 0;
		break;
	}
	return	(buff);
}

extern	char	*
StringPortName(
	Port	*port)
{
	static	char	buff[SIZE_LONGNAME];

	switch	(port->type) {
	  case	PORT_IP:
		sprintf(buff,"%s",IP_PORT(port));
		break;
	  case	PORT_UNIX:
		if		(	(  *UNIX_NAME(port)  ==  '/'  )
				||	(  *UNIX_NAME(port)  ==  '.'  ) ) {
			sprintf(buff,"%s:0%o",UNIX_NAME(port),UNIX_MODE(port));
		} else {
			sprintf(buff,"#%s:0%o",UNIX_NAME(port),UNIX_MODE(port));
		}
		break;
	  default:
		*buff = 0;
		break;
	}
	return	(buff);
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
	port = ParPort(str,NULL);
	url->host = StrDup(port->adrs.a_ip.host);
	url->port = StrDup(port->adrs.a_ip.port);
	DestroyPort(port);
	if		(  p  !=  NULL  ) {
		url->file = StrDup(p+1);
	} else {
		url->file = "";
	}
}

