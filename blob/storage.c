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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<glib.h>
#include	<pthread.h>
#include	"types.h"
#include	"libmondai.h"
#include	"pagestruct.h"
#include	"lock.h"
#include	"storage.h"
#include	"table.h"
#include	"page.h"
#include	"message.h"
#include	"debug.h"

typedef	struct {
	ObjectType	obj;
	byte		mode;
	objpos_t	node;
	objpos_t	head;
	objpos_t	curr;
	uint64_t	size;
	uint64_t	off;
	byte		*buff;
	size_t		bsize;
	size_t		use;
	int			tid;
	int			depth;
	Bool		fDurty;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	ObjectInfo;

static	void	FlushBuffer(OsekiSession *state, ObjectInfo *ent, Bool fOnly);

#ifdef	DEBUG
static	void
_DumpInfo(
	OsekiSession	*state,
	ObjectInfo		*ent)
{
	printf("ent->obj    = %lld\n",ent->obj);
	printf("ent->mode   = %02X\n",ent->mode);
	printf("ent->size   = %lld\n",ent->size);
	printf("ent->use    = %d\n",ent->use);
	printf("ent->off    = %lld\n",ent->off);
	printf("ent->bsize  = %d\n",ent->bsize);
	printf("ent->node   = %lld:%d\n",
		   OBJ_PAGE(state,ent->node),OBJ_OFF(state,ent->node));
	printf("ent->head   = %lld:%d\n",
		   OBJ_PAGE(state,ent->head),OBJ_OFF(state,ent->head));
	printf("ent->curr   = %lld:%d\n",
		   OBJ_PAGE(state,ent->curr),OBJ_OFF(state,ent->curr));
	printf("ent->fDurty = %s\n",ent->fDurty ? "TRUE" : "FALSE");
	printf("ent->depth  = %d\n",ent->depth);
}
#define	DumpInfo(s,ent)		_DumpInfo((s),(ent))

static	void
_DumpEntry(
	OsekiSession	*state,
	OsekiDataEntry	*el)
{
	printf("el->size = %d\n",el->size);
	printf("el->prio = %lld:%d\n",
		   OBJ_PAGE(state,el->prio),OBJ_OFF(state,el->prio));
	printf("el->next = %lld:%d\n",
		   OBJ_PAGE(state,el->next),OBJ_OFF(state,el->next));
}
#define	DumpEntry(s,el)		_DumpEntry((s),(el))
#define	DumpPage(s,d)		_DumpPage((s)->space,(byte *)(d))
#else
#define	DumpInfo(s,ent)		/*	*/
#define	DumpEntry(s,el)		/*	*/
#define	DumpPage(s,d)		/*	*/
#endif

static	ObjectType
GetFreeOID(
	OsekiSession	*state)
{
	pageno_t	*nodepage;
	pageno_t	stack[MAX_PAGE_LEVEL];
	pageno_t	page
		,		node
		,		leaf
		,		lower
		,		own;
	int			i
		,		j
		,		k
		,		use
		,		off;
	ObjectType	base;
	OsekiSpace	*space;
	OsekiObjectEntry	*leafpage;
	Bool		fFree;

ENTER_FUNC;
	space = state->space;
	leaf = 0;
	base = 0;
	Lock(&space->obj);
	while	(  leaf  ==  0  ) {
		for	( i = 0 ; i < state->objs->level ; i ++ ) {
			for	( j = 0 ; j < state->objs->level ; j ++ ) {
				stack[j] = state->objs->pos[j];
			}
			page = state->objs->pos[i];
			if		(  HAVE_FREECHILD(page)  ) {
				base = 0;
				for	( k = i ; k >= 0 ; k -- ) {
					stack[k] = page;
					nodepage = GetPage(state,page);
					lower = page;
					node = 0;
					for	( j = 0 ; j <  NODE_ELEMENTS(state) ; j ++ ) {
						if		(  HAVE_FREECHILD(nodepage[j])  ) {
							if		(  ( lower = PAGE_NO(nodepage[j]) )  ==  0  ) {
								nodepage = UpdatePage(state,page);
								lower = NewPage(state,1);
								if		(  k  >  0  ) {
									nodepage[j] = lower | PAGE_NODE;
								} else {
									nodepage[j] = lower;
								}
							}
							leaf = lower;
							node = j;
							break;
						}
					}
					page = lower;
					base = base + node * state->space->mul[k];
				}
				if		(  leaf  >  0  )	break;
			}
		}
		if		(  leaf  ==  0  ) {
			node = NewPage(state,1);
			(void)GetPage(state,node);
			nodepage = UpdatePage(state,node);
			nodepage[0] = state->objs->pos[state->objs->level-1] | PAGE_NODE;
			state->objs = UpdatePage(state,1LL);
			state->objs->pos[state->objs->level] = node;
			stack[state->objs->level] = node;
			state->objs->level ++;

		}
	}
	(void)GetPage(state,leaf);
	leafpage = UpdatePage(state,leaf);
	off = -1;
	use = 0;
	for	( j = 0 ; j < LEAF_ELEMENTS(state) ; j ++ ) {
		if		(  IS_FREEOBJ(leafpage[j])  )	{
			if		(  off  <  0  ) {
				USE_OBJ(leafpage[j]);
				leafpage[j].mode = OSEKI_OPEN_CLOSE;
				leafpage[j].pos = 0;
				off = j;
				use ++;
			}
		} else {
			use ++;
		}
	}
	if		(  off  <  0  ) {
		fprintf(stderr,"tree structure broken.\n");
		exit(1);
	}
	if		(  use  ==  LEAF_ELEMENTS(state)  ) {
		own = leaf;
		use = 0;
		for	( i = 0 ; i < state->objs->level ; i ++ ) {
			page = stack[i];
			(void)GetPage(state,page);
			nodepage = UpdatePage(state,page);
			fFree = FALSE;
			for	( j = 0 ; j < NODE_ELEMENTS(state) ; j ++ ) {
				if		(  PAGE_NO(nodepage[j])  ==  own  ) {
					USE_NODE(nodepage[j]);
				}
				if		(  HAVE_FREECHILD(nodepage[j])  ) {
					fFree = TRUE;
					break;
				}
			}
			own = page;
			if		(  fFree  )	break;
			if		(  PAGE_NO(state->objs->pos[i])  ==  page  ) {
				state->objs = UpdatePage(state,1LL);
				USE_NODE(state->objs->pos[i]);
			}
		}
	}
	UnLock(&space->obj);
LEAVE_FUNC;
	return	(base * LEAF_ELEMENTS(state) + off);
}

static	pageno_t
SearchLeafPage(
	OsekiSession	*state,
	ObjectType		obj,
	pageno_t		*path)
{
	pageno_t	off
		,		base
		,		page
		,		next
		,		*nodepage;
	OsekiSpace	*space;
	int			i;

ENTER_FUNC;
	space = state->space;
	base = obj / LEAF_ELEMENTS(state);
	page = 0;
	dbgprintf("level = %d\n",state->objs->level);
	for	( i = state->objs->level - 1 ; i >= 0 ; i -- ) {
		off = base / space->mul[i];
		if		(  page  ==  0  ) {
			page = state->objs->pos[i];
		}
		if		(  path  !=  NULL  ) {
			path[i] = page;
		}
		nodepage = GetPage(state,page);
		next = PAGE_NO(nodepage[off]);
		page = next;
		base %= NODE_ELEMENTS(state);
	}
LEAVE_FUNC;
	return	(page);
}

static	void
FreeEntry(
	OsekiSession		*state,
	OsekiObjectEntry	*ent)
{
	objpos_t	pos
		,		npos;
	pageno_t	page
		,		ppage
		,		npage
		,		leaf;
	size_t		off
		,		cur
		,		nsize
		,		shlink;
	OsekiDataPage	*data
		,			*wd;
	OsekiDataEntry	*el
		,			*wel;
	OsekiObjectEntry	*leafnode;

ENTER_FUNC;
	pos = ent->pos;
	switch	(ent->mode & OSEKI_TYPE_MASK) {
	  case	OSEKI_ALLOC_LINER:
		dbgmsg("*");
		break;
	  case	OSEKI_ALLOC_TREE:
		dbgmsg("*");
		break;
	  case	OSEKI_ALLOC_PACK:
		dbgmsg("*");
		while	(  pos  !=  0  ) {
			page = OBJ_PAGE(state,pos);
			(void)GetPage(state,page);
			data = UpdatePage(state,page);
			off = OBJ_OFF(state,pos);
			shlink = 0;
			for	( cur = sizeof(OsekiDataPage) ; cur < data->use ;  ) {
				el = (OsekiDataEntry *)((byte *)data + cur);
				nsize = ROUND_TO(el->size,sizeof(size_t))
					+ sizeof(OsekiDataEntry);
				if		(  cur  ==  off  ) {
					pos = el->next;
					memmove((byte *)data + cur,
							(byte *)data + cur + nsize,
							data->use - cur - nsize);
					data->use -= nsize;
					memclear((byte *)data+data->use,
							 PAGE_SIZE(state)-data->use);
					off = 0;
					el = (OsekiDataEntry *)((byte *)data + cur);
					nsize = ROUND_TO(el->size,sizeof(size_t))
						+ sizeof(OsekiDataEntry);
				}
				if		(  cur  ==  data->use  )	break;
				DumpEntry(state,el);
				if		(  cur  >  off  ) {
					npos = OBJ_POS(state,page,cur);
					if		(  !IS_OBJ_NODE(el->prio)  ) {
						ppage = OBJ_PAGE(state,el->prio);
						(void)GetPage(state,ppage);
						wd = UpdatePage(state,ppage);
						wel = (OsekiDataEntry *)((byte *)wd
												 + OBJ_OFF(state,el->prio));
						wel->next = npos;
					} else {
						leaf = OBJ_PAGE(state,el->prio);
						(void)GetPage(state,leaf);
						leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
						leafnode[OBJ_OFF(state,el->prio)].pos = npos;
					}
					if		(  el->next  >  0  ) {
						npage = OBJ_PAGE(state,el->next);
						(void)GetPage(state,npage);
						wd = UpdatePage(state,npage);
						wel = (OsekiDataEntry *)((byte *)wd
												 + OBJ_OFF(state,el->next));
						wel->prio = npos;
					}
				}
				cur += nsize;
			}
			if		(  PAGE_SIZE(state) - data->use
					   >  sizeof(OsekiDataEntry)  ) {
				ReturnPage(state,page);
			}
			break;
		}
	}
LEAVE_FUNC;
}

static	ObjectInfo	*
OpenEntry(
	OsekiSession	*state,
	pageno_t		leaf,
	ObjectType		obj,
	byte			mode)
{
	OsekiObjectEntry	*leafnode;
	int			off;
	ObjectInfo	*ent;
	uint64_t	size;

ENTER_FUNC;
	Lock(&state->space->obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  ==  NULL  ) {
		leafnode = (OsekiObjectEntry *)GetPage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		if		(  IS_FREEOBJ(leafnode[off])  ) {
			ent = NULL;
		} else
		if		(	(  ( leafnode[off].mode >> 4 )     ==  OSEKI_OPEN_CLOSE  )
				||	(	(  ( leafnode[off].mode >> 4)  ==  OSEKI_OPEN_READ  )
					&&	(  mode                ==  OSEKI_OPEN_READ  ) ) ) {
			if		(  ( leafnode[off].mode & OSEKI_OPEN_WRITE )  !=  0  ) {
				leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
			}
			if		( ( mode & OSEKI_OPEN_CREATE )  !=  0  ) {
				if		(  leafnode[off].pos  >  0  ) {
					FreeEntry(state,&leafnode[off]);
				}
				leafnode[off].pos = 0;
				leafnode[off].size = 0;
				leafnode[off].mode = ( mode & OSEKI_MODE_MASK ) >> 4;
				switch	( mode & OSEKI_TYPE_MASK ) {
				  case	OSEKI_ALLOC_LINER:
					leafnode[off].type = BODY_LINER;
					break;
				  case	OSEKI_ALLOC_TREE:
					leafnode[off].type = BODY_TREE;
					break;
				  default:
					leafnode[off].type = BODY_PACK;
					break;
				}
			} else {
				mode = ( mode & OSEKI_MODE_MASK ) | leafnode[off].type;
			}
			ent = New(ObjectInfo);
			ent->obj = obj;
			ent->mode = mode;
			ent->head = leafnode[off].pos;
			ent->curr = ent->head;
			ent->size = leafnode[off].size;
			ent->buff = NULL;
			ent->bsize = 0;
			ent->use = 0;
			ent->off = 0;
			ent->fDurty = FALSE;
			ent->node = OBJ_POS(state,leaf,off);
			ent->tid = state->tid;
			switch	( mode & OSEKI_TYPE_MASK ) {
			  case	OSEKI_ALLOC_TREE:
				ent->depth = 0;
				size = ent->size;
				size /= PAGE_SIZE(state);
				while	(  size  >  0  ) {
					ent->depth ++;
					size /= NODE_ELEMENTS(state);
				}
				break;
			  default:
				ent->depth = 0;
				break;
			}
			InitLock(ent);
			g_hash_table_insert(state->oTable,&ent->obj,ent);
			DumpInfo(state,ent);
		} else {
			ent = NULL;
		}
	}
	UnLock(&state->space->obj);
LEAVE_FUNC;
	return	(ent);
}

extern	ObjectType
NewObject(
	OsekiSession	*state,
	int				mode)
{
	ObjectType	obj;
	pageno_t	leaf;

ENTER_FUNC;
	state->objs = GetPage(state,1LL);
	obj = GetFreeOID(state);
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		if		(  OpenEntry(state,leaf,obj,(byte)mode|OSEKI_OPEN_CREATE)
				   ==  NULL  ) {
			obj = GL_OBJ_NULL;
		}
	} else {
		obj = GL_OBJ_NULL;
	}
LEAVE_FUNC;
	return	(obj);
}

extern	ssize_t
OpenObject(
	OsekiSession	*state,
	ObjectType	obj,
	int				mode)
{
	pageno_t	leaf;
	ssize_t		ret;
	ObjectInfo	*ent;

ENTER_FUNC;
	state->objs = GetPage(state,1LL);
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		if		(  ( ent = OpenEntry(state,leaf,obj,(byte)mode) )
				   ==  NULL  ) {
			ret = -1;
		} else {
			ret = ent->size;
		}
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
FlushObject(
	OsekiSession	*state,
	ObjectInfo	*ent)
{
	if		(  ( ent->mode & OSEKI_OPEN_WRITE )  !=  0  ) {
		if		(  ent->use  >  0  ) {
			FlushBuffer(state,ent,TRUE);
		}
	}
}

static	void
CloseEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	ObjectType		obj)
{
	int				off;
	pageno_t		leaf;
	OsekiObjectEntry	*leafnode;

ENTER_FUNC;
	FlushObject(state,ent);
	DumpInfo(state,ent);
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		(void)GetPage(state,leaf);
		leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		if		(  ( leafnode[off].mode & ( OSEKI_OPEN_WRITE >> 4 ) )
				   !=  0  ) {
			leafnode[off].size = ent->size;
			leafnode[off].pos = ent->head;
		}
		leafnode[off].mode = OSEKI_OPEN_CLOSE >> 4;
	}
	g_hash_table_remove(state->oTable,&obj);
	xfree(ent);
LEAVE_FUNC;
}

extern	Bool
CloseObject(
	OsekiSession	*state,
	ObjectType	obj)
{
	Bool			ret;
	ObjectInfo		*ent;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	ret = TRUE;
	Lock(&state->space->obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  !=  NULL  ) {
		CloseEntry(state,ent,obj);
	} else {
		ret = FALSE;
	}
	UnLock(&state->space->obj);
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
DestroyObject(
	OsekiSession	*state,
	ObjectType	obj)
{
	int				i
		,			j;
	OsekiObjectEntry	*leafpage;
	pageno_t		*nodepage;
	Bool			rc;
	pageno_t		stack[MAX_PAGE_LEVEL];
	pageno_t		page
		,			own;
	size_t			off;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	rc = TRUE;
	Lock(&state->space->obj);
	page = SearchLeafPage(state,obj,stack);
	if		(  page  ==  0  ) {
		rc = FALSE;
	} else {
		(void)GetPage(state,page);
		leafpage = UpdatePage(state,page);
		off = obj % LEAF_ELEMENTS(state);
		FreeEntry(state,&leafpage[off]);
		FREE_OBJ(leafpage[off]);

		own = page;
		for	( i = 0 ; i < state->objs->level ; i ++ ) {
			page = stack[i];
			(void)GetPage(state,page);
			nodepage = UpdatePage(state,page);
			for	( j = 0 ; j < NODE_ELEMENTS(state) ; j ++ ) {
				if		(  PAGE_NO(nodepage[j])  ==  own  ) {
					FREE_NODE(nodepage[j]);
					break;
				}
			}
			own = page;
			if			(  PAGE_NO(state->objs->pos[i])  ==  page  ) {
				FREE_NODE(state->objs->pos[i]);
			}
			if		(  HAVE_FREECHILD(page)  )	break;
		}
		rc = TRUE;
	}
	UnLock(&state->space->obj);
LEAVE_FUNC;
	return	(rc);
}

static	void
SeekTree(
	OsekiSession	*state,
	ObjectInfo		*ent,
	Bool			fWrite)
{
	int			depth2
		,		depth
		,		off;
	uint64_t	size
		,		base;
	pageno_t	page
		,		prio;
	pageno_t	*pages;

ENTER_FUNC;
	depth2 = 0;
	size = ent->off;
	size /= PAGE_SIZE(state);
	while	(  size  >  0  ) {
		depth2 ++;
		size /= NODE_ELEMENTS(state);
	}
	if		(  fWrite  ) {
		depth = ( depth2 > ent->depth ? depth2 : ent->depth );
		for	( ; depth2 > ent->depth ; depth2 -- ) {
			page = NewPage(state,1);
			(void)GetPage(state,page);
			pages = UpdatePage(state,page);
			memclear(pages,PAGE_SIZE(state));
			pages[0] = OBJ_PAGE(state,ent->head);
			ent->head = OBJ_POS(state,page,0);
		}
		ent->depth = depth;
	}
	page = OBJ_PAGE(state,ent->head);
	base = ent->off / PAGE_SIZE(state);
	prio = 0;
	off = 0;
	for	( depth = ent->depth ; depth > 0 ; depth -- ) {
		if		(  page  ==  0  ) {
			if		(  fWrite  ) {
				page = NewPage(state,1);
				if		(  prio  ==  0  ) {
					ent->head = OBJ_POS(state,page,0);
				} else {
					pages = UpdatePage(state,prio);
					pages[off] = page;
				}
			} else
				break;
		}
		off = base / state->space->mul[depth-1];
		pages = GetPage(state,page);
		prio = page;
		page = pages[off];
		base %= NODE_ELEMENTS(state);
	}
	if		(  page  ==  0  ) {
		if		(  fWrite  ) {
			page = NewPage(state,1);
			if		(  prio  ==  0  ) {
				ent->head = OBJ_POS(state,page,0);
			} else {
				pages = UpdatePage(state,prio);
				pages[off] = page;
			}
			ent->curr = OBJ_POS(state,page,OBJ_OFF(state,ent->off));
		} else {
			ent->curr = GL_OBJ_NULL;
		}
	} else {
		ent->curr = OBJ_POS(state,page,OBJ_OFF(state,ent->off));
	}
LEAVE_FUNC;
}

static	void
FlushBuffer(
	OsekiSession	*state,
	ObjectInfo		*ent,
	Bool			fOnly)
{
	pageno_t		page
		,			wpage;
	OsekiDataPage	*data;
	OsekiDataEntry	*el
		,			*wel;
	size_t			off;

ENTER_FUNC;
	if		(	(  ent->buff  ==  NULL  )
			||	(  ent->curr  ==  0  ) ) {
	} else
	if		(  ent->fDurty  ) {
		switch	(ent->mode & OSEKI_TYPE_MASK) {
		  case	OSEKI_ALLOC_LINER:
			dbgmsg("*");
			break;
		  case	OSEKI_ALLOC_TREE:
			dbgmsg("*");
			break;
		  case	OSEKI_ALLOC_PACK:
			dbgmsg("*");
			page = OBJ_PAGE(state,ent->curr);
			off = OBJ_OFF(state,ent->curr);
			data = GetPage(state,page);
			el = (OsekiDataEntry *)((byte *)data + off);
			ent->buff = NULL;
			if		(  el->size  ==  0  ) {
				el->size = ent->use;
				data->use += ent->use;
				if		(  ent->head  ==  0  ) {
					ent->head = ent->curr;
				} else {
					wpage = OBJ_PAGE(state,el->prio);
					(void)GetPage(state,wpage);
					data = UpdatePage(state,wpage);
					wel = (OsekiDataEntry *)
						((byte *)data + OBJ_OFF(state,el->prio));
					wel->next = ent->curr;
				}
				if		(  ( PAGE_SIZE(state) - data->use )
						   >  sizeof(OsekiDataEntry)  ) {
					ReturnPage(state,page);
				}
			}
			break;
		}
	}
	if		(  !fOnly  ) {
		switch	(ent->mode & OSEKI_TYPE_MASK) {
		  case	OSEKI_ALLOC_LINER:
			ent->curr = ent->head + ent->off + ent->use;
			break;
		  case	OSEKI_ALLOC_TREE:
			//ent->off += ent->use;
			SeekTree(state,ent,TRUE);
			break;
		  case	OSEKI_ALLOC_PACK:
			ent->curr = el->next;
			break;
		  default:
			break;
		}
	}
	ent->fDurty = FALSE;
	//ent->bsize = 0;
	DumpInfo(state,ent);
LEAVE_FUNC;
}

static	void
ReadBuffer(
	OsekiSession	*state,
	ObjectInfo		*ent,
	Bool			fOnly)
{
	pageno_t			page;
	OsekiDataPage	*data;
	OsekiDataEntry	*el;
	size_t			off;

ENTER_FUNC;
	if		(  ent->curr  >  0  ) {
		switch	(ent->mode & OSEKI_TYPE_MASK) {
		  case	OSEKI_ALLOC_LINER:
dbgmsg("*");
			page = OBJ_PAGE(state,ent->curr);
			data = GetPage(state,page);
			if		(  data  !=  NULL  ) {
				ent->buff = (byte *)data;
				ent->bsize = PAGE_SIZE(state);
			} else {
				ent->bsize = 0;
			}
			ent->use = OBJ_OFF(state,ent->curr);
			break;
		  case	OSEKI_ALLOC_TREE:
dbgmsg("*");
			SeekTree(state,ent,(ent->mode&OSEKI_OPEN_WRITE) != 0);
			if		(  ent->curr  >  0  ) {
				page = OBJ_PAGE(state,ent->curr);
				data = GetPage(state,page);
				if		(  data  !=  NULL  ) {
					ent->buff = (byte *)data;
					ent->bsize = PAGE_SIZE(state);
				} else {
					ent->bsize = 0;
				}
			} else {
				ent->bsize = 0;
			}
			ent->use = OBJ_OFF(state,ent->curr);
			break;
		  case	OSEKI_ALLOC_PACK:
dbgmsg("*");
			page = OBJ_PAGE(state,ent->curr);
			off = OBJ_OFF(state,ent->curr);
			data = GetPage(state,page);
			el = (OsekiDataEntry *)((byte *)data + off);
			ent->buff = (byte *)el + sizeof(OsekiDataEntry);
			ent->bsize = el->size;
			dbgprintf("el->size = %d\n",el->size);
			ent->use = 0;
			break;
		}
		if		(  !fOnly  ) {
			switch	(ent->mode & OSEKI_TYPE_MASK) {
			  case	OSEKI_ALLOC_LINER:
				ent->curr += PAGE_SIZE(state);
				break;
			  case	OSEKI_ALLOC_PACK:
				ent->curr = el->next;
				break;
			  default:
				break;
			}
		}
	} else {
		ent->bsize = 0;
	}
	DumpInfo(state,ent);
LEAVE_FUNC;
}

static	void
NewBuffer(
	OsekiSession	*state,
	ObjectInfo		*ent)
{
	pageno_t		page
		,			pages
		,			wpage
		,			from
		,			ip;
	OsekiDataPage	*data
		,			*fd;
	OsekiDataEntry	*el;

ENTER_FUNC;
	DumpInfo(state,ent);
	switch	(ent->mode & OSEKI_TYPE_MASK) {
	  case	OSEKI_ALLOC_LINER:
dbgmsg("*");
		wpage = 0;
		if		(  ent->head  >  0  ) {
			if		(  ent->off  <  ent->size  ) {
				page = OBJ_PAGE(state,ent->head + ent->off);
			} else {
				dbgprintf("size  = %lld\n",ent->size);
				pages = OBJ_PAGE(state,ent->size);
				dbgprintf("pages = %lld\n",pages);
				page = NewPage(state,pages+1);
				wpage = page;
				from = OBJ_PAGE(state,ent->head);
				for	( ip = 0 ; ip < pages ; ip ++ , from ++ , page ++ ) {
					fd = GetPage(state,from);
					(void)GetPage(state,page);
					data = UpdatePage(state,page);
					memcpy(data,fd,PAGE_SIZE(state));
					fd = UpdatePage(state,from);
					memclear(fd,PAGE_SIZE(state));
					ReturnPage(state,from);
				}
			}
		} else {
			page = NewPage(state,1);
			wpage = page;
		}
		(void)GetPage(state,page);
		ent->buff = UpdatePage(state,page);
		DumpInfo(state,ent);
		if		(  wpage  >  0  ) {
			ent->head = OBJ_POS(state,wpage,0);
		}
		ent->curr = OBJ_POS(state,page,0);
		ent->bsize = PAGE_SIZE(state);
		break;
	  case	OSEKI_ALLOC_TREE:
dbgmsg("*");
		SeekTree(state,ent,TRUE);
		page = OBJ_PAGE(state,ent->curr);
		(void)GetPage(state,page);
		ent->buff = UpdatePage(state,page);
		DumpInfo(state,ent);
		break;
	  case	OSEKI_ALLOC_PACK:
dbgmsg("*");
		page = GetFreePage(state);
		(void)GetPage(state,page);
		data = UpdatePage(state,page);
		if		(  data->use  ==  0  ) {
			data->use =  sizeof(OsekiDataPage);
		}
		el = (OsekiDataEntry *)((byte *)data + data->use);
		el->size = 0;
		el->prio = ent->node;
		if		(  ent->curr  ==  ent->head  ) {
			OBJ_POINT_NODE(el->prio);
		}
		ent->curr = OBJ_POS(state,page,data->use);
		ent->node = ent->curr;
		el->next = 0;
		ent->buff = (byte *)el + sizeof(OsekiDataEntry);
		ent->bsize = PAGE_SIZE(state) -
			( data->use + sizeof(OsekiDataEntry));
		break;
	}
	ent->use = 0;
LEAVE_FUNC;
}

static	ssize_t
WriteEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	byte			*buff,
	size_t			size)
{
	ssize_t		ret;
	size_t		csize
		,		left;

	ret = 0;
	while	(  size  >  0  ) {
		left = ent->bsize - ent->use;
		if		(  left  ==  0  ) {
			ReadBuffer(state,ent,TRUE);
			if		(  ent->bsize  ==  0  ) {
				NewBuffer(state,ent);
			}
			left = ent->bsize;
		}
		csize = ( size < left ) ? size : left;
		memcpy(&ent->buff[ent->use],buff,csize);
		ent->fDurty = TRUE;
		ent->use += csize;
		if		(  ent->use  ==  ent->bsize  ) {
			FlushBuffer(state,ent,FALSE);
		}
		ent->off += csize;
		ret += csize;
		size -= csize;
		buff += csize;
	}
	if		(  ent->off  >  ent->size  ) {
		ent->size = ent->off;
	}
	return	(ret);
}

static	ssize_t
ReadEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	byte			*buff,
	size_t			size)
{
	ssize_t		ret;
	size_t		csize
		,		left;

ENTER_FUNC;
	ret = 0;
	DumpInfo(state,ent);
	FlushBuffer(state,ent,TRUE);
	while	(	(  size      >  0          )
			&&	(  ent->off  <  ent->size  ) ) {
		left = ent->bsize - ent->use;
		if		(  left  ==  0  ) {
			ReadBuffer(state,ent,FALSE);
			if		(  ent->bsize  ==  0  )	break;
			left = ( ent->bsize > ( ent->size - ent->off ) ) ?
				( ent->size - ent->off ) : ent->bsize;
		}
		csize = ( size < left ) ? size : left;
		memcpy(buff,&ent->buff[ent->use],csize);
		ent->use += csize;
		size -= csize;
		ret += csize;
		buff += csize;
		ent->off += csize;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	ObjectType
InitiateObject(
	OsekiSession	*state,
	void			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	ObjectType	obj;
	pageno_t	leaf;

ENTER_FUNC;
	state->objs = GetPage(state,1LL);
	obj = GetFreeOID(state);
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		if		(  ( ent = OpenEntry(state,leaf,obj,
									 OSEKI_OPEN_WRITE|OSEKI_OPEN_CREATE) )  ==  NULL  ) {
			obj = GL_OBJ_NULL;
		}
	} else {
		ent  = NULL;
		obj = GL_OBJ_NULL;
	}
	if		(  obj  !=  GL_OBJ_NULL  ) {
		WriteEntry(state,ent,buff,size);
		CloseEntry(state,ent,obj);
	}
LEAVE_FUNC;
	return	(obj);
}

extern	ssize_t
GetObject(
	OsekiSession	*state,
	ObjectType		obj,
	void			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	pageno_t	leaf;
	ssize_t		ret;

ENTER_FUNC;
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		ent = OpenEntry(state,leaf,obj,OSEKI_OPEN_READ);
	} else {
		ent = NULL;
	}
	if		(  ent  !=  NULL  ) {
		if		(  size  ==  0  ) {
			size = ent->size;
		} else {
			size = ( size  <  ent->size ) ? size : ent->size;
		}
		ReadEntry(state,ent,buff,size);
		CloseEntry(state,ent,obj);
		ret = size;
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	ssize_t
PutObject(
	OsekiSession	*state,
	ObjectType		obj,
	void			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	pageno_t	leaf;
	OsekiObjectEntry	*leafpage;
	ssize_t		ret;
	size_t		off;

ENTER_FUNC;
	if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
		Lock(&state->space->obj);
		(void)GetPage(state,leaf);
		leafpage = UpdatePage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		FreeEntry(state,&leafpage[off]);
		leafpage[off].mode = OSEKI_OPEN_CLOSE;
		leafpage[off].pos = 0;
		leafpage[off].size = 0;
		USE_OBJ(leafpage[off]);
		UnLock(&state->space->obj);
		ent = OpenEntry(state,leaf,obj,OSEKI_OPEN_WRITE);
	} else {
		ent = NULL;
	}
	if		(  ent  !=  NULL  ) {
		WriteEntry(state,ent,buff,size);
		CloseEntry(state,ent,obj);
		ret = size;
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	int
WriteObject(
	OsekiSession	*state,
	ObjectType		obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	ssize_t		ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = WriteEntry(state,ent,buff,size);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	int
ReadObject(
	OsekiSession	*state,
	ObjectType		obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	int			ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = ReadEntry(state,ent,buff,size);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

static	ssize_t
SeekEntry(
	OsekiSession	*state,
	ObjectInfo		*ent,
	uint64_t		pos)
{
	ssize_t		ret;

	ret = 0;
	FlushBuffer(state,ent,TRUE);
	switch	(ent->mode & OSEKI_TYPE_MASK) {
	  case	OSEKI_ALLOC_LINER:
		ent->curr = ent->head + pos;
		ent->off = pos;
		ent->use = OBJ_OFF(state,pos);
		ReadBuffer(state,ent,TRUE);
		ent->curr = ent->head + pos;
		break;
	  case	OSEKI_ALLOC_TREE:
		ent->off = pos;
		ReadBuffer(state,ent,TRUE);
		break;
	  case	OSEKI_ALLOC_PACK:
		ent->curr = ent->head;
		ent->off = 0;
		while	(	(  ent->off  <  pos        )
				&&	(  ent->off  <  ent->size  ) ) {
			ReadBuffer(state,ent,FALSE);
			if		(  ent->bsize  ==  0  )	break;
			if		(  ent->off + ent->bsize >  pos  ) {
				ent->use = pos - ent->off;
				ent->off = pos;
			} else {
				ent->off += ent->bsize;
			}
		}
		break;
	}
	DumpInfo(state,ent);
	return	(ret);
}

extern	int
SeekObject(
	OsekiSession	*state,
	ObjectType		obj,
	uint64_t		pos)
{
	ObjectInfo	*ent;
	int			ret;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = SeekEntry(state,ent,pos);
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
OsekiTransactionStart(
	OsekiSession	*state)
{
	int		tid;

ENTER_FUNC;
	Lock(state->space);
	tid = state->space->cTran ++;
	state->tid = tid;
	g_tree_insert(state->space->trans,&state->tid,state);
	UnLock(state->space);
	state->oTable = NewLLHash();
	state->pages = NewLLHash();
	state->objs = NULL;
LEAVE_FUNC;
	return	(TRUE);
}

static	void
SyncObject(
	ObjectType		*obj,
	ObjectInfo		*ent,
	OsekiSession	*state)
{
ENTER_FUNC;
	xfree(ent->buff);
	xfree(ent);
LEAVE_FUNC;
}

static	void
CommitObject(
	OsekiSession	*state)
{
ENTER_FUNC;
	g_hash_table_foreach(state->oTable,(GHFunc)SyncObject,state);
	g_hash_table_destroy(state->oTable);
	state->oTable = NULL;
LEAVE_FUNC;
}

static	void
SetLeast(
	OsekiSession	*state)
{
	int		_least(
		int				tid,
		OsekiSession	*ses,
		OsekiSpace		*space)
	{
		space->lTran = ses->tid;
		dbgprintf("tid = %d\n",space->lTran);
		return	(TRUE);
	}
ENTER_FUNC;
	g_tree_remove(state->space->trans,(gpointer)state->tid);
	g_tree_traverse(state->space->trans,(GTraverseFunc)_least,
					G_IN_ORDER,state->space);
LEAVE_FUNC;
}

extern	Bool
OsekiTransactionCommit(
	OsekiSession	*state)
{
ENTER_FUNC;
	SetLeast(state);
	CommitObject(state);
	CommitPages(state);
	state->tid = 0;
LEAVE_FUNC;
	return	(TRUE);
}

static	void
FreeObject(
	ObjectType		*obj,
	ObjectInfo		*ent,
	OsekiSession	*state)
{
ENTER_FUNC;
	xfree(ent->buff);
	xfree(ent);
LEAVE_FUNC;
}

static	void
AbortObject(
	OsekiSession	*state)
{
ENTER_FUNC;
	g_hash_table_foreach(state->oTable,(GHFunc)FreeObject,state);
	g_hash_table_destroy(state->oTable);
	state->oTable = NULL;
LEAVE_FUNC;
}

extern	Bool
OsekiTransactionAbort(
	OsekiSession	*state)
{
ENTER_FUNC;
	SetLeast(state);
	AbortObject(state);
	AbortPages(state);
	state->tid = 0;
LEAVE_FUNC;
	return	(TRUE);
}
