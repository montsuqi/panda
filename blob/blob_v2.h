/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_INC_BLOB_V2_H
#define	_INC_BLOB_V2_H
#include	<stdint.h>

//#define	USE_MMAP

#ifndef	pageno_t
#define	pageno_t		uint64_t
#endif

#define	MIN_PAGE_SIZE		2048
#define	MAX_PAGE_LEVEL		8

#if	BLOB_VERSION == 2
#define	BLOB_V2_Entry		BLOB_Entry
#define	BLOB_V2_Space		BLOB_Space
#define	BLOB_V2_State		BLOB_State

#define	InitBLOB_V2			InitBLOB
#define	FinishBLOB_V2		FinishBLOB
#define	ConnectBLOB_V2		ConnectBLOB
#define	DisConnectBLOB_V2	DisConnectBLOB
#define	StartBLOB_V2		StartBLOB
#define	CommitBLOB_V2		CommitBLOB
#define	AbortBLOB_V2		AbortBLOB

#define	NewBLOB_V2			NewBLOB
#define	OpenBLOB_V2			OpenBLOB
#define	DestroyBLOB_V2		DestroyBLOB
#define	CloseBLOB_V2		CloseBLOB
#define	WriteBLOB_V2		WriteBLOB
#define	ReadBLOB_V2			ReadBLOB
#endif

#define	PAGE_NODE			0x8000000000000000LL
#define	PAGE_USECHILD		0x4000000000000000LL
#define	PAGE_FLAGS			0xFF00000000000000LL

#define	IS_POINT_NODE(p)	(((p)&PAGE_NODE) == PAGE_NODE)
#define	HAVE_FREECHILD(p)	(((p)&PAGE_USECHILD) == 0)
#define	USE_NODE(p)			((p)|=PAGE_USECHILD)
#define	FREE_NODE(p)		((p)&=~PAGE_USECHILD)
#define	PAGE_NO(p)			((p)&~PAGE_FLAGS)

#define	BODY_USE			0x40
#define	BODY_SHORT			0x20
#define	OBJ_MODE			0x0F

#define	IS_SHORTFORM(e)		((((e).flags)&BODY_SHORT) == BODY_SHORT)
#define	IS_FREEOBJ(e)		((((e).flags)&BODY_USE) == 0)
#define	USE_OBJ(e)			(((e).flags)|=BODY_USE)
#define	FREE_OBJ(e)			(((e).flags)&=~BODY_USE)
#define	GET_BODYSIZE(e)		(((e).flags)&~PAGE_FLAGS)
#define	SET_BODYSIZE(e,s)	(((e).flags)=(((e).size)&PAGE_FLAGS)|(s))

#define	NODE_ELEMENTS(state)		((state)->space->pagesize / sizeof(pageno_t))
#define	LEAF_ELEMENTS(state)		((state)->space->pagesize / sizeof(BLOB_V2_Entry))
#define	LockBLOB(blob)				pthread_mutex_lock(&(blob)->mutex)
#define	UnLockBLOB(blob)			pthread_mutex_unlock(&(blob)->mutex)
#define	ReleaseBLOB(blob)			pthread_cond_signal(&(blob)->cond)
#define	WaitBLOB(blod)				pthread_cond_wait(&(blob)->cond,&(blob)->mutex);

#define	SIZE_EXT			0x80000000
#define	IS_EXT_SIZE(p)		(((p)&SIZE_EXT) == SIZE_EXT)
#define	INPAGE_SIZE(p)		((p)&~SIZE_EXT)

#ifndef	objpos_t
#define	objpos_t		uint64_t
#endif

typedef	struct {
	byte		flags	:8;
	uint64_t	pos		:56;
	byte		mode	:8;
	uint64_t	size	:56;
}	BLOB_V2_Entry;

typedef	struct {
	MonObjectType	obj;
	byte			mode;
	objpos_t		head;
	objpos_t		first;
	objpos_t		last;
	uint64_t		size;
	byte			*buff;
	size_t			bsize;
	size_t			use;
}	BLOB_V2_Open;

typedef	struct {
	char			magic[SIZE_BLOB_HEADER];
	size_t			pagesize;
	pageno_t		pages;
	int				level;
	pageno_t		pos[MAX_PAGE_LEVEL];
	pageno_t		freedata;
}	BLOB_V2_Header;

typedef	struct _BLOB_V2_Space	{
	char			*space;
	FILE			*fp;
	size_t			pagesize;
	pageno_t		pages;
	int				level;
	pageno_t		pos[MAX_PAGE_LEVEL];
	pageno_t		mul[MAX_PAGE_LEVEL];
	pageno_t		freedatapage;
	pageno_t		*freedata;
	GHashTable		*freepage;
	int				cTran;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	BLOB_V2_Space;

typedef	struct {
	objpos_t	prio;
	objpos_t	next;
	size_t		size;
}	BLOB_V2_DataEntry;

typedef	struct {
	size_t		use;
}	BLOB_V2_DataPage;

typedef	struct {
	pageno_t	page;
	Bool		fUpdate;
	void		*body;
}	BLOB_V2_Page;

typedef	struct	_BLOB_V2_State {
	BLOB_V2_Space	*space;
	int				tid;
	GHashTable		*pages;
	GHashTable		*oTable;
}	BLOB_V2_State;

#include	"page.h"

extern	BLOB_V2_Space	*InitBLOB_V2(char *space);
extern	void			FinishBLOB_V2(BLOB_V2_Space *blob);
extern	BLOB_V2_State	*ConnectBLOB_V2(BLOB_V2_Space *blob);
extern	void			DisConnectBLOB_V2(BLOB_V2_State *state);
extern	Bool			StartBLOB_V2(BLOB_V2_State *state);
extern	Bool			CommitBLOB_V2(BLOB_V2_State *state);
extern	Bool			AbortBLOB_V2(BLOB_V2_State *state);

extern	MonObjectType	NewBLOB_V2(BLOB_V2_State *state, int mode);
extern	Bool			OpenBLOB_V2(BLOB_V2_State *state, MonObjectType obj, int mode);
extern	Bool			DestroyBLOB_V2(BLOB_V2_State *state, MonObjectType obj);
extern	Bool			CloseBLOB_V2(BLOB_V2_State *state, MonObjectType obj);
extern	int	WriteBLOB_V2(BLOB_V2_State *state, MonObjectType obj, byte *buff, size_t size);
extern	int	ReadBLOB_V2(BLOB_V2_State *state, MonObjectType obj, byte *buff, size_t size);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#endif
