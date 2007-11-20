/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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

	if		(	(  str   ==  NULL  )
			||	(  *str  ==  0     ) ) {
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

	if ( port != NULL) {
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
	} else {
		*buff = 0;
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

extern	URL	*
NewURL(void)
{
	URL		*ret;

	ret = New(URL);
	ret->protocol = NULL;
	ret->host = NULL;
	ret->port = NULL;
	ret->file = NULL;

	return	(ret);
}

extern	void
FreeURL(
	URL		*url)
{
	if		(  url  !=  NULL  ) {
		xfree(url->protocol);
		xfree(url->host);
		xfree(url->port);
		xfree(url->file);
		xfree(url);
	}
}

extern	URL	*
DuplicateURL(
	URL		*src)
{
	URL		*ret;

	if		(  src  !=  NULL  ) {
		ret = NewURL();
		ret->protocol = StrDup(src->protocol);
		ret->host = StrDup(src->host);
		ret->port = StrDup(src->port);
		ret->file = StrDup(src->file);
	} else {
		ret = NULL;
	}

	return	(ret);
}

extern	void
ParseURL(
	URL		*url,
	char	*instr,
	char	*protocol)
{
	char	*p
	,		*str;
	char	buff[256];
	Port	*port;

	strcpy(buff,instr);
	str = buff;
	if		(  ( p = strchr(str,':') )  ==  NULL  ) {
		url->protocol = StrDup(protocol);
	} else {
		*p = 0;
		url->protocol = StrDup(str);
		str = p + 1;
	}
	if		(  !strlcmp(str,"//")  ) {
		str += 2;
	}
	if		(  !stricmp(url->protocol,"file")  ) {
		url->host = NULL;
		url->port = NULL;
		url->file = StrDup(str);
	} else {
		if		(  ( p = strchr(str,'/') )  !=  NULL  ) {
			url->file = StrDup(p);
			*p = 0;
		} else {
			url->file = NULL;
		}			
		if		(  ( port = ParPort(str,NULL) ) != NULL ) {
			url->host = StrDup(port->adrs.a_ip.host);
			url->port = StrDup(port->adrs.a_ip.port);
			DestroyPort(port);
		}
	}
}

