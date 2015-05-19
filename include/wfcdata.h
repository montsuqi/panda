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

#define	APS_Null			(PacketClass)0x00
#define	APS_NULL			((PacketClass)APS_Null)

#define	APS_EVENTDATA		(PacketClass)0x01
#define	APS_WINDOW_STACK	(PacketClass)0x02
#define	APS_LINKDATA		(PacketClass)0x04
#define	APS_SPADATA			(PacketClass)0x08
#define	APS_SCRDATA			(PacketClass)0x10
/*
#define	APS_WINCTRL			(PacketClass)0x20
*/
#define	APS_CTRLDATA		(PacketClass)0x40
#define	APS_BLOB			(PacketClass)0x80

#define	APS_NOT				(PacketClass)0xF0
#define	APS_CHECK			(PacketClass)0xF1
#define	APS_STOP			(PacketClass)0xF3
#define	APS_REQ				(PacketClass)0xF4
#define	APS_API				(PacketClass)0xF5
#define	APS_WAIT			(PacketClass)0xFD
#define	APS_OK				(PacketClass)0xFE
#define	APS_END				(PacketClass)0xFF

#define SESSION_TYPE_TERM	0
#define SESSION_TYPE_API	1
#define SESSION_TYPE_CHECK	2

#define	SESSION_STATUS_NORMAL 	0
#define	SESSION_STATUS_END 		1
#define	SESSION_STATUS_ABORT 	2

#define	WFC_Null			(PacketClass)0x00
#define	WFC_DATA			(PacketClass)0x01
#define	WFC_PING			(PacketClass)0x02
#define	WFC_HEADER			(PacketClass)0x04
#define	WFC_TERM			(PacketClass)0x05
#define	WFC_API				(PacketClass)0x06
#define	WFC_TERM_INIT		(PacketClass)0x07

#define	WFC_FALSE			(PacketClass)0xE0
#define	WFC_TRUE			(PacketClass)0xE1
#define	WFC_NOT				(PacketClass)0xF0
#define	WFC_PONG			(PacketClass)0xF2
#define	WFC_NODATA			(PacketClass)0xFC
#define	WFC_DONE			(PacketClass)0xFD
#define	WFC_OK				(PacketClass)0xFE
#define	WFC_END				(PacketClass)0xFF

#define WFC_API_OK			(WFC_TRUE)
#define WFC_API_ERROR		(WFC_FALSE)
#define WFC_API_NOT_FOUND	(WFC_Null)

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

typedef struct {
	PacketClass				status;
	LargeByteString			*rec;
}	APIData;

typedef	struct _SessionData	{
	int				type;
	char			agent[SIZE_NAME+1];
	TermNode		*term;
	int				apsid;
	Bool			fInProcess;
	LD_Node			*ld;
	size_t			cWindow;
	int				retry;
	int				status;
	MessageHeader	*hdr;
	GHashTable		*spadata;
	GHashTable		*scrpool;
	GHashTable		*window_table;
	LargeByteString	*spa;
	LargeByteString	*linkdata;
	LargeByteString	**scrdata;
	APIData			*apidata;
	struct timeval	create_time;
	struct timeval	access_time;
	struct timeval	process_time;
	struct timeval	total_process_time;
	int				count;
	ValueStruct		*sysdbval;
	WindowStack		w;
}	SessionData;

typedef enum _SessionCtrlType {
	SESSION_CONTROL_INSERT = 0,
	SESSION_CONTROL_UPDATE,
	SESSION_CONTROL_DELETE,
	SESSION_CONTROL_LOOKUP,
	SESSION_CONTROL_GET_SESSION_NUM,
	SESSION_CONTROL_GET_DATA,
	SESSION_CONTROL_GET_MESSAGE,
	SESSION_CONTROL_RESET_MESSAGE,
	SESSION_CONTROL_SET_MESSAGE,
	SESSION_CONTROL_SET_MESSAGE_ALL,
	SESSION_CONTROL_GET_DATA_ALL
} SessionCtrlType;

#define	SESSION_CONTROL_OK	(PacketClass)0xFE
#define	SESSION_CONTROL_NG	(PacketClass)0xFF

#define SYSDBVAL_TEXT_SIZE 		(256)
#define SYSDBVAL_TEXT_SIZE_STR 	"256"
#define SYSDBVALS_SIZE 			(128)
#define SYSDBVALS_SIZE_STR 		"128"

typedef struct _SessionCtrl {
	SessionCtrlType	type;
	char			id[SYSDBVAL_TEXT_SIZE+1];
	unsigned int	size;
	char			rc;
	SessionData		*session;
	ValueStruct		*sysdbval;
	ValueStruct		*sysdbvals;
}	SessionCtrl;

#define SYSDBVAL_DEF ""												\
"sysdbval {\n"														\
"	id					varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	user				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	host				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	agent				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	window				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	widget				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	event				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	in_process			varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	create_time			varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	access_time			varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	process_time		varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	total_process_time	varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	count				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	popup				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	dialog				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	abort				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"};"

#define SYSDBVALS_DEF ""												\
"sysdbvals {\n"															\
"	values {\n"															\
"		id					varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		user				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		host				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		agent				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		window				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		widget				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		event				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		in_process			varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		create_time			varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		access_time			varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		process_time		varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		total_process_time	varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		count				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		popup				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		dialog				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"		abort				varchar(" SYSDBVAL_TEXT_SIZE_STR  ");\n"	\
"	}[" SYSDBVALS_SIZE_STR "];\n"										\
"};"

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
