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

#ifndef	_INC_PAGESTRUCT_H
#define	_INC_PAGESTRUCT_H

#define	OSEKI_FILE_HEADER	"OFH1"
#define	OSEKI_MAGIC_SIZE	4
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

#define	PAGE_NODE			0x8000000000000000LL
#define	PAGE_USECHILD		0x4000000000000000LL
#define	PAGE_FLAGS			0xFF00000000000000LL

#define	IS_POINT_NODE(p)	(((p)&PAGE_NODE) == PAGE_NODE)
#define	HAVE_FREECHILD(p)	(((p)&PAGE_USECHILD) == 0)
#define	USE_NODE(p)			((p)|=PAGE_USECHILD)
#define	FREE_NODE(p)		((p)&=~PAGE_USECHILD)
#define	PAGE_NO(p)			((p)&~PAGE_FLAGS)

//#define	BODY_USE			0x04

#define	BODY_PACK			0x00
#define	BODY_LINER			0x01
#define	BODY_TREE			0x02
#define	BODY_SHORT			0x03

#define	IS_FREEOBJ(e)		((e).use == 0)
#define	USE_OBJ(e)			((e).use = 1)
#define	FREE_OBJ(e)			((e).use = 0)

#define	PAGE_SIZE(state)		((state)->space->pagesize)
#define	NODE_ELEMENTS(state)	(PAGE_SIZE(state) / sizeof(pageno_t))
#define	LEAF_ELEMENTS(state)	(PAGE_SIZE(state) / sizeof(OsekiObjectEntry))

#define	SIZE_EXT			0x80000000
#define	IS_EXT_SIZE(p)		(((p)&SIZE_EXT) == SIZE_EXT)
#define	INPAGE_SIZE(p)		((p)&~SIZE_EXT)

#define	OBJ_NODE			0x8000000000000000LL
#define	OBJ_PAGE(s,p)		(((p)&~OBJ_NODE)/(s)->space->pagesize)
#define	OBJ_OFF(s,p)		(size_t)(((p)&~OBJ_NODE)%(s)->space->pagesize)
#define	OBJ_POS(s,p,o)		((((p)&~OBJ_NODE)*(s)->space->pagesize)+(o))
#define	OBJ_ADRS(p)			((p)&~OBJ_NODE)
#define	OBJ_POINT_NODE(p)	((p)|=OBJ_NODE)
#define	IS_OBJ_NODE(p)		(((p)&OBJ_NODE) == OBJ_NODE)

#define	SIZE_SMALL		14
typedef	struct {
	byte		use		:1;
	byte		type	:3;
	byte		mode	:4;
	byte		ssize	:4;
	byte		fill	:4;
	union {
		struct {
			uint64_t	pos		:56;
			uint64_t	size	:56;
		}	pointer;
		byte	small[SIZE_SMALL];
	}	body;
}	OsekiObjectEntry;

typedef	struct {
	objpos_t	prio;
	objpos_t	next;
	size_t		size;
}	OsekiDataEntry;

typedef	struct {
	size_t		use;
}	OsekiDataPage;

typedef	struct {
	pageno_t		freedata;
	int				level;
	pageno_t		pos[MAX_PAGE_LEVEL];
}	OsekiObjectTable;

typedef	struct {
	pageno_t		pages;
	uint			seq;
	pageno_t		cOld;
}	OsekiPhaseControl;

typedef	struct {
	char			magic[OSEKI_MAGIC_SIZE];
	size_t			pagesize;
	OsekiPhaseControl	phase[2];
	ObjectType		root;
}	OsekiHeaderPage;

#if	0
#define	ROUND_TO(p,s)	(p)
#else
#define	ROUND_TO(p,s)	((((p)%(s)) == 0) ? (p) : (((p)/(s))+1)*(s))
#endif

#endif
