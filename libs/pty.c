/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

/*
*	TCP/IP module
*/

/*
#define		DEBUG
#define		TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<errno.h>
#include	<fcntl.h>
#include	<grp.h>
#include	<stdlib.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include	<unistd.h>

#include	<pty.h>
#include	"debug.h"

char	PTY_Name[11];
extern	int
GetMasterPTY(void)
{
	int		i
	,		j;
	int		master = -1;

	strcpy(PTY_Name,"/dev/ptyXX");
	for	( i = 0 ; i < 16 ; i ++ ) {
		for	( j = 0 ; j < 16 ; j ++ ) {
			PTY_Name[8] = "pqrstuvwxyzabcde"[i];
			PTY_Name[9] = "0123456789abcdef"[j];
			if		(  ( master = open(PTY_Name, O_RDWR) )  <  0  ) {
				if		(  errno  ==  ENOENT  ) {
					return	(-1);
				}
			} else {
				break;
			}
		}
	}
	if		(  master  <  0  ) {
		return	(-1);
	}
	PTY_Name[5] = 't';
	return	(master);
}

extern	int
GetSlavePTY(
	char	*name)
{
	struct	group	*gptr;
	gid_t			gid;
	int				slave;

	slave = -1;
	if		(  ( gptr = getgrnam("tty") )  !=  NULL  ) {
		gid = gptr->gr_gid;
	} else {
		gid = -1;
	}
	chown(name,getuid(),gid);
	chmod(name,S_IRUSR|S_IWUSR);
	slave = open(name,O_RDWR);
	return	(slave);
}

