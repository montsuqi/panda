/*	OSEKI -- Object Store Engine Kernel Infrastructure

Copyright (C) 2004 Ogochan.

This module is part of OSEKI.

	OSEKI is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
OSEKI, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with OSEKI so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_INC_APISTRUCT_H
#define	_INC_APISTRUCT_H
#include	<stdint.h>

#ifndef	pageno_t
#define	pageno_t		uint64_t
#endif
#ifndef	objpos_t
#define	objpos_t		uint64_t
#endif
#ifndef	ObjectType
#define	ObjectType		uint64_t
#endif

#include	"pagestruct.h"

typedef	struct _OsekiSpace	{
	char			*file;
	FILE			*fp;
	size_t			pagesize;
	pageno_t		upages;
	int				inuse;
	pageno_t		mul[MAX_PAGE_LEVEL];
	pageno_t		*freedata;
	GHashTable		*freepage;
	GHashTable		*pages;
	GTree			*trans;
	int				lTran;
	int				cTran;
	int				cSeq;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	struct {
		pthread_mutex_t	mutex;
		pthread_cond_t	cond;
	}	obj;
	struct {
		pthread_mutex_t	mutex;
		pthread_cond_t	cond;
	}	page;
}	OsekiSpace;

typedef	struct	_OsekiSession {
	OsekiSpace		*space;
	int				tid;
	OsekiObjectTable	*objs;
	GHashTable		*pages;
	GHashTable		*oTable;
	int				cOld;
}	OsekiSession;

#define	OSEKI_MODE_MASK		0x0F
#define	OSEKI_OPEN_CLOSE	0x00
#define	OSEKI_OPEN_CREATE	0x01
#define	OSEKI_OPEN_READ		0x02
#define	OSEKI_OPEN_WRITE	0x04
#define	OSEKI_OPEN_APPEND	0x08

#define	OSEKI_TYPE_MASK		0xF0
#define	OSEKI_ALLOC_LINER	0x10

#endif
