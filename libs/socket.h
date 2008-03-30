/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1996-1999 Ogochan.
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
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

#ifndef	_INC_SOCKET_H
#define	_INC_SOCKET_H
#include	<stdio.h>
#include	<sys/socket.h>	/*	for socket type	*/
#include	"port.h"

extern	void	SetNodelay(int s);
extern	int		ConnectSocket(Port *port, int type);
extern	int		BindSocket(Port *port, int type);

/*	lower level	*/
extern	int		BindIP_Socket(char *port, int type);
extern	int		BindUNIX_Socket(char *name, int type, int mode);
extern	int		ConnectIP_Socket(char *port, int type, char *host);
extern	int		ConnectUNIX_Socket(char *name, int type);
extern	void	CleanUNIX_Socket(Port *port);

#endif
