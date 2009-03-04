/*
 * PANDA -- a simple transaction monitor
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

#ifndef	_INC_WFCDATA_H
#define	_INC_WFCDATA_H
#include	"queue.h"
#include	"struct.h"
#include	"net.h"

#ifndef	PacketClass
#define	PacketClass		unsigned char
#endif

#define	APS_Null		(PacketClass)0x00

#define	APS_EVENTDATA	(PacketClass)0x01
#define	APS_MCPDATA		(PacketClass)0x02
#define	APS_LINKDATA	(PacketClass)0x04
#define	APS_SPADATA		(PacketClass)0x08
#define	APS_SCRDATA		(PacketClass)0x10
#define	APS_WINCTRL		(PacketClass)0x20
#define	APS_CTRLDATA	(PacketClass)0x40
#define	APS_BLOB		(PacketClass)0x80

#define	APS_NOT			(PacketClass)0xF0
#define	APS_PONG		(PacketClass)0xF1
#define	APS_PING		(PacketClass)0xF2
#define	APS_STOP		(PacketClass)0xF3
#define	APS_REQ			(PacketClass)0xF4
#define	APS_TERM		(PacketClass)0xF8
#define	APS_API			(PacketClass)0xF9
#define	APS_OK			(PacketClass)0xFE
#define	APS_END			(PacketClass)0xFF

#define SESSION_TYPE_TERM	0
#define SESSION_TYPE_API	1

typedef	struct {
	NETFILE	*fp;
	Queue	*que;
}	TermNode;

typedef	struct {
	NETFILE	*fp;
	int		id;
	long	count;
}	APS_Node;

typedef	struct {
	LD_Struct	*info;
	size_t	nports;
	APS_Node	*aps;
	pthread_cond_t	conn;
	pthread_mutex_t	lock;
}	LD_Node;

typedef	struct {
	char	*name;
	Queue	*que;
	pthread_t	thr;
}	MQ_Node;

typedef	struct {
	char	window[SIZE_NAME+1]
	,		widget[SIZE_NAME+1]
	,		event[SIZE_EVENT+1]
	,		term[SIZE_TERM+1]
	,		user[SIZE_USER+1]
	,		status
	,		dbstatus
	,		rc;
	byte	puttype;
}	MessageHeader;

typedef struct {
	PacketClass				status;
	char					method[SIZE_NAME+1];
	LargeByteString			*arguments;
	LargeByteString			*headers;
	LargeByteString			*body;
}	APIData;

typedef	struct _SessionData	{
	int						type;
	char					*name;
	TermNode				*term;
	int						apsid;
	Bool					fKeep;
	Bool					fInProcess;
	LD_Node					*ld;
	size_t					cWindow;
	int						retry;
	Bool					fAbort;
	MessageHeader			*hdr;
	int						tnest;
	GHashTable				*spadata;
	GHashTable				*scrpool;
	LargeByteString			*spa;
	LargeByteString			*mcpdata;
	LargeByteString			*linkdata;
	LargeByteString			**scrdata;
	APIData					*apidata;
	time_t					create_time;
	time_t					access_time;
	WindowControl			w;
}	SessionData;

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	GHashTable	*WindowHash;
GLOBAL	GHashTable	*APS_Hash;
GLOBAL	GHashTable	*MQ_Hash;

#undef	GLOBAL

#endif
