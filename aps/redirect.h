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

#ifndef	_INC_LOG_H
#define	_INC_LOG_H

#ifndef	PacketClass
#define	PacketClass		unsigned char
#endif

#define	RED_PING		(PacketClass)0x01
#define	RED_DATA		(PacketClass)0x02
#define	RED_NOT			(PacketClass)0xF0
#define	RED_PONG		(PacketClass)0xF1
#define	RED_OK			(PacketClass)0xFE
#define	RED_END			(PacketClass)0xFF

extern	void	OpenDB_RedirectPort(DBG_Struct *dbg);
extern	void	CloseDB_RedirectPort(DBG_Struct *dbg);
extern	void	PutDB_Redirect(DBG_Struct *dbg, char *data);
extern	void	BeginDB_Redirect(DBG_Struct *dbg);
extern	void	CommitDB_Redirect(DBG_Struct *dbg);

#endif
