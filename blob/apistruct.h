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

#define	MAX_PAGE_LEVEL		8

#ifndef	pageno_t
#define	pageno_t		uint64_t
#endif
#ifndef	objpos_t
#define	objpos_t		uint64_t
#endif
#ifndef	ObjectType
#define	ObjectType		uint64_t
#endif

typedef	struct _OsekiSpace	{
	char			*file;
	FILE			*fp;
	size_t			pagesize;
	pageno_t		upages;
	int				level;
	pageno_t		pos[MAX_PAGE_LEVEL];
	pageno_t		mul[MAX_PAGE_LEVEL];
	pageno_t		freedatapage;
	pageno_t		*freedata;
	GHashTable		*freepage;
	GHashTable		*pages;
	GTree			*trans;
	int				lTran;
	int				cTran;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	struct {
		pthread_mutex_t	mutex;
		pthread_cond_t	cond;
	}	obj;
}	OsekiSpace;

typedef	struct	_OsekiSession {
	OsekiSpace		*space;
	int				tid;
	GHashTable		*pages;
	GHashTable		*oTable;
}	OsekiSession;

#define	OSEKI_OPEN_CLOSE	0x00
#define	OSEKI_OPEN_CREATE	0x01
#define	OSEKI_OPEN_READ		0x02
#define	OSEKI_OPEN_WRITE	0x04
#define	OSEKI_OPEN_APPEND	0x08

#endif
