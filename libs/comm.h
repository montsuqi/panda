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

#ifndef	_INC_COMM_H
#define	_INC_COMM_H
#include	"glterm.h"
extern	void			SendPacketClass(FILE *fp, PacketClass c);
extern	PacketClass		RecvPacketClass(FILE *fp);
extern	void			SendDataType(FILE *fp, PacketClass c);
extern	PacketClass		RecvDataType(FILE *fp);
extern	void			SendPacketDataType(FILE *fp, PacketDataType t);
extern	PacketDataType	RecvPacketDataType(FILE *fp);
extern	void			SendLength(FILE *fp, size_t size);
extern	size_t			RecvLength(FILE *fp);
extern	void			SendString(FILE *fp, char *str);
extern	void			SendFixedString(FILE *fp, char *str, size_t size);
extern	void			SendLBS(FILE *fp, LargeByteString *lbs);
extern	void			RecvLBS(FILE *fp, LargeByteString *lbs);
extern	void			RecvStringBody(FILE *fp, char *str, size_t size);
extern	void			RecvString(FILE *fp, char *str);
extern	long			RecvLong(FILE *fp);
extern	void			SendLong(FILE *fp,long data);
extern	int				RecvInt(FILE *fp);
extern	void			SendInt(FILE *fp,int data);
extern	int				RecvChar(FILE *fp);
extern	void			SendChar(FILE *fp,char data);
extern	double			RecvFloat(FILE *fp);
extern	void			SendFloat(FILE *fp,double data);
extern	Bool			RecvBool(FILE *fp);
extern	void			SendBool(FILE *fp, Bool data);
extern	Fixed			*RecvFixed(FILE *fp);
extern	void			SendFixed(FILE *fp, Fixed *fixed);
extern	void			SendStringData(FILE *fp, PacketDataType type, char *str);
extern	Bool			RecvStringData(FILE *fp, char *str);
extern	void			SendIntegerData(FILE *fp, PacketDataType type, int val);
extern	Bool			RecvIntegerData(FILE *fp, int *val);
extern	void			SendBoolData(FILE *fp, PacketDataType type, Bool val);
extern	Bool			RecvBoolData(FILE *fp, Bool *val);
extern	void			SendFloatData(FILE *fp, PacketDataType type, double val);
extern	Bool			RecvFloatData(FILE *fp, double *val);
extern	void			SendFixedData(FILE *fp, PacketDataType type, Fixed *xval);
extern	Bool			RecvFixedData(FILE *fp, Fixed **xval);

extern	void		SendValue(FILE *fp, ValueStruct *value);
extern	void		SendValueBody(FILE *fp, ValueStruct *value);
extern	void		RecvValueBody(FILE *fp, ValueStruct *value);

#undef	GLOBAL
#ifdef	_COMM
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
GLOBAL	PacketDataType	DataType;
#undef	GLOBAL

#endif
