/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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

#ifndef	_INC_PROTOCOL_H
#define	_INC_PROTOCOL_H
#include	"queue.h"

typedef	Bool	(*RecvHandler)(GtkWidget *, NETFILE *);
typedef	Bool	(*SendHandler)(char *, GtkWidget *, NETFILE *);

typedef	struct {
	GtkType		type;
	RecvHandler	rfunc;
	SendHandler	sfunc;
}	HandlerNode;

extern	void		InitProtocol(void);
extern  void        TermProtocol(void);
extern	void		CheckScreens(NETFILE *fp, Bool);
extern	Bool		SendConnect(NETFILE *fpComm, char *apl);
extern	Bool		RecvWidgetData(GtkWidget *widget, NETFILE *fp);
extern	void		RecvValue(NETFILE *fp, char *longname);
extern	Bool		GetScreenData(NETFILE *fpComm);
extern  void		SendWindowData(void);
extern	void		SendEvent(NETFILE *fpComm, char *window, char *widget, char *event);

extern	void		GL_SendPacketClass(NETFILE *fp, PacketClass c);
extern	PacketClass	GL_RecvPacketClass(NETFILE *fp);
extern	PacketDataType	GL_RecvDataType(NETFILE *fp);
extern	void			GL_SendInt(NETFILE *fp, int data);
extern	int				GL_RecvInt(NETFILE *fp);
extern	void			GL_SendDataType(NETFILE *fp, PacketClass c);
extern	void			GL_RecvName(NETFILE *fp, size_t size, char *name);
extern	void			GL_SendName(NETFILE *fp, char *name);

extern	PacketDataType	RecvPacketDataType(NETFILE *fp);
extern	void			SendStringData(NETFILE *fp, PacketDataType type, char *str);
extern	char			*RecvStringData(NETFILE *fp, char *str, size_t size);
extern	void			SendIntegerData(NETFILE *fp, PacketDataType type, int val);
extern	Bool			RecvIntegerData(NETFILE *fp, int *val);
extern	void			SendBoolData(NETFILE *fp, PacketDataType type, Bool val);
extern	Bool			RecvBoolData(NETFILE *fp, Bool *val);
extern	void			SendFloatData(NETFILE *fp, PacketDataType type, double val);
extern	Bool			RecvFloatData(NETFILE *fp, double *val);
extern	void			SendFixedData(NETFILE *fp, PacketDataType type, Fixed *xval);
extern	Bool			RecvFixedData(NETFILE *fp, Fixed **xval);
extern  void			RecvBinaryData(NETFILE *fp, LargeByteString *binary);
extern  void			SendBinaryData(NETFILE *fp, PacketDataType type, LargeByteString *binary);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	Bool	Protocol1;
GLOBAL	Bool	Protocol2;
GLOBAL	PacketDataType	DataType;
GLOBAL	GHashTable	*ClassTable;

#endif
