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

#ifndef	_INC_COMM_H
#define	_INC_COMM_H
#include	"value.h"
#include	"net.h"
extern	void			SendPacketClass(NETFILE *fp, PacketClass c);
extern	PacketClass		RecvPacketClass(NETFILE *fp);
extern	PacketClass		RecvDataType(NETFILE *fp);
extern	void			SendLength(NETFILE *fp, size_t size);
extern	size_t			RecvLength(NETFILE *fp);
extern	void			SendLBS(NETFILE *fp, LargeByteString *lbs);
extern	void			RecvLBS(NETFILE *fp, LargeByteString *lbs);
extern	void			SendString(NETFILE *fp, char *str);
extern	void			RecvString(NETFILE *fp, char *str);
extern	int				RecvInt(NETFILE *fp);
extern	void			SendInt(NETFILE *fp,int data);
extern	unsigned int	RecvUInt(NETFILE *fp);
extern	void			SendUInt(NETFILE *fp, unsigned int data);
extern	int				RecvChar(NETFILE *fp);
extern	void			SendChar(NETFILE *fp,int data);
extern	Bool			RecvBool(NETFILE *fp);
extern	void			SendBool(NETFILE *fp, Bool data);
extern	void			SendObject(NETFILE *fp, MonObjectType obj);
extern	MonObjectType	RecvObject(NETFILE *fp);

extern	void			InitComm(void);

#define	RecvName(fp,name)	RecvString((fp),(name))

#undef	GLOBAL
#ifdef	_COMM
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
#undef	GLOBAL

#endif
