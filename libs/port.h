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
