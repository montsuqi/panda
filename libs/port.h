/*
PANDA -- a simple transaction monitor
Copyright (C) 2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef	_INC_PORT_H
#define	_INC_PORT_H
#include	"types.h"

typedef	struct	{
	char	*protocol;
	char	*host;
	char	*port;
	char	*file;
}	URL;

#define	PORT_NULL	0
#define	PORT_IP		1
#define	PORT_UNIX	2

typedef	struct {
	int		type;
	union {
		struct {
			char	*name;
			int		mode;
		}	a_unix;
		struct {
			char	*host;
			char	*port;
		}	a_ip;
	}	adrs;
}	Port;

#define	IP_PORT(p)		((p)->adrs.a_ip.port)
#define	IP_HOST(p)		((p)->adrs.a_ip.host)
#define	UNIX_NAME(p)	((p)->adrs.a_unix.name)
#define	UNIX_MODE(p)	((p)->adrs.a_unix.mode)

extern	void		ParseURL(URL *url, char *instr, char *protocol);
extern	void		DestroyPort(Port *port);
extern	Port		*ParPort(char *str, char *def);
extern	Port		*ParPortName(char *str);
extern	char		*StringPort(Port *port);
extern	char		*StringPortName(Port *port);

extern	char		*ExpandPath(char *org,char *base);
extern	Port		*NewIP_Port(char *host, char *port);
extern	Port		*NewUNIX_Port(char *name, int mode);

#endif
