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

#ifndef	_INC_WFC_H
#define	_INC_WFC_H
#include	"queue.h"
#include	"LDparser.h"

#ifndef	PacketClass
#define	PacketClass		unsigned char
#endif

#define	APS_Null		(PacketClass)0x00
#define	APS_PING		(PacketClass)0x01

#define	APS_EVENTDATA	(PacketClass)0x02
#define	APS_MCPDATA		(PacketClass)0x03
#define	APS_LINKDATA	(PacketClass)0x04
#define	APS_SPADATA		(PacketClass)0x05
#define	APS_SCRDATA		(PacketClass)0x06
#define	APS_CTRLDATA	(PacketClass)0x07
#define	APS_CLSWIN		(PacketClass)0x08

#define	APS_NOT			(PacketClass)0xF0
#define	APS_PONG		(PacketClass)0xF1
#define	APS_STOP		(PacketClass)0xF2
#define	APS_OK			(PacketClass)0xFE
#define	APS_END			(PacketClass)0xFF

typedef	struct {
	size_t	n;
	struct {
		char	window[SIZE_NAME];
	}	close[15];
}	CloseWindows;

typedef	struct {
	FILE	*fp;
	Queue	*que;
}	TermNode;

typedef	struct {
	LD_Struct	*ld;
	size_t	nports;
	FILE	**fp;
	pthread_cond_t	conn;
	pthread_mutex_t	lock;
}	APS_Node;

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
	,		puttype
	,		rc;
}	MessageHeader;

typedef	struct {
	char		*name;
	TermNode	*term;
	Bool		fKeep;
	APS_Node	*aps;
	CloseWindows	w;
	size_t			cWindow;
	MessageHeader	*hdr;
	LargeByteString	*mcpdata;
	LargeByteString	*spadata;
	LargeByteString	*linkdata;
	LargeByteString	**scrdata;
}	SessionData;

extern	int		PutAPS(APS_Node *aps, SessionData *data);
extern	Bool	GetAPS_Control(FILE *fpAPS, MessageHeader *hdr);
extern	Bool	GetAPS_Value(FILE *fpAPS, SessionData *data, PacketClass c);

#undef	GLOBAL
#ifdef	_WFC
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

GLOBAL	GHashTable	*WindowHash;
GLOBAL	GHashTable	*APS_Hash;
GLOBAL	GHashTable	*MQ_Hash;

#endif
