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
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<glib.h>
#include	<pthread.h>
#include	<fcntl.h>
#include	<errno.h>
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
	objpos_t	first;
	objpos_t	last;
	uint64_t	size;
	byte		*buff;
	size_t		bsize;
	size_t		use;
	int			tid;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	ObjectInfo;

static	void	WriteBuffer(OsekiSession *state, ObjectInfo *ent);

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
								lower = NewPage(state);
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
			node = NewPage(state);
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

ENTER_FUNC;
	Lock(&state->space->obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  ==  NULL  ) {
		leafnode = (OsekiObjectEntry *)GetPage(state,leaf);
		off = obj % LEAF_ELEMENTS(state);
		if		(  IS_FREEOBJ(leafnode[off])  ) {
			ent = NULL;
		} else
		if		(	(  leafnode[off].mode  ==  OSEKI_OPEN_CLOSE  )
				||	(	(  leafnode[off].mode  ==  OSEKI_OPEN_READ  )
					&&	(  mode                ==  OSEKI_OPEN_READ  ) ) ) {
			leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
			leafnode[off].mode = mode;
			if		(	(  leafnode[off].pos            !=  0  )
					&&	(  ( mode & OSEKI_OPEN_CREATE )  !=  0  ) ) {
				/*	remove	*/
				leafnode[off].pos = 0;
				leafnode[off].size = 0;
			}
			ent = New(ObjectInfo);
			ent->obj = obj;
			ent->mode = mode;
			ent->head = leafnode[off].pos;
			ent->first = leafnode[off].pos;
			ent->last = ent->head;
			ent->size = leafnode[off].size;
			ent->buff = xmalloc(state->space->pagesize);
			ent->bsize = 0;
			ent->use = 0;
			ent->node = OBJ_POS(state,leaf,off);
			ent->tid = state->tid;
			InitLock(ent);
			g_hash_table_insert(state->oTable,&ent->obj,ent);
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
	pageno_t		leaf;

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
		while	(  ent->use  >  0  ) {
			WriteBuffer(state,ent);
		}
	}
}

extern	Bool
CloseObject(
	OsekiSession	*state,
	ObjectType	obj)
{
	pageno_t		leaf;
	OsekiObjectEntry	*leafnode;
	Bool			ret;
	int				off;
	ObjectInfo		*ent;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	ret = TRUE;
	Lock(&state->space->obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,&obj) )  !=  NULL  ) {
		FlushObject(state,ent);
		if		(  ( leaf = SearchLeafPage(state,obj,NULL) )  !=  0  ) {
			(void)GetPage(state,leaf);
			leafnode = (OsekiObjectEntry *)UpdatePage(state,leaf);
			off = obj % LEAF_ELEMENTS(state);
			if		(  ( leafnode[off].mode & OSEKI_OPEN_WRITE )  !=  0  ) {
				leafnode[off].size = ent->size;
				leafnode[off].pos = ent->first;
			}
			leafnode[off].mode = OSEKI_OPEN_CLOSE;
		}
		xfree(ent->buff);
		g_hash_table_remove(state->oTable,&obj);
		xfree(ent);
	}
	UnLock(&state->space->obj);
LEAVE_FUNC;
	return	(ret);
}

static	void
FreeEntry(
	OsekiSession	*state,
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
				off = 0;
				el = (OsekiDataEntry *)((byte *)data + cur);
				nsize = ROUND_TO(el->size,sizeof(size_t))
					+ sizeof(OsekiDataEntry);
			}
			if		(  cur  >  off  ) {
				npos = OBJ_POS(state,page,cur);
				if		(  !IS_OBJ_NODE(el->prio)  ) {
					ppage = OBJ_PAGE(state,el->prio);
					(void)GetPage(state,ppage);
					wd = UpdatePage(state,ppage);
					wel = (OsekiDataEntry *)((byte *)wd
												+ sizeof(OsekiDataPage)
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
												+ sizeof(OsekiDataPage)
												+ OBJ_OFF(state,el->next));
					wel->prio = npos;
				}
			} else {
			}
			cur += nsize;
		}
		if		(  data->use  <  PAGE_SIZE(state)  ) {
			ReturnPage(state,page);
		}
	}
LEAVE_FUNC;
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
WriteBuffer(
	OsekiSession	*state,
	ObjectInfo	*ent)
{
	pageno_t			page
		,				wpage;
	OsekiDataPage	*data;
	OsekiDataEntry	*el
		,				*wel;
	objpos_t			pos;
	size_t				csize
		,				left;

ENTER_FUNC;
	if		(  ent->head  ==  0  ) {
		page = GetFreePage(state);
		(void)GetPage(state,page);
		data = UpdatePage(state,page);
		if		(  data->use  ==  0  ) {
			data->use =  sizeof(OsekiDataPage);
		}
		left = state->space->pagesize
			- ( data->use + sizeof(OsekiDataEntry) );
		if		(  left  <  ent->use  ) {
			csize = left;
		} else {
			csize = ent->use;
		}
		if		(  csize  >  0  ) {
			el = (OsekiDataEntry *)((byte *)data + data->use);
			pos = OBJ_POS(state,page,data->use);
			el->size = csize;
			el->next = 0;
			if		(  ent->last  !=  ent->head  ) {
				el->prio = ent->last;
			} else {
				el->prio = ent->node;
				OBJ_POINT_NODE(el->prio);
			}
			memcpy((byte *)el + sizeof(OsekiDataEntry),ent->buff,csize);
			memmove(ent->buff,ent->buff + csize,
					(state->space->pagesize - csize));
			ent->use -= csize;
			data->use += ROUND_TO(csize,sizeof(size_t))
				+ sizeof(OsekiDataEntry);
			if		(  state->space->pagesize - data->use 
					   >  sizeof(OsekiDataEntry)  ) {
				ReturnPage(state,page);
			}
			if		(  ent->last  !=  ent->head  ) {
				wpage = OBJ_PAGE(state,ent->last);
				(void)GetPage(state,wpage);
				data = UpdatePage(state,wpage);
				wel = (OsekiDataEntry *)
					((byte *)data + OBJ_OFF(state,ent->last));
				wel->next = pos;
			} else {
				ent->first = pos;
			}
			ent->last = pos;
		}
	} else {
	}
LEAVE_FUNC;
}

extern	int
WriteObject(
	OsekiSession	*state,
	ObjectType	obj,
	byte			*buff,
	size_t			size)
{
	ObjectInfo	*ent;
	int			ret;
	size_t		csize
		,		left;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = 0;
		while	(  size  >  0  ) {
			left = state->space->pagesize - ent->use;
			csize = ( size < left ) ? size : left;
			memcpy(&ent->buff[ent->use],buff,csize);
			ent->use += csize;
			ent->size += csize;
			ret += csize;
			size -= csize;
			buff += csize;
			if		(  ent->use  ==  state->space->pagesize  ) {
				WriteBuffer(state,ent);
			}
		}
	} else {
		ret = -1;
	}
LEAVE_FUNC;
	return	(ret);
}

static	void
ReadBuffer(
	OsekiSession	*state,
	ObjectInfo	*ent)
{
	pageno_t			page;
	OsekiDataPage	*data;
	OsekiDataEntry	*el;
	size_t				off;

ENTER_FUNC;
	if		(  ent->last  >  0  ) {
		page = OBJ_PAGE(state,ent->last);
		off = OBJ_OFF(state,ent->last);
		dbgprintf("page = %lld\n",page);
		dbgprintf("off = %d\n",off);
		data = GetPage(state,page);
		el = (OsekiDataEntry *)((byte *)data + off);
		memcpy(ent->buff,(byte *)el + sizeof(OsekiDataEntry),el->size);
		ent->bsize = el->size;
		dbgprintf("el->size = %d\n",el->size);
		ent->last = el->next;
		ent->use = 0;
	} else {
		ent->bsize = 0;
	}
LEAVE_FUNC;
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
	size_t		csize
		,		left;

ENTER_FUNC;
	dbgprintf("obj = %lld\n",obj);
	if		(  ( ent = g_hash_table_lookup(state->oTable,(gpointer)&obj) )
			   !=  NULL  ) {
		ret = 0;
		while	(  size  >  0  ) {
			left = ent->bsize - ent->use;
			if		(  left  ==  0  ) {
				ReadBuffer(state,ent);
				if		(  ent->bsize  ==  0  )	break;
				left = ent->bsize;
			}
			csize = ( size < left ) ? size : left;
			memcpy(buff,&ent->buff[ent->use],csize);
			ent->use += csize;
			size -= csize;
			ret += csize;
			buff += csize;
		}
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
		printf("tid = %d\n",space->lTran);
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
