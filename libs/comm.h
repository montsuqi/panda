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
extern	void			SendDataType(NETFILE *fp, PacketClass c);
extern	PacketClass		RecvDataType(NETFILE *fp);
extern	void			SendPacketDataType(NETFILE *fp, PacketDataType t);
extern	PacketDataType	RecvPacketDataType(NETFILE *fp);
extern	void			SendLength(NETFILE *fp, size_t size);
extern	size_t			RecvLength(NETFILE *fp);
extern	void			SendString(NETFILE *fp, char *str);
extern	void			SendFixedString(NETFILE *fp, char *str, size_t size);
extern	void			SendLBS(NETFILE *fp, LargeByteString *lbs);
extern	void			RecvLBS(NETFILE *fp, LargeByteString *lbs);
extern	void			RecvStringBody(NETFILE *fp, char *str, size_t size);
extern	void			RecvString(NETFILE *fp, char *str);
extern	long			RecvLong(NETFILE *fp);
extern	void			SendLong(NETFILE *fp,long data);
extern	int				RecvInt(NETFILE *fp);
extern	void			SendInt(NETFILE *fp,int data);
extern	int				RecvChar(NETFILE *fp);
extern	void			SendChar(NETFILE *fp,int data);
extern	double			RecvFloat(NETFILE *fp);
extern	void			SendFloat(NETFILE *fp,double data);
extern	Bool			RecvBool(NETFILE *fp);
extern	void			SendBool(NETFILE *fp, Bool data);
extern	Fixed			*RecvFixed(NETFILE *fp);
extern	void			SendFixed(NETFILE *fp, Fixed *fixed);
extern	void			SendStringData(NETFILE *fp, PacketDataType type, char *str);
extern	Bool			RecvStringData(NETFILE *fp, char *str);
extern	void			SendIntegerData(NETFILE *fp, PacketDataType type, int val);
extern	Bool			RecvIntegerData(NETFILE *fp, int *val);
extern	void			SendBoolData(NETFILE *fp, PacketDataType type, Bool val);
extern	Bool			RecvBoolData(NETFILE *fp, Bool *val);
extern	void			SendFloatData(NETFILE *fp, PacketDataType type, double val);
extern	Bool			RecvFloatData(NETFILE *fp, double *val);
extern	void			SendFixedData(NETFILE *fp, PacketDataType type, Fixed *xval);
extern	Bool			RecvFixedData(NETFILE *fp, Fixed **xval);

extern	void			SendValue(NETFILE *fp, ValueStruct *value);
extern	void			SendValueBody(NETFILE *fp, ValueStruct *value);
extern	void			RecvValueBody(NETFILE *fp, ValueStruct *value);

#undef	GLOBAL
#ifdef	_COMM
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
GLOBAL	PacketDataType	DataType;
#undef	GLOBAL

#endif
